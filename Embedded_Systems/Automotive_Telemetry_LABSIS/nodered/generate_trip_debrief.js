/**
 * LABSIS TELEMETRY v2.2 - EXECUTIVE DRIVER TRIP DEBRIEF & SIMULATOR CSV ENGINE
 * Architecture v2.2.0:
 * - Configurable paths & zero module-load side-effects (pure functions exported for node:test)
 * - 3D Initial Stationary Tare (Accel + Gyro Bias) & Gap-Aware Road-Grade Tracking
 * - Continuous Asymmetric Kamm Friction Circle with Tire-Width, Segment & Payload Adaptation
 * - Explicit Category Mapping (zero multi-pillar double counting) & Excess-Ratio Tie-Breaking
 * - Temporal Clustering (1.0s accumulated dt debounce, gap-safe)
 * - Resilient OSM Tile Fetching (timeouts + concurrency limit) & Nominatim 1.1s Rate-Limited Disk Cache
 * - Secure Execution (sessionId validation, execFileSync, full HTML escaping, fresh PDF check)
 * - Simulator-Ready RFC 4180 CSV Export (no temp/hum/incident_reason; includes full weights & kinematics)
 */

const fs = require('fs');
const os = require('os');
const path = require('path');
const https = require('https');
const { spawn } = require('child_process');
const { DatabaseSync } = require('node:sqlite');

const ALGORITHM_VERSION = '2.2.0';
const GRAVITY_MS2 = 9.80665;

const getDbPath = () => process.env.LABSIS_DB_PATH || 'C:/Users/danie/labsis_telemetry/labsis_telemetry.db';
const getReportsDir = () => process.env.LABSIS_REPORTS_DIR || 'C:/Users/danie/labsis_telemetry/reports';
const getCsvDir = () => process.env.LABSIS_CSV_DIR || 'C:/Users/danie/labsis_telemetry/csv';
const getEdgePath = () => process.env.LABSIS_EDGE_PATH || 'C:/Program Files (x86)/Microsoft/Edge/Application/msedge.exe';
const getGeocodeCachePath = () => process.env.LABSIS_GEOCODE_CACHE_PATH || 'C:/Users/danie/labsis_telemetry/geocode_cache.json';

/**
 * Escapes HTML special characters to prevent injection/rendering glitches from DB or network strings.
 */
