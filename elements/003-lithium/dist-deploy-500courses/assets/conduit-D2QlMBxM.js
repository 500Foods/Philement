/**
 * Radar Status Controller
 *
 * Drives the SVG radar icon in the sidebar header. Tracks:
 *   - WebSocket messages (targets appear/disappear per send/receive)
 *   - REST API requests (targets appear/disappear per request)
 *   - WS connection health (rim color, sweep animation)
 *   - WS keepalive heartbeats (sweep line flash)
 *
 * Sweep animation is now handled by CSS for smoother performance,
 * while angle tracking remains time-based (not frame-count-based) for
 * accurate ping detection. The sweep completes a full 360° rotation
 * in exactly SWEEP_PERIOD_MS.
 *
 * Target lifecycle:
 *   1. Appear bright white at the sweep arm tip position (60% radius)
 *   2. Quickly settle to their normal color (red for REST, green for WS)
 *   3. Flash bright again each time the sweep arm passes over them
 *   4. On removal: flash to final color, then fade out over half a sweep cycle
 */

// ─── DOM References (cached on init) ────────────────────────────────────────

let radarIcon = null;
let radarIconSidebar = null;
let sweepGroup = null;
let sweepArm = null;
let targetsGroup = null;
let sweepGroupSidebar = null;
let sweepArmSidebar = null;
let targetsGroupSidebar = null;
let styleEl = null;

// ─── Internal State ─────────────────────────────────────────────────────────

let angle = 0;                   // current sweep angle in degrees (0–360)
let rafId = null;
let lastTimestamp = null;        // timestamp of last animation frame
let isConnected = false;
let activeRadar = 'header';      // 'header' or 'sidebar'
const activeTargets = new Map(); // id → { element, category, targetAngle }
let targetIdCounter = 0;

// Track current connection state for sync on radar switch
let connectionStateClass = 'radar-disconnected';
let connectionTooltip = 'Status: Disconnected';

// ─── Timing Constants (time-based, not frame-based) ─────────────────────────

// Full 360° sweep period in milliseconds
const SWEEP_PERIOD_MS = 3333;    // ~3.3s per full rotation

// Angular velocity: degrees per millisecond
const SWEEP_DPS = 360 / SWEEP_PERIOD_MS; // ~0.108°/ms

// Half a sweep cycle — used for fade-out duration
const HALF_PERIOD_MS = SWEEP_PERIOD_MS / 2; // ~1.67s

// How long the initial "bright flash" lasts when a target is added
const TARGET_FLASH_MS = 300;

// How long the ping glow lasts when sweep passes a target
const PING_DURATION_MS = 400;

// How far (in degrees) the sweep arm can be from a target to trigger ping
const PING_THRESHOLD_DEG = 5;

// ─── SVG Geometry ───────────────────────────────────────────────────────────

const CX = 32;
const CY = 32;

// Target placement: 60% of the distance from center to rim
const RIM_RADIUS = 27.5;             // center to outer rim
const TARGET_RADIUS = RIM_RADIUS * 0.6; // 16.5 — where targets sit

// ─── Colors ─────────────────────────────────────────────────────────────────

const COLOR_SWEEP = '#ffffff';
const COLOR_WS = '#10b981';      // green — WebSocket targets
const COLOR_REST = '#ef4444';    // red — REST API targets
const COLOR_BRIGHT = '#ffffff';  // bright flash color

// ─── CSS (injected once) ────────────────────────────────────────────────────

