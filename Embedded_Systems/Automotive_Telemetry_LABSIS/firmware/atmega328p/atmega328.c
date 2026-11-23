#ifndef F_CPU
#define F_CPU 16000000UL // External crystal frequency on ATmega328P (16 MHz)
#endif

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

// ============================================================================
// SERIAL COMMUNICATION (UART0) AND BUFFER CONFIGURATION
// ============================================================================
#define BAUD 115200
// ATmega328P datasheet formula 1X mode (U2X0 = 0): UBRR = (F_CPU / (16 * BAUD)) - 1 -> 103 (9600 bps)
#define BRC_9600 ((F_CPU / 16 / 9600) - 1)
// ATmega328P datasheet formula 2X mode (U2X0 = 1): UBRR = (F_CPU / (8 * BAUD)) - 1 -> 16 (~117647 bps, +2.12% error)
#define BRC ((F_CPU / 8 / BAUD) - 1)

#define RX_BUF_SIZE 256 // Must be a power of 2 (256 fits uint8_t bitmasking & (RX_BUF_SIZE - 1) atomically)
#define TX_BUF_SIZE 384 // Maximum capacity for one full JSON report (includes ISO ms timestamp)

// ============================================================================
// SAMPLING, FILTERING AND HARDWARE CONSTANTS
// ============================================================================
// Base IMU tick: 40 samples x 5 ms = 1 full JSON report every 200 ms (5 Hz)
#define IMU_TICK_MS 5
#define IMU_PER_REPORT 40

// ADC management (LM35 vs Battery) to keep AREF reference voltage rock-steady
#define ADC_TEMP_TICK_DIV 5       // Read LM35 (ADC0) every 5 ticks = 25 ms (8 readings per 200 ms report)
#define BAT_READ_EVERY_REPORTS 25 // Read battery (ADC1) only every 25 reports = 5.0 seconds
#define ADC_BAT_SAMPLES 8         // Number of battery samples averaged before switching back to 1.1V
#define AREF_SETTLE_TICKS 32      // 32 x 5 ms = 160 ms settle wait + dummy reads for AREF cap to discharge 5V -> 1.1V
#define BAT_OFFSET_MV 190         // Compensation for battery ESR / load voltage drop (190 mV)

// Safety timeout for I2C bus (prevents total lockup if vehicle vibration causes a loose wire)
#define TWI_TIMEOUT 2000

// ============================================================================
// MPU6050 INERTIAL SENSOR INTERNAL REGISTERS (I2C)
// ============================================================================
#define MPU6050_ADDR 0x68         // Default I2C address (AD0 pin tied to GND)
#define MPU6050_PWR_MGMT_1 0x6B   // Power management register (0x00 = wake up chip)
#define MPU6050_CONFIG 0x1A       // Internal Digital Low-Pass Filter (DLPF)
#define MPU6050_GYRO_CONFIG 0x1B  // Gyroscope full-scale range
#define MPU6050_ACCEL_CONFIG 0x1C // Accelerometer full-scale range
#define MPU6050_ACCEL_XOUT 0x3B   // First of 14 sequential registers (Accel XYZ, Temp, Gyro XYZ)

// Possible states of the ADC sampling state machine
typedef enum
{
  ADC_STATE_TEMP = 0,  // Normal state: ADC0 locked on Internal 1.1V reference (LM35)
  ADC_STATE_BAT = 1,   // Brief state (40 ms every 5 s): ADC1 on AVcc 5.0V (Battery)
  ADC_STATE_SETTLE = 2 // Settling state (160 ms): returns to 1.1V and discards reads until AREF settles
} ADC_State;

// ============================================================================
// GLOBAL VARIABLES SHARED WITH INTERRUPTS (volatile)
// ============================================================================
volatile char rx_ring[RX_BUF_SIZE];      // Circular GPS receive buffer
volatile uint8_t rx_head = 0;            // Write index (only RX ISR writes here)
volatile uint8_t rx_tail = 0;            // Read index (only main loop writes here)
volatile uint8_t flag_5ms = 0;           // Set every 5 ms by Timer0
volatile uint8_t flag_200ms = 0;         // Set every 200 ms (5 Hz) by Timer0
volatile uint16_t gps_wait_ms = 0;       // Down-counter in ms for non-blocking hardware waits
volatile uint8_t flag_gps_wait_done = 0; // Signals completion of gps_wait_ms countdown

// Non-blocking UART transmit buffer (USART_UDRE ISR sends byte by byte in background)
static char tx_buf[TX_BUF_SIZE];
static volatile uint16_t tx_pos = 0;
static volatile uint16_t tx_len = 0;

// ============================================================================
// UBX BINARY CONFIGURATION PACKETS FOR u-blox NEO-6M GPS MODULE
// Stored in Flash memory (PROGMEM) to save scarce SRAM (2 KB)
// ============================================================================

// UBX-CFG-RATE: Sets 200 ms measurement period (5 Hz), 1 cycle per navigation solution, timeRef = 0 (UTC)
static const uint8_t UBX_5HZ[] PROGMEM = {
    0xB5, 0x62, // UBX sync chars
    0x06, 0x08, // Class: CFG (0x06), ID: RATE (0x08)
    0x06, 0x00, // Payload length: 6 bytes (little-endian)
    0xC8, 0x00, // measRate: 200 ms (0x00C8) -> 5 Hz
    0x01, 0x00, // navRate: 1 cycle
    0x00, 0x00, // timeRef: 0 (UTC time - broadest compatibility across u-blox 6 modules)
    0xDD, 0x68  // Fletcher-8 Checksum: CK_A, CK_B
};

// UBX-CFG-PRT: Configures NEO-6M UART1 to 115200 bps, 8N1, in: UBX+NMEA+RTCM, out: UBX+NMEA
static const uint8_t UBX_BAUD_115200[] PROGMEM = {
    0xB5, 0x62,             // Sync header
    0x06, 0x00,             // Class: CFG, ID: PRT
    0x14, 0x00,             // Length: 20 bytes
    0x01, 0x00, 0x00, 0x00, // PortID: 1 (UART1)
    0xD0, 0x08, 0x00, 0x00, // Mode: 8N1 (0x08D0)
    0x00, 0xC2, 0x01, 0x00, // Baudrate: 115200 (0x0001C200)
    0x07, 0x00,             // inProtoMask (UBX + NMEA + RTCM)
    0x03, 0x00,             // outProtoMask (UBX + NMEA)
    0x00, 0x00,             // Flags: 0
    0x00, 0x00,             // Reserved
    0xC0, 0x7E              // Checksum: CK_A, CK_B
};

// UBX-CFG-NAV5: Sets Dynamic Platform Model to 4 (Automotive) — optimizes GPS Kalman filter for vehicle dynamics
static const uint8_t UBX_AUTOMOTIVE[] PROGMEM = {
    0xB5, 0x62,             // Sync header
    0x06, 0x24,             // Class: CFG (0x06), ID: NAV5 (0x24)
    0x24, 0x00,             // Length: 36 bytes
    0x01, 0x00,             // Mask: 0x0001 (Apply dynamic model only)
    0x04,                   // dynModel: 4 (Automotive)
    0x00,                   // fixMode
    0x00, 0x00, 0x00, 0x00, // fixedAlt
    0x00, 0x00, 0x00, 0x00, // fixedAltVar
    0x00,                   // minElev
    0x00,                   // drLimit
    0x00, 0x00,             // pDop
    0x00, 0x00,             // tDop
    0x00, 0x00,             // pAcc
    0x00, 0x00,             // tAcc
    0x00,                   // staticHoldThresh
    0x00,                   // dgnssTimeout
    0x00, 0x00, 0x00, 0x00, // Reserved
    0x00, 0x00, 0x00, 0x00, // Reserved
    0x00, 0x00, 0x00, 0x00, // Reserved
    0x53, 0x70              // Checksum: CK_A, CK_B
};