function escapeHtml(value) {
    if (value === undefined || value === null) return '';
    return String(value)
        .replace(/&/g, '&amp;')
        .replace(/</g, '&lt;')
        .replace(/>/g, '&gt;')
        .replace(/"/g, '&quot;')
        .replace(/'/g, '&#39;');
}

/**
 * Escapes an individual RFC 4180 CSV cell value.
 */
function escapeCsvField(value) {
    if (value === undefined || value === null) return '';
    const str = String(value);
    if (/[",\r\n]/.test(str)) {
        return `"${str.replace(/"/g, '""')}"`;
    }
    return str;
}

/**
 * Parses epoch milliseconds or ISO-8601 date/time strings (with optional space separator and milliseconds).
 * Returns NaN if unparseable (e.g. legacy "HH:MM:SS DD/MM/YY" without full year/ISO order).
 */
function parseTimestampMs(ts) {
    if (typeof ts === 'number') {
        return Number.isFinite(ts) && ts > 0 ? ts : NaN;
    }
    if (typeof ts !== 'string') return NaN;
    const trimmed = ts.trim();
    if (!trimmed) return NaN;

    if (/^\d{10,16}$/.test(trimmed)) {
        const num = Number(trimmed);
        if (!Number.isFinite(num) || num <= 0) return NaN;
        return trimmed.length <= 10 ? num * 1000 : num;
    }

    // Require YYYY-MM-DD prefix so ambiguous slash formats don't parse unpredictably
    if (!/^\d{4}-\d{2}-\d{2}/.test(trimmed)) {
        return NaN;
    }
    const isoCandidate = trimmed.includes(' ') && !trimmed.includes('T')
        ? trimmed.replace(' ', 'T')
        : trimmed;
    const parsed = Date.parse(isoCandidate);
    return Number.isFinite(parsed) ? parsed : NaN;
}

// Exact Web Mercator world pixel coordinates (at standard 256px tile size)
function lonToWorldPx(lon, zoom) {
    return ((lon + 180.0) / 360.0) * (256.0 * Math.pow(2, zoom));
}
function latToWorldPx(lat, zoom) {
    const latClamped = Math.min(85.0511, Math.max(-85.0511, lat));
    const latRad = latClamped * Math.PI / 180.0;
    return (1.0 - Math.log(Math.tan(latRad) + 1.0 / Math.cos(latRad)) / Math.PI) / 2.0 * (256.0 * Math.pow(2, zoom));
}

// Tile fetching with in-memory cache, overall deadline timeout, and concurrency limiting
const tileCache = new Map();
function fetchTileBase64(z, x, y, timeoutMs = 4500) {
    const key = `${z}/${x}/${y}`;
    if (tileCache.has(key)) return Promise.resolve(tileCache.get(key));

    return new Promise((resolve) => {
        let settled = false;
        let req = null;
        let deadlineTimer = null;

        const finish = (val) => {
            if (settled) return;
            settled = true;
            if (deadlineTimer) clearTimeout(deadlineTimer);
            resolve(val);
        };

        const url = `https://tile.openstreetmap.org/${z}/${x}/${y}.png`;
        deadlineTimer = setTimeout(() => {
            if (req) req.destroy();
            finish(null);
        }, timeoutMs);

        req = https.get(url, {
            headers: { 'User-Agent': 'LabsisTelemetryDebrief/2.2 (isep_telemetry_project)' },
            timeout: Math.min(3000, timeoutMs)
        }, (res) => {
            if (res.statusCode !== 200) {
                res.resume();
                finish(null);
                return;
            }
            const chunks = [];
            res.on('data', c => chunks.push(c));
            res.on('error', () => finish(null));
            res.on('end', () => {
                const buf = Buffer.concat(chunks);
                const b64 = `data:image/png;base64,${buf.toString('base64')}`;
                tileCache.set(key, b64);
                finish(b64);
            });
        });

        req.on('timeout', () => {
            req.destroy();
            finish(null);
        });
        req.on('error', () => finish(null));
    });
}

async function mapWithConcurrency(items, concurrency, worker) {
    const results = new Array(items.length);
    let nextIdx = 0;
    const workers = Array.from({ length: Math.min(concurrency, items.length) }, async () => {
        while (true) {
            const idx = nextIdx++;
            if (idx >= items.length) break;
            results[idx] = await worker(items[idx], idx);
        }
    });
    await Promise.all(workers);
    return results;
}

// Reverse geocoding with persistent JSON disk cache & 1100 ms rate-limiting queue (Nominatim policy)
const geoCache = new Map();
let geoCacheLoaded = false;
let geoCacheDirty = false;
let lastGeoRequestAt = 0;
let geoQueue = Promise.resolve();

function resetGeocodeCache() {
    geoCache.clear();
    geoCacheLoaded = false;
    geoCacheDirty = false;
    lastGeoRequestAt = 0;
    geoQueue = Promise.resolve();
}

function loadGeocodeCache() {
    if (geoCacheLoaded) return;
    geoCacheLoaded = true;
    const cachePath = getGeocodeCachePath();
    try {
        if (fs.existsSync(cachePath)) {
            const parsed = JSON.parse(fs.readFileSync(cachePath, 'utf8'));
            if (parsed && typeof parsed === 'object') {
                Object.entries(parsed).forEach(([k, v]) => {
                    if (typeof v === 'string' && v.trim()) {
                        geoCache.set(k, v);
                    }
                });
            }
        }
    } catch (_err) {
        // Ignore corrupt cache files safely
    }
}

function saveGeocodeCache() {
    if (!geoCacheDirty) return;
    const cachePath = getGeocodeCachePath();
    try {
        fs.mkdirSync(path.dirname(cachePath), { recursive: true });
        fs.writeFileSync(cachePath, JSON.stringify(Object.fromEntries(geoCache.entries()), null, 2), 'utf8');
        geoCacheDirty = false;
    } catch (_err) {
        // Non-fatal cache write failure
    }
}

function formatCoordinateFallback(lat, lon) {
    const latHem = lat >= 0 ? '°N' : '°S';
    const lonHem = lon >= 0 ? '°E' : '°W';
    return `${Math.abs(lat).toFixed(4)}${latHem}, ${Math.abs(lon).toFixed(4)}${lonHem}`;
}

function reverseGeocode(lat, lon, minIntervalMs = 1100) {
    loadGeocodeCache();
    const key = `${lat.toFixed(4)},${lon.toFixed(4)}`;
    if (geoCache.has(key)) return Promise.resolve(geoCache.get(key));

    const job = geoQueue.then(async () => {
        if (geoCache.has(key)) return geoCache.get(key);

        const now = Date.now();
        const waitMs = Math.max(0, lastGeoRequestAt + minIntervalMs - now);
        if (waitMs > 0) {
            await new Promise(r => setTimeout(r, waitMs));
        }
        lastGeoRequestAt = Date.now();

        return new Promise((resolve) => {
            const fallback = formatCoordinateFallback(lat, lon);
            let settled = false;
            let req = null;
            let deadlineTimer = null;

            const finish = (val, cacheable) => {
                if (settled) return;
                settled = true;
                if (deadlineTimer) clearTimeout(deadlineTimer);
                if (cacheable && val) {
                    geoCache.set(key, val);
                    geoCacheDirty = true;
                }
                resolve(val);
            };

            const url = `https://nominatim.openstreetmap.org/reverse?lat=${lat}&lon=${lon}&format=json&accept-language=en`;
            deadlineTimer = setTimeout(() => {
                if (req) req.destroy();
                finish(fallback, false);
            }, 4000);

            req = https.get(url, {
                headers: { 'User-Agent': 'LabsisTelemetryDebrief/2.2 (isep_auto_telemetry)' },
                timeout: 2500
            }, (res) => {
                if (res.statusCode !== 200) {
                    res.resume();
                    finish(fallback, false);
                    return;
                }
                let data = '';
                res.on('data', chunk => { data += chunk; });
                res.on('error', () => finish(fallback, false));
                res.on('end', () => {
                    try {
                        const json = JSON.parse(data);
                        const addr = json.address || {};
                        const place = addr.road || addr.pedestrian || addr.suburb || addr.neighbourhood || '';
                        const city = addr.city || addr.town || addr.village || addr.municipality || addr.county || '';
                        let formatted;
                        if (place && city) {
                            formatted = `${place}, ${city}`;
                        } else if (place) {
                            formatted = place;
                        } else if (typeof json.display_name === 'string' && json.display_name.trim()) {
                            formatted = json.display_name.split(',').slice(0, 2).join(',').trim();
                        } else {
                            formatted = fallback;
                        }
                        finish(formatted, true);
                    } catch (_e) {
                        finish(fallback, false);
                    }
                });
            });

            req.on('timeout', () => {
                req.destroy();
                finish(fallback, false);
            });
            req.on('error', () => finish(fallback, false));
        });
    });

    geoQueue = job.catch(() => undefined);
    return job;
}

// Haversine distance in km
function haversineKm(lat1, lon1, lat2, lon2) {
    const R = 6371.0;
    const dLat = (lat2 - lat1) * Math.PI / 180.0;
    const dLon = (lon2 - lon1) * Math.PI / 180.0;
    const a = Math.sin(dLat / 2) * Math.sin(dLat / 2) +
              Math.cos(lat1 * Math.PI / 180.0) * Math.cos(lat2 * Math.PI / 180.0) *
              Math.sin(dLon / 2) * Math.sin(dLon / 2);
    const c = 2 * Math.atan2(Math.sqrt(a), Math.sqrt(1 - a));
    return R * c;
}

// Generate Real 2D Dark Street Map SVG by stitching genuine OpenStreetMap tiles
async function generateReal2DStreetMapSvg(points, incidents) {
    const validGps = points.filter(p => p.lat !== null && p.lon !== null && Number.isFinite(p.lat) && Number.isFinite(p.lon) && (Math.abs(p.lat) > 0.01 || Math.abs(p.lon) > 0.01));
    if (validGps.length < 2) {
        return `
        <svg width="100%" height="450" viewBox="0 0 740 450" style="background: #080f1e; border-radius: 8px; border: 1px solid #1e293b; display: block;">
            <text x="370" y="225" text-anchor="middle" fill="#64748b" font-family="'Orbitron', 'Segoe UI', sans-serif" font-size="14" font-weight="700">INSUFFICIENT GPS FIX FOR 2D TRAJECTORY RECONSTRUCTION</text>
        </svg>`;
    }

    let minLat = 90, maxLat = -90, minLon = 180, maxLon = -180;
    validGps.forEach(p => {
        if (p.lat < minLat) minLat = p.lat;
        if (p.lat > maxLat) maxLat = p.lat;
        if (p.lon < minLon) minLon = p.lon;
        if (p.lon > maxLon) maxLon = p.lon;
    });

    const svgWidth = 740;
    const svgHeight = 450;
    const targetW = 700;
    const targetH = 430;

    let zoom = 4;
    for (let z = 16; z >= 4; z--) {
        const dx = lonToWorldPx(maxLon, z) - lonToWorldPx(minLon, z);
        const dy = latToWorldPx(minLat, z) - latToWorldPx(maxLat, z);
        if (dx <= targetW && dy <= targetH) {
            zoom = z;
            break;
        }
    }

    const centerLat = (minLat + maxLat) / 2.0;
    const centerLon = (minLon + maxLon) / 2.0;
    const centerWorldX = lonToWorldPx(centerLon, zoom);
    const centerWorldY = latToWorldPx(centerLat, zoom);

    const viewOriginX = centerWorldX - svgWidth / 2.0;
    const viewOriginY = centerWorldY - svgHeight / 2.0;

    function projectToSvg(lat, lon) {
        return {
            x: lonToWorldPx(lon, zoom) - viewOriginX,
            y: latToWorldPx(lat, zoom) - viewOriginY
        };
    }

    const tileMinX = Math.floor(viewOriginX / 256.0);
    const tileMaxX = Math.floor((viewOriginX + svgWidth) / 256.0);
    const tileMinY = Math.floor(viewOriginY / 256.0);
    const tileMaxY = Math.floor((viewOriginY + svgHeight) / 256.0);

    const tileCoords = [];
    for (let x = tileMinX; x <= tileMaxX; x++) {
        for (let y = tileMinY; y <= tileMaxY; y++) {
            tileCoords.push({ x, y });
        }
    }

    // Fetch tiles with max 3 concurrent requests to comply with OSM tile server policy
    const tileResults = await mapWithConcurrency(tileCoords, 3, async (tc) => {
        const b64 = await fetchTileBase64(zoom, tc.x, tc.y);
        return { x: tc.x, y: tc.y, b64 };
    });

    let tilesSvg = '';
    tileResults.forEach(t => {
        if (!t || !t.b64) return;
        const posX = (t.x * 256.0 - viewOriginX).toFixed(2);
        const posY = (t.y * 256.0 - viewOriginY).toFixed(2);
        tilesSvg += `
        <image href="${t.b64}" x="${posX}" y="${posY}" width="257" height="257" style="filter: brightness(0.70) invert(1) contrast(1.7) hue-rotate(200deg) saturate(0.35); opacity: 1.0;" />
        `;
    });

    const step = Math.max(1, Math.floor(validGps.length / 500));
    let pathD = '';
    for (let i = 0; i < validGps.length; i += step) {
        const pt = projectToSvg(validGps[i].lat, validGps[i].lon);
        pathD += (i === 0) ? `M ${pt.x.toFixed(1)} ${pt.y.toFixed(1)}` : ` L ${pt.x.toFixed(1)} ${pt.y.toFixed(1)}`;
    }
    const lastPt = projectToSvg(validGps[validGps.length - 1].lat, validGps[validGps.length - 1].lon);
    pathD += ` L ${lastPt.x.toFixed(1)} ${lastPt.y.toFixed(1)}`;

    const startPt = projectToSvg(validGps[0].lat, validGps[0].lon);
    const finishPt = lastPt;

    let markersSvg = '';
    incidents.forEach((inc, idx) => {
        if (!inc.lat || !inc.lon) return;
        const pt = projectToSvg(inc.lat, inc.lon);
        const num = idx + 1;
        markersSvg += `
        <g transform="translate(${pt.x.toFixed(1)}, ${pt.y.toFixed(1)})">
            <circle cx="0" cy="0" r="10" fill="#f43f5e" stroke="#ffffff" stroke-width="2" style="filter: drop-shadow(0 2px 4px rgba(0,0,0,0.8));"/>
            <text x="0" y="3.5" text-anchor="middle" fill="#ffffff" font-family="'Orbitron', 'Segoe UI', sans-serif" font-size="9" font-weight="900">${num}</text>
        </g>`;
    });

    return `
    <svg width="100%" height="${svgHeight}" viewBox="0 0 ${svgWidth} ${svgHeight}" style="background: #070c18; border-radius: 8px; border: 1px solid #1e293b; display: block; overflow: hidden;">
        <!-- Base Stitched Real OSM Street Map (Conformal 1:1 Isometric Web Mercator) -->
        <g>
            ${tilesSvg}
        </g>

        <!-- GPS Trajectory Glow -->
        <path d="${pathD}" fill="none" stroke="#00f2fe" stroke-width="6" stroke-linecap="round" stroke-linejoin="round" opacity="0.3"/>
        <!-- GPS Trajectory Core -->
        <path d="${pathD}" fill="none" stroke="#38bdf8" stroke-width="2.8" stroke-linecap="round" stroke-linejoin="round" opacity="0.95"/>

        <!-- Start Marker -->
        <g transform="translate(${startPt.x.toFixed(1)}, ${startPt.y.toFixed(1)})">
            <circle cx="0" cy="0" r="7" fill="#10b981" stroke="#ffffff" stroke-width="1.8"/>
            <text x="11" y="4" fill="#10b981" font-family="'Orbitron', 'Segoe UI', sans-serif" font-size="9" font-weight="800">START</text>
        </g>

        <!-- Finish Marker -->
        <g transform="translate(${finishPt.x.toFixed(1)}, ${finishPt.y.toFixed(1)})">
            <circle cx="0" cy="0" r="7" fill="#f59e0b" stroke="#ffffff" stroke-width="1.8"/>
            <text x="11" y="4" fill="#f59e0b" font-family="'Orbitron', 'Segoe UI', sans-serif" font-size="9" font-weight="800">FINISH</text>
        </g>

        <!-- Incident Numbered Badges -->
        ${markersSvg}

        <!-- Scale & Watermark -->
        <g transform="translate(14, ${svgHeight - 16})">
            <rect x="0" y="-12" width="220" height="18" fill="rgba(7, 12, 24, 0.85)" rx="3" stroke="#1e293b" stroke-width="1"/>
            <text x="8" y="0" fill="#94a3b8" font-family="'Orbitron', 'Segoe UI', sans-serif" font-size="8" font-weight="700">ZOOM LVL ${zoom} &bull; OPENSTREETMAP 2D TILES</text>
        </g>
    </svg>
    `;
}

/**
 * Extracts average tire width in mm from catalog tire strings, supporting staggered sizes
 * (e.g. '245/40 ZR 18 // 325/30 ZR 18' -> 285 mm) as well as '185 R15 C' or 'P24570 R17'.
 */
function parseAverageTireWidthMm(tireSizeStr) {
    if (typeof tireSizeStr !== 'string' || !tireSizeStr.trim()) {
        return 205.0;
    }

    const widths = [];
    // Pattern 1: Standard slash formats (205/55 R16, 175/65TR14, 245/40 ZR 18)
    for (const m of tireSizeStr.matchAll(/(\d{3})\s*\//g)) {
        const w = Number(m[1]);
        if (Number.isFinite(w) && w >= 125 && w <= 385) widths.push(w);
    }

    // Pattern 2: Compact or non-slash formats if Pattern 1 matched nothing (e.g. 185 R15 C, P24570 R17)
    if (widths.length === 0) {
        for (const m of tireSizeStr.matchAll(/(?:^|[\s/P-])(\d{3})(?:\d{2})?\s*R\s*\d{2}/gi)) {
            const w = Number(m[1]);
            if (Number.isFinite(w) && w >= 125 && w <= 385) widths.push(w);
        }
    }

    if (widths.length === 0) {
        return 205.0;
    }
    return widths.reduce((acc, cur) => acc + cur, 0) / widths.length;
}

/**
 * Unifies vehicle specification resolution between physics computations and HTML rendering.
 * Keeps missing DB attributes clearly as 'N/A' in display while supplying safe physical fallbacks.
 */
function resolveVehicleSpecs(vehicleRow = {}, catalog = {}, session = {}) {
    const finitePos = (v, min = 0) => {
        if (v === undefined || v === null || v === '') return null;
        const n = Number(v);
        return Number.isFinite(n) && n > min ? n : null;
    };
    const finiteNonNeg = (v) => {
        if (v === undefined || v === null || v === '') return null;
        const n = Number(v);
        return Number.isFinite(n) && n >= 0 ? n : null;
    };

    const rawCatalogWeight = finitePos(catalog.weight_kg, 400);
    const rawVehicleWeight = finitePos(vehicleRow.weight_kg, 400);
    const rawSessionCurb = finitePos(session.curb_weight_kg, 400);
    const curbMassKg = rawCatalogWeight || rawVehicleWeight || rawSessionCurb || 1470.0;

    const pilotWeightKg = finiteNonNeg(session.pilot_weight_kg);
    const passengerWeightKg = finiteNonNeg(session.passenger_weight_kg);
    const sessionPayload = finiteNonNeg(session.payload_kg);
    const sessionGross = finitePos(session.gross_mass_kg, 400);

    let payloadKg = 0;
    if (sessionPayload !== null) {
        payloadKg = sessionPayload;
    } else if (pilotWeightKg !== null || passengerWeightKg !== null) {
        payloadKg = (pilotWeightKg || 0) + (passengerWeightKg || 0);
    } else if (sessionGross !== null && sessionGross >= curbMassKg) {
        payloadKg = sessionGross - curbMassKg;
    }

    const grossMassKg = (sessionGross !== null && sessionGross >= curbMassKg)
        ? sessionGross
        : (curbMassKg + payloadKg);

    const rawCatalogPower = finitePos(catalog.power_hp, 20);
    const rawVehiclePower = finitePos(vehicleRow.power_hp, 20);
    const powerHp = rawCatalogPower || rawVehiclePower || 116.0;

    const drivetrainRaw = String(catalog.drivetrain || vehicleRow.drivetrain || '').trim().toUpperCase();
    const drivetrainPhysics = drivetrainRaw || 'FWD';
    const powerToWeight = powerHp / (grossMassKg / 1000.0);

    const hasKnownWeight = Boolean(sessionGross || rawCatalogWeight || rawVehicleWeight || rawSessionCurb);
    const hasKnownPower = Boolean(rawCatalogPower || rawVehiclePower);

    const vehicleName = (session.vehicle && String(session.vehicle).trim())
        || (catalog.make && catalog.model ? `${catalog.make} ${catalog.model}` : '')
        || 'Unknown Vehicle';

    const torqueNm = finitePos(catalog.torque_nm, 0);
    const torqueRpm = catalog.torque_rpm ? String(catalog.torque_rpm).trim() : '';
    const torqueDisplay = torqueNm
        ? `${torqueNm} Nm${torqueRpm ? ` @ ${torqueRpm} RPM` : ''}`
        : 'N/A';

    const dispCc = finitePos(catalog.displacement_cc, 0);

    return {
        physics: {
            curbMassKg,
            grossMassKg,
            pilotWeightKg,
            passengerWeightKg,
            payloadKg,
            effectivePayloadKg: payloadKg > 0 ? payloadKg : Math.max(0, grossMassKg - curbMassKg),
            powerHp,
            powerToWeight,
            drivetrain: drivetrainPhysics,
            topSpeedKmh: finitePos(catalog.top_speed_kmh, 50) || 250
        },
        display: {
            vehicleName,
            drivetrain: drivetrainRaw || 'N/A',
            gearbox: (catalog.gearbox && String(catalog.gearbox).trim()) || 'N/A',
            powerHpText: hasKnownPower ? `${powerHp} HP` : 'N/A',
            massKgText: hasKnownWeight ? `${Math.round(grossMassKg)} kg` : 'N/A',
            powerToWeightText: (hasKnownPower && hasKnownWeight) ? ` (${powerToWeight.toFixed(1)} hp/t)` : '',
            displacementText: dispCc ? `${dispCc} cc` : 'N/A',
            fuelType: (catalog.fuel_type && String(catalog.fuel_type).trim()) || 'N/A',
            torqueDisplay
        }
    };
}

/**
 * Parses a commercial tire size string (e.g. "195/55 R16" or staggered "245/40 R18 / 285/35 R18")
 * and returns the nominal average outer radius in meters.
 */
function calculateTireRadius(tireSizeStr) {
    const DEFAULT_RADIUS_M = 0.316;
    if (typeof tireSizeStr !== 'string' || !tireSizeStr.trim()) {
        return DEFAULT_RADIUS_M;
    }
    const radii = [];
    for (const m of tireSizeStr.matchAll(/(\d{3})\s*\/\s*(\d{2})\s*[A-Za-z]*[Rr]\s*(\d{2})/g)) {
        const widthMm = Number(m[1]);
        const aspectRatio = Number(m[2]);
        const rimDiameterIn = Number(m[3]);
        if (widthMm >= 125 && aspectRatio >= 20 && rimDiameterIn >= 10) {
            const sidewallMm = widthMm * (aspectRatio / 100.0);
            const rimRadiusMm = (rimDiameterIn * 25.4) / 2.0;
            radii.push((rimRadiusMm + sidewallMm) / 1000.0);
        }
    }
    if (radii.length === 0) return DEFAULT_RADIUS_M;
    return radii.reduce((acc, cur) => acc + cur, 0) / radii.length;
}

/**
 * Computes continuous 3-axis Kamm friction ellipse bounds (+Y Accel, -Y Brake, +/-X Lat)
 * fusing physical power-to-mass propulsion (P/(m*v*g) + 0-100 + torque + drivetrain),
 * tire contact patch (width & outer radius), chassis segment/CG modifiers, and trip payload.
 */
function computeDynamicKammLimits(session = {}, vehicleRow = {}, catalog = {}) {
    const specs = resolveVehicleSpecs(vehicleRow, catalog, session);
    const {
        curbMassKg,
        grossMassKg,
        payloadKg,
        effectivePayloadKg,
        powerHp,
        powerToWeight: pTon,
        drivetrain
    } = specs.physics;

    const massRatio = curbMassKg / grossMassKg;
    const segment = String(catalog.segment || '').trim().toLowerCase();
    const bodyStyle = String(catalog.body_style || '').trim().toLowerCase();

    // 1. Longitudinal Traction Limit (+Y): physical powerAccelG (15 m/s) + 0-100 + torque + drivetrain
    const powerAccelG = (powerHp * 745.7) / (grossMassKg * 15.0 * GRAVITY_MS2);
    const t0100 = Number(catalog.accel_0_100_s);
    const hasValid0100 = Number.isFinite(t0100) && t0100 >= 1.8 && t0100 <= 25.0;
    const a0100 = hasValid0100
        ? (27.7778 / (t0100 * GRAVITY_MS2)) * massRatio
        : powerAccelG * 0.85;

    const isAwd = drivetrain.includes('AWD') || drivetrain.includes('4WD') || drivetrain.includes('4X4');
    const kDrive = isAwd ? 1.08 : (drivetrain.includes('RWD') ? 1.02 : 0.92);
    const torqueNm = Number(catalog.torque_nm) || 0;
    const torqueBonus = torqueNm > 0
        ? Math.min(0.06, Math.max(0.0, ((torqueNm / (grossMassKg / 1000.0)) - 120.0) / 2500.0))
        : 0.0;

    const rawAccelG = (0.18 + 0.34 * powerAccelG + 0.32 * a0100 + torqueBonus) * kDrive;
    const accelLimit = Number(Math.min(0.85, Math.max(0.30, rawAccelG)).toFixed(2));

    // 2. Tire Footprint (width + outer radius) & Chassis Class (decoupled from engine HP on normal cars)
    const avgTireWidthMm = parseAverageTireWidthMm(catalog.tire_size);
    const tireRadiusM = calculateTireRadius(catalog.tire_size);
    const kTire = Math.min(1.18, Math.max(0.90,
        Math.pow(avgTireWidthMm / 205.0, 0.32) * Math.pow(tireRadiusM / 0.316, 0.15)
    ));

    let kChassis = 1.00;
    if (segment.includes('exotic') || segment.includes('supercar')) {
        kChassis = 1.32;
    } else if (
        segment.includes('sport') ||
        segment.includes('coupe') ||
        bodyStyle.includes('roadster') ||
        bodyStyle.includes('spyder') ||
        bodyStyle.includes('spider')
    ) {
        kChassis = 1.15;
    } else if (
        segment.includes('executive') ||
        segment.includes('premium') ||
        segment.includes('luxury') ||
        segment.includes('midsize') ||
        segment.includes('large')
    ) {
        kChassis = 1.06;
    } else if (segment.includes('compact')) {
        kChassis = 1.02;
    } else if (
        segment.includes('small') ||
        segment.includes('mini') ||
        segment.includes('city') ||
        segment.includes('micro') ||
        segment.includes('kei')
    ) {
        kChassis = 0.94;
    }

    // 3. Body Style / Center-of-Gravity roll modifier
    let kBody = 1.00;
    if (
        segment.includes('exotic') ||
        segment.includes('supercar') ||
        bodyStyle.includes('coup') ||
        bodyStyle.includes('spyder') ||
        bodyStyle.includes('spider') ||
        bodyStyle.includes('roadster') ||
        segment.includes('roadster')
    ) {
        kBody = 1.04;
    } else if (
        bodyStyle.includes('suv') ||
        bodyStyle.includes('crossover') ||
        segment.includes('crossover') ||
        segment.includes('suv') ||
        segment.includes('pickup') ||
        bodyStyle.includes('truck')
    ) {
        kBody = 0.91;
    } else if (segment.includes('mpv') || bodyStyle.includes('van')) {
        kBody = 0.93;
    } else if (bodyStyle.includes('wagon') || bodyStyle.includes('estate') || bodyStyle.includes('sw')) {
        kBody = 0.97;
    }

    // Payload Center-of-Gravity & Tire Load Sensitivity Penalty
    const ratioPayload = Math.min(0.35, effectivePayloadKg / curbMassKg);
    const deltaPayload = 0.12 * ratioPayload;

    // 4. Lateral Cornering Limit (+/-X) & Longitudinal Braking Limit (-Y >= accelLimit & latLimit)
    const rawLatG = (0.47 * kTire * kChassis * kBody) - deltaPayload;
    const latLimit = Number(Math.min(0.82, Math.max(0.38, rawLatG)).toFixed(2));

    const rawBrakeG = (0.52 * kTire * kChassis) - (0.50 * deltaPayload);
    const brakeLimit = Number(Math.min(0.85, Math.max(0.42, accelLimit, latLimit, rawBrakeG)).toFixed(2));

    return {
        accelLimit,
        brakeLimit,
        latLimit,
        massKg: grossMassKg,
        powerHp,
        powerToWeight: pTon,
        diagnostics: {
            curbMassKg,
            grossMassKg,
            pilotWeightKg: specs.physics.pilotWeightKg,
            passengerWeightKg: specs.physics.passengerWeightKg,
            payloadKg,
            effectivePayloadKg,
            pTon: Number(pTon.toFixed(1)),
            avgTireWidthMm: Math.round(avgTireWidthMm),
            tireRadiusM: Number(tireRadiusM.toFixed(3)),
            kChassis: Number(kChassis.toFixed(3)),
            kTire: Number(kTire.toFixed(3)),
            kBody
        }
    };
}

/**
 * Renders the 2-line technical footer for the G-G Acceleration Ellipse card.
 */
function renderGGCardFooterHtml(peaks, limits) {
    const latViolated = peaks.peakLatG > limits.latLimit;
    const accelViolated = peaks.peakAccelG > limits.accelLimit;
    const brakeViolated = peaks.peakBrakeG > limits.brakeLimit;

    const latStyle = latViolated ? 'color: #f43f5e; font-weight: 700;' : 'color: #f8fafc; font-weight: 600;';
    const accelStyle = accelViolated ? 'color: #f43f5e; font-weight: 700;' : 'color: #f8fafc; font-weight: 600;';
    const brakeStyle = brakeViolated ? 'color: #f43f5e; font-weight: 700;' : 'color: #f8fafc; font-weight: 600;';

    return `
        <div class="gg-footer-metrics" style="display: flex; flex-direction: column; gap: 3px; font-family: 'JetBrains Mono', 'Consolas', 'DejaVu Sans Mono', monospace; font-size: 9.5px; margin-top: 6px; padding-top: 6px; border-top: 1px solid rgba(148, 163, 184, 0.2);">
            <div style="display: flex; justify-content: space-between; color: #94a3b8;">
                <span><strong style="color: #cbd5e1;">PEAKS:</strong></span>
                <span>LAT: <span style="${latStyle}">${peaks.peakLatG.toFixed(2)}G</span></span>
                <span>&bull;</span>
                <span>ACCEL (+Y): <span style="${accelStyle}">+${peaks.peakAccelG.toFixed(2)}G</span></span>
                <span>&bull;</span>
                <span>BRAKE (-Y): <span style="${brakeStyle}">-${peaks.peakBrakeG.toFixed(2)}G</span></span>
            </div>
            <div style="display: flex; justify-content: space-between; color: #64748b;">
                <span><strong style="color: #94a3b8;">LIMITS:</strong></span>
                <span>LAT: &plusmn;${limits.latLimit.toFixed(2)}G</span>
                <span>&bull;</span>
                <span>ACCEL (+Y): +${limits.accelLimit.toFixed(2)}G</span>
                <span>&bull;</span>
                <span>BRAKE (-Y): -${limits.brakeLimit.toFixed(2)}G</span>
            </div>
        </div>
    `;
}

/**
 * Generates G-G Diagram SVG with asymmetric tire envelope and clamped overflow red markers.
 */
function generateGGDiagramSvg(points, limits = {}) {
    const width = 310;
    const height = 180;
    const cx = 155;
    const cy = 90;
    const scale = 90; // 1.0 G = 90 px

    const latLimit = limits.latLimit || 0.50;
    const brakeLimit = limits.brakeLimit || 0.48;
    const accelLimit = limits.accelLimit || 0.42;

    const rx = latLimit * scale;
    const ryBrake = brakeLimit * scale;
    const ryAccel = accelLimit * scale;

    const validPts = points.filter(p => (p.speed || 0) > 5.0);
    const step = Math.max(1, Math.floor(validPts.length / 400));

    const clampX = (x) => Math.max(10, Math.min(width - 10, x));
    const clampY = (y) => Math.max(8, Math.min(height - 8, y));

    let dotsSvg = '';
    // 1. Safe driving cloud (downsampled)
    for (let i = 0; i < validPts.length; i += step) {
        const p = validPts[i];
        const gx = p.compensatedLatG !== undefined ? p.compensatedLatG : (p.accelX || 0);
        const gy = p.compensatedLonG !== undefined ? p.compensatedLonG : (p.accelY || 0);
        const px = clampX(cx + (gx * scale));
        const py = clampY(cy - (gy * scale));

        const isExceeded = Math.hypot(gx / latLimit, gy / (gy < 0 ? brakeLimit : accelLimit)) > 1.0;
        if (!isExceeded) {
            dotsSvg += `<circle cx="${px.toFixed(1)}" cy="${py.toFixed(1)}" r="1.4" fill="#06b6d4" opacity="0.45"/>`;
        }
    }

    // 2. Full-resolution unsafe points (clamped at SVG border if > 0.9 G so no peak disappears)
    for (let i = 0; i < validPts.length; i++) {
        const p = validPts[i];
        const gx = p.compensatedLatG !== undefined ? p.compensatedLatG : (p.accelX || 0);
        const gy = p.compensatedLonG !== undefined ? p.compensatedLonG : (p.accelY || 0);
        const isExceeded = Math.hypot(gx / latLimit, gy / (gy < 0 ? brakeLimit : accelLimit)) > 1.0;
        if (isExceeded) {
            const px = clampX(cx + (gx * scale));
            const py = clampY(cy - (gy * scale));
            dotsSvg += `<circle cx="${px.toFixed(1)}" cy="${py.toFixed(1)}" r="2.8" fill="#f43f5e" opacity="0.85"/>`;
        }
    }

    const asymmetricKammPath = `M ${(cx - rx).toFixed(1)} ${cy} A ${rx.toFixed(1)} ${ryAccel.toFixed(1)} 0 0 1 ${(cx + rx).toFixed(1)} ${cy} A ${rx.toFixed(1)} ${ryBrake.toFixed(1)} 0 0 1 ${(cx - rx).toFixed(1)} ${cy} Z`;

    return `
    <svg width="100%" height="${height}" viewBox="0 0 ${width} ${height}" style="background: #080f1e; border-radius: 6px; border: 1px solid #1e293b; display: block;">
        <!-- Concentric Radial Grid Rings -->
        <circle cx="${cx}" cy="${cy}" r="${(0.25 * scale).toFixed(1)}" fill="none" stroke="#1e293b" stroke-width="1" stroke-dasharray="2,2"/>
        <circle cx="${cx}" cy="${cy}" r="${(0.50 * scale).toFixed(1)}" fill="none" stroke="#334155" stroke-width="1" stroke-dasharray="3,3"/>
        <circle cx="${cx}" cy="${cy}" r="${(0.75 * scale).toFixed(1)}" fill="none" stroke="#1e293b" stroke-width="1"/>

        <!-- Axes -->
        <line x1="20" y1="${cy}" x2="${width - 20}" y2="${cy}" stroke="#1e293b" stroke-width="1.2"/>
        <line x1="${cx}" y1="12" x2="${cx}" y2="${height - 12}" stroke="#1e293b" stroke-width="1.2"/>
        <circle cx="${cx}" cy="${cy}" r="2" fill="#64748b"/>

        <!-- Kamm Traction Boundary Asymmetric Ellipse -->
        <path d="${asymmetricKammPath}" fill="rgba(6, 182, 212, 0.05)" stroke="#06b6d4" stroke-width="1.2" stroke-dasharray="3,2"/>

        <!-- Dynamic G-Force Data Points -->
        ${dotsSvg}

        <!-- Axis Labels -->
        <text x="${cx}" y="11" text-anchor="middle" font-family="'Orbitron', 'Segoe UI', sans-serif" font-size="7.5" font-weight="700" fill="#64748b">+ACCEL</text>
        <text x="${cx}" y="${height - 4}" text-anchor="middle" font-family="'Orbitron', 'Segoe UI', sans-serif" font-size="7.5" font-weight="700" fill="#64748b">-BRAKE</text>
        <text x="6" y="${cy + 3}" text-anchor="start" font-family="'Orbitron', 'Segoe UI', sans-serif" font-size="7" font-weight="700" fill="#64748b">← LEFT</text>
        <text x="${width - 6}" y="${cy + 3}" text-anchor="end" font-family="'Orbitron', 'Segoe UI', sans-serif" font-size="7" font-weight="700" fill="#64748b">RIGHT →</text>
    </svg>
    `;
}

const PILLAR_CATEGORY_MAP = {
    cornering: ['lateral'],
    braking: ['braking'],
    traction: ['slip', 'combined_brake', 'combined_traction'],
    chassis: ['bump'],
    powertrain: ['acceleration']
};

/**
 * Computes 5 Dynamic Skill Pillars using explicit incident categories (1x weight, zero double counting)
 * and distance-weighted attenuation.
 */
function computeDrivingSkillPillars(points, allIncidents, effectiveDistanceKm = 10.0) {
    // Reference distance D0 = 10 km (factor = 1.0): 5 km -> ~1.34, 100 km -> ~0.38, 500 km -> ~0.19, 2000 km -> ~0.11.
    const effDist = Math.max(3.0, effectiveDistanceKm || 10.0);
    const distFactor = Math.min(1.35, Math.max(0.08, Math.pow(10.0 / effDist, 0.42)));

    const sumByCategories = (cats, weight) => allIncidents
        .filter(i => cats.includes(i.category))
        .reduce((acc, i) => acc + (i.points * weight * distFactor), 0);

    const corneringScore = Math.max(20, Math.min(100, Math.round(100 - sumByCategories(PILLAR_CATEGORY_MAP.cornering, 2.5))));
    const brakingScore = Math.max(20, Math.min(100, Math.round(100 - sumByCategories(PILLAR_CATEGORY_MAP.braking, 2.5))));
    const slipScore = Math.max(20, Math.min(100, Math.round(100 - sumByCategories(PILLAR_CATEGORY_MAP.traction, 2.2))));
    const chassisScore = Math.max(20, Math.min(100, Math.round(100 - sumByCategories(PILLAR_CATEGORY_MAP.chassis, 3.5))));

    // Powertrain Sympathy: acceleration incidents + longitudinal drivetrain jerk across valid contiguous pairs
    let harshShifts = 0;
    for (let i = 1; i < points.length; i++) {
        const pCur = points[i];
        const pPrev = points[i - 1];
        if (pCur.isGap) continue;
        const dt = pCur.dt || 0.20;
        const curLon = pCur.compensatedLonG !== undefined ? pCur.compensatedLonG : (pCur.accelY || 0);
        const prevLon = pPrev.compensatedLonG !== undefined ? pPrev.compensatedLonG : (pPrev.accelY || 0);

        if ((pCur.speed || 0) > 12.0 && (curLon > -0.10 || prevLon > -0.10)) {
            const jerk = Math.abs(curLon - prevLon) / dt;
            if (jerk > 1.75) {
                harshShifts++;
            }
        }
    }
    const powertrainLoss = sumByCategories(PILLAR_CATEGORY_MAP.powertrain, 2.5) + Math.min(15, harshShifts * 1.5 * distFactor);
    const powertrainScore = Math.max(25, Math.min(100, Math.round(100 - powertrainLoss)));

    const getGradeInfo = (score) => {
        if (score >= 90) return { grade: 'GRADE A', color: '#10b981' };
        if (score >= 80) return { grade: 'GRADE B', color: '#38bdf8' };
        if (score >= 70) return { grade: 'GRADE C', color: '#fbbf24' };
        if (score >= 50) return { grade: 'GRADE D', color: '#f97316' };
        return { grade: 'GRADE F', color: '#f43f5e' };
    };

    return [
        { name: 'CORNERING STABILITY', desc: 'Apex load & lateral G envelope', score: corneringScore, ...getGradeInfo(corneringScore) },
        { name: 'BRAKING DISCIPLINE', desc: 'Deceleration linearity & lockup avoidance', score: brakingScore, ...getGradeInfo(brakingScore) },
        { name: 'TRACTION CONTROL', desc: 'Yaw alignment & combined grip saturation', score: slipScore, ...getGradeInfo(slipScore) },
        { name: 'CHASSIS CARE', desc: 'Vertical shock & curb dampening', score: chassisScore, ...getGradeInfo(chassisScore) },
        { name: 'POWERTRAIN SYMPATHY', desc: 'Drivetrain smoothness & progressive torque', score: powertrainScore, ...getGradeInfo(powertrainScore) }
    ];
}

/**
 * Pure Signal Processing Stage:
 * - Session-wide milli-G detection (never divides isolated ±1.0 G floats by 1000)
 * - Real dt and gap detection from millisecond timestamps
 * - Stationary Tare (Accel + Gyro Bias) without < 1.5 °/s gyroNorm lockouts
 * - Gap-aware Road-Grade tracking, heading computation, and speed-bounded distance integration
 */
function processSignals(rawPoints, opts = {}) {
    const maxPlausibleKmh = opts.maxPlausibleKmh || 260;

    // Detect legacy milli-G session globally using median(|accelZ|) or max(|ax|,|ay|)
    const zAbsVals = [];
    let maxXY = 0;
    for (const p of rawPoints) {
        const ax = Math.abs(Number(p.accelX) || 0);
        const ay = Math.abs(Number(p.accelY) || 0);
        if (ax > maxXY) maxXY = ax;
        if (ay > maxXY) maxXY = ay;
        if (p.accelZ !== undefined && p.accelZ !== null) {
            zAbsVals.push(Math.abs(Number(p.accelZ) || 0));
        }
    }
    let medianZ = 1.0;
    if (zAbsVals.length > 0) {
        zAbsVals.sort((a, b) => a - b);
        medianZ = zAbsVals[Math.floor(zAbsVals.length / 2)];
    }
    const isSessionMilliG = (medianZ > 100 || maxXY > 20);
    const accelScale = isSessionMilliG ? 1000.0 : 1.0;

    const points = rawPoints.map((p, idx) => {
        const latNum = (p.lat !== undefined && p.lat !== null && p.lat !== '') ? Number(p.lat) : NaN;
        const lonNum = (p.lon !== undefined && p.lon !== null && p.lon !== '') ? Number(p.lon) : NaN;
        const validLat = Number.isFinite(latNum) && Math.abs(latNum) > 0.001 ? latNum : null;
        const validLon = Number.isFinite(lonNum) && Math.abs(lonNum) > 0.001 ? lonNum : null;

        const rawZ = (p.accelZ !== undefined && p.accelZ !== null) ? Number(p.accelZ) : (isSessionMilliG ? 1000.0 : 1.0);
        const timestampMs = parseTimestampMs(p.timestamp);

        return {
            ...p,
            sampleIndex: idx,
            timestampMs,
            lat: (validLat !== null && validLon !== null) ? validLat : null,
            lon: (validLat !== null && validLon !== null) ? validLon : null,
            speed: Number(p.speed) || 0,
            accelX: (Number(p.accelX) || 0) / accelScale,
            accelY: (Number(p.accelY) || 0) / accelScale,
            accelZ: (Number.isFinite(rawZ) ? rawZ : (isSessionMilliG ? 1000.0 : 1.0)) / accelScale,
            gyroX: Number(p.gyroX) || 0,
            gyroY: Number(p.gyroY) || 0,
            gyroZ: Number(p.gyroZ) || 0,
            temp: (p.temp !== undefined && p.temp !== null && p.temp !== '') ? Number(p.temp) : NaN,
            hum: (p.hum !== undefined && p.hum !== null && p.hum !== '') ? Number(p.hum) : NaN
        };
    });

    // 1. Stationary Tare (First 25 valid stationary samples with consecutive gyro stability)
    let sumAx = 0, sumAy = 0, sumGx = 0, sumGy = 0, sumGz = 0, tareCount = 0;
    let prevStatGx = null, prevStatGy = null, prevStatGz = null;

    for (let i = 0; i < points.length; i++) {
        const p = points[i];
        const gNorm = Math.hypot(p.accelX, p.accelY, p.accelZ);
        if (p.speed < 0.8 && Math.abs(p.accelX) < 0.35 && Math.abs(p.accelY) < 0.35 && gNorm >= 0.85 && gNorm <= 1.15) {
            const gyroStable = (prevStatGx === null) ||
                (Math.abs(p.gyroX - prevStatGx) < 0.5 && Math.abs(p.gyroY - prevStatGy) < 0.5 && Math.abs(p.gyroZ - prevStatGz) < 0.5);
            prevStatGx = p.gyroX;
            prevStatGy = p.gyroY;
            prevStatGz = p.gyroZ;

            if (gyroStable) {
                sumAx += p.accelX;
                sumAy += p.accelY;
                sumGx += p.gyroX;
                sumGy += p.gyroY;
                sumGz += p.gyroZ;
                tareCount++;
                if (tareCount >= 25) break;
            }
        } else {
            prevStatGx = null;
        }
    }

    const biasRollX = tareCount >= 10 ? (sumAx / tareCount) : 0.0;
    const initialPitchBiasY = tareCount >= 10 ? (sumAy / tareCount) : 0.0;
    let dynamicPitchBiasY = initialPitchBiasY;
    const biasGyroX = tareCount >= 10 ? (sumGx / tareCount) : 0.0;
    const biasGyroY = tareCount >= 10 ? (sumGy / tareCount) : 0.0;
    const biasGyroZ = tareCount >= 10 ? (sumGz / tareCount) : 0.0;

    const ALPHA_GRADE = 0.025;
    let peakSpeed = 0;
    let peakLatG = 0;
    let peakLonG = 0;
    let peakAccelG = 0;
    let peakBrakeG = 0;
    let peakTemp = -Infinity;
    let peakHum = -Infinity;
    let totalDistanceKm = 0;
    let movingDurationSec = 0;
    let elapsedSec = 0;

    let lastValidGps = null;
    let lastValidGpsTsMs = NaN;
    let stepsSinceLastGps = 0;
    let consecutiveJumpCount = 0;
    let lastHeadingDeg = null;

    for (let i = 0; i < points.length; i++) {
        const p = points[i];
        const pPrev = i > 0 ? points[i - 1] : null;

        let dt = 0.20;
        let isGap = false;
        if (i === 0) {
            dt = 0.0;
        } else if (Number.isFinite(p.timestampMs) && Number.isFinite(pPrev.timestampMs)) {
            const rawDt = (p.timestampMs - pPrev.timestampMs) / 1000.0;
            if (rawDt > 1.0) {
                isGap = true;
                dt = rawDt;
            } else if (rawDt > 0) {
                dt = Math.min(1.0, Math.max(0.05, rawDt));
            }
        }
        p.dt = i === 0 ? 0.20 : (isGap ? 0.20 : dt);
        p.isGap = isGap;
        elapsedSec += (i === 0 ? 0 : dt);
        p.elapsedSec = elapsedSec;

        if (p.speed > peakSpeed) peakSpeed = p.speed;
        if (!isGap && p.speed > 2.0) {
            movingDurationSec += (i === 0 ? 0.20 : dt);
        }
        if (Number.isFinite(p.temp) && p.temp > peakTemp) peakTemp = p.temp;
        if (Number.isFinite(p.hum) && p.hum > peakHum) peakHum = p.hum;

        p.compensatedGyroX = p.gyroX - biasGyroX;
        p.compensatedGyroY = p.gyroY - biasGyroY;
        p.compensatedGyroZ = p.gyroZ - biasGyroZ;

        // Road Grade Tracking across valid non-gap pairs
        if (i > 0 && !isGap && dt > 0) {
            const vDeltaMs = (p.speed - pPrev.speed) / 3.6;
            const gpsKinematicG = (vDeltaMs / dt) / GRAVITY_MS2;
            const yawRate = Math.abs(p.compensatedGyroZ);
            const apparentGravityY = p.accelY - gpsKinematicG;
            // Only adapt slope bias when vehicle is steady (not during cornering or sudden IMU braking/traction transients lagging GPS)
            if (yawRate < 8.0 && Math.abs(gpsKinematicG) < 0.35 && p.speed > 10.0 && Math.abs(apparentGravityY - dynamicPitchBiasY) < 0.25) {
                dynamicPitchBiasY = (1.0 - ALPHA_GRADE) * dynamicPitchBiasY + ALPHA_GRADE * apparentGravityY;
            }
        }

        p.compensatedLatG = p.accelX - biasRollX;
        p.compensatedLonG = p.accelY - dynamicPitchBiasY;
        p.roadGradePct = Number(((dynamicPitchBiasY - initialPitchBiasY) * 100.0).toFixed(1));

        // Motion-only peak G tracking
        if (p.speed > 5.0) {
            const latG = Math.abs(p.compensatedLatG);
            const lonG = Math.abs(p.compensatedLonG);
            if (latG > peakLatG) peakLatG = latG;
            if (lonG > peakLonG) peakLonG = lonG;
            if (p.compensatedLonG > peakAccelG) peakAccelG = p.compensatedLonG;
            if (p.compensatedLonG < 0 && lonG > peakBrakeG) peakBrakeG = lonG;
        }

        // GPS distance integration bridging NULL/(0,0) gaps & clamping implausible speed jumps
        stepsSinceLastGps++;
        if (p.lat !== null && p.lon !== null) {
            if (lastValidGps === null) {
                lastValidGps = { lat: p.lat, lon: p.lon };
                lastValidGpsTsMs = p.timestampMs;
                stepsSinceLastGps = 0;
            } else {
                const dKm = haversineKm(lastValidGps.lat, lastValidGps.lon, p.lat, p.lon);
                let dtGpsSec = stepsSinceLastGps * 0.20;
                if (Number.isFinite(p.timestampMs) && Number.isFinite(lastValidGpsTsMs) && p.timestampMs > lastValidGpsTsMs) {
                    dtGpsSec = (p.timestampMs - lastValidGpsTsMs) / 1000.0;
                }
                const impliedKmh = dtGpsSec > 0 ? (dKm / (dtGpsSec / 3600.0)) : 0;

                if (p.speed >= 2.0) {
                    if (impliedKmh <= maxPlausibleKmh) {
                        totalDistanceKm += dKm;
                        if (dKm > 0.0002) {
                            const dLonRad = (p.lon - lastValidGps.lon) * Math.PI / 180.0;
                            const lat1Rad = lastValidGps.lat * Math.PI / 180.0;
                            const lat2Rad = p.lat * Math.PI / 180.0;
                            const yB = Math.sin(dLonRad) * Math.cos(lat2Rad);
                            const xB = Math.cos(lat1Rad) * Math.sin(lat2Rad) - Math.sin(lat1Rad) * Math.cos(lat2Rad) * Math.cos(dLonRad);
                            lastHeadingDeg = ((Math.atan2(yB, xB) * 180.0 / Math.PI) + 360.0) % 360.0;
                        }
                        lastValidGps = { lat: p.lat, lon: p.lon };
                        lastValidGpsTsMs = p.timestampMs;
                        stepsSinceLastGps = 0;
                        consecutiveJumpCount = 0;
                    } else {
                        consecutiveJumpCount++;
                        // If GPS relocated genuinely across 3 consecutive samples, re-anchor without adding jump km
                        if (consecutiveJumpCount >= 3) {
                            lastValidGps = { lat: p.lat, lon: p.lon };
                            lastValidGpsTsMs = p.timestampMs;
                            stepsSinceLastGps = 0;
                            consecutiveJumpCount = 0;
                        }
                    }
                } else {
                    // Stationary (< 2 km/h): accept re-anchor only if drifted slowly or relocated consistently
                    if (impliedKmh <= 15.0) {
                        lastValidGps = { lat: p.lat, lon: p.lon };
                        lastValidGpsTsMs = p.timestampMs;
                        stepsSinceLastGps = 0;
                    }
                }
            }
        }
        p.distCumKm = totalDistanceKm;
        p.headingDeg = lastHeadingDeg;
    }

    return {
        points,
        metrics: {
            isSessionMilliG,
            tareCount,
            biasRollX,
            initialPitchBiasY,
            biasGyroX,
            biasGyroY,
            biasGyroZ,
            peakSpeed,
            peakLatG,
            peakLonG,
            peakAccelG,
            peakBrakeG,
            peakTemp,
            peakHum,
            totalDistanceKm,
            movingDurationSec,
            totalElapsedSec: elapsedSec
        }
    };
}

/**
 * Detects Kamm Friction Circle violations, vertical bumps, and kinematic yaw slip,
 * clustering within a 1.0 s accumulated window using (points, excessRatio) comparison.
 */
function detectIncidents(points, limits, opts = {}) {
    const accelLimit = limits.accelLimit;
    const brakeLimit = limits.brakeLimit;
    const latLimit = limits.latLimit;
    const durationTotalSec = opts.durationTotalSec || (points.length > 0 ? points[points.length - 1].elapsedSec : 0) || (points.length * 0.20);

    const allIncidents = [];
    let activeCluster = null;
    let clusterCooldownSec = 0;

    const firstMs = points.length > 0 ? points[0].timestampMs : NaN;

    const isMoreSevere = (cand, cur) => {
        if (!cur) return true;
        if (cand.points !== cur.points) return cand.points > cur.points;
        return cand.excessRatio > cur.excessRatio;
    };

    for (let i = 0; i < points.length; i++) {
        const p = points[i];
        const dt = p.dt || 0.20;

        // Close open cluster across long telemetry gaps (> 1.0s)
        if (p.isGap && activeCluster) {
            allIncidents.push(activeCluster);
            activeCluster = null;
            clusterCooldownSec = 0;
        }

        const speedKmh = p.speed || 0;
        const latGX = p.compensatedLatG;
        const lonGY = p.compensatedLonG;
        const vertGZ = p.accelZ !== undefined ? p.accelZ : 1.0;
        const pitchRate = p.compensatedGyroY !== undefined ? p.compensatedGyroY : (p.gyroY || 0);
        const yawRate = Math.abs(p.compensatedGyroZ !== undefined ? p.compensatedGyroZ : (p.gyroZ || 0));

        const currentLonLimit = lonGY < 0 ? brakeLimit : accelLimit;
        const normLon = Math.abs(lonGY) / currentLonLimit;
        const normLat = Math.abs(latGX) / latLimit;
        const kammIndex = Math.hypot(normLon, normLat);
        p.kammIndex = kammIndex;

        const verticalDeltaG = Math.abs(vertGZ - 1.0);
        const isHarshBump = (verticalDeltaG > 0.80 && Math.abs(pitchRate) > 18.0 && speedKmh > 35.0);

        let isSlip = false;
        let slipDelta = 0;
        if (speedKmh > 25.0) {
            const speedMs = speedKmh / 3.6;
            const yawRadS = yawRate * (Math.PI / 180.0);
            const expectedLatG = (speedMs * yawRadS) / GRAVITY_MS2;
            slipDelta = Math.abs(Math.abs(latGX) - Math.abs(expectedLatG));
            if (slipDelta > 0.35) {
                isSlip = true;
            }
        }

        if (speedKmh > 5.0 && (kammIndex > 1.0 || isHarshBump || isSlip)) {
            clusterCooldownSec = 1.0;

            let elapsedSec = 0;
            if (Number.isFinite(firstMs) && Number.isFinite(p.timestampMs) && p.timestampMs >= firstMs) {
                elapsedSec = Math.round((p.timestampMs - firstMs) / 1000);
            } else if (Number.isFinite(p.elapsedSec) && points[points.length - 1].elapsedSec > 0) {
                elapsedSec = Math.round(p.elapsedSec);
            } else {
                elapsedSec = Math.round(durationTotalSec * (i / Math.max(1, points.length - 1)));
            }

            const min = String(Math.floor(elapsedSec / 60)).padStart(2, '0');
            const sec = String(elapsedSec % 60).padStart(2, '0');
            const timeStr = `+${min}:${sec}`;

            let candidate = null;

            if (kammIndex > 1.0) {
                if ((normLon < 1.0 && normLat < 1.0) || (normLon > 0.65 && normLat > 0.65)) {
                    const excess = kammIndex - 1.0;
                    let pts = 3;
                    if (excess < 0.15) pts = 2;
                    else if (excess <= 0.35) pts = 4;
                    else if (excess > 0.50) pts = 7;
                    else pts = 6;

                    const isBrakeCombo = lonGY < 0;
                    const desc = isBrakeCombo ? 'Trail-Braking' : 'Traction-Cornering';
                    candidate = {
                        timeStr, timestamp: p.timestamp, lat: p.lat, lon: p.lon,
                        category: isBrakeCombo ? 'combined_brake' : 'combined_traction',
                        reason: `Combined Cornering & Longitudinal Load (${desc})`,
                        intensity: `${Math.abs(latGX).toFixed(2)} G Lat + ${lonGY >= 0 ? '+' : '-'}${Math.abs(lonGY).toFixed(2)} G Lon`,
                        excessRatio: excess,
                        points: pts
                    };
                } else if (normLon >= normLat) {
                    if (lonGY < 0) {
                        const excess = (Math.abs(lonGY) - brakeLimit) / brakeLimit;
                        let pts = 4;
                        if (excess < 0.15) pts = 2;
                        else if (excess <= 0.35) pts = 4;
                        else if (excess > 0.50) pts = 7;
                        else pts = 6;

                        candidate = {
                            timeStr, timestamp: p.timestamp, lat: p.lat, lon: p.lon,
                            category: 'braking',
                            reason: 'Harsh Braking Shock',
                            intensity: `${Math.abs(lonGY).toFixed(2)} G (Decel)`,
                            excessRatio: excess,
                            points: pts
                        };
                    } else {
                        const excess = (lonGY - accelLimit) / accelLimit;
                        let pts = 3;
                        if (excess < 0.15) pts = 2;
                        else if (excess <= 0.35) pts = 4;
                        else if (excess > 0.50) pts = 7;
                        else pts = 6;

                        candidate = {
                            timeStr, timestamp: p.timestamp, lat: p.lat, lon: p.lon,
                            category: 'acceleration',
                            reason: 'Excessive Traction Acceleration',
                            intensity: `+${lonGY.toFixed(2)} G (Traction)`,
                            excessRatio: excess,
                            points: pts
                        };
                    }
                } else {
                    const excess = (Math.abs(latGX) - latLimit) / latLimit;
                    let pts = 4;
                    if (excess < 0.15) pts = 2;
                    else if (excess <= 0.35) pts = 4;
                    else if (excess > 0.50) pts = 7;
                    else pts = 6;

                    candidate = {
                        timeStr, timestamp: p.timestamp, lat: p.lat, lon: p.lon,
                        category: 'lateral',
                        reason: 'High Lateral Cornering G',
                        intensity: `${Math.abs(latGX).toFixed(2)} G (Lateral)`,
                        excessRatio: excess,
                        points: pts
                    };
                }
            } else if (isHarshBump) {
                const excess = (verticalDeltaG - 0.80) / 0.80;
                const pts = verticalDeltaG > 1.20 ? 3 : 2;
                candidate = {
                    timeStr, timestamp: p.timestamp, lat: p.lat, lon: p.lon,
                    category: 'bump',
                    reason: 'Harsh Speed Bump / Curb Impact',
                    intensity: `${verticalDeltaG.toFixed(2)} G (Vertical Shock)`,
                    excessRatio: excess,
                    points: pts
                };
            } else if (isSlip) {
                const excess = (slipDelta - 0.35) / 0.35;
                const pts = slipDelta > 0.50 ? 5 : 3;
                candidate = {
                    timeStr, timestamp: p.timestamp, lat: p.lat, lon: p.lon,
                    category: 'slip',
                    reason: 'Kinematic Slip (Oversteer / Understeer)',
                    intensity: `${yawRate.toFixed(1)} °/s (Yaw Rate)`,
                    excessRatio: excess,
                    points: pts
                };
            }

            if (candidate && isMoreSevere(candidate, activeCluster)) {
                activeCluster = candidate;
            }
        } else if (activeCluster) {
            clusterCooldownSec -= dt;
            if (clusterCooldownSec <= 0) {
                allIncidents.push(activeCluster);
                activeCluster = null;
                clusterCooldownSec = 0;
            }
        }
    }

    if (activeCluster) {
        allIncidents.push(activeCluster);
    }

    // Consistent descending sort by points then excessRatio
    allIncidents.sort((a, b) => {
        if (b.points !== a.points) return b.points - a.points;
        return b.excessRatio - a.excessRatio;
    });

    return allIncidents;
}

/**
 * Builds simulator-friendly RFC 4180 CSV content containing all samples, vehicle specs,
 * vehicle curb weight + pilot weight + passenger weight, gross mass, kinematics and Kamm bounds.
 * Excludes temperature, humidity, and incident_reason per simulator ingestion requirements.
 */
function buildTripCsvContent({ sessionId, session, points, kammLimits }) {
    const diag = kammLimits.diagnostics;
    const headers = [
        'sample_index',
        'timestamp',
        'timestamp_ms',
        'elapsed_s',
        'dt_s',
        'lat',
        'lon',
        'speed_kmh',
        'speed_ms',
        'distance_cum_km',
        'heading_deg',
        'accel_x_raw_g',
        'accel_y_raw_g',
        'accel_z_raw_g',
        'accel_lat_comp_g',
        'accel_lon_comp_g',
        'gyro_x_dps',
        'gyro_y_dps',
        'gyro_z_dps',
        'road_grade_pct',
        'kamm_index',
        'session_id',
        'driver',
        'vehicle',
        'power_hp',
        'curb_weight_kg',
        'pilot_weight_kg',
        'passenger_weight_kg',
        'payload_kg',
        'gross_mass_kg',
        'limit_lat_g',
        'limit_accel_g',
        'limit_brake_g'
    ];

    const pilotW = diag.pilotWeightKg !== null ? diag.pilotWeightKg : '';
    const passengerW = diag.passengerWeightKg !== null ? diag.passengerWeightKg : '';

    const staticCols = [
        escapeCsvField(sessionId),
        escapeCsvField(session.driver || ''),
        escapeCsvField(session.vehicle || ''),
        kammLimits.powerHp,
        diag.curbMassKg,
        pilotW,
        passengerW,
        diag.payloadKg,
        diag.grossMassKg,
        kammLimits.latLimit.toFixed(2),
        kammLimits.accelLimit.toFixed(2),
        kammLimits.brakeLimit.toFixed(2)
    ];

    const rows = [headers.join(',')];
    for (let i = 0; i < points.length; i++) {
        const p = points[i];
        const tsMs = Number.isFinite(p.timestampMs) ? Math.round(p.timestampMs) : '';
        const speedMs = p.speed / 3.6;
        const row = [
            i,
            escapeCsvField(p.timestamp || ''),
            tsMs,
            (p.elapsedSec || 0).toFixed(3),
            (p.dt || 0.20).toFixed(3),
            p.lat !== null ? p.lat.toFixed(7) : '',
            p.lon !== null ? p.lon.toFixed(7) : '',
            p.speed.toFixed(2),
            speedMs.toFixed(3),
            (p.distCumKm || 0).toFixed(5),
            p.headingDeg !== null && p.headingDeg !== undefined ? p.headingDeg.toFixed(2) : '',
            p.accelX.toFixed(4),
            p.accelY.toFixed(4),
            p.accelZ.toFixed(4),
            (p.compensatedLatG || 0).toFixed(4),
            (p.compensatedLonG || 0).toFixed(4),
            (p.compensatedGyroX !== undefined ? p.compensatedGyroX : p.gyroX).toFixed(2),
            (p.compensatedGyroY !== undefined ? p.compensatedGyroY : p.gyroY).toFixed(2),
            (p.compensatedGyroZ !== undefined ? p.compensatedGyroZ : p.gyroZ).toFixed(2),
            p.roadGradePct !== undefined ? p.roadGradePct.toFixed(1) : '0.0',
            (p.kammIndex || 0).toFixed(4),
            ...staticCols
        ];
        rows.push(row.join(','));
    }

    return rows.join('\n') + '\n';
}

async function generateDebriefReport(sessionId) {
    // Validate sessionId before touching filesystem, SQLite, or subprocess args
    if (typeof sessionId !== 'string' || !/^[\w-]{1,64}$/.test(sessionId)) {
        throw new Error(`Invalid sessionId: ${sessionId}`);
    }

    const dbPath = getDbPath();
    const reportsDir = getReportsDir();
    const csvDir = getCsvDir();
    const edgePath = getEdgePath();

    fs.mkdirSync(reportsDir, { recursive: true });
    fs.mkdirSync(csvDir, { recursive: true });

    console.log(`[DEBRIEF v${ALGORITHM_VERSION}] Generating report for session: ${sessionId}...`);
    const db = new DatabaseSync(dbPath, { readOnly: true });

    try {
        const session = db.prepare('SELECT * FROM sessions WHERE id = ?').get(sessionId);
        if (!session) {
            throw new Error(`Session ${sessionId} not found in database!`);
        }

        const driverRow = db.prepare('SELECT * FROM drivers WHERE name = ?').get(session.driver) || {};
        const driverEmail = (driverRow.email || '').trim();

        let vehicleRow = db.prepare('SELECT * FROM vehicles WHERE name = ?').get(session.vehicle);
        if (!vehicleRow && session.vehicle) {
            const tokens = session.vehicle.trim().split(/\s+/).filter(Boolean);
            if (tokens.length > 0) {
                console.warn(`[DEBRIEF] Exact vehicle match failed for "${session.vehicle}"; falling back to deterministic LIKE lookup.`);
                const likeClause = tokens.map(() => 'name LIKE ?').join(' AND ');
                const likeParams = tokens.map(t => `%${t}%`);
                vehicleRow = db.prepare(`SELECT * FROM vehicles WHERE ${likeClause} ORDER BY LENGTH(name) ASC LIMIT 1`).get(...likeParams);
            }
        }
        vehicleRow = vehicleRow || {};
        let catalog = {};
        if (vehicleRow.catalog_id) {
            catalog = db.prepare('SELECT * FROM car_catalog WHERE id = ?').get(vehicleRow.catalog_id) || {};
        }

        const rawPoints = db.prepare('SELECT * FROM telemetry_points WHERE session_id = ? ORDER BY id ASC').all(sessionId);
        console.log(`[DEBRIEF] Loaded ${rawPoints.length} telemetry points.`);

        // Compute dynamic vehicle specs & Kamm bounds first (needed for maxPlausibleKmh & incident engine)
        const specs = resolveVehicleSpecs(vehicleRow, catalog, session);
        const kammLimits = computeDynamicKammLimits(session, vehicleRow, catalog);
        const { accelLimit, brakeLimit, latLimit } = kammLimits;
        const maxPlausibleKmh = Math.max(260, specs.physics.topSpeedKmh * 1.15);

        // Signal processing (tare, gyro bias, gap-aware road-grade, peaks, distance)
        const processed = processSignals(rawPoints, { maxPlausibleKmh });
        const points = processed.points;
        const {
            peakSpeed: signalPeakSpeed,
            peakLatG,
            peakLonG,
            peakAccelG,
            peakBrakeG,
            peakTemp,
            peakHum,
            totalDistanceKm,
            movingDurationSec,
            totalElapsedSec
        } = processed.metrics;

        const peakSpeed = Math.max(Number(session.top_speed) || 0, signalPeakSpeed);

        // Duration formatting (rounded total seconds first so 60s never renders as "0m 60s")
        let rawDurationSec = Number(session.duration_sec);
        if (!Number.isFinite(rawDurationSec) || rawDurationSec <= 0) {
            rawDurationSec = totalElapsedSec > 0 ? totalElapsedSec : (points.length * 0.20);
        }
        const totalDurationSec = Math.max(0, Math.round(rawDurationSec));
        const durationMins = Math.floor(totalDurationSec / 60);
        const durationSecs = totalDurationSec % 60;
        const durationFormatted = `${durationMins}m ${durationSecs}s`;

        // Incident detection & clustering
        const allIncidents = detectIncidents(points, kammLimits, { durationTotalSec: totalDurationSec });

        // Data sufficiency check: >= 1500 samples (5 min) AND >= 120 s (2 min) moving time
        const isInsufficientData = points.length < 1500 || movingDurationSec < 120;

        // Always export the simulator-ready CSV at trip completion (regardless of duration sufficiency)
        const csvPath = path.join(csvDir, `session_${sessionId}_telemetry.csv`);
        const csvContent = buildTripCsvContent({ sessionId, session, points, kammLimits });
        fs.writeFileSync(csvPath, csvContent, 'utf8');
        console.log(`[DEBRIEF] Simulator CSV exported to: ${csvPath}`);

        // Hybrid Distance-Duration Normalization
        const movingMinutes = movingDurationSec / 60.0;
        const equivalentUrbanKm = movingMinutes / 2.5;
        const effectiveDistanceKm = Math.max(2.0, 0.70 * totalDistanceKm + 0.30 * equivalentUrbanKm);

        const totalDeductions = isInsufficientData ? 0 : allIncidents.reduce((acc, cur) => acc + cur.points, 0);
        const deductionsPer10Km = (totalDeductions / effectiveDistanceKm) * 10.0;

        const skillPillars = isInsufficientData
            ? [
                { name: 'CORNERING STABILITY', desc: 'Apex load & lateral G envelope', score: 0, grade: 'N/A', color: '#64748b' },
                { name: 'BRAKING DISCIPLINE', desc: 'Deceleration linearity & lockup avoidance', score: 0, grade: 'N/A', color: '#64748b' },
                { name: 'TRACTION CONTROL', desc: 'Yaw alignment & combined grip saturation', score: 0, grade: 'N/A', color: '#64748b' },
                { name: 'CHASSIS CARE', desc: 'Vertical shock & curb dampening', score: 0, grade: 'N/A', color: '#64748b' },
                { name: 'POWERTRAIN SYMPATHY', desc: 'Drivetrain smoothness & progressive torque', score: 0, grade: 'N/A', color: '#64748b' }
            ]
            : computeDrivingSkillPillars(points, allIncidents, effectiveDistanceKm);

        const finalScore = isInsufficientData
            ? 0
            : Math.min(100, Math.round(skillPillars.reduce((acc, p) => acc + p.score, 0) / 5.0));

        let tierLabel = 'EXCELLENT';
        let tierColor = '#10b981';
        let grade = 'A+';

        if (isInsufficientData) {
            tierLabel = 'INSUFFICIENT DATA';
            tierColor = '#64748b';
            grade = 'N/A';
        } else if (finalScore >= 93) {
            tierLabel = 'EXCELLENT / PRO';
            tierColor = '#10b981';
            grade = 'A+';
        } else if (finalScore >= 84) {
            tierLabel = 'VERY GOOD / ADVANCED';
            tierColor = '#38bdf8';
            grade = 'A';
        } else if (finalScore >= 70) {
            tierLabel = 'GOOD / SAFE';
            tierColor = '#fbbf24';
            grade = 'B';
        } else if (finalScore >= 50) {
            tierLabel = 'AVERAGE / CAUTION';
            tierColor = '#f97316';
            grade = 'C';
        } else {
            tierLabel = 'AGGRESSIVE / HIGH RISK';
            tierColor = '#ef4444';
            grade = 'D';
        }

        // Skip reverse-geocoding & extended audit pages when session data is insufficient
        const validIncidents = isInsufficientData ? [] : allIncidents;
        if (!isInsufficientData && validIncidents.length > 0) {
            console.log(`[DEBRIEF] Geocoding ${validIncidents.length} incident locations...`);
            for (const inc of validIncidents) {
                if (inc.lat && inc.lon) {
                    inc.placeName = await reverseGeocode(inc.lat, inc.lon);
                } else {
                    inc.placeName = 'Location coordinates N/A';
                }
            }
            saveGeocodeCache();
        }

        const displayedIncidents = validIncidents.slice(0, 8);
        const remainingIncidents = validIncidents.slice(8);
        const EXTENDED_PAGE_SIZE = 14;
        const extendedPageCount = remainingIncidents.length > 0 ? Math.ceil(remainingIncidents.length / EXTENDED_PAGE_SIZE) : 0;
        const totalPages = 2 + extendedPageCount;

        console.log(`[DEBRIEF] Total Incidents: ${validIncidents.length} | Total Report Pages: ${totalPages}`);
        console.log(`[DEBRIEF] Generating real 2D dark street map tiles...`);
        const mapSvg = await generateReal2DStreetMapSvg(points, displayedIncidents);

        function renderIncidentRow(inc, badgeNumber) {
            return `
            <tr style="border-bottom: 1px solid rgba(51, 65, 85, 0.4);">
                <td style="padding: 7px 10px; font-weight: 700; color: #94a3b8; font-family: 'Orbitron', 'Segoe UI', sans-serif;">
                    <span style="display: inline-block; width: 18px; height: 18px; line-height: 18px; text-align: center; background: rgba(239,68,68,0.2); border: 1px solid #ef4444; border-radius: 50%; color: #ef4444; font-size: 10px; margin-right: 6px;">${badgeNumber}</span>
                    ${escapeHtml(inc.timeStr)}
                </td>
                <td style="padding: 7px 10px; font-weight: 600; color: #f8fafc;">${escapeHtml(inc.reason)}</td>
                <td style="padding: 7px 10px; font-family: 'JetBrains Mono', 'Consolas', monospace; color: #cbd5e1;">${escapeHtml(inc.intensity)}</td>
                <td style="padding: 7px 10px; font-weight: 800; color: #f43f5e; text-align: center;">-${inc.points} pts</td>
                <td style="padding: 7px 10px; color: #38bdf8; font-size: 10.5px;">${escapeHtml(inc.placeName)}</td>
            </tr>`;
        }

        let page2RowsHtml = '';
        if (isInsufficientData) {
            page2RowsHtml = `
            <tr>
                <td colspan="5" style="text-align: center; padding: 22px; color: #94a3b8; font-weight: 700; letter-spacing: 0.5px;">
                    INSUFFICIENT DATA FOR FLIGHT RECORDER AUDIT (&lt; 1500 SAMPLES / 5 MIN &amp; 2 MIN MOVING REQUIRED)
                </td>
            </tr>`;
        } else if (displayedIncidents.length === 0) {
            page2RowsHtml = `
            <tr>
                <td colspan="5" style="text-align: center; padding: 22px; color: #10b981; font-weight: 700; letter-spacing: 0.5px;">
                    FLIGHT RECORDER CLEAN: ZERO SAFETY INFRACTIONS DETECTED
                </td>
            </tr>`;
        } else {
            displayedIncidents.forEach((inc, idx) => {
                page2RowsHtml += renderIncidentRow(inc, idx + 1);
            });
        }

        let extendedPagesHtml = '';
        if (extendedPageCount > 0) {
            for (let p = 0; p < extendedPageCount; p++) {
                const pageNumber = 3 + p;
                const chunkStart = p * EXTENDED_PAGE_SIZE;
                const chunkEnd = Math.min(remainingIncidents.length, (p + 1) * EXTENDED_PAGE_SIZE);
                const chunk = remainingIncidents.slice(chunkStart, chunkEnd);

                let chunkRowsHtml = '';
                chunk.forEach((inc, idx) => {
                    const badgeNum = 8 + chunkStart + idx + 1;
                    chunkRowsHtml += renderIncidentRow(inc, badgeNum);
                });

                extendedPagesHtml += `
                <!-- EXTENDED FLIGHT RECORDER LOG (PAGE ${pageNumber} OF ${totalPages}) -->
                <div class="page">
                    <div>
                        <div class="header-bar">
                            <div class="brand-title" style="font-size: 16px;">EXTENDED FLIGHT RECORDER LOG</div>
                        </div>

                        <div style="display: flex; justify-content: space-between; align-items: baseline; margin-bottom: 8px;">
                            <div style="font-family: 'Orbitron', 'Segoe UI', sans-serif; font-size: 10.5px; font-weight: 800; color: #38bdf8; letter-spacing: 1.5px;">
                                AUDIT REGISTER: INCIDENTS ${8 + chunkStart + 1} TO ${8 + chunkEnd} OF ${validIncidents.length}
                            </div>
                            <div style="font-size: 9.5px; color: #64748b;">
                                * Complete severity-ranked telemetry log of all recorded score deduction moments
                            </div>
                        </div>

                        <table class="incident-table">
                            <thead>
                                <tr>
                                    <th style="width: 12%;">TIME</th>
                                    <th style="width: 28%;">EVENT / TRIGGER</th>
                                    <th style="width: 18%;">INTENSITY</th>
                                    <th style="width: 12%; text-align: center;">PENALTY</th>
                                    <th style="width: 30%;">LOCATION (REVERSE GEOCODED)</th>
                                </tr>
                            </thead>
                            <tbody>
                                ${chunkRowsHtml}
                            </tbody>
                        </table>
                    </div>

                    <div class="footer">
                        <div>LABSIS TELEMETRY DEBRIEF &bull; v${ALGORITHM_VERSION}</div>
                        <div>PAGE ${pageNumber} OF ${totalPages} &bull; EXTENDED AUDIT REGISTER</div>
                    </div>
                </div>`;
            }
        }

        const limits = { accelLimit, brakeLimit, latLimit, diagnostics: kammLimits.diagnostics };
        const ggDiagramSvg = generateGGDiagramSvg(points, limits);
        const peaks = { peakLatG, peakAccelG, peakBrakeG };
        const ggFooterHtml = renderGGCardFooterHtml(peaks, limits);

        let skillPillarsHtml = '';
        skillPillars.forEach(p => {
            skillPillarsHtml += `
            <div class="skill-row">
                <div class="skill-info">
                    <div class="skill-name">${escapeHtml(p.name)}</div>
                    <div class="skill-desc">${escapeHtml(p.desc)}</div>
                </div>
                <div class="skill-bar-wrap">
                    <div class="skill-bar-fill" style="width: ${p.score}%; background: ${p.color};"></div>
                </div>
                <div class="skill-badge" style="color: ${p.color}; border-color: ${p.color};">
                    ${isInsufficientData ? '&mdash; &bull; N/A' : `${p.score}% &bull; ${escapeHtml(p.grade)}`}
                </div>
            </div>`;
        });

        // Dynamic KPI Colors
        let speedColor = '#f8fafc';
        if (peakSpeed > 140) speedColor = '#f43f5e';
        else if (peakSpeed > 120) speedColor = '#fbbf24';

        const latGColor = peakLatG > latLimit ? '#f43f5e' : '#38bdf8';
        const lonGColor = (peakAccelG > accelLimit || peakBrakeG > brakeLimit) ? '#f43f5e' : '#38bdf8';

        const hasValidTemp = Number.isFinite(peakTemp);
        let tempColor = '#94a3b8';
        if (hasValidTemp) {
            if (peakTemp < 17.0) tempColor = '#38bdf8';
            else if (peakTemp <= 25.0) tempColor = '#10b981';
            else if (peakTemp <= 29.0) tempColor = '#fbbf24';
            else tempColor = '#f43f5e';
        }

        const hasValidHum = Number.isFinite(peakHum) && peakHum > 0;
        let humColor = '#94a3b8';
        if (hasValidHum) {
            if (peakHum >= 35.0 && peakHum <= 65.0) humColor = '#10b981';
            else if ((peakHum >= 25.0 && peakHum < 35.0) || (peakHum > 65.0 && peakHum <= 75.0)) humColor = '#fbbf24';
            else humColor = '#f43f5e';
        }

        const htmlContent = `<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>Labsis Telemetry Debrief - Session #${escapeHtml(sessionId)}</title>
    <style>
        @page {
            size: A4 portrait;
            margin: 0;
        }
        * {
            box-sizing: border-box;
            -webkit-print-color-adjust: exact !important;
            print-color-adjust: exact !important;
        }
        body {
            margin: 0;
            padding: 0;
            background-color: #070c18;
            color: #f8fafc;
            font-family: 'Inter', 'Segoe UI', -apple-system, BlinkMacSystemFont, sans-serif;
            font-size: 11px;
            line-height: 1.4;
        }
        .page {
            width: 210mm;
            height: 297mm;
            position: relative;
            padding: 16mm 18mm 14mm 18mm;
            box-sizing: border-box;
            display: flex;
            flex-direction: column;
            justify-content: space-between;
            background-color: #070c18 !important;
            page-break-after: always;
            break-after: page;
        }

        .header-bar {
            display: flex;
            justify-content: space-between;
            align-items: center;
            border-bottom: 2px solid rgba(6, 182, 212, 0.4);
            padding-bottom: 10px;
            margin-bottom: 12px;
        }
        .brand-title {
            font-family: 'Orbitron', 'Segoe UI', sans-serif;
            font-size: 20px;
            font-weight: 900;
            color: #38bdf8;
            letter-spacing: 2px;
        }

        .info-grid {
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 12px;
            margin-bottom: 12px;
        }
        .info-card {
            background: #0c1427;
            border: 1px solid #1e293b;
            border-radius: 8px;
            padding: 12px 14px;
        }
        .info-title {
            font-family: 'Orbitron', 'Segoe UI', sans-serif;
            font-size: 10px;
            font-weight: 800;
            color: #94a3b8;
            letter-spacing: 1.5px;
            margin-bottom: 8px;
            border-bottom: 1px solid rgba(51, 65, 85, 0.5);
            padding-bottom: 4px;
        }
        .info-row {
            display: flex;
            justify-content: space-between;
            font-size: 11px;
            margin-bottom: 5px;
        }
        .info-label { color: #64748b; font-weight: 600; }
        .info-val { color: #f8fafc; font-weight: 700; text-align: right; }

        .kpi-grid {
            display: grid;
            grid-template-columns: repeat(4, 1fr);
            gap: 7px;
            margin-bottom: 10px;
        }
        .kpi-box {
            background: #0c1427;
            border: 1px solid #1e293b;
            border-radius: 6px;
            padding: 7px 6px;
            text-align: center;
        }
        .kpi-label {
            font-size: 9px;
            color: #64748b;
            font-weight: 700;
            text-transform: uppercase;
            letter-spacing: 0.5px;
            margin-bottom: 4px;
        }
        .kpi-val {
            font-family: 'Orbitron', 'Segoe UI', sans-serif;
            font-size: 18px;
            font-weight: 800;
            color: #f8fafc;
        }
        .kpi-unit {
            font-size: 10px;
            color: #06b6d4;
            font-weight: 600;
            margin-left: 2px;
        }

        .score-banner {
            background: linear-gradient(135deg, #0c1427 0%, #111e38 100%);
            border: 1px solid #334155;
            border-radius: 8px;
            padding: 12px 18px;
            display: flex;
            justify-content: space-between;
            align-items: center;
            margin-bottom: 12px;
        }
        .score-badge-circle {
            width: 58px;
            height: 58px;
            border-radius: 50%;
            display: flex;
            flex-direction: column;
            justify-content: center;
            align-items: center;
            font-family: 'Orbitron', 'Segoe UI', sans-serif;
            font-weight: 900;
            font-size: 18px;
            margin-right: 14px;
            flex-shrink: 0;
            border: 2px solid;
        }

        .analysis-grid {
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 12px;
            margin-bottom: 6px;
        }
        .analysis-card {
            background: #0c1427;
            border: 1px solid #1e293b;
            border-radius: 8px;
            padding: 10px 12px;
        }
        .analysis-title {
            font-family: 'Orbitron', 'Segoe UI', sans-serif;
            font-size: 9.5px;
            font-weight: 800;
            color: #94a3b8;
            letter-spacing: 1.2px;
            margin-bottom: 6px;
            display: flex;
            justify-content: space-between;
            align-items: center;
        }
        .analysis-tag {
            font-size: 8px;
            padding: 2px 6px;
            border-radius: 3px;
            background: rgba(6, 182, 212, 0.15);
            color: #06b6d4;
            font-family: 'Inter', 'Segoe UI', sans-serif;
            font-weight: 700;
        }
        .analysis-sub {
            font-size: 9px;
            color: #64748b;
            display: flex;
            justify-content: space-between;
            margin-top: 4px;
            font-weight: 600;
        }

        .skill-list {
            display: flex;
            flex-direction: column;
            gap: 6px;
        }
        .skill-row {
            display: flex;
            align-items: center;
            gap: 8px;
            font-size: 9.5px;
        }
        .skill-info {
            width: 140px;
            flex-shrink: 0;
        }
        .skill-name {
            font-weight: 700;
            color: #f1f5f9;
            white-space: nowrap;
            overflow: hidden;
            text-overflow: ellipsis;
        }
        .skill-desc {
            font-size: 8px;
            color: #64748b;
        }
        .skill-bar-wrap {
            flex-grow: 1;
            height: 7px;
            background: #1e293b;
            border-radius: 4px;
            overflow: hidden;
        }
        .skill-bar-fill {
            height: 100%;
            border-radius: 4px;
        }
        .skill-badge {
            width: 82px;
            text-align: right;
            font-family: 'Orbitron', 'Segoe UI', sans-serif;
            font-size: 8.5px;
            font-weight: 700;
            flex-shrink: 0;
        }

        .incident-table {
            width: 100%;
            border-collapse: collapse;
            font-size: 10.5px;
            margin-bottom: 8px;
        }
        .incident-table th {
            text-align: left;
            padding: 6px 10px;
            background: #0f172a;
            color: #94a3b8;
            font-family: 'Orbitron', 'Segoe UI', sans-serif;
            font-size: 9px;
            font-weight: 800;
            letter-spacing: 0.8px;
            border-bottom: 2px solid #334155;
        }
        .incident-table td {
            padding: 6px 10px;
            vertical-align: middle;
        }

        .footer {
            border-top: 1px solid #1e293b;
            padding-top: 8px;
            display: flex;
            justify-content: space-between;
            align-items: center;
            font-size: 8.5px;
            color: #64748b;
            font-family: 'Orbitron', 'Segoe UI', sans-serif;
            letter-spacing: 0.5px;
        }
    </style>
</head>
<body>

    <!-- PAGE 1: EXECUTIVE BENCHMARKS, SCORE & DYNAMICS ANALYSIS -->
    <div class="page">
        <div>
            <div class="header-bar">
                <div class="brand-title">LABSIS TELEMETRY DEBRIEF</div>
            </div>

            <div class="info-grid">
                <div class="info-card">
                    <div class="info-title">TEST VEHICLE & SPECIFICATIONS</div>
                    <div class="info-row"><span class="info-label">Vehicle:</span> <span class="info-val">${escapeHtml(specs.display.vehicleName)}</span></div>
                    <div class="info-row"><span class="info-label">Drivetrain & Gearbox:</span> <span class="info-val">${escapeHtml(specs.display.drivetrain)} &bull; ${escapeHtml(specs.display.gearbox)}</span></div>
                    <div class="info-row"><span class="info-label">Output & Weight:</span> <span class="info-val">${escapeHtml(specs.display.powerHpText)} &bull; ${escapeHtml(specs.display.massKgText)}${escapeHtml(specs.display.powerToWeightText)}</span></div>
                    <div class="info-row"><span class="info-label">Engine Config:</span> <span class="info-val">${escapeHtml(specs.display.displacementText)} &bull; ${escapeHtml(specs.display.fuelType)}</span></div>
                    <div class="info-row"><span class="info-label">Torque Peak:</span> <span class="info-val">${escapeHtml(specs.display.torqueDisplay)}</span></div>
                </div>

                <div class="info-card">
                    <div class="info-title">MISSION PARAMETERS & DRIVER PROFILE</div>
                    <div class="info-row"><span class="info-label">Session ID:</span> <span class="info-val" style="font-family: 'Orbitron', 'Segoe UI', sans-serif; color: #38bdf8;">${escapeHtml(sessionId)}</span></div>
                    <div class="info-row"><span class="info-label">Driver:</span> <span class="info-val">${escapeHtml(session.driver || 'N/A')}</span></div>
                    <div class="info-row"><span class="info-label">Pilot Email:</span> <span class="info-val">${escapeHtml(driverEmail || 'N/A')}</span></div>
                    <div class="info-row"><span class="info-label">Start Time:</span> <span class="info-val">${escapeHtml(session.start_time || 'N/A')} &bull; ${escapeHtml(durationFormatted)}</span></div>
                </div>
            </div>

            <div class="kpi-grid">
                <div class="kpi-box">
                    <div class="kpi-label">TOP SPEED</div>
                    <div class="kpi-val" style="color: ${speedColor};">${peakSpeed.toFixed(1)}<span class="kpi-unit">KM/H</span></div>
                </div>
                <div class="kpi-box">
                    <div class="kpi-label">PEAK LATERAL G</div>
                    <div class="kpi-val" style="color: ${latGColor};">${peakLatG.toFixed(2)}<span class="kpi-unit">G</span></div>
                </div>
                <div class="kpi-box">
                    <div class="kpi-label">PEAK LONGITUDINAL G</div>
                    <div class="kpi-val" style="color: ${lonGColor};">${peakLonG.toFixed(2)}<span class="kpi-unit">G</span></div>
                </div>
                <div class="kpi-box">
                    <div class="kpi-label">DISTANCE TRAVELLED</div>
                    <div class="kpi-val" style="color: #f8fafc;">${totalDistanceKm.toFixed(1)}<span class="kpi-unit">KM</span></div>
                </div>
                <div class="kpi-box">
                    <div class="kpi-label">SESSION DURATION</div>
                    <div class="kpi-val" style="color: #f8fafc;">${escapeHtml(durationFormatted)}</div>
                </div>
                <div class="kpi-box">
                    <div class="kpi-label">DATA SAMPLES</div>
                    <div class="kpi-val" style="color: #94a3b8;">${points.length}</div>
                </div>
                <div class="kpi-box">
                    <div class="kpi-label">CABIN TEMPERATURE</div>
                    <div class="kpi-val" style="color: ${tempColor};">${hasValidTemp ? peakTemp.toFixed(1) : '--'}<span class="kpi-unit">°C</span></div>
                </div>
                <div class="kpi-box">
                    <div class="kpi-label">CABIN HUMIDITY</div>
                    <div class="kpi-val" style="color: ${humColor};">${hasValidHum ? peakHum.toFixed(1) : '--'}<span class="kpi-unit">% RH</span></div>
                </div>
            </div>

            <div class="score-banner">
                <div style="display: flex; align-items: center;">
                    <div class="score-badge-circle" style="background: ${isInsufficientData ? 'rgba(100,116,139,0.15)' : (tierColor + '20')}; border-color: ${tierColor}; color: ${tierColor};">
                        ${escapeHtml(grade)}
                    </div>
                    <div>
                        <div style="font-size: 10px; color: #94a3b8; font-weight: 700; letter-spacing: 1px;">DRIVER EFFICIENCY RATING</div>
                        <div style="font-family: 'Orbitron', 'Segoe UI', sans-serif; font-size: 20px; font-weight: 900; color: ${tierColor};">
                            ${isInsufficientData ? 'INSUFFICIENT DATA' : `${finalScore} / 100 &bull; ${escapeHtml(tierLabel)}`}
                        </div>
                        <div style="font-size: 10px; color: #64748b; margin-top: 2px;">
                            ${isInsufficientData
                                ? 'Trip requires at least 1500 samples (5 min) and 2 min moving telemetry to calibrate driver rating.'
                                : (finalScore >= 84
                                    ? 'Excellent vehicle control and chassis balance. Weight transfer maintained within Kamm friction ellipse with optimal mechanical sympathy.'
                                    : 'Dynamic infractions recorded. Recommended smoother trail braking and gradual throttle application to preserve tire adhesion.')}
                        </div>
                    </div>
                </div>
                <div style="text-align: right; border-left: 1px solid #334155; padding-left: 20px; min-width: 140px;">
                    <div style="font-size: 10px; color: #94a3b8; font-weight: 700;">TOTAL DEDUCTIONS</div>
                    <div style="font-family: 'Orbitron', 'Segoe UI', sans-serif; font-size: ${isInsufficientData ? '18px' : '22px'}; font-weight: 800; color: ${isInsufficientData ? '#94a3b8' : (validIncidents.length > 0 ? '#f43f5e' : '#10b981')};">
                        ${isInsufficientData ? 'N/A' : `-${totalDeductions} pts`}
                    </div>
                    <div style="font-size: 9.5px; color: #64748b;">
                        ${isInsufficientData ? `< 1500 SAMPLES (${points.length})` : `${validIncidents.length} EVENTS (${deductionsPer10Km.toFixed(1)} PTS/10KM)`}
                    </div>
                </div>
            </div>

            <div class="analysis-grid">
                <div class="analysis-card">
                    <div class="analysis-title">
                        <span>G-G ACCELERATION ELLIPSE</span>
                        <span class="analysis-tag">KAMM FRICTION CIRCLE</span>
                    </div>
                    <div style="margin: 2px 0;">
                        ${ggDiagramSvg}
                    </div>
                    ${ggFooterHtml}
                </div>

                <div class="analysis-card">
                    <div class="analysis-title">
                        <span>DYNAMICS COMPETENCY PILLARS</span>
                        <span class="analysis-tag">5-VECTOR AUDIT</span>
                    </div>
                    <div class="skill-list">
                        ${skillPillarsHtml}
                    </div>
                    <div class="analysis-sub" style="margin-top: 6px;">
                        <span>SESSION AVERAGE: ${isInsufficientData ? 'N/A' : `${finalScore}%`}</span>
                        <span style="color: ${isInsufficientData ? '#94a3b8' : tierColor}; font-weight: 700;">${isInsufficientData ? 'INSUFFICIENT DATA' : escapeHtml(tierLabel)}</span>
                    </div>
                </div>
            </div>
        </div>

        <div class="footer">
            <div>LABSIS TELEMETRY DEBRIEF &bull; v${ALGORITHM_VERSION}</div>
            <div>PAGE 1 OF ${totalPages} &bull; BENCHMARKS & SCORING</div>
        </div>
    </div>

    <!-- PAGE 2: REAL 2D STREET MAP & TOP 8 BLACK BOX INCIDENT LOG -->
    <div class="page">
        <div>
            <div class="header-bar">
                <div class="brand-title" style="font-size: 16px;">GPS TRAJECTORY & INCIDENT LOG</div>
            </div>

            <div style="margin-bottom: 14px;">
                ${mapSvg}
            </div>

            <div style="display: flex; justify-content: space-between; align-items: baseline; margin-bottom: 6px;">
                <div style="font-family: 'Orbitron', 'Segoe UI', sans-serif; font-size: 10.5px; font-weight: 800; color: #38bdf8; letter-spacing: 1.5px;">
                    BLACK BOX INCIDENT LOG (SCORE DEDUCTION MOMENTS)
                </div>
                <div style="font-size: 9.5px; color: #64748b;">
                    ${isInsufficientData
                        ? `* Minimum 5 min &amp; 2 min moving required (${points.length} / 1500 samples recorded)`
                        : (validIncidents.length > 8 ? `* Showing Top 8 most severe events (${validIncidents.length} total recorded) &bull; See extended log on Page 3+` : `* ${validIncidents.length} distinct events logged`)}
                </div>
            </div>
            <table class="incident-table">
                <thead>
                    <tr>
                        <th style="width: 12%;">TIME</th>
                        <th style="width: 28%;">EVENT / TRIGGER</th>
                        <th style="width: 18%;">INTENSITY</th>
                        <th style="width: 12%; text-align: center;">PENALTY</th>
                        <th style="width: 30%;">LOCATION (REVERSE GEOCODED)</th>
                    </tr>
                </thead>
                <tbody>
                    ${page2RowsHtml}
                </tbody>
            </table>
        </div>

        <div class="footer">
            <div>LABSIS TELEMETRY DEBRIEF &bull; v${ALGORITHM_VERSION}</div>
            <div>PAGE 2 OF ${totalPages} &bull; GPS TRAJECTORY & INCIDENT LOG</div>
        </div>
    </div>

    ${extendedPagesHtml}

</body>
</html>`;

        const htmlPath = path.join(reportsDir, `session_${sessionId}_debrief.html`);
        const pdfPath = path.join(reportsDir, `session_${sessionId}_debrief.pdf`);
        fs.writeFileSync(htmlPath, htmlContent, 'utf8');
        console.log(`[DEBRIEF] Wrote HTML report to: ${htmlPath}`);

        // Delete any stale PDF before printing so an Edge failure can never return an old artifact
        if (fs.existsSync(pdfPath)) {
            fs.unlinkSync(pdfPath);
        }

        const printStartMs = Date.now();
        await renderPdfWithEdge(edgePath, htmlPath, pdfPath, printStartMs, 60000);

        if (fs.existsSync(pdfPath)) {
            const stat = fs.statSync(pdfPath);
            if (stat.size <= 0 || stat.mtimeMs < (printStartMs - 1000)) {
                throw new Error(`PDF validation failed (stale or empty) at ${pdfPath}`);
            }
            console.log(`[DEBRIEF] SUCCESS! PDF generated: ${pdfPath} (${stat.size} bytes)`);
            return {
                success: true,
                algorithmVersion: ALGORITHM_VERSION,
                pdfPath,
                htmlPath,
                csvPath,
                driverEmail,
                driverName: session.driver,
                score: isInsufficientData ? 'N/A' : finalScore,
                tier: tierLabel,
                grade,
                isInsufficientData,
                incidentsCount: validIncidents.length,
                totalDistanceKm: totalDistanceKm.toFixed(2),
                totalPages,
                sessionId
            };
        } else {
            throw new Error(`PDF file was not created at ${pdfPath}`);
        }
    } finally {
        db.close();
    }
}

function isPdfCompleteAndValid(pdfPath, minMtimeMs) {
    try {
        if (!fs.existsSync(pdfPath)) return null;
        const stat = fs.statSync(pdfPath);
        if (stat.size < 128 || stat.mtimeMs < minMtimeMs) return null;
        const fd = fs.openSync(pdfPath, 'r');
        try {
            const head = Buffer.alloc(5);
            fs.readSync(fd, head, 0, 5, 0);
            if (head.toString('ascii') !== '%PDF-') return null;
            const tailLen = Math.min(128, stat.size);
            const tail = Buffer.alloc(tailLen);
            fs.readSync(fd, tail, 0, tailLen, stat.size - tailLen);
            if (!tail.toString('ascii').includes('%%EOF')) return null;
            return stat.size;
        } finally {
            fs.closeSync(fd);
        }
    } catch (_e) {
        return null;
    }
}

function killProcessTree(pid) {
    if (!pid) return;
    try {
        if (process.platform === 'win32') {
            const killer = spawn('taskkill', ['/PID', String(pid), '/T', '/F'], { stdio: 'ignore', windowsHide: true });
            killer.unref();
        } else {
            process.kill(pid, 'SIGKILL');
        }
    } catch (_e) {}
}

/**
 * Renders HTML to PDF using headless Microsoft Edge in an isolated temp profile.
 * Monitors %PDF-...%%EOF stabilization directly on disk so Windows Edge/crashpad
 * teardown hangs cannot trigger false 60s timeouts after PDF creation completes.
 */
function renderPdfWithEdge(edgePath, htmlPath, pdfPath, printStartMs, timeoutMs = 60000) {
    const edgeProfileDir = fs.mkdtempSync(path.join(os.tmpdir(), 'labsis_edge_'));
    return new Promise((resolve, reject) => {
        const args = [
            '--headless',
            '--disable-gpu',
            '--no-first-run',
            '--no-default-browser-check',
            '--disable-background-networking',
            '--disable-sync',
            '--metrics-recording-only',
            '--disable-default-apps',
            `--user-data-dir=${edgeProfileDir}`,
            '--run-all-compositor-stages-before-draw',
            '--no-pdf-header-footer',
            `--print-to-pdf=${pdfPath}`,
            htmlPath
        ];

        const child = spawn(edgePath, args, { stdio: 'ignore', windowsHide: true });
        let settled = false;
        let stableSize = -1;
        let stableTicks = 0;
        let graceTimer = null;

        const cleanupProfile = () => {
            try { fs.rmSync(edgeProfileDir, { recursive: true, force: true }); } catch (_e) {}
        };

        const finish = (err) => {
            if (settled) return;
            settled = true;
            clearInterval(pollTimer);
            clearTimeout(hardTimer);
            if (graceTimer) clearTimeout(graceTimer);
            const neededKill = child.exitCode === null;
            if (neededKill) killProcessTree(child.pid);
            setTimeout(() => {
                cleanupProfile();
                if (err) reject(err);
                else resolve();
            }, neededKill ? 150 : 20);
        };

        const pollTimer = setInterval(() => {
            const sz = isPdfCompleteAndValid(pdfPath, printStartMs - 1000);
            if (sz === null) return;
            if (sz === stableSize) stableTicks++;
            else {
                stableSize = sz;
                stableTicks = 1;
            }
            if (stableTicks >= 2 && !graceTimer) {
                graceTimer = setTimeout(() => finish(null), 350);
            }
        }, 150);

        const hardTimer = setTimeout(() => {
            finish(new Error(`Edge headless PDF print timed out after ${timeoutMs} ms`));
        }, timeoutMs);

        child.on('error', (err) => finish(err));
        child.on('exit', (code) => {
            if (settled) return;
            const sz = isPdfCompleteAndValid(pdfPath, printStartMs - 1000);
            if (sz !== null) {
                finish(null);
            } else {
                finish(new Error(`Edge exited (${code}) without producing a valid PDF at ${pdfPath}`));
            }
        });
    });
}

if (require.main === module) {
    const sessionId = process.argv[2] || 'TRIP_20261028_153133';
    generateDebriefReport(sessionId)
        .then(res => {
            console.log('\n--- RESULT JSON ---');
            console.log(JSON.stringify(res, null, 2));
            process.exit(0);
        })
        .catch(err => {
            console.error('[DEBRIEF ERROR]:', err);
            process.exit(1);
        });
}

module.exports = {
    ALGORITHM_VERSION,
    PILLAR_CATEGORY_MAP,
    escapeHtml,
    escapeCsvField,
    parseTimestampMs,
    parseAverageTireWidthMm,
    calculateTireRadius,
    resolveVehicleSpecs,
    computeDynamicKammLimits,
    processSignals,
    detectIncidents,
    computeDrivingSkillPillars,
    haversineKm,
    generateGGDiagramSvg,
    renderGGCardFooterHtml,
    buildTripCsvContent,
    fetchTileBase64,
    reverseGeocode,
    loadGeocodeCache,
    saveGeocodeCache,
    resetGeocodeCache,
    generateDebriefReport
};