function ensurePingCSS() {
  if (styleEl) return;
  styleEl = document.createElement('style');
  styleEl.textContent = `
    #targets-group polygon,
    #targets-group rect,
    #targets-group-sidebar polygon,
    #targets-group-sidebar rect {
      transition: fill 0.25s ease, opacity 0.25s ease;
    }
    #targets-group g.ping polygon,
    #targets-group g.ping rect,
    #targets-group-sidebar g.ping polygon,
    #targets-group-sidebar g.ping rect {
      filter: brightness(2.0) drop-shadow(0 0 5px #ffffff);
    }
    #targets-group g.fading,
    #targets-group-sidebar g.fading {
      opacity: 0;
      transition: opacity var(--radar-fade-duration, 1.67s) linear;
    }
    #radar-icon,
    #radar-icon-sidebar {
      color: var(--accent-primary, #5C6BC0);
      transition: color 0.4s ease;
    }
    /* Sweep animation - moved to CSS for smoother performance */
    #sweep-group,
    #sweep-arm,
    #sweep-group-sidebar,
    #sweep-arm-sidebar {
      animation: sweep-rotation ${SWEEP_PERIOD_MS}ms linear infinite;
      transform-origin: ${CX}px ${CY}px;
    }
    /* Pause sweep when disconnected */
    .radar-disconnected #sweep-group,
    .radar-disconnected #sweep-arm,
    .radar-disconnected #sweep-group-sidebar,
    .radar-disconnected #sweep-arm-sidebar {
      animation-play-state: paused;
    }
    @keyframes sweep-rotation {
      from { transform: rotate(0deg); }
      to { transform: rotate(360deg); }
    }
  `;
  document.head.appendChild(styleEl);
}

// ─── Initialization ─────────────────────────────────────────────────────────

function initRadar() {
  radarIcon = document.getElementById('radar-icon');
  radarIconSidebar = document.getElementById('radar-icon-sidebar');
  
  // Header radar
  sweepGroup = document.getElementById('sweep-group');
  sweepArm = document.getElementById('sweep-arm');
  targetsGroup = document.getElementById('targets-group');
  
  // Sidebar radar
  sweepGroupSidebar = document.getElementById('sweep-group-sidebar');
  sweepArmSidebar = document.getElementById('sweep-arm-sidebar');
  targetsGroupSidebar = document.getElementById('targets-group-sidebar');

  ensurePingCSS();
  applySweepColors();
  
  // Set default active radar to header
  activeRadar = 'header';
  setConnectionState(false, false);

  return true;
}

function applySweepColors() {
  // Header radar
  if (sweepArm) sweepArm.setAttribute('stroke', COLOR_SWEEP);
  if (sweepGroup) {
    sweepGroup.querySelectorAll('path').forEach(p => {
      p.setAttribute('fill', COLOR_SWEEP);
    });
  }
  // Sidebar radar
  if (sweepArmSidebar) sweepArmSidebar.setAttribute('stroke', COLOR_SWEEP);
  if (sweepGroupSidebar) {
    sweepGroupSidebar.querySelectorAll('path').forEach(p => {
      p.setAttribute('fill', COLOR_SWEEP);
    });
  }
}

// ─── Connection State ───────────────────────────────────────────────────────

function setActiveRadar(location) {
  activeRadar = location === 'sidebar' ? 'sidebar' : 'header';

  // Sync connection state to the newly-active radar using tracked state variable
  const targetElement = activeRadar === 'header' ? radarIcon : radarIconSidebar;
  if (targetElement) {
    targetElement.classList.remove('radar-connected', 'radar-flaky', 'radar-disconnected');
    targetElement.classList.add(connectionStateClass);
  }

  // Sync tooltip text to the newly-active radar
  syncRadarTooltip();
}

function syncRadarTooltip() {
  const targetElement = activeRadar === 'header' ? radarIcon : radarIconSidebar;
  if (targetElement) {
    targetElement.dataset.tooltip = connectionTooltip;
  }
}

function setRadarTooltip(text) {
  connectionTooltip = text;
  // Also update the active radar's tooltip immediately
  const activeElement = activeRadar === 'header' ? radarIcon : radarIconSidebar;
  if (activeElement) {
    activeElement.dataset.tooltip = text;
  }
}

function getRadarTooltip() {
  return connectionTooltip;
}