// UBX-CFG-MSG: Disables unnecessary NMEA sentences (GGA, GLL, GSA, GSV, VTG), keeping only RMC active.
// Shrinks GPS burst from >400 bytes to ~70 bytes per fix, preventing rx_ring overflow.
static const uint8_t UBX_GGA_OFF[] PROGMEM = {0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x23};
static const uint8_t UBX_GLL_OFF[] PROGMEM = {0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0xF0, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2A};
static const uint8_t UBX_GSA_OFF[] PROGMEM = {0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0xF0, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x31};
static const uint8_t UBX_GSV_OFF[] PROGMEM = {0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0xF0, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x38};
static const uint8_t UBX_VTG_OFF[] PROGMEM = {0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0xF0, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x46};

// UBX-CFG-SBAS: Disables SBAS/EGNOS corrections so the NEO-6M CPU never throttles 5 Hz under high satellite load
static const uint8_t UBX_SBAS_OFF[] PROGMEM = {
    0xB5, 0x62, 0x06, 0x16, 0x08, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x24, 0x8A
};

// UBX-CFG-RXM: Forces Continuous / Max Performance mode (prevents Power Save Mode from dropping to 1 Hz)
static const uint8_t UBX_CONTINUOUS[] PROGMEM = {
    0xB5, 0x62, 0x06, 0x11, 0x02, 0x00,
    0x08, 0x00,
    0x21, 0x91
};

// UBX-CFG-CFG: Clears saved config in BBR/Flash/EEPROM so next cold boot resets to 9600 bps defaults
static const uint8_t UBX_CFG_CLEAR[] PROGMEM = {
    0xB5, 0x62, 0x06, 0x09, 0x0D, 0x00,
    0xFF, 0xFF, 0x00, 0x00, // clearMask: all sections
    0x00, 0x00, 0x00, 0x00, // saveMask: none
    0x00, 0x00, 0x00, 0x00, // loadMask: none (keep UART port up for ACK; next command sets 9600)
    0x17,                   // deviceMask: BBR + Flash + EEPROM + SPI Flash
    0x31, 0xB7              // Checksum: CK_A, CK_B
};

// UBX-CFG-PRT: Configures NEO-6M UART1 to 9600 bps, 8N1, in: UBX+NMEA+RTCM, out: UBX+NMEA
static const uint8_t UBX_BAUD_9600[] PROGMEM = {
    0xB5, 0x62,             // Sync header
    0x06, 0x00,             // Class: CFG, ID: PRT
    0x14, 0x00,             // Length: 20 bytes
    0x01, 0x00, 0x00, 0x00, // PortID: 1 (UART1)
    0xD0, 0x08, 0x00, 0x00, // Mode: 8N1 (0x08D0)
    0x80, 0x25, 0x00, 0x00, // Baudrate: 9600 (0x00002580)
    0x07, 0x00,             // inProtoMask (UBX + NMEA + RTCM)
    0x03, 0x00,             // outProtoMask (UBX + NMEA)
    0x00, 0x00,             // Flags: 0
    0x00, 0x00,             // Reserved
    0xA2, 0xB5              // Checksum: CK_A, CK_B
};

// ============================================================================
// IMU (MPU6050) DATA STRUCTURES
// ============================================================================
typedef struct
{
  int16_t ax, ay, az; // Averaged acceleration in mg (e.g. 1000 = 1.000 G)
  int32_t gx, gy, gz; // Averaged angular rate in (deg/s) * 100 (e.g. 1146 = 11.46 deg/s)
  int32_t temp_x10;   // Internal chip temperature in °C * 10 (e.g. 246 = 24.6 °C)
} MPU6050_SI_Data;

typedef struct
{
  int16_t ax, ay, az, temp, gx, gy, gz; // Single raw sample (LSB) already remapped to vehicle axes
} MPU6050_Raw;

typedef struct
{
  int32_t ax, ay, az, temp, gx, gy, gz; // Running sums of raw samples over one 200 ms cycle
  uint8_t n;                            // Actual count of valid samples accumulated
} MPU6050_Accum;

// ============================================================================
// FUNCTION PROTOTYPES
// ============================================================================
void uart_init(void);
void uart_init_9600(void);
void uart_transmit(char data);
void uart_print(const char *str);
void uart_print_P(const char *str_P);
void send_ubx(const uint8_t *data, size_t len);
uint8_t send_ubx_wait_ack(const uint8_t *data, size_t len, uint8_t cls_id, uint8_t msg_id, uint8_t use_exact_bb);
uint8_t detect_gps_nmea(void);
void timer0_init_1ms(void);
void wait_ms_timer(uint16_t ms);
uint8_t uart_available(void);
char uart_receive(void);
void adc_init(void);
uint16_t adc_read(void);
uint8_t get_battery_level(uint16_t adc_avg_bat);
void TWI_Init(void);
uint8_t TWI_Start(void);
void TWI_Stop(void);
uint8_t TWI_Write(uint8_t data);
uint8_t TWI_Read_ACK(uint8_t *out);
uint8_t TWI_Read_NACK(uint8_t *out);
void MPU6050_Init(void);
uint8_t MPU6050_Read_Raw(MPU6050_Raw *out);
void MPU6050_Accumulate(MPU6050_Accum *acc);
void MPU6050_Average_SI(const MPU6050_Accum *acc, MPU6050_SI_Data *out);
void print_status_report(uint16_t counter, const char *timestamp, const char *status,
                         const char *lat, const char *ns, const char *lon, const char *ew,
                         const char *speed, const char *course, uint8_t gps_seq,
                         int16_t temp_disp_x10, uint8_t battery_life,
                         const MPU6050_SI_Data *imu);
void parse_gps(char *buffer, char *timestamp, uint8_t ts_size, char *status, char *lat, char *ns,
               char *lon, char *ew, char *speed, char *course);
uint8_t nmea_checksum_ok(const char *s);
void setup_inits(void);
uint8_t days_in_month(uint16_t year, uint8_t month);
uint8_t day_of_week(uint16_t year, uint8_t month, uint8_t day);
uint8_t is_portugal_dst_utc(uint16_t year, uint8_t month, uint8_t day, uint8_t hour);
void format_nmea_to_pt_iso(const char *utc_time_str, const char *utc_date_str, char *out_buf, uint8_t out_size);
void format_speed_kmh(char *speed);
void adjust_pwm_fan(int16_t temperature);
void pwm_init(void);

// ============================================================================
// INTERRUPT SERVICE ROUTINES (ISRs)
// ============================================================================

// Timer0 Compare Match A ISR: fires every 1.000 ms
ISR(TIMER0_COMPA_vect)
{
  static uint8_t ms = 0;
  static uint8_t ticks = 0;

  // 1. Auxiliary countdown timer used by wait_ms_timer() during GPS startup
  if (gps_wait_ms > 0)
  {
    if (--gps_wait_ms == 0)
    {
      flag_gps_wait_done = 1; // Atomic 1-byte flag signaling wait completion
    }
  }

  // 2. Tick divider for IMU (5 ms) and JSON report (200 ms)
  if (++ms >= IMU_TICK_MS)
  {
    ms = 0;
    flag_5ms = 1;
    if (++ticks >= IMU_PER_REPORT)
    {
      ticks = 0;
      flag_200ms = 1; // Set together with flag_5ms -> ensures 40th sample is taken before averaging
    }
  }
}

// UART RX ISR: stores every incoming GPS byte without blocking the main loop
ISR(USART_RX_vect)
{
  char data = UDR0;
  uint8_t next = (rx_head + 1) & (RX_BUF_SIZE - 1);
  if (next != rx_tail) // Guard against ring buffer overflow
  {
    rx_ring[rx_head] = data;
    rx_head = next;
  }
}

// UART Data Register Empty ISR: transmits next byte from tx_buf in the background
ISR(USART_UDRE_vect)
{
  UDR0 = tx_buf[tx_pos++];
  if (tx_pos >= tx_len)
  {
    UCSR0B &= ~(1 << UDRIE0); // Last byte loaded; disable interrupt until next JSON report
  }
}

// ============================================================================
// TIMING, UART AND PWM
// ============================================================================

