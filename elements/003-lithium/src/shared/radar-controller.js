/**
 * Radar Status Controller
 *
 * Drives the SVG radar icon in the sidebar header. Tracks:
 *   - WebSocket connection health (rim color, sweep animation)
 *   - REST API requests (targets appear/disappear per request)
 *   - WS keepalive heartbeats (sweep line flash)
 *
 * The sweep animation runs via requestAnimationFrame only when connected
 * and targets exist, or when the WS connection is live.
 */

import { log, Subsystems, Status } from '../core/log.js';

// ─── DOM References (cached on init) ────────────────────────────────────────

let radarIcon = null;
let sweepGroup = null;
let sweepArm = null;
let targetsGroup = null;
let styleEl = null;

// ─── Internal State ─────────────────────────────────────────────────────────

let angle = 0;
let rafId = null;
let isConnected = false;
let activeTargets = new Map();
let targetIdCounter = 0;

// Sweep speed: degrees per frame at 60fps → full 360° in ~10s
const SWEEP_SPEED = 3.6;

// Colors
const COLOR_SWEEP = '#67e8f9';       // cyan sweep
const COLOR_SWEEP_BRIGHT = '#a5f3fc'; // brighter flash
const COLOR_CONNECTED = '#10b981';    // green
const COLOR_FLAKY = '#f59e0b';        // amber
const COLOR_DISCONNECTED = '#ef4444'; // red

// ─── CSS Ping Glow (injected once) ──────────────────────────────────────────

function ensurePingCSS() {
  if (styleEl) return;
  styleEl = document.createElement('style');
  styleEl.textContent = `
    #targets-group g.ping {
      filter: brightness(2.8) drop-shadow(0 0 6px #67e8f9);
      transition: filter 0.15s ease-out;
    }
    #radar-icon {
      transition: color 0.4s ease;
    }
  `;
  document.head.appendChild(styleEl);
}

// ─── Initialization ─────────────────────────────────────────────────────────

/**
 * Initialize the radar controller by caching DOM references.
 * Must be called after the sidebar HTML is rendered.
 * @returns {boolean} True if the radar SVG was found in the DOM
 */
export function initRadar() {
  radarIcon = document.getElementById('radar-icon');
  if (!radarIcon) {
    log(Subsystems.UI, Status.WARN, '[Radar] radar-icon element not found');
    return false;
  }

  sweepGroup = document.getElementById('sweep-group');
  sweepArm = document.getElementById('sweep-arm');
  targetsGroup = document.getElementById('targets-group');

  ensurePingCSS();

  // Start in disconnected state
  setConnectionState(false, false);

  log(Subsystems.UI, Status.DEBUG, '[Radar] Initialized');
  return true;
}

// ─── Connection State ───────────────────────────────────────────────────────

/**
 * Update radar based on WebSocket connection state.
 * @param {boolean} connected - Whether the WS is connected
 * @param {boolean} [flaky=false] - Whether the connection is flaky/warning
 */
export function setConnectionState(connected, flaky = false) {
  isConnected = connected;

  if (!radarIcon) return;

  if (!connected) {
    radarIcon.style.color = COLOR_DISCONNECTED;
  } else if (flaky) {
    radarIcon.style.color = COLOR_FLAKY;
  } else {
    radarIcon.style.color = COLOR_CONNECTED;
  }

  // Start or stop the sweep loop based on connection + targets
  if (connected) {
    startRadarLoop();
  } else {
    stopRadarLoop();
    // Shake animation on disconnect
    radarIcon.animate([
      { transform: 'rotate(0deg)' },
      { transform: 'rotate(8deg)' },
      { transform: 'rotate(-8deg)' },
      { transform: 'rotate(0deg)' },
    ], { duration: 420 });
  }
}

/**
 * Convenience: mark WS as connected
 */
export function wsConnected() {
  setConnectionState(true, false);
}

/**
 * Convenience: mark WS as flaky (amber)
 */
export function wsFlaky() {
  setConnectionState(true, true);
}