function setConnectionState(connected, flaky = false) {
  isConnected = connected;

  // Track the state for sync on radar switch
  if (!connected) {
    connectionStateClass = 'radar-disconnected';
  } else if (flaky) {
    connectionStateClass = 'radar-flaky';
  } else {
    connectionStateClass = 'radar-connected';
  }

  // Only update the active radar's visual state
  if (activeRadar === 'header' && radarIcon) {
    radarIcon.classList.remove('radar-connected', 'radar-flaky', 'radar-disconnected');

    if (!connected) {
      radarIcon.classList.add('radar-disconnected');
      stopRadarLoop();
      radarIcon.animate([
        { transform: 'rotate(0deg)' },
        { transform: 'rotate(8deg)' },
        { transform: 'rotate(-8deg)' },
        { transform: 'rotate(0deg)' },
      ], { duration: 420 });
    } else if (flaky) {
      radarIcon.classList.add('radar-flaky');
      startRadarLoop();
    } else {
      radarIcon.classList.add('radar-connected');
      startRadarLoop();
    }
  } else if (activeRadar === 'sidebar' && radarIconSidebar) {
    radarIconSidebar.classList.remove('radar-connected', 'radar-flaky', 'radar-disconnected');

    if (!connected) {
      radarIconSidebar.classList.add('radar-disconnected');
      stopRadarLoop();
      radarIconSidebar.animate([
        { transform: 'rotate(0deg)' },
        { transform: 'rotate(8deg)' },
        { transform: 'rotate(-8deg)' },
        { transform: 'rotate(0deg)' },
      ], { duration: 420 });
    } else if (flaky) {
      radarIconSidebar.classList.add('radar-flaky');
      startRadarLoop();
    } else {
      radarIconSidebar.classList.add('radar-connected');
      startRadarLoop();
    }
  }
}

function wsConnected() { setConnectionState(true, false); }
function wsFlaky() { setConnectionState(true, true); }
function wsDisconnected() { setConnectionState(false, false); }

// ─── Heartbeat Flash ────────────────────────────────────────────────────────

function onHeartbeat() {
  if (!isConnected) return;

  // Only flash the active radar
  if (activeRadar === 'header' && sweepArm) {
    sweepArm.setAttribute('stroke', '#a5f3fc');
    setTimeout(() => {
      if (sweepArm) sweepArm.setAttribute('stroke', COLOR_SWEEP);
    }, 140);
  } else if (activeRadar === 'sidebar' && sweepArmSidebar) {
    sweepArmSidebar.setAttribute('stroke', '#a5f3fc');
    setTimeout(() => {
      if (sweepArmSidebar) sweepArmSidebar.setAttribute('stroke', COLOR_SWEEP);
    }, 140);
  }
}

// ─── Position Calculation ───────────────────────────────────────────────────

/**
 * Compute the position at a given angle on the target ring (60% radius).
 * Uses SVG coordinate conventions: 0° = up, clockwise.
 *
 * @param {number} deg - Angle in degrees (0° = up, clockwise)
 * @returns {{ x: number, y: number }}
 */
function positionAtAngle(deg) {
  const rad = (deg * Math.PI) / 180;
  return {
    x: CX + TARGET_RADIUS * Math.sin(rad),
    y: CY - TARGET_RADIUS * Math.cos(rad),
  };
}

// ─── Targets ────────────────────────────────────────────────────────────────

/**
 * Add a target blip at the current sweep position (60% radius).
 * Appears bright white, then settles to normal color.
 *
 * @param {'triangle'|'square'} [type='triangle']
 * @param {'ws'|'rest'} [category='rest']
 * @returns {string} Target ID
 */