// Configures Timer0 in CTC mode to generate 1 interrupt every exact 1.000 ms
void timer0_init_1ms(void)
{
  TCCR0A = (1 << WGM01);              // CTC mode (Clear Timer on Compare Match)
  TCCR0B = (1 << CS01) | (1 << CS00); // Prescaler = 64 -> timer clock = 16 MHz / 64 = 250 kHz (4 µs/tick)
  OCR0A = 249;                        // 250 ticks * 4 µs = 1 ms (counts 0..249)
  TIMSK0 = (1 << OCIE0A);             // Enable OCR0A Compare Match interrupt
}

// Waits exactly 'ms' milliseconds using the Timer0 1 ms counter (replaces _delay_ms)
void wait_ms_timer(uint16_t ms)
{
  if (ms == 0)
    return;
  cli(); // Briefly disable interrupts for atomic 16-bit write to gps_wait_ms
  flag_gps_wait_done = 0;
  gps_wait_ms = ms;
  sei();
  while (!flag_gps_wait_done)
    ; // Wait until Timer0 decrements gps_wait_ms to 0
}

// Configures UART0 for normal 115200 bps operation and enables GPS RX interrupt
void uart_init(void)
{
  cli();
  UBRR0H = (BRC >> 8);
  UBRR0L = BRC;
  UCSR0A = (1 << U2X0);                                // Double-speed mode (117647 bps, +2.12% error)
  UCSR0B = (1 << RXEN0) | (1 << TXEN0) | (1 << RXCIE0); // Enable RX, TX and RX interrupt (UDRIE0 stays off)
  UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);              // 8N1 frame (8 data bits, no parity, 1 stop bit)
  sei();
}

// Configures UART0 for 9600 bps (+0.16% error) with RX interrupt enabled
void uart_init_9600(void)
{
  cli();
  UBRR0H = (BRC_9600 >> 8);
  UBRR0L = BRC_9600;
  UCSR0A = 0;                                          // 1X mode (9615 bps, +0.16% error)
  UCSR0B = (1 << RXEN0) | (1 << TXEN0) | (1 << RXCIE0); // Enable RX, TX and RX interrupt
  UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);              // 8N1 frame
  sei();
}

// Cycle-accurate software TX on PD1 at 115108 bps (139 cycles/bit @ 16 MHz, -0.08% error)
// Used only during startup recovery if the GPS is already locked at 115200 bps
static void bb_tx_byte_115200(uint8_t b)
{
  uint8_t port_hi = PORTD | (1 << PD1);
  uint8_t port_lo = PORTD & (uint8_t)~(1 << PD1);
  uint8_t cnt = 8;
  uint8_t sreg = SREG;
  cli();
  __asm__ __volatile__(
      "out %[port], %[lo] \n\t" // Start bit (0)
      "ldi r18, 44        \n\t"
      "1: dec r18         \n\t"
      "brne 1b            \n\t"
      "rjmp .+0           \n\t"
      "2:                 \n\t"
      "mov r19, %[lo]     \n\t"
      "ror %[val]         \n\t"
      "brcc 3f            \n\t"
      "mov r19, %[hi]     \n\t"
      "3:                 \n\t"
      "out %[port], r19   \n\t" // Data bit (LSB first), 139 cycles/bit
      "ldi r18, 43        \n\t"
      "4: dec r18         \n\t"
      "brne 4b            \n\t"
      "rjmp .+0           \n\t"
      "dec %[cnt]         \n\t"
      "brne 2b            \n\t"
      "rjmp .+0           \n\t"
      "rjmp .+0           \n\t"
      "nop                \n\t"
      "out %[port], %[hi] \n\t" // Stop bit (1) at 139 cycles
      "ldi r18, 44        \n\t"
      "5: dec r18         \n\t"
      "brne 5b            \n\t"
      : [val] "+r"(b), [cnt] "+r"(cnt)
      : [port] "I"(_SFR_IO_ADDR(PORTD)), [lo] "r"(port_lo), [hi] "r"(port_hi)
      : "r18", "r19", "cc", "memory");
  SREG = sreg;
}

// Sends a PROGMEM UBX buffer either via hardware UART0 or exact bit-bang 115200 TX
static void send_ubx_bytes(const uint8_t *data, size_t len, uint8_t use_exact_bb)
{
  if (use_exact_bb)
  {
    while (!(UCSR0A & (1 << UDRE0)))
      ;
    _delay_us(100);
    PORTD |= (1 << PD1);
    DDRD |= (1 << PD1);
    UCSR0B &= ~(1 << TXEN0);
    for (size_t i = 0; i < len; i++)
    {
      bb_tx_byte_115200(pgm_read_byte(&data[i]));
      _delay_us(15);
    }
    UCSR0B |= (1 << TXEN0);
  }
  else
  {
    for (size_t i = 0; i < len; i++)
    {
      uart_transmit(pgm_read_byte(&data[i]));
    }
  }
}

// Configures Timer1 on pin D10 (PB2) for fan PWM control (10-bit Fast PWM, ~61 Hz)
void pwm_init(void)
{
  DDRB |= (1 << PB2);                                   // Set PB2 (Arduino Nano D10) as output
  TCCR1A = (1 << COM1B1) | (1 << WGM11) | (1 << WGM10); // Non-inverting output on OC1B, Mode 7 (10-bit Fast PWM)
  TCCR1B = (1 << CS12) | (1 << WGM12);                  // Prescaler = 256 -> 16 MHz / (256 * 1024) = 61 Hz
  OCR1B = 0;                                            // Fan initially OFF (0% duty cycle)
}

// Blocking 1-byte transmit (used only during startup configuration before 5 Hz reports)
void uart_transmit(char data)
{
  while (!(UCSR0A & (1 << UDRE0)))
    ; // Wait until hardware transmit buffer is empty
  UDR0 = data;
}

static const uint8_t UBX_SYNC_FF[] PROGMEM = {0xFF, 0xFF};

// Sends a UBX binary command stored in Flash (PROGMEM) to the GPS module
void send_ubx(const uint8_t *data, size_t len)
{
  send_ubx_bytes(data, len, 0);
  wait_ms_timer(50); // Minimum 50 ms pause between consecutive UBX frames
}

// Blocking print of an SRAM string (waits for any active interrupt-driven report to finish)
void uart_print(const char *str)
{
  while (UCSR0B & (1 << UDRIE0))
    ;
  while (*str)
  {
    uart_transmit(*str++);
  }
}

// Blocking print of a constant Flash string (PSTR) without consuming SRAM
void uart_print_P(const char *str_P)
{
  while (UCSR0B & (1 << UDRIE0))
    ;
  char c;
  while ((c = pgm_read_byte(str_P++)) != '\0')
  {
    uart_transmit(c);
  }
}

// Returns non-zero if unread bytes are waiting in the GPS circular buffer
uint8_t uart_available(void)
{
  return (rx_head != rx_tail);
}

// Pops 1 byte from the circular receive buffer
char uart_receive(void)
{
  char data = rx_ring[rx_tail];
  rx_tail = (rx_tail + 1) & (RX_BUF_SIZE - 1);
  return data;
}

// ============================================================================
// I2C (TWI) BUS WITH TIMEOUT GUARD AND MPU6050 DRIVER
// ============================================================================

void TWI_Init(void)
{
  TWSR = 0x00;        // I2C prescaler = 1
  TWBR = 12;          // SCL = 16 MHz / (16 + 2 * 12 * 1) = 400 kHz (Fast Mode)
  TWCR = (1 << TWEN); // Enable TWI hardware
}

// Sends START condition; returns 1 on success or 0 on timeout
uint8_t TWI_Start(void)
{
  uint16_t timeout = TWI_TIMEOUT;
  TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);
  while (!(TWCR & (1 << TWINT)))
  {
    if (--timeout == 0)
      return 0;
  }
  return 1;
}

void TWI_Stop(void)
{
  TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN);
}

uint8_t TWI_Write(uint8_t data)
{
  uint16_t timeout = TWI_TIMEOUT;
  TWDR = data;
  TWCR = (1 << TWINT) | (1 << TWEN);
  while (!(TWCR & (1 << TWINT)))
  {
    if (--timeout == 0)
      return 0;
  }
  return 1;
}