/**
 * Convenience: mark WS as disconnected
 */
export function wsDisconnected() {
  setConnectionState(false, false);
}

// ─── Heartbeat Flash ────────────────────────────────────────────────────────

/**
 * Flash the sweep arm briefly on WS keepalive heartbeat.
 */
export function onHeartbeat() {
  if (!isConnected || !sweepArm) return;

  const original = sweepArm.getAttribute('stroke');
  sweepArm.setAttribute('stroke', COLOR_SWEEP_BRIGHT);
  setTimeout(() => {
    if (isConnected && sweepArm) {
      sweepArm.setAttribute('stroke', COLOR_SWEEP);
    }
  }, 140);
}

// ─── Targets (REST API requests) ────────────────────────────────────────────

/**
 * Add a radar target blip. Call when a REST API request starts.
 * @param {'triangle'|'square'} [type='triangle'] - Shape of the blip
 * @returns {string} Unique target ID (pass to removeTarget when done)
 */
export function addTarget(type = 'triangle') {
  if (!targetsGroup) return null;

  const id = `target-${++targetIdCounter}`;

  const group = document.createElementNS('http://www.w3.org/2000/svg', 'g');
  group.dataset.targetId = id;

  // Place at 60% radius
  const radius = 16.5;
  const targetAngle = Math.random() * 360;
  const rad = (targetAngle * Math.PI) / 180;

  const x = 32 + radius * Math.cos(rad);
  const y = 32 + radius * Math.sin(rad);

  group.setAttribute('transform', `translate(${x},${y}) rotate(${targetAngle})`);

  let shape;
  if (type === 'triangle') {
    shape = document.createElementNS('http://www.w3.org/2000/svg', 'polygon');
    shape.setAttribute('points', '0,-4.5  3.5,3.5  -3.5,3.5');
    shape.setAttribute('fill', COLOR_SWEEP);
  } else {
    shape = document.createElementNS('http://www.w3.org/2000/svg', 'rect');
    shape.setAttribute('x', '-3');
    shape.setAttribute('y', '-3');
    shape.setAttribute('width', '6');
    shape.setAttribute('height', '6');
    shape.setAttribute('fill', COLOR_SWEEP);
  }

  group.appendChild(shape);
  targetsGroup.appendChild(group);
  activeTargets.set(id, { element: group, angle: targetAngle });

  startRadarLoop();
  return id;
}

/**
 * Remove a radar target blip. Call when a REST API request completes.
 * @param {string} id - The target ID returned by addTarget
 */
export function removeTarget(id) {
  const data = activeTargets.get(id);
  if (data) {
    data.element.remove();
    activeTargets.delete(id);
  }
}

// ─── Animation Loop ─────────────────────────────────────────────────────────

function startRadarLoop() {
  if (rafId) return; // already running

  function animate() {
    if (!isConnected && activeTargets.size === 0) {
      rafId = null;
      return;
    }

    angle = (angle + SWEEP_SPEED) % 360;

    if (sweepGroup) {
      sweepGroup.setAttribute('transform', `rotate(${angle} 32 32)`);
    }
    if (sweepArm) {
      sweepArm.setAttribute('transform', `rotate(${angle} 32 32)`);
    }

    // Check for ping highlights when sweep passes over targets
    if (activeTargets.size > 0) {
      activeTargets.forEach(({ element, angle: targetAngle }) => {
        const diff = Math.abs(((angle - targetAngle + 180) % 360) - 180);
        if (diff < 4) {
          element.classList.add('ping');
          setTimeout(() => element.classList.remove('ping'), 180);
        }
      });
    }

    rafId = requestAnimationFrame(animate);
  }

  rafId = requestAnimationFrame(animate);
}

function stopRadarLoop() {
  if (rafId) {
    cancelAnimationFrame(rafId);
    rafId = null;
  }
}

// ─── Cleanup ────────────────────────────────────────────────────────────────

/**
 * Destroy the radar controller and clean up DOM elements.
 */
export function destroyRadar() {
  stopRadarLoop();
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