function addTarget(type = 'triangle', category = 'rest') {
  // Add target to the active radar's targets group
  const activeTargetsGroup = activeRadar === 'sidebar' ? targetsGroupSidebar : targetsGroup;
  
  if (!activeTargetsGroup) return null;

  const id = `target-${++targetIdCounter}`;
  const pos = positionAtAngle(angle);

  const group = document.createElementNS('http://www.w3.org/2000/svg', 'g');
  group.dataset.targetId = id;
  group.setAttribute('transform', `translate(${pos.x},${pos.y})`);

  let shape;
  if (type === 'triangle') {
    shape = document.createElementNS('http://www.w3.org/2000/svg', 'polygon');
    shape.setAttribute('points', '0,-4.5  3.5,3.5  -3.5,3.5');
  } else {
    shape = document.createElementNS('http://www.w3.org/2000/svg', 'rect');
    shape.setAttribute('x', '-3');
    shape.setAttribute('y', '-3');
    shape.setAttribute('width', '6');
    shape.setAttribute('height', '6');
  }

  // Start bright
  shape.setAttribute('fill', COLOR_BRIGHT);
  group.appendChild(shape);
  activeTargetsGroup.appendChild(group);

  // After initial flash, settle to normal color
  const normalColor = category === 'ws' ? COLOR_WS : COLOR_REST;
  const settleTimeout = setTimeout(() => {
    if (shape.parentNode) {
      shape.setAttribute('fill', normalColor);
    }
  }, TARGET_FLASH_MS);

  // Store target data
  // We store the angle the target was placed at so we can detect sweep passes
  const targetAngle = angle;
  activeTargets.set(id, { element: group, shape, category, targetAngle, settleTimeout });

  startRadarLoop();
  return id;
}

/**
 * Remove a target with a fade-out animation.
 * If no id, removes the oldest (FIFO).
 * On removal: flash bright → final color → fade to transparent.
 *
 * @param {string} [id]
 */
function removeTarget(id) {
  let data;
  if (id) {
    data = activeTargets.get(id);
  } else {
    const firstKey = activeTargets.keys().next().value;
    if (firstKey) {
      id = firstKey;
      data = activeTargets.get(firstKey);
    }
  }
  if (!data) return;

  const { element, shape, settleTimeout } = data;

  // Cancel any pending settle timeout
  clearTimeout(settleTimeout);

  // Flash bright, then to final color
  const finalColor = data.category === 'ws' ? COLOR_WS : COLOR_REST;
  shape.setAttribute('fill', COLOR_BRIGHT);
  setTimeout(() => {
    if (shape.parentNode) shape.setAttribute('fill', finalColor);
  }, 80);
  setTimeout(() => {
    if (shape.parentNode) shape.setAttribute('fill', COLOR_BRIGHT);
  }, 160);
  setTimeout(() => {
    if (shape.parentNode) shape.setAttribute('fill', finalColor);
  }, 240);

  // Set fade duration to half a sweep cycle
  element.style.setProperty('--radar-fade-duration', `${(HALF_PERIOD_MS / 1000).toFixed(2)}s`);

  // Start fade after the color flash sequence
  setTimeout(() => {
    element.classList.add('fading');
  }, 320);

  // Remove from DOM after fade completes
  const totalMs = 320 + HALF_PERIOD_MS;
  setTimeout(() => {
    element.remove();
    activeTargets.delete(id);
    if (activeTargets.size === 0 && !isConnected) {
      stopRadarLoop();
    }
  }, totalMs);
}

// ─── Animation Loop (time-based) ────────────────────────────────────────────

function raf(fn) {
  if (typeof window !== 'undefined' && typeof window.requestAnimationFrame === 'function') {
    return window.requestAnimationFrame(fn);
  }
  return setTimeout(fn, 16);
}

function caf(id) {
  if (typeof window !== 'undefined' && typeof window.cancelAnimationFrame === 'function') {
    window.cancelAnimationFrame(id);
  } else {
    clearTimeout(id);
  }
}

