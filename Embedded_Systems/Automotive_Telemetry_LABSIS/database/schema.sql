-- =============================================================================
-- LABSIS Automotive Telemetry — SQLite Database Schema
-- =============================================================================
-- Creates all 5 relational tables used by Node-RED:
--   car_catalog (25,374 reference vehicles) -> vehicles -> sessions -> telemetry_points (5 Hz)
--   drivers -> sessions
-- =============================================================================

PRAGMA foreign_keys = ON;

CREATE TABLE IF NOT EXISTS drivers (
    name  TEXT UNIQUE PRIMARY KEY,
    email TEXT
);

CREATE TABLE IF NOT EXISTS car_catalog (
    id               INTEGER PRIMARY KEY AUTOINCREMENT,
    make             TEXT NOT NULL,
    model            TEXT NOT NULL,
    serie            TEXT,
    body_style       TEXT,
    segment          TEXT,
    year_start       INTEGER,
    year_end         INTEGER,
    production_years TEXT,
    cylinders        TEXT,
    displacement_cc  INTEGER,
    power_hp         REAL,
    power_rpm        INTEGER,
    power_kw         REAL,
    torque_nm        REAL,
    torque_rpm       INTEGER,
    fuel_type        TEXT,
    drivetrain       TEXT,
    gearbox          TEXT,
    top_speed_kmh    INTEGER,
    accel_0_100_s    REAL,
    weight_kg        INTEGER,
    gross_weight_kg  INTEGER,
    drag_cd          REAL,
    tire_size        TEXT,
    spec_summary     TEXT
);

CREATE TABLE IF NOT EXISTS vehicles (
    name         TEXT UNIQUE PRIMARY KEY,
    catalog_id   INTEGER REFERENCES car_catalog(id),
    weight_kg    INTEGER,
    power_hp     REAL,
    drivetrain   TEXT,
    spec_summary TEXT
);

CREATE TABLE IF NOT EXISTS sessions (
    id                  TEXT PRIMARY KEY,
    driver              TEXT REFERENCES drivers(name),
    vehicle             TEXT REFERENCES vehicles(name),
    start_time          DATETIME,
    end_time            DATETIME,
    top_speed           REAL,
    peak_g              REAL,
    points_count        INTEGER DEFAULT 0,
    duration_sec        INTEGER,
    efficiency_score    INTEGER,
    gross_mass_kg       REAL,
    payload_kg          REAL,
    curb_weight_kg      REAL,
    pilot_weight_kg     REAL,
    passenger_weight_kg REAL
);

CREATE TABLE IF NOT EXISTS telemetry_points (
    id         INTEGER PRIMARY KEY AUTOINCREMENT,
    session_id TEXT REFERENCES sessions(id),
    timestamp  DATETIME,
    speed      REAL,
    temp       REAL,
    hum        REAL,
    lat        REAL,
    lon        REAL,
    accelX     REAL,
    accelY     REAL,
    accelZ     REAL,
    gyroX      REAL,
    gyroY      REAL,
    gyroZ      REAL
);

CREATE INDEX IF NOT EXISTS idx_telemetry_session ON telemetry_points(session_id, timestamp);
CREATE INDEX IF NOT EXISTS idx_catalog_make_model ON car_catalog(make, model);