uint8_t TWI_Read_ACK(uint8_t *out)
{
  uint16_t timeout = TWI_TIMEOUT;
  TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWEA);
  while (!(TWCR & (1 << TWINT)))
  {
    if (--timeout == 0)
      return 0;
  }
  *out = TWDR;
  return 1;
}

uint8_t TWI_Read_NACK(uint8_t *out)
{
  uint16_t timeout = TWI_TIMEOUT;
  TWCR = (1 << TWINT) | (1 << TWEN);
  while (!(TWCR & (1 << TWINT)))
  {
    if (--timeout == 0)
      return 0;
  }
  *out = TWDR;
  return 1;
}

// Initializes MPU6050: wakes chip, enables 20 Hz anti-aliasing DLPF, sets ±8g and ±1000 deg/s scales
void MPU6050_Init(void)
{
  // 1. Wake up MPU6050 (PWR_MGMT_1 = 0x00)
  if (TWI_Start())
  {
    TWI_Write((MPU6050_ADDR << 1) | 0);
    TWI_Write(MPU6050_PWR_MGMT_1);
    TWI_Write(0x00);
  }
  TWI_Stop();
  wait_ms_timer(10);

  // 2. Internal Digital Low-Pass Filter: DLPF_CFG = 4 (~21 Hz Accel / 20 Hz Gyro).
  //    Rejects high-frequency engine/road vibration above the 100 Hz Nyquist limit for 5 ms sampling.
  if (TWI_Start())
  {
    TWI_Write((MPU6050_ADDR << 1) | 0);
    TWI_Write(MPU6050_CONFIG);
    TWI_Write(0x04);
  }
  TWI_Stop();
  wait_ms_timer(10);

  // 3. Accelerometer range: ±8g (0x10 -> sensitivity = 4096 LSB/g)
  if (TWI_Start())
  {
    TWI_Write((MPU6050_ADDR << 1) | 0);
    TWI_Write(MPU6050_ACCEL_CONFIG);
    TWI_Write(0x10);
  }
  TWI_Stop();
  wait_ms_timer(10);

  // 4. Gyroscope range: ±1000 deg/s (0x10 -> sensitivity = 32.8 LSB/(deg/s))
  if (TWI_Start())
  {
    TWI_Write((MPU6050_ADDR << 1) | 0);
    TWI_Write(MPU6050_GYRO_CONFIG);
    TWI_Write(0x10);
  }
  TWI_Stop();
  wait_ms_timer(100);
}

// Burst-reads 14 bytes from MPU6050 and remaps sensor axes to the vehicle coordinate frame.
// Returns 1 on I2C success, or 0 on timeout/error (prevents corrupted data).
uint8_t MPU6050_Read_Raw(MPU6050_Raw *out)
{
  uint8_t buf[14];
  if (!TWI_Start()) { TWI_Stop(); return 0; }
  if (!TWI_Write((MPU6050_ADDR << 1) | 0)) { TWI_Stop(); return 0; }
  if (!TWI_Write(MPU6050_ACCEL_XOUT)) { TWI_Stop(); return 0; }
  if (!TWI_Start()) { TWI_Stop(); return 0; }
  if (!TWI_Write((MPU6050_ADDR << 1) | 1)) { TWI_Stop(); return 0; }

  for (uint8_t i = 0; i < 13; i++)
  {
    if (!TWI_Read_ACK(&buf[i])) { TWI_Stop(); return 0; }
  }
  if (!TWI_Read_NACK(&buf[13])) { TWI_Stop(); return 0; }
  TWI_Stop();

  // Reconstruct signed 16-bit Big-Endian integers
  int16_t raw_ax   = (int16_t)((buf[0]  << 8) | buf[1]);
  int16_t raw_ay   = (int16_t)((buf[2]  << 8) | buf[3]);
  int16_t raw_az   = (int16_t)((buf[4]  << 8) | buf[5]);
  int16_t raw_temp = (int16_t)((buf[6]  << 8) | buf[7]);
  int16_t raw_gx   = (int16_t)((buf[8]  << 8) | buf[9]);
  int16_t raw_gy   = (int16_t)((buf[10] << 8) | buf[11]);
  int16_t raw_gz   = (int16_t)((buf[12] << 8) | buf[13]);

  // Vertical board mount in vehicle (right-hand rule):
  // X_car = X_sensor (Lateral), Y_car = -Z_sensor (Longitudinal), Z_car = +Y_sensor (Vertical)
  out->ax = raw_ax;
  out->ay = (int16_t)(-raw_az);
  out->az = raw_ay;
  out->gx = raw_gx;
  out->gy = (int16_t)(-raw_gz);
  out->gz = raw_gy;
  out->temp = raw_temp;
  return 1;
}

// Accumulates one 5 ms raw sample into 32-bit running sums (avoids premature rounding loss)
void MPU6050_Accumulate(MPU6050_Accum *acc)
{
  if (acc->n == 255)
    return; // Keeps sums safely within int32_t even if a 200 ms tick is delayed

  MPU6050_Raw r;
  if (!MPU6050_Read_Raw(&r))
    return; // I2C failure: skip this sample instead of adding garbage

  acc->ax += r.ax;
  acc->ay += r.ay;
  acc->az += r.az;
  acc->gx += r.gx;
  acc->gy += r.gy;
  acc->gz += r.gz;
  acc->temp += r.temp;
  acc->n++;
}

// Computes round((sum * num) / (den * n)) in exact integer math (round half away from zero)
static int32_t scale_avg(int32_t sum, uint8_t n, int32_t num, int32_t den)
{
  int32_t numer = sum * num;
  int32_t denom = den * (int32_t)n;
  if (numer >= 0)
    return (numer + denom / 2) / denom;
  return -((-numer + denom / 2) / denom);
}

// Converts 200 ms cycle sums to real SI units using the actual number of collected samples
void MPU6050_Average_SI(const MPU6050_Accum *acc, MPU6050_SI_Data *out)
{
  uint8_t n = acc->n;
  if (n == 0)
    return; // No new samples: keep previous valid readings

  // Acceleration: 4096 LSB/g -> mg = raw * 1000 / 4096 = raw * 125 / 512
  out->ax = (int16_t)scale_avg(acc->ax, n, 125, 512);
  out->ay = (int16_t)scale_avg(acc->ay, n, 125, 512);
  out->az = (int16_t)scale_avg(acc->az, n, 125, 512);

  // Gyroscope: 32.8 LSB/(deg/s) -> (deg/s) * 100 = raw * 100 / 32.8 = raw * 125 / 41
  out->gx = scale_avg(acc->gx, n, 125, 41);
  out->gy = scale_avg(acc->gy, n, 125, 41);
  out->gz = scale_avg(acc->gz, n, 125, 41);

  // MPU6050 internal silicon die temperature (datasheet): T(°C) = raw / 340 + 36.53 -> T*10 = raw/34 + 365
  out->temp_x10 = scale_avg(acc->temp, n, 1, 34) + 365;
}

// ============================================================================
// ANALOG-TO-DIGITAL CONVERTER (ADC) AND BATTERY
// ============================================================================

// Initializes ADC with 125 kHz clock (16 MHz / 128) and locks onto Internal 1.1V ref (ADC0)
void adc_init(void)
{
  ADMUX = (1 << REFS1) | (1 << REFS0);                              // Channel ADC0, Internal 1.1V Reference
  ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); // Enable ADC, Prescaler = 128 (125 kHz)
  ADCSRA |= (1 << ADSC);                                            // First dummy conversion (discarded by hardware)
  while (ADCSRA & (1 << ADSC))
    ;
}

// Performs a single 10-bit conversion (~104 µs) on the currently selected ADMUX channel/reference
uint16_t adc_read(void)
{
  ADCSRA |= (1 << ADSC);
  while (ADCSRA & (1 << ADSC))
    ;
  return ADC;
}