function startRadarLoop() {
  if (rafId) return;

  lastTimestamp = null;

  function animate(timestamp) {
    if (!isConnected && activeTargets.size === 0) {
      rafId = null;
      lastTimestamp = null;
      return;
    }

    // Time-based: compute delta from actual elapsed time
    if (lastTimestamp === null) {
      lastTimestamp = timestamp;
    }
    const dt = timestamp - lastTimestamp;
    lastTimestamp = timestamp;

    // Advance angle based on elapsed time (for ping detection)
    angle = (angle + SWEEP_DPS * dt) % 360;

    // Only update the active radar's sweep - NOW CSS-HANDLED
    // We keep the angle variable updated for ping detection only
    // Actual sweep rotation is handled by CSS animation

    // Check for sweep-pass pings on active targets
    activeTargets.forEach((target) => {
      const { element, targetAngle } = target;

      // Angular distance between sweep arm and target
      const diff = Math.abs(((angle - targetAngle + 180) % 360) - 180);

      if (diff < PING_THRESHOLD_DEG) {
        // Only ping if not already pinging
        if (!element.classList.contains('ping')) {
          element.classList.add('ping');

          // Flash the target bright while under the sweep
          const shape = element.querySelector('polygon, rect');
          if (shape) shape.setAttribute('fill', COLOR_BRIGHT);

          setTimeout(() => {
            element.classList.remove('ping');
            // Restore normal color
            if (shape && shape.parentNode) {
              const normalColor = target.category === 'ws' ? COLOR_WS : COLOR_REST;
              shape.setAttribute('fill', normalColor);
            }
          }, PING_DURATION_MS);
        }
      }
    });

    rafId = raf(animate);
  }

  rafId = raf(animate);
}

function stopRadarLoop() {
  if (rafId) {
    caf(rafId);
    rafId = null;
    lastTimestamp = null;
  }
}

// ─── Cleanup ────────────────────────────────────────────────────────────────

function destroyRadar() {
  stopRadarLoop();
  activeTargets.forEach(({ settleTimeout }) => clearTimeout(settleTimeout));
  activeTargets.clear();
  if (styleEl) {
    styleEl.remove();
    styleEl = null;
  }
  radarIcon = null;
  radarIconSidebar = null;
  sweepGroup = null;
  sweepArm = null;
  targetsGroup = null;
  sweepGroupSidebar = null;
  sweepArmSidebar = null;
  targetsGroupSidebar = null;
}

/**
 * Conduit - Shared helper functions for Hydrogen Conduit API queries
 *
 * Provides reusable, testable functions for building conduit payloads,
 * executing queries, and extracting results. All managers that interact
 * with conduit endpoints should use these helpers instead of hand-crafting
 * API calls.
 *
 * Conduit endpoints (see LITHIUM-API.md):
 *   POST /api/conduit/query        — single public query
 *   POST /api/conduit/queries       — batch public queries
 *   POST /api/conduit/auth_query    — single authenticated query (JWT)
 *   POST /api/conduit/auth_queries  — batch authenticated queries (JWT)
 *
 * Payload format:
 *   { query_ref: 25, params: { STRING: { status: "active" }, INTEGER: { limit: 10 } } }
 *
 * Batch payload:
 *   { queries: [ { query_ref: 1, params: {} }, { query_ref: 25, params: {} } ] }
 *   Optionally includes database: "DatabaseName" for public endpoints.
 */


// ─── Pure Functions (testable, no side-effects) ──────────────────────────────

/**
 * Build a single-query payload for conduit endpoints.
 * Always includes a `params` key (defaults to empty object).
 *
 * @param {number} queryRef - The QueryRef number to execute
 * @param {Object} [params={}] - Typed parameters, e.g. { STRING: { key: "val" }, INTEGER: { n: 10 } }
 * @returns {Object} Payload ready for POST body
 *
 * @example
 *   buildQueryPayload(25);
 *   // → { query_ref: 25, params: {} }
 *
 *   buildQueryPayload(32, { STRING: { SEARCH: "USERS" } });
 *   // → { query_ref: 32, params: { STRING: { SEARCH: "USERS" } } }
 */
