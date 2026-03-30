/**
 * Radar Status Controller
 *
 * Drives the SVG radar icon in the sidebar header. Tracks:
 *   - WebSocket messages (targets appear/disappear per send/receive)
 *   - REST API requests (targets appear/disappear per request)
 *   - WS connection health (rim color, sweep animation)
 *   - WS keepalive heartbeats (sweep line flash)
 *
 * Animation is time-based (not frame-count-based), so all timing is
 * deterministic and independent of frame rate. The sweep completes a
 * full 360° rotation in exactly SWEEP_PERIOD_MS.
 *
 * Target lifecycle:
 *   1. Appear bright white at the sweep arm tip position (60% radius)
 *   2. Quickly settle to their normal color (red for REST, green for WS)
 *   3. Flash bright again each time the sweep arm passes over them
 *   4. On removal: flash to final color, then fade out over half a sweep cycle
 */

// ─── DOM References (cached on init) ────────────────────────────────────────

let radarIcon = null;
let sweepGroup = null;
let sweepArm = null;
let targetsGroup = null;
let styleEl = null;

// ─── Internal State ─────────────────────────────────────────────────────────

let angle = 0;                   // current sweep angle in degrees (0–360)
let rafId = null;
let lastTimestamp = null;        // timestamp of last animation frame
let isConnected = false;
const activeTargets = new Map(); // id → { element, category, targetAngle }
let targetIdCounter = 0;

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
    #targets-group rect {
      transition: fill 0.25s ease, opacity 0.25s ease;
    }
    #targets-group g.ping polygon,
    #targets-group g.ping rect {
      filter: brightness(2.0) drop-shadow(0 0 5px #ffffff);
    }
    #targets-group g.fading {
      opacity: 0;
      transition: opacity var(--radar-fade-duration, 1.67s) linear;
    }
    #radar-icon {
      color: var(--accent-primary, #5C6BC0);
      transition: color 0.4s ease;
    }
  `;
  document.head.appendChild(styleEl);
}

// ─── Initialization ─────────────────────────────────────────────────────────

export function initRadar() {
  radarIcon = document.getElementById('radar-icon');
  if (!radarIcon) return false;

  sweepGroup = document.getElementById('sweep-group');
  sweepArm = document.getElementById('sweep-arm');
  targetsGroup = document.getElementById('targets-group');

  ensurePingCSS();
  applySweepColors();
  setConnectionState(false, false);

  return true;
}

function applySweepColors() {
  if (sweepArm) sweepArm.setAttribute('stroke', COLOR_SWEEP);
  if (sweepGroup) {
    sweepGroup.querySelectorAll('path').forEach(p => {
      p.setAttribute('fill', COLOR_SWEEP);
    });
  }
}

// ─── Connection State ───────────────────────────────────────────────────────

export function setConnectionState(connected, flaky = false) {
  isConnected = connected;
  if (!radarIcon) return;

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
}

export function wsConnected() { setConnectionState(true, false); }
export function wsFlaky() { setConnectionState(true, true); }
export function wsDisconnected() { setConnectionState(false, false); }

// ─── Heartbeat Flash ────────────────────────────────────────────────────────

export function onHeartbeat() {
  if (!isConnected || !sweepArm) return;

  sweepArm.setAttribute('stroke', '#a5f3fc');
  setTimeout(() => {
    if (sweepArm) sweepArm.setAttribute('stroke', COLOR_SWEEP);
  }, 140);
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
export function addTarget(type = 'triangle', category = 'rest') {
  if (!targetsGroup) return null;

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
  targetsGroup.appendChild(group);

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
export function removeTarget(id) {
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

    // Advance angle based on elapsed time
    angle = (angle + SWEEP_DPS * dt) % 360;

    // Update sweep transform
    if (sweepGroup) {
      sweepGroup.setAttribute('transform', `rotate(${angle} ${CX} ${CY})`);
    }
    if (sweepArm) {
      sweepArm.setAttribute('transform', `rotate(${angle} ${CX} ${CY})`);
    }

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

export function destroyRadar() {
  stopRadarLoop();
  activeTargets.forEach(({ settleTimeout }) => clearTimeout(settleTimeout));
  activeTargets.clear();
  if (styleEl) {
    styleEl.remove();
    styleEl = null;
  }
  radarIcon = null;
  sweepGroup = null;
  sweepArm = null;
  targetsGroup = null;
}

// ─── Default Export ─────────────────────────────────────────────────────────

export default {
  initRadar,
  setConnectionState,
  wsConnected,
  wsFlaky,
  wsDisconnected,
  onHeartbeat,
  addTarget,
  removeTarget,
  destroyRadar,
};