// Maps ADC1 average (AVcc = 5000 mV ref) to 4 Li-Ion battery levels (25, 50, 75, 100%)
uint8_t get_battery_level(uint16_t adc_avg_bat)
{
  // Add 190 mV to compensate for internal ESR / load voltage drop
  uint16_t bat_mv = (uint16_t)(((uint32_t)adc_avg_bat * 5000UL + 512UL) / 1024UL) + BAT_OFFSET_MV;

  if (bat_mv < 3300)
    return 25;  // < 3.30V
  else if (bat_mv < 3600)
    return 50;  // 3.30V - 3.59V
  else if (bat_mv < 3900)
    return 75;  // 3.60V - 3.89V
  else
    return 100; // >= 3.90V
}

// Listens at the currently configured UART0 baud rate for up to 1300 ms for a valid NMEA sentence
uint8_t detect_gps_nmea(void)
{
  char line[100];
  uint8_t pos = 0;

  cli();
  rx_head = 0;
  rx_tail = 0;
  flag_gps_wait_done = 0;
  gps_wait_ms = 1300;
  sei();

  while (!flag_gps_wait_done)
  {
    if (uart_available())
    {
      char c = uart_receive();
      if (c == '\n' || c == '\r')
      {
        line[pos] = '\0';
        if (pos > 6 && line[0] == '$' && nmea_checksum_ok(line))
        {
          return 1; // Valid NMEA sentence received at active baud rate
        }
        pos = 0;
      }
      else if (pos < sizeof(line) - 1)
      {
        line[pos++] = c;
      }
    }
  }
  return 0;
}

// Sends a UBX command and waits up to 400 ms for UBX-ACK-ACK (1) or UBX-ACK-NAK (2); returns 0 on timeout
uint8_t send_ubx_wait_ack(const uint8_t *data, size_t len, uint8_t cls_id, uint8_t msg_id, uint8_t use_exact_bb)
{
  cli();
  rx_head = 0;
  rx_tail = 0;
  sei();

  send_ubx_bytes(data, len, use_exact_bb);

  cli();
  flag_gps_wait_done = 0;
  gps_wait_ms = 400;
  sei();

  uint8_t state = 0;
  uint8_t ack_type = 0;
  uint8_t payload_cls = 0;
  uint8_t payload_id = 0;
  uint8_t ckA = 0, ckB = 0;
  uint8_t rx_ckA = 0;

  while (!flag_gps_wait_done)
  {
    if (!uart_available())
      continue;

    uint8_t b = (uint8_t)uart_receive();

    switch (state)
    {
      case 0: if (b == 0xB5) state = 1; break;
      case 1: state = (b == 0x62) ? 2 : (b == 0xB5 ? 1 : 0); break;
      case 2:
        if (b == 0x05) { ckA = b; ckB = ckA; state = 3; }
        else { state = (b == 0xB5) ? 1 : 0; }
        break;
      case 3:
        if (b == 0x01 || b == 0x00) { ack_type = b; ckA += b; ckB += ckA; state = 4; }
        else { state = (b == 0xB5) ? 1 : 0; }
        break;
      case 4:
        if (b == 0x02) { ckA += b; ckB += ckA; state = 5; }
        else { state = 0; }
        break;
      case 5:
        if (b == 0x00) { ckA += b; ckB += ckA; state = 6; }
        else { state = 0; }
        break;
      case 6: payload_cls = b; ckA += b; ckB += ckA; state = 7; break;
      case 7: payload_id = b; ckA += b; ckB += ckA; state = 8; break;
      case 8: rx_ckA = b; state = 9; break;
      case 9:
        if (rx_ckA == ckA && b == ckB && payload_cls == cls_id && payload_id == msg_id)
        {
          wait_ms_timer(50);
          return (ack_type == 0x01) ? 1 : 2; // 1 = ACK_OK, 2 = ACK_NAK
        }
        state = 0;
        break;
      default: state = 0; break;
    }
  }
  return 0; // NO_REPLY
}

static void uart_print_ack_code(uint8_t code)
{
  if (code == 1)        uart_print_P(PSTR("ACK_OK"));
  else if (code == 2)   uart_print_P(PSTR("ACK_NAK"));
  else if (code == 255) uart_print_P(PSTR("SKIP"));
  else                  uart_print_P(PSTR("NO_REPLY"));
}

// ============================================================================
// SYSTEM INITIALIZATION
// ============================================================================
void setup_inits(void)
{
  // 1. Initialize peripherals and interrupts so Timer0 wait/detection works
  adc_init(); // ADC starts locked on channel 0 with Internal 1.1V reference
  timer0_init_1ms();
  sei();
  pwm_init();
  TWI_Init();

  // 2. Wait 1500 ms for NEO-6M cold-boot ROM initialization (also settles AREF cap to 1.1V)
  wait_ms_timer(1500);

  // 3. Detect whether NEO-6M booted at 115200 bps (e.g. previously saved) or 9600 bps default
  uart_init(); // 115200 bps + RXCIE0 enabled
  uint8_t already_115200 = detect_gps_nmea();
  uint8_t ack_clr = 255; // SKIP unless GPS started at 115200
  uint8_t at_9600 = 0;

  if (already_115200)
  {
    // Use cycle-accurate software TX (-0.08% error) to clear saved 115200 config & drop GPS to 9600
    send_ubx_bytes(UBX_SYNC_FF, sizeof(UBX_SYNC_FF), 1);
    wait_ms_timer(50);

    ack_clr = send_ubx_wait_ack(UBX_CFG_CLEAR, sizeof(UBX_CFG_CLEAR), 0x06, 0x09, 1);
    send_ubx_bytes(UBX_BAUD_9600, sizeof(UBX_BAUD_9600), 1);
    wait_ms_timer(150);

    uart_init_9600();
    wait_ms_timer(50);
    at_9600 = detect_gps_nmea();
    if (!at_9600)
    {
      uart_init(); // GPS stayed at 115200; return UART0 to 115200
      wait_ms_timer(50);
    }
  }
  else
  {
    // GPS booted at factory 9600 bps (+0.16% error)
    uart_init_9600();
    wait_ms_timer(50);
    at_9600 = 1;
  }

  // 4. Configure GPS cleanly at 9600 bps (or via exact bit-bang -0.08% TX if stuck at 115200)
  uint8_t use_bb = at_9600 ? 0 : 1;
  send_ubx_bytes(UBX_SYNC_FF, sizeof(UBX_SYNC_FF), use_bb);
  wait_ms_timer(50);

  uint8_t ack_sbas = send_ubx_wait_ack(UBX_SBAS_OFF, sizeof(UBX_SBAS_OFF), 0x06, 0x16, use_bb);
  uint8_t ack_rxm  = send_ubx_wait_ack(UBX_CONTINUOUS, sizeof(UBX_CONTINUOUS), 0x06, 0x11, use_bb);
  uint8_t ack_5hz  = send_ubx_wait_ack(UBX_5HZ, sizeof(UBX_5HZ), 0x06, 0x08, use_bb);
  if (ack_5hz == 0)
  {
    wait_ms_timer(100);
    ack_5hz = send_ubx_wait_ack(UBX_5HZ, sizeof(UBX_5HZ), 0x06, 0x08, use_bb);
  }

  static const struct { const uint8_t *pkt; size_t len; } extra_cmds[] = {
      {UBX_AUTOMOTIVE, sizeof(UBX_AUTOMOTIVE)},
      {UBX_GGA_OFF, sizeof(UBX_GGA_OFF)},
      {UBX_GLL_OFF, sizeof(UBX_GLL_OFF)},
      {UBX_GSA_OFF, sizeof(UBX_GSA_OFF)},
      {UBX_GSV_OFF, sizeof(UBX_GSV_OFF)},
      {UBX_VTG_OFF, sizeof(UBX_VTG_OFF)}
  };
  for (uint8_t i = 0; i < sizeof(extra_cmds) / sizeof(extra_cmds[0]); i++)
  {
    send_ubx_bytes(extra_cmds[i].pkt, extra_cmds[i].len, use_bb);
    wait_ms_timer(50);
  }

  // 5. Switch GPS from 9600 -> 115200 bps at the very end (without UBX_SAVE_CFG so cold boot resets to 9600)
  uint8_t link_ok = 1;
  if (at_9600)
  {
    send_ubx(UBX_BAUD_115200, sizeof(UBX_BAUD_115200));
    wait_ms_timer(150);
    uart_init();
    wait_ms_timer(100);
    link_ok = detect_gps_nmea();
  }
  else
  {
    uart_init();
  }

  // Print one-line startup diagnostic so Serial Monitor / Node-RED shows full UBX recovery + ACK status
  uart_print_P(PSTR("[GPS_DIAG] init="));
  if (already_115200 && at_9600)
    uart_print_P(PSTR("115200->9600->115200"));
  else if (!already_115200)
    uart_print_P(PSTR("9600->115200"));
  else
    uart_print_P(PSTR("STUCK_115200"));

  uart_print_P(PSTR(" clr="));
  uart_print_ack_code(ack_clr);
  uart_print_P(PSTR(" sbas="));
  uart_print_ack_code(ack_sbas);
  uart_print_P(PSTR(" rxm="));
  uart_print_ack_code(ack_rxm);
  uart_print_P(PSTR(" 5hz="));
  uart_print_ack_code(ack_5hz);
  uart_print_P(PSTR(" link="));
  uart_print_P(link_ok ? PSTR("OK\r\n") : PSTR("FAIL\r\n"));

  // 6. Initialize MPU6050 inertial sensor
  MPU6050_Init();

  // 7. Flush any residual startup bytes or UBX-ACK packets from rx_ring before entering main loop
  cli();
  rx_head = 0;
  rx_tail = 0;
  sei();
}