function buildQueryPayload(queryRef, params = {}) {
  if (queryRef == null || typeof queryRef !== 'number') {
    throw new Error(`buildQueryPayload: queryRef must be a number, got ${typeof queryRef}`);
  }

  return {
    query_ref: queryRef,
    params,
  };
}

/**
 * Build a batch payload for conduit multi-query endpoints.
 *
 * @param {Array<{queryRef: number, params?: Object}>} queries - Array of query descriptors
 * @param {string} [database] - Optional database name (required for public endpoints)
 * @returns {Object} Payload ready for POST body
 *
 * @example
 *   buildBatchPayload([
 *     { queryRef: 1, params: {} },
 *     { queryRef: 25, params: { STRING: { status: "active" } } },
 *   ]);
 *   // → { queries: [ { query_ref: 1, params: {} }, { query_ref: 25, params: { STRING: { status: "active" } } } ] }
 *
 *   buildBatchPayload([ { queryRef: 1 } ], 'Acuranzo');
 *   // → { database: "Acuranzo", queries: [ { query_ref: 1, params: {} } ] }
 */
function buildBatchPayload(queries, database) {
  if (!Array.isArray(queries) || queries.length === 0) {
    throw new Error('buildBatchPayload: queries must be a non-empty array');
  }

  const payload = {
    queries: queries.map(q => buildQueryPayload(q.queryRef, q.params)),
  };

  if (database) {
    payload.database = database;
  }

  return payload;
}

/**
 * Extract the rows array from a single-query conduit response.
 *
 * Conduit returns different shapes depending on success/failure:
 *   Success: { data: [...] }  or  { rows: [...] }  or  { results: [{ rows: [...] }] }
 *
 * This function normalises all known shapes into a plain rows array.
 *
 * @param {Object} response - The parsed JSON response from the conduit endpoint
 * @returns {Array} The rows array, or an empty array if none found
 */
function extractRows(response) {
  if (!response) return [];

  // Direct data array (most common for single query)
  if (Array.isArray(response.data)) return response.data;

  // Direct rows array
  if (Array.isArray(response.rows)) return response.rows;

  // Wrapped in results array (batch-style single response)
  if (Array.isArray(response.results) && response.results.length > 0) {
    const first = response.results[0];
    if (first.success && Array.isArray(first.rows)) return first.rows;
  }

  // Response is itself an array
  if (Array.isArray(response)) return response;

  return [];
}

/**
 * Extract rows from a batch (multi-query) conduit response.
 * Returns a Map keyed by query_ref → rows array.
 *
 * @param {Object} response - The parsed JSON response from the batch endpoint
 * @returns {Map<number, Array>} Map of queryRef → rows
 */
function extractBatchRows(response) {
  const map = new Map();

  if (!response || !Array.isArray(response.results)) return map;

  for (const result of response.results) {
    const ref = result.query_ref;
    if (result.success) {
      map.set(ref, result.rows || []);
    } else {
      // Store error information so callers can handle failures per-query
      map.set(ref, { error: result.error || 'Unknown error', rows: [] });
    }
  }

  return map;
}

/**
 * Extract error information from a conduit response.
 * Returns null if no error, or an object with error details.
 *
 * @param {Object} response - The parsed JSON response from the conduit endpoint
 * @returns {Object|null} Error object with message, error, query_ref, database or null
 */
function extractError(response) {
  if (!response) return null;

  // Check for explicit success flag
  if (response.success === false) {
    return {
      message: response.message || response.error || 'Unknown error',
      error: response.error || 'Unknown error',
      queryRef: response.query_ref,
      database: response.database,
    };
  }

  return null;
}


// ─── API Functions (thin wrappers using the api client) ──────────────────

/**
 * Execute a single authenticated conduit query.
 * Requires a valid JWT (uses auth_query endpoint).
 *
 * @param {Object} api - The API client (from createRequest / app.api)
 * @param {number} queryRef - The QueryRef number
 * @param {Object} [params={}] - Typed parameters
 * @returns {Promise<Array>} The rows from the query result
 * @throws {Error} If the query fails, contains the server error message
 *
 * @example
 *   const rows = await authQuery(app.api, 25);
 *   const rows = await authQuery(app.api, 32, { STRING: { SEARCH: "USERS" } });
 */