// ============================================================================
// NMEA SENTENCE VALIDATION AND PARSING ($GPRMC / $GNRMC)
// ============================================================================
static uint8_t hex_val(char c)
{
  if (c >= '0' && c <= '9')
    return (uint8_t)(c - '0');
  if (c >= 'A' && c <= 'F')
    return (uint8_t)(c - 'A' + 10);
  if (c >= 'a' && c <= 'f')
    return (uint8_t)(c - 'a' + 10);
  return 0xFF;
}

// Validates NMEA checksum "$...*HH": XOR of all characters between '$' and '*' must equal HH.
uint8_t nmea_checksum_ok(const char *s)
{
  if (*s != '$')
    return 0;
  s++;
  uint8_t cs = 0;
  while (*s && *s != '*')
  {
    cs ^= (uint8_t)*s++;
  }
  if (*s != '*')
    return 0;
  s++;
  uint8_t hi = hex_val(s[0]);
  uint8_t lo = (hi == 0xFF) ? 0xFF : hex_val(s[1]);
  if (hi > 15 || lo > 15)
    return 0;
  return (uint8_t)((hi << 4) | lo) == cs;
}

// Extracts fields from a validated RMC sentence into temp buffers before updating active state
void parse_gps(char *buffer, char *timestamp, uint8_t ts_size, char *status, char *lat, char *ns,
               char *lon, char *ew, char *speed, char *course)
{
  uint8_t commas = 0;
  uint8_t i = 0;

  char temp_time[12] = "";
  char temp_status[2] = "V";
  char temp_lat[16] = "";
  char temp_ns[2] = "";
  char temp_lon[16] = "";
  char temp_ew[2] = "";
  char temp_speed[10] = "";
  char temp_course[8] = "";
  char temp_date[12] = "";

  // Walk sentence splitting fields by commas (standard NMEA RMC field indices)
  while (*buffer)
  {
    if (*buffer == ',')
    {
      commas++;
      i = 0;
    }
    else
    {
      if (commas == 1 && i < 11) { temp_time[i++] = *buffer; temp_time[i] = '\0'; }
      else if (commas == 2 && i < 1) { temp_status[i++] = *buffer; temp_status[i] = '\0'; }
      else if (commas == 3 && i < 15) { temp_lat[i++] = *buffer; temp_lat[i] = '\0'; }
      else if (commas == 4 && i < 1) { temp_ns[i++] = *buffer; temp_ns[i] = '\0'; }
      else if (commas == 5 && i < 15) { temp_lon[i++] = *buffer; temp_lon[i] = '\0'; }
      else if (commas == 6 && i < 1) { temp_ew[i++] = *buffer; temp_ew[i] = '\0'; }
      else if (commas == 7 && i < 9) { temp_speed[i++] = *buffer; temp_speed[i] = '\0'; }
      else if (commas == 8 && i < 7) { temp_course[i++] = *buffer; temp_course[i] = '\0'; }
      else if (commas == 9 && i < 11) { temp_date[i++] = *buffer; temp_date[i] = '\0'; }
    }
    buffer++;
  }

  status[0] = temp_status[0]; // 'A' = Active/Valid Fix | 'V' = Void/Searching for satellites
  status[1] = '\0';

  // Format timestamp with milliseconds directly from NEO-6M NMEA fields + automatic PT DST offset
  if (temp_time[0] != '\0' && temp_date[0] != '\0')
  {
    format_nmea_to_pt_iso(temp_time, temp_date, timestamp, ts_size);
  }

  if (status[0] == 'A') // Update position and speed only when GPS fix is valid
  {
    if (temp_lat[0] != '\0') { strncpy(lat, temp_lat, 15); lat[15] = '\0'; }
    if (temp_ns[0] != '\0') { ns[0] = temp_ns[0]; ns[1] = '\0'; }
    if (temp_lon[0] != '\0') { strncpy(lon, temp_lon, 15); lon[15] = '\0'; }
    if (temp_ew[0] != '\0') { ew[0] = temp_ew[0]; ew[1] = '\0'; }

    if (temp_speed[0] != '\0')
    {
      strncpy(speed, temp_speed, 9);
      speed[9] = '\0';
      format_speed_kmh(speed);
    }
    else
    {
      strncpy(speed, "0.0", 9);
      speed[9] = '\0';
    }

    strncpy(course, temp_course, 7);
    course[7] = '\0';
  }
  else
  {
    // No valid fix ('V'): clear coordinates so false positions are never plotted
    strncpy(lat, "0.0", 15);
    ns[0] = '\0';
    strncpy(lon, "0.0", 15);
    ew[0] = '\0';
    strncpy(speed, "0.0", 9);
    course[0] = '\0';
  }
}

// Parses two ASCII digits into an unsigned byte; returns 1 on success, 0 if non-digit
static uint8_t parse_two_digits(const char *p, uint8_t *out)
{
  if (p[0] < '0' || p[0] > '9' || p[1] < '0' || p[1] > '9')
    return 0;
  *out = (uint8_t)((p[0] - '0') * 10U + (p[1] - '0'));
  return 1;
}