async function authQuery(api, queryRef, params = {}) {
  const targetId = addTarget('triangle', 'rest');
  const payload = buildQueryPayload(queryRef, params);

  let response;
  try {
    response = await api.post('conduit/auth_query', payload);
  } catch (httpError) {
    const serverErr = extractError(httpError.data);
    if (serverErr) {
      httpError.serverError = serverErr;
    }
    throw httpError;
  } finally {
    if (targetId) removeTarget(targetId);
  }

  const error = extractError(response);
  if (error) {
    const err = new Error(error.message);
    err.serverError = error;
    throw err;
  }

  return extractRows(response);
}

/**
 * Execute multiple authenticated conduit queries in one request.
 *
 * @param {Object} api - The API client
 * @param {Array<{queryRef: number, params?: Object}>} queries - Query descriptors
 * @returns {Promise<Map<number, Array>>} Map of queryRef → rows
 *
 * @example
 *   const results = await authQueries(app.api, [
 *     { queryRef: 25 },
 *     { queryRef: 32, params: { STRING: { SEARCH: "USERS" } } },
 *   ]);
 *   const queryList = results.get(25);  // rows array
 */
async function authQueries(api, queries) {
  const targetId = addTarget('square', 'rest');
  const payload = buildBatchPayload(queries);
  try {
    const response = await api.post('conduit/auth_queries', payload);
    return extractBatchRows(response);
  } finally {
    if (targetId) removeTarget(targetId);
  }
}

/**
 * Execute a single public conduit query (no JWT required).
 *
 * @param {Object} api - The API client
 * @param {number} queryRef - The QueryRef number
 * @param {Object} [params={}] - Typed parameters
 * @param {string} [database] - Database name
 * @returns {Promise<Array>} The rows from the query result
 */
async function query(api, queryRef, params = {}, database) {
  const targetId = addTarget('triangle', 'rest');
  const payload = buildQueryPayload(queryRef, params);
  if (database) payload.database = database;
  try {
    const response = await api.post('conduit/query', payload);
    return extractRows(response);
  } finally {
    if (targetId) removeTarget(targetId);
  }
}

/**
 * Execute multiple public conduit queries in one request.
 *
 * @param {Object} api - The API client
 * @param {Array<{queryRef: number, params?: Object}>} queryList - Query descriptors
 * @param {string} [database] - Database name
 * @returns {Promise<Map<number, Array>>} Map of queryRef → rows
 */
async function queries(api, queryList, database) {
  const targetId = addTarget('square', 'rest');
  const payload = buildBatchPayload(queryList, database);
  try {
    const response = await api.post('conduit/queries', payload);
    return extractBatchRows(response);
  } finally {
    if (targetId) removeTarget(targetId);
  }
}


// ─── Default Export ──────────────────────────────────────────────────────

const conduit = {
  // Pure helpers
  buildQueryPayload,
  buildBatchPayload,
  extractRows,
  extractBatchRows,
  extractError,
  // API wrappers
  authQuery,
  authQueries,
  query,
  queries,
};

const conduit$1 = /*#__PURE__*/Object.freeze(/*#__PURE__*/Object.defineProperty({
  __proto__: null,
  authQueries,
  authQuery,
  buildBatchPayload,
  buildQueryPayload,
  default: conduit,
  extractBatchRows,
  extractError,
  extractRows,
  queries,
  query
}, Symbol.toStringTag, { value: 'Module' }));

export { authQuery as a, addTarget as b, wsFlaky as c, wsConnected as d, setRadarTooltip as e, destroyRadar as f, getRadarTooltip as g, conduit$1 as h, initRadar as i, onHeartbeat as o, removeTarget as r, setActiveRadar as s, wsDisconnected as w };