// Returns the number of days in a given Gregorian calendar month (handles leap years)
uint8_t days_in_month(uint16_t year, uint8_t month)
{
  static const uint8_t dim[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  if (month < 1 || month > 12)
    return 31;
  uint8_t d = dim[month - 1];
  if (month == 2 && (((year % 4U == 0U) && (year % 100U != 0U)) || (year % 400U == 0U)))
  {
    d = 29;
  }
  return d;
}

// Tomohiko Sakamoto's day-of-week algorithm (0 = Sunday .. 6 = Saturday)
uint8_t day_of_week(uint16_t year, uint8_t month, uint8_t day)
{
  static const uint8_t t[12] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
  if (month < 3U)
  {
    year--;
  }
  return (uint8_t)((year + year / 4U - year / 100U + year / 400U + t[month - 1U] + day) % 7U);
}

// Returns 1 during Continental Portugal Daylight Saving Time (WEST = UTC+1),
// 0 during Winter Time (WET = UTC+0). EU/PT rule: last Sunday of March 01:00 UTC -> last Sunday of October 01:00 UTC.
uint8_t is_portugal_dst_utc(uint16_t year, uint8_t month, uint8_t day, uint8_t hour)
{
  if (month < 3U || month > 10U)
    return 0; // Jan, Feb, Nov, Dec -> WET (UTC+0)
  if (month > 3U && month < 10U)
    return 1; // Apr .. Sep -> WEST (UTC+1)

  if (month == 3U)
  {
    uint8_t last_sun = (uint8_t)(31U - day_of_week(year, 3U, 31U));
    if (day > last_sun)
      return 1;
    if (day < last_sun)
      return 0;
    return (hour >= 1U) ? 1U : 0U;
  }

  // month == 10
  uint8_t last_sun = (uint8_t)(31U - day_of_week(year, 10U, 31U));
  if (day < last_sun)
    return 1;
  if (day > last_sun)
    return 0;
  return (hour < 1U) ? 1U : 0U;
}

// Converts NEO-6M RMC fields ("HHMMSS.sss" + "DDMMYY" UTC) directly into Portugal local "YYYY-MM-DD HH:MM:SS.mmm"
void format_nmea_to_pt_iso(const char *utc_time_str, const char *utc_date_str, char *out_buf, uint8_t out_size)
{
  if (strlen(utc_time_str) < 6U || strlen(utc_date_str) < 6U)
    return;

  uint8_t hour, min, sec, day, month, yy;
  if (!parse_two_digits(&utc_time_str[0], &hour) ||
      !parse_two_digits(&utc_time_str[2], &min) ||
      !parse_two_digits(&utc_time_str[4], &sec) ||
      !parse_two_digits(&utc_date_str[0], &day) ||
      !parse_two_digits(&utc_date_str[2], &month) ||
      !parse_two_digits(&utc_date_str[4], &yy))
  {
    return;
  }

  // Reject pre-fix cold-start GPS dates (e.g. NEO-6M default 1980-01-06) or out-of-range values
  if (hour > 23U || min > 59U || sec > 59U || yy < 24U || yy > 79U)
    return;

  uint16_t year = (uint16_t)(2000U + yy);
  if (month < 1U || month > 12U || day < 1U || day > days_in_month(year, month))
    return;

  uint16_t ms = 0;
  if (utc_time_str[6] == '.')
  {
    if (utc_time_str[7] >= '0' && utc_time_str[7] <= '9')
    {
      ms += (uint16_t)(utc_time_str[7] - '0') * 100U;
      if (utc_time_str[8] >= '0' && utc_time_str[8] <= '9')
      {
        ms += (uint16_t)(utc_time_str[8] - '0') * 10U;
        if (utc_time_str[9] >= '0' && utc_time_str[9] <= '9')
        {
          ms += (uint16_t)(utc_time_str[9] - '0');
        }
      }
    }
  }

  // Automatic Portugal UTC offset (UTC+1 in WEST, UTC+0 in WET) with midnight rollover
  if (is_portugal_dst_utc(year, month, day, hour))
  {
    hour++;
    if (hour >= 24U)
    {
      hour = 0;
      day++;
      if (day > days_in_month(year, month))
      {
        day = 1;
        month++;
        if (month > 12U)
        {
          month = 1;
          year++;
        }
      }
    }
  }

  snprintf_P(out_buf, out_size, PSTR("%04u-%02u-%02u %02u:%02u:%02u.%03u"),
             year, (unsigned)month, (unsigned)day,
             (unsigned)hour, (unsigned)min, (unsigned)sec, ms);
}

// Converts NMEA speed from knots to km/h using integer math (1 knot = 1.852 km/h), with < 1.2 km/h deadband
void format_speed_kmh(char *speed)
{
  if (speed[0] == '\0' || speed[0] == '-')
    return;

  uint32_t raw_val = 0;
  uint8_t has_decimal = 0;
  uint8_t decimals_read = 0;
  char *p = speed;

  while (*p)
  {
    if (*p == '.')
    {
      has_decimal = 1;
    }
    else if (*p >= '0' && *p <= '9')
    {
      if (!has_decimal)
      {
        raw_val = (raw_val * 10) + (*p - '0');
      }
      else if (decimals_read < 1)
      {
        raw_val = (raw_val * 10) + (*p - '0');
        decimals_read++;
      }
    }
    p++;
  }

  if (!has_decimal || decimals_read == 0)
  {
    raw_val *= 10; // Ensure fixed x10 scaling (tenths of a knot)
  }

  uint32_t kmh_x10 = (raw_val * 1852UL) / 1000UL;

  if (kmh_x10 < 12) // Filter stationary GPS drift noise below 1.2 km/h
  {
    snprintf(speed, 10, "0.0");
  }
  else
  {
    snprintf(speed, 10, "%u.%u", (uint16_t)(kmh_x10 / 10), (uint16_t)(kmh_x10 % 10));
  }
}

// ============================================================================
// JSON REPORT BUILDER AND PWM FAN CONTROL
// ============================================================================
#define SGN(v) ((v) < 0 ? "-" : "")

// Builds the JSON packet into tx_buf using a Flash format string (snprintf_P) and starts async TX
void print_status_report(uint16_t counter, const char *timestamp, const char *status,
                         const char *lat, const char *ns, const char *lon, const char *ew,
                         const char *speed, const char *course, uint8_t gps_seq,
                         int16_t temp_disp_x10, uint8_t battery_life,
                         const MPU6050_SI_Data *imu)
{
  if (UCSR0B & (1 << UDRIE0))
    return; // Previous transmission still active: skip to avoid corrupting tx_buf

  uint16_t abs_temp_x10 = (uint16_t)abs(temp_disp_x10);
  uint16_t temp_int = abs_temp_x10 / 10;
  uint16_t temp_dec = abs_temp_x10 % 10;

  int str_len = snprintf_P(tx_buf, sizeof(tx_buf),
                           PSTR("{\"counter\":%u, \"timestamp\":\"%s\", \"status\":\"%s\", "
                                "\"lat\":\"%s\", \"ns\":\"%s\", \"lon\":\"%s\", \"ew\":\"%s\", \"speed\":\"%s\", "
                                "\"course\":\"%s\", \"gps\":%u, "
                                "\"temp\":%s%u.%u, \"battery_life\":%u, "
                                "\"accelX\":%d, \"accelY\":%d, \"accelZ\":%d, "
                                "\"gyroX\":%s%ld.%02u, \"gyroY\":%s%ld.%02u, \"gyroZ\":%s%ld.%02u}\n"),
                           counter,
                           timestamp,
                           status,
                           lat,
                           ns,
                           lon,
                           ew,
                           speed,
                           course,
                           (unsigned)gps_seq,
                           SGN(temp_disp_x10), temp_int, temp_dec,
                           (unsigned)battery_life,
                           imu->ax, imu->ay, imu->az,
                           SGN(imu->gx), (long)(labs(imu->gx) / 100), (unsigned)(labs(imu->gx) % 100),
                           SGN(imu->gy), (long)(labs(imu->gy) / 100), (unsigned)(labs(imu->gy) % 100),
                           SGN(imu->gz), (long)(labs(imu->gz) / 100), (unsigned)(labs(imu->gz) % 100));

  if (str_len > 0)
  {
    tx_len = (str_len < (int)sizeof(tx_buf)) ? (uint16_t)str_len : (uint16_t)(sizeof(tx_buf) - 1);
    tx_pos = 0;
    UCSR0B |= (1 << UDRIE0); // Enable UDRE interrupt: bytes shift out automatically in background
  }
}

// Controls PWM cooling fan with hysteresis (turns ON >= 20°C, turns OFF < 19°C) to avoid rapid cycling
void adjust_pwm_fan(int16_t temperature)
{
  static uint8_t fan_running = 0;

  if (temperature < 19 || (!fan_running && temperature < 20))
  {
    OCR1B = 0;
    fan_running = 0;
    return;
  }

  if (fan_running == 0)
  {
    OCR1B = 1023; // 100% kick-start pulse for 200 ms to overcome rotor stiction
    fan_running = 1;
    return;
  }

  if (temperature < 22)      { OCR1B = 200; } // ~19.5% minimum spin
  else if (temperature < 24) { OCR1B = 282; } // ~27.5%
  else if (temperature < 26) { OCR1B = 364; } // ~35.5%
  else if (temperature < 28) { OCR1B = 446; } // ~43.5%
  else if (temperature < 30) { OCR1B = 518; } // ~51.5%
  else if (temperature < 32) { OCR1B = 610; } // ~59.5%
  else if (temperature < 34) { OCR1B = 692; } // ~67.5%
  else if (temperature < 36) { OCR1B = 774; } // ~75.5%
  else if (temperature < 38) { OCR1B = 856; } // ~83.5%
  else if (temperature < 40) { OCR1B = 938; } // ~91.5%
  else                       { OCR1B = 1023; } // 100%
}

// ============================================================================
// MAIN LOOP
// ============================================================================
int main(void)
{
  setup_inits();

  MPU6050_SI_Data accel_gyro;
  MPU6050_Accum acc;
  memset(&accel_gyro, 0, sizeof(accel_gyro));
  memset(&acc, 0, sizeof(acc));

  // GPS state variables (retained between reports if GPS hasn't sent a new fix yet)
  char buffer[100];
  uint8_t index = 0;
  char timestamp[28] = "-";
  char status[2] = "V";
  char lat[16] = "0.0";
  char ns[2] = "";
  char lon[16] = "0.0";
  char ew[2] = "";
  char speed[10] = "0.0";
  char course[8] = "";
  uint8_t gps_seq = 0;
  uint16_t counter = 0;

  // ADC state machine variables (decouples battery reads so AREF 1.1V stays undisturbed)
  uint8_t adc_state = ADC_STATE_TEMP;
  uint8_t adc_ticks = 0;
  uint8_t bat_report_div = BAT_READ_EVERY_REPORTS - 1; // Triggers first battery read right after first 200 ms report
  uint32_t adc0_accum = 0;
  uint8_t adc0_count = 0;
  uint32_t adc1_accum = 0;
  uint8_t adc1_count = 0;
  uint8_t battery_life = 100;

  // Temperature EMA low-pass filter state in hundredths of °C (100 = 1.00 °C)
  int32_t last_adc_temp_x100 = 2500;
  uint8_t has_adc_temp = 0;
  int32_t temp_ema_acc = 0; // Scaled x16 accumulator to eliminate integer truncation deadband
  uint8_t temp_filt_init = 0;

  uart_print_P(PSTR("Starting GPS and Stabilized Temp Test...\r\n"));

  flag_5ms = 0;
  flag_200ms = 0;

  while (1)
  {
    // ------------------------------------------------------------------------
    // 1. CONTINUOUS NON-BLOCKING GPS BYTE PROCESSING
    // ------------------------------------------------------------------------
    while (uart_available())
    {
      char c = uart_receive();

      if (c == '\n' || c == '\r')
      {
        buffer[index] = '\0';

        // Accept only complete $GPRMC or $GNRMC sentences with a valid checksum
        if (index > 6 && buffer[0] == '$' && buffer[1] == 'G' &&
            (buffer[2] == 'P' || buffer[2] == 'N') &&
            strncmp(&buffer[3], "RMC", 3) == 0 &&
            nmea_checksum_ok(buffer))
        {
          parse_gps(buffer, timestamp, (uint8_t)sizeof(timestamp), status, lat, ns, lon, ew, speed, course);
          gps_seq++;
        }
        index = 0;
      }
      else if (index < sizeof(buffer) - 1)
      {
        buffer[index++] = c;
      }
    }

    // ------------------------------------------------------------------------
    // 2. FAST 5 ms TICK (200 Hz): IMU SAMPLING + ADC STATE MACHINE
    // ------------------------------------------------------------------------
    if (flag_5ms == 1)
    {
      flag_5ms = 0;

      // Accumulate 1 IMU sample (40-sample average = temporal anti-aliasing filter)
      MPU6050_Accumulate(&acc);

      if (adc_state == ADC_STATE_TEMP)
      {
        // ADC0 is steady at 1.1V Internal: sample LM35 every 25 ms (8 samples per 200 ms)
        if (++adc_ticks >= ADC_TEMP_TICK_DIV)
        {
          adc_ticks = 0;
          adc0_accum += adc_read();
          adc0_count++;
        }
      }
      else if (adc_state == ADC_STATE_BAT)
      {
        // ADC1 is on AVcc 5.0V: collect 8 samples over 40 ms then switch right back to 1.1V
        adc1_accum += adc_read();
        if (++adc1_count >= ADC_BAT_SAMPLES)
        {
          uint16_t adc1_avg = (uint16_t)(adc1_accum / adc1_count);
          battery_life = get_battery_level(adc1_avg);

          // Return to ADC0 with Internal 1.1V reference and enter AREF settling window
          ADMUX = (1 << REFS1) | (1 << REFS0);
          (void)adc_read(); // Discard 1st conversion after Vref switch (per datasheet)
          adc_ticks = 0;
          adc_state = ADC_STATE_SETTLE;
        }
      }
      else // ADC_STATE_SETTLE
      {
        // Perform dummy reads every 5 ms for 160 ms while the AREF cap discharges from 5V -> 1.1V
        (void)adc_read();
        if (++adc_ticks >= AREF_SETTLE_TICKS)
        {
          adc0_accum = 0;
          adc0_count = 0;
          adc_ticks = 0;
          adc_state = ADC_STATE_TEMP;
        }
      }
    }

    // ------------------------------------------------------------------------
    // 3. 200 ms REPORT TICK (5 Hz): AVERAGING, TEMP FILTERING AND JSON TX
    // ------------------------------------------------------------------------
    if (flag_200ms == 1)
    {
      flag_200ms = 0;
      counter++;

      // Compute physical SI averages for IMU and reset accumulator for the next 200 ms
      MPU6050_Average_SI(&acc, &accel_gyro);
      memset(&acc, 0, sizeof(acc));

      // Update LM35 reading only if clean samples were collected (outside battery/AREF settle window)
      if (adc0_count > 0)
      {
        uint16_t adc0_avg = (uint16_t)(adc0_accum / adc0_count);
        // LM35 = 10 mV/°C; Ref = 1100 mV -> T(x100) = (adc0_avg * 11000) / 1024
        last_adc_temp_x100 = (int32_t)(((uint32_t)adc0_avg * 11000UL + 512UL) / 1024UL);
        has_adc_temp = 1;
        adc0_accum = 0;
        adc0_count = 0;
      }

      // Thermal fusion (50% LM35 + 50% MPU6050); falls back to MPU6050 until first LM35 read
      int32_t inst_temp_x100 = has_adc_temp
                                   ? (last_adc_temp_x100 + (int32_t)accel_gyro.temp_x10 * 10L) / 2
                                   : ((int32_t)accel_gyro.temp_x10 * 10L);

      // Exponential Moving Average (EMA, alpha = 1/16 at 5 Hz -> time constant tau ~ 3.0 s).
      // Smooths residual jitter to a rock-steady 0.1 °C while preserving real thermal response.
      if (!temp_filt_init)
      {
        temp_ema_acc = inst_temp_x100 * 16L;
        temp_filt_init = 1;
      }
      else
      {
        int32_t current_filt = (temp_ema_acc >= 0) ? (temp_ema_acc + 8) / 16 : (temp_ema_acc - 8) / 16;
        temp_ema_acc = temp_ema_acc - current_filt + inst_temp_x100;
      }

      // Convert filtered hundredths of °C to rounded tenths of °C (e.g. 254 = 25.4 °C)
      int32_t filt_x100 = (temp_ema_acc >= 0) ? (temp_ema_acc + 8) / 16 : (temp_ema_acc - 8) / 16;
      int16_t temp_disp_x10 = (int16_t)((filt_x100 >= 0) ? (filt_x100 + 5) / 10 : (filt_x100 - 5) / 10);

      // Trigger periodic battery measurement only once every 5.0 s (25 reports) while in TEMP state
      if (adc_state == ADC_STATE_TEMP && ++bat_report_div >= BAT_READ_EVERY_REPORTS)
      {
        bat_report_div = 0;
        ADMUX = (1 << REFS0) | (1 << MUX0); // Select ADC1 with AVcc (5.0V) reference
        (void)adc_read();                   // Discard 1st conversion after channel/reference switch
        adc1_accum = 0;
        adc1_count = 0;
        adc_state = ADC_STATE_BAT;
      }

      // Update cooling fan with stabilized integer temperature and start async JSON TX
      adjust_pwm_fan(temp_disp_x10 / 10);
      print_status_report(counter, timestamp, status, lat, ns, lon, ew, speed, course, gps_seq,
                          temp_disp_x10, battery_life, &accel_gyro);
    }
  }
  return 0;
}
