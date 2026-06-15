const __vite__mapDeps=(i,m=__vite__mapDeps,d=(m.f||(m.f=["./login-CJ3GMAAP.js","./languages-Coru4QdC.js","./scrollbar-manager-Bw-KcEhe.js","./login-CGvuXpXr.css","./vendor-tabulator-CgpSKb-e.css","./dashboard-Dyj7k_ns.js","./dashboard-DgDvod-K.css","./mail-manager-tqaUe5TF.js","./mail-manager-BUcQlbJg.css","./server-profiles-BT5GzYq5.js","./server-profiles-CcKEE3gH.css","./server-sessions-DkOUj5VY.js","./server-sessions-NYEjzkWX.css","./version-history-DhTvkkD5.js","./lithium-table-main-ClaptTZo.js","./tabulator_esm-DsDetXL7.js","./conduit-B8Ld7ghl.js","./lithium-table-main-BKkowb5Q.css","./manager-edit-helper-CB5F076B.js","./manager-edit-helper-CX-3QgZH.css","./manager-ui-B8HP2uIT.js","./codemirror-setup-lxLGSWU-.js","./codemirror-Bg1rM8n2.js","./manager-ui-BfAXUPa9.css","./toolbar-D02XmjBN.js","./version-history-ZmLnnjHa.css","./calendar-manager-BbwTkDp9.js","./calendar-manager-C6ve4CGL.css","./contact-manager-BmLpgjLX.js","./contact-manager-DhgbeVX0.css","./file-manager-B4k_RVAx.js","./file-manager-BUfp0UPV.css","./document-library-CPVBtoUE.js","./document-library-CltSgYqX.css","./media-library-DBSIGjBk.js","./media-library-BJYAbry0.css","./diagram-library-Bejw-KFP.js","./diagram-library-CebTa5V1.css","./chats-DfR9kckd.js","./chats-_6x8etvk.css","./notification-manager-BYbN9li5.js","./notification-manager--67bg3Zq.css","./annotation-manager-sq6PTP15.js","./annotation-manager-Z34lW4Mk.css","./ticketing-manager-BQ7P7EkV.js","./ticketing-manager-dJ9pGcff.css","./style-manager-C-T7mqzw.js","./font-popup-D81b7hea.js","./style-manager-6EubRfO4.css","./lookups-B_m-1PlQ.js","./audit-info-footer-CP2R6gh2.js","./luxon-DbyXf6dF.js","./_commonjsHelpers-B85MJLTf.js","./lookups-qr5KPaHv.css","./reports-oxF4xt_m.js","./reports-DnbEvUPV.css","./role-manager-edDk62YU.js","./role-manager-BLaoJ8PY.css","./security-manager-D_d_wBKH.js","./security-manager-D4k41ZQG.css","./ai-auditor-B0PTDy_L.js","./ai-auditor-YE4bc4LY.css","./job-manager-DE98yyDl.js","./job-manager-B6u72CDv.css","./queries-DWQDr50V.js","./queries-nosdh-iN.css","./sync-manager-DGd7MsAz.js","./sync-manager-Dx2Nfedr.css","./camera-manager-fk-CGRVm.js","./camera-manager-B-freztQ.css","./terminal-_IKFZm20.js","./terminal-BJU_cDIN.css","./session-log-DxaXKH9Q.js","./session-log-BU3D75pj.css","./profile-manager-YaKVbSkH.js","./profile-manager-BAdNDvLN.css","./popup-BXcIgOoO.css","./main-BzM9uq37.js","./zoom-control-BOsJBiLz.js","./main-BU23_-dd.css"])))=>i.map(i=>d[i]);
true              &&(function polyfill() {
	const relList = document.createElement("link").relList;
	if (relList && relList.supports && relList.supports("modulepreload")) return;
	for (const link of document.querySelectorAll("link[rel=\"modulepreload\"]")) processPreload(link);
	new MutationObserver((mutations) => {
		for (const mutation of mutations) {
			if (mutation.type !== "childList") continue;
			for (const node of mutation.addedNodes) if (node.tagName === "LINK" && node.rel === "modulepreload") processPreload(node);
		}
	}).observe(document, {
		childList: true,
		subtree: true
	});
	function getFetchOpts(link) {
		const fetchOpts = {};
		if (link.integrity) fetchOpts.integrity = link.integrity;
		if (link.referrerPolicy) fetchOpts.referrerPolicy = link.referrerPolicy;
		if (link.crossOrigin === "use-credentials") fetchOpts.credentials = "include";
		else if (link.crossOrigin === "anonymous") fetchOpts.credentials = "omit";
		else fetchOpts.credentials = "same-origin";
		return fetchOpts;
	}
	function processPreload(link) {
		if (link.ep) return;
		link.ep = true;
		const fetchOpts = getFetchOpts(link);
		fetch(link.href, fetchOpts);
	}
}());

/**
 * Console Originals - Shared reference to native console methods
 *
 * This module exports an object that console-capture.js populates with
 * the original console methods before overriding them. Other modules
 * (especially log.js) can import and use these to avoid infinite recursion
 * when console methods are wrapped.
 *
 * In test environments where console-capture.js is not loaded, these
 * remain null and modules should fall back to console.X directly.
 */

const originalConsole = {
  log: null,
  error: null,
  warn: null,
  info: null,
  debug: null,
};

/**
 * Actions Log - Lightweight, non-blocking logging system
 * 
 * Provides a simple API for tracking application events.
 * Data is stored as JSON with raw values, formatted only on display.
 * 
 * Raw storage format (all raw values, formatted on display):
 * - counter: integer (1, 2, 3...)
 * - timestamp: epoch number (milliseconds since Unix epoch)
 * - subsystem: string ('Startup', 'JWT', etc.)
 * - status: string ('INFO', 'WARN', 'ERROR'...)
 * - duration: number in ms (0, 32.5, 100...)
 * - description: string
 * - sessionId: string (generated on first log)
 * 
 * Display format (two-space separated):
 * COUNTER | TIMESTAMP | SUBSYSTEM | STATUS | DURATION | DESCRIPTION
 * e.g.: "000 123  |  2026-03-07 23:51:32.844  |  Startup  |  INFO  |  0.0  |  Application starting"
 */


// Log entry counter
let _counter = 0;

// HTTP request counter for RESTAPI calls
let _httpRequestCounter = 0;

// Session identifier - generated on first log entry
let _sessionId = null;

// In-memory log store - accumulates all log entries, never cleared
const _buffer = [];

// Server sync queue - entries pending upload to server
const _syncQueue = [];
const BUFFER_SIZE = 10;
const FLUSH_INTERVAL = 5000; // 5 seconds

// Storage key for current-session log persistence (sessionStorage)
const LOG_STORAGE_KEY = 'lithium_action_logs';

// Prefix for archived session logs (localStorage)
// Key format: lithium_log_archive_<shortSessionId>_<isoTimestamp>
const LOG_ARCHIVE_PREFIX = 'lithium_log_archive_';

// Pending flush timer
let _flushTimer = null;

// Console logging enabled (controlled by config)
let _consoleLogging = false;

// Subsystem constants for consistency
const Subsystems = {
  STARTUP: 'Startup',
  JWT: 'JWT',
  EVENTBUS: 'EventBus',
  HTTP: 'HTTP',
  RESTAPI: 'Conduit',
  CONDUIT: 'Conduit',
  MANAGER: 'Manager',
  SESSION: 'Session',
  AUTH: 'Auth',
  CONFIG: 'Config',
  LOOKUPS: 'Lookups',
  ICONS: 'Icons',
  PERMS: 'Permissions',
  THEME: 'Theme',
  LOGIN: 'Login',
  SESSIONLOG: 'SessionLog',
  WEBSOCKET: 'WebSocket',
  CRIMSON: 'Crimson',
  CONSOLE: 'Console',
};

// Status constants
const Status = {
  INFO: 'INFO',
  WARN: 'WARN',
  ERROR: 'ERROR',
  FAIL: 'FAIL',
  DEBUG: 'DEBUG',
  SUCCESS: 'SUCCESS',
};

/**
 * Set console logging enabled/disabled
 * @param {boolean} enabled - Whether to log to console
 */
function setConsoleLogging(enabled) {
  _consoleLogging = enabled;
}

/**
 * Return (or create) the session ID for this page load.
 * A new ID is generated the first time this is called — which happens on the first
 * call to log().  Since every page reload evaluates the module fresh, _sessionId
 * always starts as null and a brand-new ID is always created, giving each startup
 * a distinct session automatically.
 */
function _ensureSessionId() {
  if (!_sessionId) {
    const timestamp = Date.now().toString(36);
    const random1 = _getRandomString(8);
    const random2 = _getRandomString(8);
    _sessionId = `${timestamp}-${random1}-${random2}`;
  }
  return _sessionId;
}

/**
 * Generate a cryptographically secure random string
 * @param {number} length - Length of the string to generate
 * @returns {string} Random alphanumeric string
 */
function _getRandomString(length) {
  const chars = 'abcdefghijklmnopqrstuvwxyz0123456789';
  let result = '';
  const randomValues = new Uint8Array(length);
  
  // Use crypto.getRandomValues for cryptographically secure randomness
  crypto.getRandomValues(randomValues);
  for (let i = 0; i < length; i++) {
    result += chars[randomValues[i] % chars.length];
  }
  return result;
}

/**
 * Archive a completed session's log entries to localStorage.
 * Key format: lithium_log_archive_<shortId>_<isoTimestamp>
 * The short ID is the first 8 characters of the session ID.
 * @param {string} sessionId - The session ID of the archived log
 * @param {Array}  entries   - The log entries to archive
 */
function _archiveSessionLog(sessionId, entries) {
  try {
    const shortId = sessionId.substring(0, 8);
    const ts = new Date().toISOString().replace(/[:.]/g, '-');
    const key = `${LOG_ARCHIVE_PREFIX}${shortId}_${ts}`;
    localStorage.setItem(key, JSON.stringify({ sessionId, entries }));
  } catch (_e) {
    // Ignore storage errors (quota exceeded, private browsing, etc.)
  }
}

/**
 * On startup, check sessionStorage for any log entries left by the previous page load.
 * Because _sessionId is always null at this point (first log() call hasn't happened yet),
 * any stored entries belong to a previous session and are archived to localStorage for
 * future server upload.  The current session always starts with a clean slate.
 */
function _loadFromStorage() {
  try {
    const stored = sessionStorage.getItem(LOG_STORAGE_KEY);
    if (!stored) return;

    const data = JSON.parse(stored);
    if (!Array.isArray(data) || data.length === 0) return;

    // Archive previous session's entries to localStorage
    const storedSessionId = data[0]?.sessionId;
    if (storedSessionId) {
      _archiveSessionLog(storedSessionId, data);
    }

    // Clear sessionStorage so this session starts fresh
    try {
      sessionStorage.removeItem(LOG_STORAGE_KEY);
    } catch (_e) {
      // Ignore
    }
  } catch (_e) {
    // Ignore storage errors
  }
}

/**
 * Save buffer to sessionStorage (for cross-chunk persistence)
 */
function _saveToStorage() {
  try {
    sessionStorage.setItem(LOG_STORAGE_KEY, JSON.stringify(_buffer));
  } catch (_e) {
    // Ignore storage errors (quota exceeded, private browsing, etc.)
  }
}

/**
 * Format counter as 6-digit with space in middle (e.g., "000 123")
 * @param {number} n - Counter number
 * @returns {string} Formatted counter
 */
function _formatCounter(n) {
  const s = String(n).padStart(6, '0');
  return s.slice(0, 3) + ' ' + s.slice(3);
}

/**
 * Format epoch timestamp for display: YYYY-MM-DD HH:MM:SS.ZZZ
 * @param {number} epoch - Epoch timestamp in milliseconds
 * @returns {string} Formatted timestamp
 */
function _formatTimestamp(epoch) {
  const date = new Date(epoch);
  const dateStr = date.getUTCFullYear() + '-' +
    String(date.getUTCMonth() + 1).padStart(2, '0') + '-' +
    String(date.getUTCDate()).padStart(2, '0');
  const timeStr = String(date.getUTCHours()).padStart(2, '0') + ':' +
    String(date.getUTCMinutes()).padStart(2, '0') + ':' +
    String(date.getUTCSeconds()).padStart(2, '0') + '.' +
    String(date.getUTCMilliseconds()).padStart(3, '0');
  return `${dateStr} ${timeStr}`;
}

/**
 * Format a single log entry for display.
 * If description is a grouped object ({ title, items }), expands to multiple lines.
 * @param {Object} entry - Log entry with raw values
 * @returns {string} Formatted log line (may contain newlines for grouped entries)
 */
function _formatEntry(entry) {
  const counter = _formatCounter(entry.counter);
  const timestamp = _formatTimestamp(entry.timestamp);
  const duration = typeof entry.duration === 'number' ? entry.duration.toFixed(1) : '0.0';
  const prefix = `${counter}  |  ${timestamp}  |  ${entry.subsystem}  |  ${entry.status}  |  ${duration}  |  `;

  const desc = entry.description;
  if (desc && typeof desc === 'object' && desc.title !== undefined && Array.isArray(desc.items)) {
    // Grouped entry: render title + continuation lines
    const lines = [`${prefix}${desc.title}`];
    for (const item of desc.items) {
      lines.push(`${prefix}― ${item}`);
    }
    return lines.join('\n');
  }

  return `${prefix}${desc}`;
}

/**
 * Create a log entry object (JSON format with raw values)
 * @param {string} subsystem - Source subsystem
 * @param {string} status - Status code
 * @param {number} duration - Duration in ms
 * @param {string} description - Log description
 * @returns {Object} Log entry object
 */
function _createEntry(subsystem, status, duration, description) {
  return {
    sessionId: _ensureSessionId(),
    counter: ++_counter,
    timestamp: Date.now(), // Store as epoch number
    subsystem,
    status,
    duration,
    description,
  };
}

// Initialize: load any existing logs from sessionStorage (handles dynamic imports)
_loadFromStorage();

/**
 * Log an action entry (non-blocking)
 * 
 * @param {string} subsystem - The subsystem generating the log (e.g., 'JWT', 'Startup')
 * @param {string} status - Status: INFO, WARN, ERROR, FAIL, DEBUG, SUCCESS
 * @param {string} description - What happened
 * @param {number} [duration=0] - Optional duration in milliseconds
 * 
 * @example
 * // Simple usage - just log what happened
 * log('Startup', 'INFO', 'Application starting');
 * 
 * // With duration for timing
 * log('HTTP', 'INFO', 'Request /api/data', 32.5);
 * 
 * // Error case
 * log('JWT', 'ERROR', 'Token validation failed: expired');
 */
function log(subsystem, status, description, duration = 0) {
  // Create JSON entry with raw values
  const entry = _createEntry(subsystem, status, duration, description);
  
  // Output to console only if enabled
  if (_consoleLogging) {
    // Use originalConsole if available (production with console-capture),
    // otherwise fall back to console.log (tests, or production without capture)
    const consoleMethod = originalConsole.log || console.log;
    consoleMethod(`[LOG] ${_formatEntry(entry)}`);
  }
  
  // Add to in-memory log store (never cleared)
  _buffer.push(entry);

  // Add to server sync queue
  _syncQueue.push(entry);

  // Persist to sessionStorage for cross-chunk access
  _saveToStorage();

  // Flush if sync queue is full
  if (_syncQueue.length >= BUFFER_SIZE) {
    _scheduleFlush();
  }
  
  // Fire-and-forget - return immediately, don't await anything
  return;
}

/**
 * Schedule a flush (debounced)
 */
function _scheduleFlush() {
  if (_flushTimer) return;
  
  _flushTimer = setTimeout(() => {
    _flushTimer = null;
    _flushToServer();
  }, 100); // Small delay to batch multiple rapid logs
}

/**
 * Flush sync queue to server (async, non-blocking)
 * NOTE: Only the sync queue is drained; the in-memory _buffer is never modified.
 * @private
 */
async function _flushToServer() {
  // Server logging endpoint is not configured - clear the queue without sending
  // Logs remain available client-side via window.lithiumLogs
  _syncQueue.length = 0;
}

// Set up periodic flush
setInterval(() => {
  if (_syncQueue.length > 0) {
    _flushToServer();
  }
}, FLUSH_INTERVAL);

/**
 * Convenience methods for common log types
 */

// Shortcut for startup logs
function logStartup(description, duration = 0) {
  return log(Subsystems.STARTUP, Status.INFO, description, duration);
}

// Shortcut for JWT/auth logs
function logAuth(status, description, duration = 0) {
  return log(Subsystems.AUTH, status, description, duration);
}

/**
 * Get next HTTP request number (for Request 001, Request 002, etc.)
 * @returns {string} Zero-padded request number (e.g., "001")
 */
function getNextHttpRequestNum() {
  return String(++_httpRequestCounter).padStart(3, '0');
}

/**
 * Log an HTTP request (RESTAPI subsystem)
 * @param {string} method - HTTP method (GET, POST, PUT, DELETE, PATCH)
 * @param {string} path - API path
 * @returns {string} Request number for matching with response
 */
function logHttpRequest(method, path) {
  const num = getNextHttpRequestNum();
  log(Subsystems.RESTAPI, Status.INFO, `Request ${num}: ${method} ${path}`, 0);
  return num;
}

/**
 * Log an HTTP response with optional code, size, and duration
 * @param {string} requestNum - Request number from logHttpRequest
 * @param {string} method - HTTP method
 * @param {string} path - API path
 * @param {number|null} code - HTTP status code (e.g., 200)
 * @param {number|null} size - Response size in bytes
 * @param {number|null} duration - Response time in ms
 */
function logHttpResponse(requestNum, method, path, responseInfo = {}) {
  const { code = null, size = null, duration = null } = responseInfo;
  
  // Build description based on what's available
  const items = [];
  if (code !== null) items.push(`Code: ${code}`);
  if (size !== null) items.push(`Size: ${size.toLocaleString()} bytes`);
  if (duration !== null) items.push(`Time: ${duration}ms`);
  
  if (items.length > 0) {
    logGroup(Subsystems.RESTAPI, Status.INFO, `Response ${requestNum}: ${method} ${path}`, items, duration || 0);
  } else {
    log(Subsystems.RESTAPI, Status.INFO, `Response ${requestNum}: ${method} ${path}`, duration || 0);
  }
}

// Shortcut for manager logs
function logManager(status, description, duration = 0) {
  return log(Subsystems.MANAGER, status, description, duration);
}

/**
 * Log a grouped entry: one line with a title and continuation items.
 * Stored as a single JSON entry; displayed as title + ― item lines.
 *
 * @param {string} subsystem - The subsystem generating this entry
 * @param {string} status - Status code
 * @param {string} title - Top-level description (the header line)
 * @param {string[]} items - Continuation lines shown as ― item
 * @param {number} [duration=0] - Optional duration in ms
 *
 * @example
 * logGroup(Subsystems.STARTUP, Status.INFO, 'Browser Environment', [
 *   `Browser: Firefox 148`,
 *   `Platform: Linux x86_64`,
 *   `Language: en-US`,
 * ]);
 */
function logGroup(subsystem, status, title, items, duration = 0) {
  return log(subsystem, status, { title, items }, duration);
}

/**
 * Get all log entries as formatted display strings
 * @returns {Array} Array of formatted log lines
 */
function getDisplayLog() {
  return _buffer.map(entry => _formatEntry(entry));
}

/**
 * Get recent logs formatted for UI display
 * Format: local time (HH:MM:SS.ZZZ) + subsystem + description
 * Grouped entries ({ title, items }) are expanded to multiple lines with ― prefix.
 * @param {number} maxEntries - Maximum number of entries to return (default 50)
 * @returns {Array} Array of formatted log strings (may include multi-line entries)
 */
function getRecentLogs(maxEntries = 50) {
  const recent = _buffer.slice(-maxEntries);
  const lines = [];
  for (const entry of recent) {
    const date = new Date(entry.timestamp);
    const time = String(date.getHours()).padStart(2, '0') + ':' +
      String(date.getMinutes()).padStart(2, '0') + ':' +
      String(date.getSeconds()).padStart(2, '0') + '.' +
      String(date.getMilliseconds()).padStart(3, '0');
    const prefix = `${time}  ${entry.subsystem}`;
    const desc = entry.description;
    if (desc && typeof desc === 'object' && desc.title !== undefined && Array.isArray(desc.items)) {
      lines.push(`${prefix}  ${desc.title}`);
      for (const item of desc.items) {
        lines.push(`${prefix}  ― ${item}`);
      }
    } else {
      lines.push(`${prefix}  ${desc}`);
    }
  }
  return lines;
}

/**
 * Get raw log entries for JSON export/analysis
 * @returns {Array} Array of raw log entries
 */
function getRawLog() {
  return [..._buffer];
}

/**
 * Get current session ID
 * @returns {string|null} Current session ID
 */
function getSessionId() {
  return _sessionId;
}

/**
 * Get current counter value
 * @returns {number} Current counter (integer)
 */
function getCounter() {
  return _counter;
}

/**
 * Force flush buffer to server (for cleanup on logout, etc.)
 */
async function flush() {
  if (_flushTimer) {
    clearTimeout(_flushTimer);
    _flushTimer = null;
  }
  await _flushToServer();
}

/**
 * Parse a single archived session entry from localStorage.
 * @param {string} key - The localStorage key
 * @returns {Object|null} Parsed entry or null if invalid
 * @private
 */
function _parseArchivedEntry(key) {
  try {
    const value = localStorage.getItem(key);
    const parsed = JSON.parse(value);
    return { key, ...parsed };
  } catch (_e) {
    // Skip malformed entries
    return null;
  }
}

/**
 * Check if a key is an archive key and extract it.
 * @param {number} index - Index in localStorage
 * @returns {string|null} Key if valid archive key, null otherwise
 * @private
 */
function _getArchiveKeyAtIndex(index) {
  const key = localStorage.key(index);
  if (key && key.startsWith(LOG_ARCHIVE_PREFIX)) {
    return key;
  }
  return null;
}

/**
 * Get all archived session log entries from localStorage.
 * Keys are of the form: lithium_log_archive_<shortId>_<timestamp>
 * Returns an array of { key, sessionId, entries } objects sorted by key (oldest first).
 * @returns {Array} Archived session records
 */
function getArchivedSessions() {
  const results = [];
  
  try {
    for (let i = 0; i < localStorage.length; i++) {
      const key = _getArchiveKeyAtIndex(i);
      if (!key) continue;
      
      const entry = _parseArchivedEntry(key);
      if (entry) {
        results.push(entry);
      }
    }
  } catch (_e) {
    // Ignore storage errors
  }
  
  // Sort oldest first by key (key contains ISO timestamp)
  return results.sort((a, b) => a.key.localeCompare(b.key));
}

/**
 * Remove a specific archived session from localStorage.
 * Called after successfully uploading to server.
 * @param {string} key - The localStorage key returned by getArchivedSessions()
 */
function removeArchivedSession(key) {
  try {
    localStorage.removeItem(key);
  } catch (_e) {
    // Ignore storage errors
  }
}

/**
 * Console Capture - Intercept console methods and global errors
 *
 * Captures all console output (log, error, warn, info, debug) and unhandled
 * errors/rejections into the Lithium session log for comprehensive debugging.
 *
 * This module should be imported as early as possible (first import in app.js)
 * to capture as much console activity as possible.
 */


// Save original console methods to shared reference BEFORE overriding
originalConsole.log = console.log;
originalConsole.error = console.error;
originalConsole.warn = console.warn;
originalConsole.info = console.info;
originalConsole.debug = console.debug;

// Map console method to status level
const _methodStatusMap = {
  log: Status.INFO,
  info: Status.INFO,
  debug: Status.DEBUG,
  warn: Status.WARN,
  error: Status.ERROR,
};

// Format arguments into a readable string
function _formatArgs(args) {
  return Array.from(args).map(arg => {
    if (arg instanceof Error) {
      return arg.stack || arg.message;
    }
    if (typeof arg === 'object' && arg !== null) {
      try {
        return JSON.stringify(arg);
      } catch (_e) {
        return String(arg);
      }
    }
    return String(arg);
  }).join(' ');
}

// Create handler that logs to session and calls original
function _createHandler(method) {
  return function (...args) {
    // Call original console method first to preserve normal output
    if (originalConsole[method]) {
      originalConsole[method].apply(console, args);
    }

    // Log to session logger
    try {
      const message = _formatArgs(args);
      const status = _methodStatusMap[method] || Status.INFO;
      log(Subsystems.CONSOLE, status, `[${method.toUpperCase()}] ${message}`, 0);
    } catch (_e) {
      // Never throw from console override
    }
  };
}

// Override console methods
console.log = _createHandler('log');
console.error = _createHandler('error');
console.warn = _createHandler('warn');
console.info = _createHandler('info');
console.debug = _createHandler('debug');

// Global uncaught error handler
window.onerror = function (message, source, lineno, colno, error) {
  try {
    let logMessage = `Uncaught Error: ${message}`;
    if (source) {
      logMessage += ` at ${source}:${lineno}:${colno}`;
    }
    if (error && error.stack) {
      logMessage += `\nStack: ${error.stack}`;
    }
    log(Subsystems.CONSOLE, Status.ERROR, logMessage, 0);
  } catch (_e) {
    // Ignore errors in error handler
  }
  // Return false to allow default browser error handling as well
  return false;
};

// Global unhandled promise rejection handler
window.addEventListener('unhandledrejection', function (event) {
  try {
    const reason = event.reason;
    let logMessage = 'Unhandled Promise Rejection: ';
    if (reason instanceof Error) {
      logMessage += reason.message;
      if (reason.stack) {
        logMessage += `\nStack: ${reason.stack}`;
      }
    } else {
      logMessage += String(reason);
    }
    log(Subsystems.CONSOLE, Status.ERROR, logMessage, 0);
  } catch (_e) {
    // Ignore errors in rejection handler
  }
  // Do not prevent default - let it still appear in console
});

log(Subsystems.CONSOLE, Status.INFO, 'Console capture initialized - all console output will be logged to session', 0);

/**
 * Suppress vendor CSS parsing warnings that flood the console
 * These come from Tabulator, CodeMirror, etc. dynamically injecting CSS
 */
(function() {
  const originalConsoleError = console.error;
  const cssWarningPattern = /Error in parsing value for '(text-align|min-height)'\. Declaration dropped\./;

  console.error = function(...args) {
    if (args.length > 0 && typeof args[0] === 'string' && cssWarningPattern.test(args[0])) {
      return;
    }
    originalConsoleError.apply(console, args);
  };
})();

/**
 * Monkey-patch CSSStyleDeclaration to intercept invalid values at the source
 * This prevents the browser's CSS parser from ever seeing invalid values
 */
(function() {
  const INVALID_VALUES = new Set(['', 'null', 'undefined', 'NaN', 'inherit', 'initial']);
  const PROBLEMATIC_PROPS = ['text-align', 'min-height'];

  function isInvalidValue(value) {
    if (value == null) return true;
    const str = String(value).trim();
    return str === '' || INVALID_VALUES.has(str.toLowerCase());
  }

  // Patch setProperty
  const originalSetProperty = CSSStyleDeclaration.prototype.setProperty;
  CSSStyleDeclaration.prototype.setProperty = function(property, value, priority) {
    const propLower = property.toLowerCase();
    if (PROBLEMATIC_PROPS.includes(propLower) && isInvalidValue(value)) {
      return;
    }
    return originalSetProperty.call(this, property, value, priority);
  };

  // Patch cssText setter
  const cssTextDescriptor = Object.getOwnPropertyDescriptor(CSSStyleDeclaration.prototype, 'cssText');
  if (cssTextDescriptor && cssTextDescriptor.set) {
    const originalCssTextSetter = cssTextDescriptor.set;
    Object.defineProperty(CSSStyleDeclaration.prototype, 'cssText', {
      set: function(value) {
        if (typeof value === 'string') {
          let filtered = value;
          PROBLEMATIC_PROPS.forEach(prop => {
            const regex = new RegExp(`${prop}\\s*:\\s*[^;]*(?:;|$)`, 'gi');
            filtered = filtered.replace(regex, (match) => {
              const val = match.split(':')[1]?.replace(';', '').trim();
              return isInvalidValue(val) ? '' : match;
            });
          });
          originalCssTextSetter.call(this, filtered);
         }
         originalCssTextSetter.call(this, value);
      },
      get: cssTextDescriptor.get,
      configurable: true,
    });
  }

  // Patch individual property setters for camelCase properties
  const CAMEL_PROPS = { 'textAlign': 'text-align', 'minHeight': 'min-height' };
  Object.entries(CAMEL_PROPS).forEach(([camel, _kebab]) => {
    const descriptor = Object.getOwnPropertyDescriptor(CSSStyleDeclaration.prototype, camel);
    if (descriptor && descriptor.set) {
      const originalSetter = descriptor.set;
      Object.defineProperty(CSSStyleDeclaration.prototype, camel, {
        set: function(value) {
          if (isInvalidValue(value)) return;
          originalSetter.call(this, value);
        },
        get: descriptor.get,
        configurable: true,
      });
    }
  });

  // Patch insertRule on CSSStyleSheet
  const originalInsertRule = CSSStyleSheet.prototype.insertRule;
  CSSStyleSheet.prototype.insertRule = function(rule, index) {
    let filtered = rule;
    PROBLEMATIC_PROPS.forEach(prop => {
      const regex = new RegExp(`${prop}\\s*:\\s*[^;{}]*`, 'gi');
      filtered = filtered.replace(regex, (match) => {
        const val = match.split(':')[1]?.trim();
        return isInvalidValue(val) ? `${prop}: initial` : match;
      });
    });
    return originalInsertRule.call(this, filtered, index);
  };

  // Patch appendRule
  if (CSSStyleSheet.prototype.appendRule) {
    const originalAppendRule = CSSStyleSheet.prototype.appendRule;
    CSSStyleSheet.prototype.appendRule = function(rule) {
      let filtered = rule;
      PROBLEMATIC_PROPS.forEach(prop => {
        const regex = new RegExp(`${prop}\\s*:\\s*[^;{}]*`, 'gi');
        filtered = filtered.replace(regex, (match) => {
          const val = match.split(':')[1]?.trim();
          return isInvalidValue(val) ? `${prop}: initial` : match;
        });
      });
      return originalAppendRule.call(this, filtered);
    };
  }

  // Patch innerHTML setter on HTMLStyleElement
  const styleInnerHTMLDescriptor = Object.getOwnPropertyDescriptor(Element.prototype, 'innerHTML');
  if (styleInnerHTMLDescriptor && styleInnerHTMLDescriptor.set) {
    const originalInnerHTMLSetter = styleInnerHTMLDescriptor.set;
    Object.defineProperty(HTMLStyleElement.prototype, 'innerHTML', {
      set: function(value) {
        if (typeof value === 'string') {
          let filtered = value;
          PROBLEMATIC_PROPS.forEach(prop => {
            const regex = new RegExp(`${prop}\\s*:\\s*[^;{}]*`, 'gi');
            filtered = filtered.replace(regex, (match) => {
              const val = match.split(':')[1]?.trim();
              return isInvalidValue(val) ? `${prop}: initial` : match;
            });
          });
          originalInnerHTMLSetter.call(this, filtered);
         }
         originalInnerHTMLSetter.call(this, value);
      },
      get: function() {
        return styleInnerHTMLDescriptor.get.call(this);
      },
      configurable: true,
    });
  }

  // Patch textContent setter on HTMLStyleElement
  const textContentDescriptor = Object.getOwnPropertyDescriptor(Node.prototype, 'textContent');
  if (textContentDescriptor && textContentDescriptor.set) {
    const originalTextContentSetter = textContentDescriptor.set;
    Object.defineProperty(HTMLStyleElement.prototype, 'textContent', {
      set: function(value) {
        if (typeof value === 'string' && this instanceof HTMLStyleElement) {
          let filtered = value;
          PROBLEMATIC_PROPS.forEach(prop => {
            const regex = new RegExp(`${prop}\\s*:\\s*[^;{}]*`, 'gi');
            filtered = filtered.replace(regex, (match) => {
              const val = match.split(':')[1]?.trim();
              return isInvalidValue(val) ? `${prop}: initial` : match;
            });
          });
          originalTextContentSetter.call(this, filtered);
         }
         originalTextContentSetter.call(this, value);
      },
      get: function() {
        return textContentDescriptor.get.call(this);
      },
      configurable: true,
    });
  }
})();

const scriptRel = 'modulepreload';const assetsURL = function(dep, importerUrl) { return new URL(dep, importerUrl).href };const seen = {};const __vitePreload = function preload(baseModule, deps, importerUrl) {
	let promise = Promise.resolve();
	if (true               && deps && deps.length > 0) {
		const links = document.getElementsByTagName("link");
		const cspNonceMeta = document.querySelector("meta[property=csp-nonce]");
		const cspNonce = cspNonceMeta?.nonce || cspNonceMeta?.getAttribute("nonce");
		function allSettled(promises$2) {
			return Promise.all(promises$2.map((p) => Promise.resolve(p).then((value$1) => ({
				status: "fulfilled",
				value: value$1
			}), (reason) => ({
				status: "rejected",
				reason
			}))));
		}
		promise = allSettled(deps.map((dep) => {
			dep = assetsURL(dep, importerUrl);
			if (dep in seen) return;
			seen[dep] = true;
			const isCss = dep.endsWith(".css");
			const cssSelector = isCss ? "[rel=\"stylesheet\"]" : "";
			if (!!importerUrl) for (let i$1 = links.length - 1; i$1 >= 0; i$1--) {
				const link$1 = links[i$1];
				if (link$1.href === dep && (!isCss || link$1.rel === "stylesheet")) return;
			}
			else if (document.querySelector(`link[href="${dep}"]${cssSelector}`)) return;
			const link = document.createElement("link");
			link.rel = isCss ? "stylesheet" : scriptRel;
			if (!isCss) link.as = "script";
			link.crossOrigin = "";
			link.href = dep;
			if (cspNonce) link.setAttribute("nonce", cspNonce);
			document.head.appendChild(link);
			if (isCss) return new Promise((res, rej) => {
				link.addEventListener("load", res);
				link.addEventListener("error", () => rej(/* @__PURE__ */ new Error(`Unable to preload CSS for ${dep}`)));
			});
		}));
	}
	function handlePreloadError(err$2) {
		const e$1 = new Event("vite:preloadError", { cancelable: true });
		e$1.payload = err$2;
		window.dispatchEvent(e$1);
		if (!e$1.defaultPrevented) throw err$2;
	}
	return promise.then((res) => {
		for (const item of res || []) {
			if (item.status !== "rejected") continue;
			handlePreloadError(item.reason);
		}
		return baseModule().catch(handlePreloadError);
	});
};

class GlobalSettingsService {
  // Static method to get the global instance
  static getInstance() {
    if (!window.lithiumSettings) {
      window.lithiumSettings = new GlobalSettingsService();
    }
    return window.lithiumSettings;
  }

  // Compatibility methods for ProfileSettingsService API
  getSetting(path, defaultValue = null) {
    return this.get(path, defaultValue);
  }

  setSetting(path, value) {
    return this.set(path, value);
  }

  getSection(sectionKey) {
    const section = this._data[sectionKey];
    return section && typeof section === 'object' ? { ...section } : {};
  }

  constructor() {
    this._data = {};
    this._listeners = [];
    this._accountId = null;
    this._clientId = 'lithium';
    this._serverSyncTimer = null;
    this._isLoaded = false;
    this.storageKey = 'lithium_preferences';
    this._load();
  }

  _save() {
    this._notifyListeners();
    try {
      localStorage.setItem(this.storageKey, JSON.stringify(this._data));
    } catch (error) {
      log(Subsystems.CONFIG, Status.WARN, `Failed to persist settings to localStorage: ${error.message}`);
    }
  }

  _load() {
    try {
      const stored = localStorage.getItem(this.storageKey);
      this._data = stored ? JSON.parse(stored) : {};
    } catch (e) {
      log(Subsystems.CONFIG, Status.WARN, `Failed to load settings from localStorage: ${e.message}`);
      this._data = {};
    }
  }

  // Server sync methods
  async syncFromServer(api) {
    if (!this._accountId || !this._clientId || !api) return;

    try {
      const { authQuery } = await __vitePreload(async () => { const { authQuery } = await import('./conduit-B8Ld7ghl.js').then(n => n.h);return { authQuery }},true              ?[]:void 0,import.meta.url);
      const rows = await authQuery(api, 73, {
        INTEGER: { ACCOUNTID: this._accountId },
        STRING: { CLIENTID: this._clientId }
      });

      if (rows && rows.length > 0) {
        const collection = rows[0].collection || '{}';
        this._data = JSON.parse(collection);
        this._isLoaded = true;
        log(Subsystems.MANAGER, Status.SUCCESS, `User Profile Settings Retrieved - ${collection.length} bytes`);
        this._notifyListeners();
      } else {
        log(Subsystems.MANAGER, Status.INFO, 'User Profile Settings Not Retrieved');
        this._data = {};
        this._isLoaded = true;
        await this.createToServer(api);
      }
    } catch (error) {
      log(Subsystems.MANAGER, Status.ERROR, `Failed to sync settings from server: ${error.message}`);
      this._data = {};
      this._isLoaded = true;
    }
  }

  async createToServer(api) {
    if (!this._accountId || !this._clientId || !api) return;

    try {
      const { authQuery } = await __vitePreload(async () => { const { authQuery } = await import('./conduit-B8Ld7ghl.js').then(n => n.h);return { authQuery }},true              ?[]:void 0,import.meta.url);
      const collection = JSON.stringify(this._data);

      await authQuery(api, 75, {
        INTEGER: { ACCOUNTID: this._accountId },
        STRING: { CLIENTID: this._clientId, COLLECTION: collection }
      });
      log(Subsystems.MANAGER, Status.SUCCESS, 'User Profile Settings Created on Server');
    } catch (error) {
      log(Subsystems.MANAGER, Status.ERROR, `Failed to create server settings: ${error.message}`);
    }
  }

  async syncToServer(api) {
    if (!this._accountId || !this._clientId || !api) return;

    try {
      const { authQuery } = await __vitePreload(async () => { const { authQuery } = await import('./conduit-B8Ld7ghl.js').then(n => n.h);return { authQuery }},true              ?[]:void 0,import.meta.url);
      const collection = JSON.stringify(this._data);

      try {
        await authQuery(api, 74, {
          INTEGER: { ACCOUNTID: this._accountId },
          STRING: { CLIENTID: this._clientId, COLLECTION: collection }
        });
        log(Subsystems.MANAGER, Status.SUCCESS, 'User Profile Settings Updated on Server');
      } catch (_updateError) {
        try {
          await authQuery(api, 75, {
            INTEGER: { ACCOUNTID: this._accountId },
            STRING: { CLIENTID: this._clientId, COLLECTION: collection }
          });
          log(Subsystems.MANAGER, Status.SUCCESS, 'User Profile Settings Created on Server');
        } catch (_createError) {
          log(Subsystems.MANAGER, Status.ERROR, `Failed to create server settings: ${_createError.message}`);
        }
      }
    } catch (error) {
      log(Subsystems.MANAGER, Status.ERROR, `Failed to sync settings to server: ${error.message}`);
    }
  }

  setUserContext(accountId, clientId = 'lithium') {
    this._accountId = accountId;
    this._clientId = clientId;
  }

  /**
   * Clear all settings data (in-memory and localStorage).
   * Used for public/global logout to remove all local settings.
   */
  clear() {
    // Cancel any pending server syncs first
    if (this._serverSyncTimer) {
      clearTimeout(this._serverSyncTimer);
      this._serverSyncTimer = null;
    }

    this._data = {};
    this._isLoaded = false;
    this._accountId = null;
    this._clientId = 'lithium';

    try {
      localStorage.removeItem(this.storageKey);
    } catch (error) {
      log(Subsystems.CONFIG, Status.WARN, `Failed to clear settings from localStorage: ${error.message}`);
    }
    this._notifyListeners();
  }

  _scheduleServerSync() {
    if (this._serverSyncTimer) clearTimeout(this._serverSyncTimer);
    this._serverSyncTimer = setTimeout(() => {
      this._serverSyncTimer = null;
      if (window.lithiumApp && window.lithiumApp.api) {
        this.syncToServer(window.lithiumApp.api);
      }
    }, 2000);
  }

  // Flat access (for global settings like theme)
  get(path, defaultValue = null) {
    const keys = path.split('.');
    let current = this._data;
    for (const key of keys) {
      if (current && typeof current === 'object' && key in current) {
        current = current[key];
      } else {
        return defaultValue;
      }
    }
    return current;
  }

  set(path, value) {
    const keys = path.split('.');
    let current = this._data;
    for (let i = 0; i < keys.length - 1; i++) {
      const key = keys[i];
      if (!current[key] || typeof current[key] !== 'object') {
        current[key] = {};
      }
      current = current[key];
    }
    current[keys[keys.length - 1]] = value;
    this._save();
    this._scheduleServerSync();
    return this;
  }

  getAll() {
    return { ...this._data };
  }

  // Add removeSection method to GlobalSettingsService for Profile Manager
  removeSection(path) {
    const keys = path.split('.');
    if (keys.length === 1) {
      // Top-level key deletion
      delete this._data[keys[0]];
    } else {
      // Nested key deletion
      let current = this._data;
      for (let i = 0; i < keys.length - 1; i++) {
        const key = keys[i];
        if (!current[key] || typeof current[key] !== 'object') {
          return this;
        }
        current = current[key];
      }
      delete current[keys[keys.length - 1]];
    }
    this._save();
    this._scheduleServerSync();
    return this;
  }

  /**
   * Force an immediate server sync, bypassing the debounce.
   * Call this before logout to ensure pending changes are saved.
   */
  flush() {
    if (this._serverSyncTimer) {
      clearTimeout(this._serverSyncTimer);
      this._serverSyncTimer = null;
    }
    if (window.lithiumApp && window.lithiumApp.api) {
      this.syncToServer(window.lithiumApp.api);
    }
  }

  onChange(callback) {
    this._listeners.push(callback);
    return () => {
      const index = this._listeners.indexOf(callback);
      if (index > -1) this._listeners.splice(index, 1);
    };
  }

  _notifyListeners() {
    this._listeners.forEach(callback => {
      try {
        callback(this._data);
      } catch (_e) {
        console.error('Settings listener error:', _e);
      }
    });
  }
}

/**
 * Config - Application configuration loader
 * 
 * Loads and provides access to the lithium.json configuration file.
 * Falls back to sensible defaults if config is missing or partial.
 */

// Default configuration values
const DEFAULT_CONFIG = {
  server: {
    url: 'http://localhost:8080',
    api_prefix: '/api',
    websocket_url: 'ws://localhost:8080/ws',
  },
  auth: {
    api_key: '',
    default_database: 'Demo_PG',
    session_timeout_minutes: 30,
  },
  app: {
    name: 'Lithium',
    tagline: 'A Philement Project',
    version: '1.0.0',
    logo: '/assets/images/logo-li.svg',
    default_theme: 'dark',
    default_language: 'en-US',
  },
  branding: {
    title: 'Lithium',
    description: 'Lithium - A Philement Project. A modern Progressive Web App built with vanilla JavaScript.',
    keywords: 'Lithium, Philement, PWA, Progressive Web App, Vanilla JavaScript, SPA',
    author: 'Philement',
    theme_color: '#1A1A1A',
    icon: 'favicon.ico',
    apple_touch_icon: '/assets/images/logo-192x192.png',
    apple_mobile_web_app_title: 'Lithium',
    application_name: 'Lithium',
    splash_welcome: 'Welcome to Lithium',
  },
  features: {
    offline_mode: true,
    install_prompt: true,
    debug_logging: false,
  },
};

let cachedConfig = null;

/**
 * Deep merge two objects
 * @param {Object} target - Target object
 * @param {Object} source - Source object
 * @returns {Object} Merged object
 */
function deepMerge(target, source) {
  const result = { ...target };
  
  for (const key in source) {
    if (source[key] && typeof source[key] === 'object' && !Array.isArray(source[key])) {
      result[key] = deepMerge(result[key] || {}, source[key]);
    } else {
      result[key] = source[key];
    }
  }
  
  return result;
}

/**
 * Load configuration from server
 * @returns {Promise<Object>} Configuration object
 */
async function loadConfig() {
  // Return cached config if available
  if (cachedConfig) {
    return cachedConfig;
  }

  try {
    const response = await fetch('/config/lithium.json');
    
    if (!response.ok) {
      cachedConfig = DEFAULT_CONFIG;
      return cachedConfig;
    }

    const config = await response.json();
    
    // Merge with defaults
    cachedConfig = deepMerge(DEFAULT_CONFIG, config);
    
    return cachedConfig;
  } catch (_error) {
    cachedConfig = DEFAULT_CONFIG;
    return cachedConfig;
  }
}

/**
 * Get current configuration
 * @returns {Object} Configuration object (may be defaults if not loaded)
 */
function getConfig() {
  if (!cachedConfig) {
    return DEFAULT_CONFIG;
  }
  return cachedConfig;
}

/**
 * Get a specific config value by path
 * @param {string} path - Dot-notation path (e.g., 'server.url')
 * @param {*} defaultValue - Default value if path not found
 * @returns {*} Config value or default
 */
function getConfigValue(path, defaultValue = null) {
  const config = getConfig();
  const parts = path.split('.');
  
  let current = config;
  for (const part of parts) {
    if (current === null || current === undefined || !(part in current)) {
      return defaultValue;
    }
    current = current[part];
  }
  
  return current !== undefined ? current : defaultValue;
}

/**
 * Check if config has been loaded
 * @returns {boolean}
 */
function isConfigLoaded() {
  return cachedConfig !== null;
}

/**
 * Event Bus - Singleton EventTarget for cross-module communication
 * 
 * Provides a lightweight publish/subscribe mechanism for decoupled
 * communication between managers and core modules.
 * 
 * Automatically logs every emitted event using the log system.
 * The subsystem label is derived from the event name's first segment
 * (e.g. "lookups:loaded" → "[Lookups]") and is stored with brackets
 * so that the log viewer can render it in bracket notation without
 * disrupting column alignment.
 */


/**
 * Derive a display subsystem label from an event name.
 * e.g. "lookups:loaded"          → "[Lookups]"
 *      "auth:login"              → "[Auth]"
 *      "startup:complete"        → "[Startup]"
 *      "lookups:themes:loaded"   → "[Lookups]"
 * @param {string} eventName
 * @returns {string} Bracket-wrapped, title-cased first segment
 */
function _subsystemFromEvent(eventName) {
  const segment = eventName.split(':')[0] || eventName;
  const label = segment.charAt(0).toUpperCase() + segment.slice(1).toLowerCase();
  return `[${label}]`;
}

class EventBus extends EventTarget {
  /**
   * Emit an event with optional detail payload.
   * Automatically logs the emission so callers don't need to.
   * @param {string} name - Event name
   * @param {*} detail - Optional data payload
   */
  emit(name, detail = null) {
    const subsystem = _subsystemFromEvent(name);
    // Strip the first segment (already shown as the subsystem label) from the description.
    // e.g. "network:online" → "online", "lookups:loaded" → "loaded", "startup:complete" → "complete"
    const colonIdx = name.indexOf(':');
    const suffix = colonIdx >= 0 ? name.slice(colonIdx + 1) : name;
    const payload = detail !== null ? ' ' + JSON.stringify(detail).substring(0, 100) : '';
    log(subsystem, Status.INFO, `${suffix}${payload}`);
    this.dispatchEvent(new CustomEvent(name, { detail }));
  }

  /**
   * Emit an event without logging.
   * Use when the caller has already produced a richer log entry and the
   * automatic EventBus log line would be redundant or noisy.
   * @param {string} name - Event name
   * @param {*} detail - Optional data payload
   */
  emitSilent(name, detail = null) {
    this.dispatchEvent(new CustomEvent(name, { detail }));
  }

  /**
   * Subscribe to an event
   * @param {string} name - Event name
   * @param {Function} handler - Event handler callback
   */
  on(name, handler) {
    this.addEventListener(name, handler);
  }

  /**
   * Unsubscribe from an event
   * @param {string} name - Event name
   * @param {Function} handler - Event handler callback to remove
   */
  off(name, handler) {
    this.removeEventListener(name, handler);
  }

  /**
   * Subscribe to an event once, then automatically unsubscribe
   * @param {string} name - Event name
   * @param {Function} handler - Event handler callback
   */
  once(name, handler) {
    this.addEventListener(name, handler, { once: true });
  }
}

// Export singleton instance
const eventBus = new EventBus();

// Standard event names for consistency
const Events = {
  // Application lifecycle events
  STARTUP_COMPLETE: 'startup:complete',

  // Authentication events
  AUTH_LOGIN: 'auth:login',
  AUTH_LOGOUT: 'auth:logout',
  AUTH_EXPIRED: 'auth:expired',
  AUTH_RENEWED: 'auth:renewed',

  // Permission events
  PERMISSIONS_UPDATED: 'permissions:updated',

  // Manager events
  MANAGER_LOADED: 'manager:loaded',
  MANAGER_SWITCHED: 'manager:switched',
  MANAGER_CLOSED: 'manager:closed',

  // Theme events
  THEME_CHANGED: 'theme:changed',

  // Locale events
  LOCALE_CHANGED: 'locale:changed',

  // Network events
  NETWORK_ONLINE: 'network:online',
  NETWORK_OFFLINE: 'network:offline',

  // Lookup events
  LOOKUPS_LOADING: 'lookups:loading',
  LOOKUPS_LOADED: 'lookups:loaded',
  LOOKUPS_ERROR: 'lookups:error',
  LOOKUPS_REFRESHED: 'lookups:refreshed',

  // Lookup-specific events (for monitoring specific lookup categories)
  // Pattern: lookups:{category}:loaded - e.g., lookups:themes:loaded
  LOOKUPS_THEMES_LOADED: 'lookups:themes:loaded',
  LOOKUPS_ICONS_LOADED: 'lookups:icons:loaded',
  LOOKUPS_SYSTEM_INFO_LOADED: 'lookups:system_info:loaded',
  LOOKUPS_MANAGERS_LOADED: 'lookups:managers:loaded',
  LOOKUPS_LOOKUP_NAMES_LOADED: 'lookups:lookup_names:loaded',
};

/**
 * JWT Utilities - Client-side JWT handling
 * 
 * Provides functions for decoding, validating, and extracting claims
 * from JWT tokens. Note: This does NOT verify signatures (server-side only).
 */

const STORAGE_KEY$1 = 'lithium_jwt';

/**
 * Base64Url decode a string - works in browser and Node.js/test environments
 * @param {string} base64Url - Base64Url encoded string
 * @returns {string} Decoded string
 */
function base64UrlDecode(base64Url) {
  // Replace Base64Url characters with Base64 characters
  let base64 = base64Url.replace(/-/g, '+').replace(/_/g, '/');
  
  // Add padding if necessary
  const padding = base64.length % 4;
  if (padding) {
    base64 += '='.repeat(4 - padding);
  }
  
  // Use atob if available (browser), otherwise use a fallback
  if (typeof atob !== 'undefined') {
    try {
      // Decode and handle UTF-8 properly
      const decoded = atob(base64);
      // Convert from binary string to UTF-8
      return decodeURIComponent(decoded.split('').map(c => 
        '%' + ('00' + c.charCodeAt(0).toString(16)).slice(-2)
      ).join(''));
    } catch (_e) {
    throw new Error('Invalid Base64Url encoding', { cause: _e });
    }
  }
  
  // Fallback: use Buffer in Node.js or throw error
  if (typeof Buffer !== 'undefined') {
    return Buffer.from(base64, 'base64').toString('utf-8');
  }
  
  throw new Error('No Base64 decoder available');
}

/**
 * Decode a JWT token without verifying signature
 * @param {string} token - JWT token string
 * @returns {Object} Decoded payload
 */
function decodeJWT(token) {
  if (!token || typeof token !== 'string') {
    throw new Error('Token is required');
  }

  const parts = token.split('.');
  if (parts.length !== 3) {
    throw new Error('Invalid JWT format');
  }

  try {
    const payload = base64UrlDecode(parts[1]);
    return JSON.parse(payload);
  } catch (_e) {
    throw new Error('Failed to decode JWT payload: ' + _e.message, { cause: _e });
  }
}

/**
 * Check if a JWT token is expired
 * @param {string|Object} token - JWT token string or decoded payload
 * @param {number} [clockSkew=60] - Clock skew tolerance in seconds
 * @returns {boolean} True if expired
 */
function isExpired(token, clockSkew = 60) {
  try {
    const claims = typeof token === 'string' ? decodeJWT(token) : token;
    
    if (!claims.exp || typeof claims.exp !== 'number') {
      return false; // No expiry = never expires
    }

    const now = Math.floor(Date.now() / 1000);
    return now >= (claims.exp - clockSkew);
  } catch (_err) {
    return true; // Invalid token = treat as expired
  }
}

/**
 * Validate a JWT token (format and expiry only, not signature)
 * @param {string} token - JWT token string
 * @returns {Object} Validation result
 */
function validateJWT(token) {
  if (!token || typeof token !== 'string') {
    return { valid: false, error: 'Token is required' };
  }

  try {
    const claims = decodeJWT(token);
    
    if (isExpired(claims)) {
      return { valid: false, error: 'Token expired', claims };
    }

    return { valid: true, claims };
  } catch (_e) {
    return { valid: false, error: _e.message };
  }
}

/**
 * Get claims from a JWT token
 * @param {string} token - JWT token string
 * @returns {Object|null} Claims object or null if invalid
 */
function getClaims(token) {
  try {
    return decodeJWT(token);
  } catch (_e) {
    return null;
  }
}

/**
 * Store JWT in localStorage
 * @param {string} token - JWT token string
 */
function storeJWT(token) {
  if (!token) {
    throw new Error('Token is required');
  }
  localStorage.setItem(STORAGE_KEY$1, token);
}

/**
 * Retrieve JWT from localStorage
 * @returns {string|null} JWT token or null
 */
function retrieveJWT() {
  return localStorage.getItem(STORAGE_KEY$1);
}

/**
 * Clear JWT from localStorage
 */
function clearJWT() {
  localStorage.removeItem(STORAGE_KEY$1);
}

/**
 * Get time until token expires in milliseconds
 * @param {string|Object} token - JWT token string or decoded payload
 * @returns {number} Milliseconds until expiry (negative if expired)
 */
function getTimeUntilExpiry(token) {
  try {
    const claims = typeof token === 'string' ? decodeJWT(token) : token;
    
    if (!claims.exp || typeof claims.exp !== 'number') {
      return Infinity;
    }

    const now = Math.floor(Date.now() / 1000);
    return (claims.exp - now) * 1000;
  } catch (_e) {
    return -1;
  }
}

const jwt = /*#__PURE__*/Object.freeze(/*#__PURE__*/Object.defineProperty({
	__proto__: null,
	clearJWT,
	decodeJWT,
	getClaims,
	getTimeUntilExpiry,
	isExpired,
	retrieveJWT,
	storeJWT,
	validateJWT
}, Symbol.toStringTag, { value: 'Module' }));

/**
 * JSON Request - Centralized fetch wrapper for API calls
 * 
 * Handles authentication, error handling, logging, and provides
 * a consistent interface for all API requests.
 */


// Default request timeout (30 seconds)
const DEFAULT_TIMEOUT_MS = 30000;

/**
 * Build full URL from path
 * @param {string} path - API path
 * @param {Object} config - App configuration
 * @returns {string} Full URL
 */
function buildUrl(path, config) {
  const baseUrl = config?.server?.url || '';
  const apiPrefix = config?.server?.api_prefix || '/api';
  
  // If path already starts with http, use as-is
  if (path.startsWith('http')) {
    return path;
  }
  
  // Remove leading slash from path if present
  const cleanPath = path.startsWith('/') ? path.slice(1) : path;
  
  // Build full URL
  return `${baseUrl}${apiPrefix}/${cleanPath}`;
}

/**
 * Get default headers for requests
 * @param {boolean} includeAuth - Whether to include auth header
 * @returns {Object} Headers object
 */
function getHeaders(includeAuth = true) {
  const headers = {
    'Content-Type': 'application/json',
    'Accept': 'application/json',
  };

  if (includeAuth) {
    const token = retrieveJWT();
    if (token) {
      headers['Authorization'] = `Bearer ${token}`;
    }
  }

  return headers;
}

/**
 * Handle HTTP errors
 * @param {Response} response - Fetch response
 * @returns {Promise} Resolved with data or rejected with error
 */
async function handleResponse(response, requestNum) {
  // Parse JSON body if present
  let data = null;
  const contentType = response.headers.get('content-type');
  
  if (contentType && contentType.includes('application/json')) {
    try {
      data = await response.json();
    } catch (_e) {
      // JSON parse error, continue with null data
    }
  }

  // Log error response body as RESTAPI grouped entry when available
  if (!response.ok && data && typeof data === 'object' && requestNum) {
    const items = Object.entries(data).map(([key, value]) => `${key}: ${value}`);
    if (items.length > 0) {
      logGroup(Subsystems.RESTAPI, Status.ERROR, `Response ${requestNum}: Body`, items);
    }
  }

  // Handle errors
  if (!response.ok) {
    throw _createError(response, data);
  }

  return data;
}

/**
 * Create an error object from HTTP response
 * @param {Response} response - Fetch response
 * @param {Object|null} data - Parsed response data
 * @returns {Error} Configured error object
 * @private
 */
function _createError(response, data) {
  const error = new Error(data?.message || `HTTP ${response.status}: ${response.statusText}`);
  error.status = response.status;
  error.data = data;
  error.response = response;

  // Handle specific status codes
  _applyStatusError(error, response, data);
  return error;
}

/**
 * Apply status-specific error handling
 * @param {Error} error - Error object to configure
 * @param {Response} response - Fetch response
 * @param {Object|null} data - Parsed response data
 * @private
 */
function _applyStatusError(error, response, data) {
  const statusHandlers = {
    401: () => eventBus.emit(Events.AUTH_EXPIRED, { error }),
    403: () => { error.message = data?.message || 'Access forbidden'; },
    429: () => {
      const retryAfter = response.headers.get('retry-after');
      error.retryAfter = retryAfter ? parseInt(retryAfter, 10) : null;
    },
  };
  
  const serverErrorCodes = [500, 502, 503, 504];
  
  if (statusHandlers[response.status]) {
    statusHandlers[response.status]();
  } else if (serverErrorCodes.includes(response.status)) {
    error.message = data?.message || 'Server error. Please try again later.';
  }
}

/**
 * Make a fetch request with timeout support
 * @param {string} url - Request URL
 * @param {Object} options - Fetch options
 * @param {number} timeoutMs - Timeout in milliseconds
 * @returns {Promise<Response>} Fetch response
 */
async function fetchWithTimeout(url, options, timeoutMs = DEFAULT_TIMEOUT_MS) {
  const controller = new AbortController();
  const timeoutId = setTimeout(() => controller.abort(), timeoutMs);

  try {
    const response = await fetch(url, {
      ...options,
      signal: controller.signal,
    });
    clearTimeout(timeoutId);
    return response;
  } catch (error) {
    clearTimeout(timeoutId);
    if (error.name === 'AbortError') {
      const timeoutError = new Error(`Request timeout after ${timeoutMs}ms`);
      timeoutError.status = 0;
      timeoutError.data = { error: 'timeout', message: timeoutError.message };
      throw timeoutError;
    }
    throw error;
  }
}

/**
 * Make a GET request
 * @param {string} path - API path
 * @param {Object} options - Request options
 * @returns {Promise} Response data
 */
async function get(path, options = {}) {
  const startTime = Date.now();
  const { config, includeAuth = true, ...fetchOptions } = options;
  
  const url = buildUrl(path, config);
  const requestNum = logHttpRequest('GET', path);
  
  const response = await fetchWithTimeout(url, {
    method: 'GET',
    headers: getHeaders(includeAuth),
    ...fetchOptions,
  });

  const duration = Date.now() - startTime;
  const contentLength = response.headers.get('content-length') || 0;
  
  logHttpResponse(requestNum, 'GET', path, {
    code: response.status,
    size: response.ok ? contentLength : null,
    duration,
  });
  
  return handleResponse(response, requestNum);
}

/**
 * Make a POST request
 * @param {string} path - API path
 * @param {Object} body - Request body
 * @param {Object} options - Request options
 * @returns {Promise} Response data
 */
async function post(path, body, options = {}) {
  const startTime = Date.now();
  const { config, includeAuth = true, ...fetchOptions } = options;
  
  const url = buildUrl(path, config);
  const requestNum = logHttpRequest('POST', path);
  
  const response = await fetchWithTimeout(url, {
    method: 'POST',
    headers: getHeaders(includeAuth),
    body: JSON.stringify(body),
    ...fetchOptions,
  });

  const duration = Date.now() - startTime;
  const contentLength = response.headers.get('content-length') || 0;
  
  logHttpResponse(requestNum, 'POST', path, {
    code: response.status,
    size: response.ok ? contentLength : null,
    duration,
  });
  
  return handleResponse(response, requestNum);
}

/**
 * Make a PUT request
 * @param {string} path - API path
 * @param {Object} body - Request body
 * @param {Object} options - Request options
 * @returns {Promise} Response data
 */
async function put(path, body, options = {}) {
  const startTime = Date.now();
  const { config, includeAuth = true, ...fetchOptions } = options;
  
  const url = buildUrl(path, config);
  const requestNum = logHttpRequest('PUT', path);
  
  const response = await fetchWithTimeout(url, {
    method: 'PUT',
    headers: getHeaders(includeAuth),
    body: JSON.stringify(body),
    ...fetchOptions,
  });

  const duration = Date.now() - startTime;
  const contentLength = response.headers.get('content-length') || 0;
  
  logHttpResponse(requestNum, 'PUT', path, {
    code: response.status,
    size: response.ok ? contentLength : null,
    duration,
  });
  
  return handleResponse(response, requestNum);
}

/**
 * Make a DELETE request
 * @param {string} path - API path
 * @param {Object} options - Request options
 * @returns {Promise} Response data
 */
async function del(path, options = {}) {
  const startTime = Date.now();
  const { config, includeAuth = true, ...fetchOptions } = options;
  
  const url = buildUrl(path, config);
  const requestNum = logHttpRequest('DELETE', path);
  
  const response = await fetchWithTimeout(url, {
    method: 'DELETE',
    headers: getHeaders(includeAuth),
    ...fetchOptions,
  });

  const duration = Date.now() - startTime;
  const contentLength = response.headers.get('content-length') || 0;
  
  logHttpResponse(requestNum, 'DELETE', path, {
    code: response.status,
    size: response.ok ? contentLength : null,
    duration,
  });
  
  return handleResponse(response, requestNum);
}

/**
 * Make a PATCH request
 * @param {string} path - API path
 * @param {Object} body - Request body
 * @param {Object} options - Request options
 * @returns {Promise} Response data
 */
async function patch(path, body, options = {}) {
  const startTime = Date.now();
  const { config, includeAuth = true, ...fetchOptions } = options;
  
  const url = buildUrl(path, config);
  const requestNum = logHttpRequest('PATCH', path);
  
  const response = await fetchWithTimeout(url, {
    method: 'PATCH',
    headers: getHeaders(includeAuth),
    body: JSON.stringify(body),
    ...fetchOptions,
  });

  const duration = Date.now() - startTime;
  const contentLength = response.headers.get('content-length') || 0;
  
  logHttpResponse(requestNum, 'PATCH', path, {
    code: response.status,
    size: response.ok ? contentLength : null,
    duration,
  });
  
  return handleResponse(response, requestNum);
}

/**
 * Create a configured request instance with default config
 * @param {Object} defaultConfig - Default configuration
 * @returns {Object} Request methods bound to config
 */
function createRequest(defaultConfig) {
  return {
    get: (path, options = {}) => get(path, { ...options, config: defaultConfig }),
    post: (path, body, options = {}) => post(path, body, { ...options, config: defaultConfig }),
    put: (path, body, options = {}) => put(path, body, { ...options, config: defaultConfig }),
    del: (path, options = {}) => del(path, { ...options, config: defaultConfig }),
    patch: (path, body, options = {}) => patch(path, body, { ...options, config: defaultConfig }),
  };
}

// In-memory cache for lookups
let cache = null;
let fetchPromise = null;
let tabulatorSchemasTimestamp = null;  // Timestamp of last server fetch for version comparison

// Simple logger that works before any manager is loaded
const logger = {
  info: (message) => {
    log(Subsystems.LOOKUPS, Status.INFO, message);
  },
  warn: (message) => {
    log(Subsystems.LOOKUPS, Status.WARN, message);
  },
  error: (message) => {
    log(Subsystems.LOOKUPS, Status.ERROR, message);
  },
};

// Friendly metadata for each lookup category
// Maps category key → { queryRef, label, countLabel }
const LOOKUP_META = {
  system_info:       { queryRef: '001', label: 'System Info',       countLabel: 'elements' },
  lookup_names:      { queryRef: '030', label: 'Lookup Names',      countLabel: 'lookups' },
  themes:            { queryRef: '053', label: 'Themes',            countLabel: 'themes' },
  icons:             { queryRef: '054', label: 'Icons',             countLabel: 'icons' },
  tabulator_schemas: { queryRef: '060', label: 'Tabulator Schemas', countLabel: 'schemas' },
  macros:            { queryRef: '046', label: 'Macro Expansion', countLabel: 'macros' },
};

/**
 * Emit lookups-loaded events via the event bus.
 * For each category a rich logGroup entry is written directly, and the
 * per-category event is dispatched silently (no duplicate EventBus log line).
 * The general LOOKUPS_LOADED event is still emitted normally so any
 * subscribers (e.g. future analytics) can act on it.
 *
 * @param {string} source - 'cache', 'server', or 'cache_fallback'
 * @param {string[]} categoryKeys - Names of loaded categories
 * @param {Object} categoryData - Map of category → data (array or object)
 */
function emitLookupsLoaded(source, categoryKeys, categoryData) {
  // General event: only emitted when data is new or refreshed (server / cache_fallback),
  // NOT when restoring from localStorage cache — that's not a state change for subscribers.
  if (source !== 'cache') {
    eventBus.emit(Events.LOOKUPS_LOADED, { source, lookups: categoryKeys });
  }

  // Per-category: rich logGroup entry + silent event dispatch
  categoryKeys.forEach(category => {
    const eventName = `lookups:${category}:loaded`;
    const data = categoryData[category];
    const count = Array.isArray(data) ? data.length : Object.keys(data || {}).length;
    const meta = LOOKUP_META[category];
    const label = meta ? meta.label : category;
    const queryRef = meta ? meta.queryRef : '???';
    const countLabel = meta ? meta.countLabel : 'items';

    // Compute byte size of the serialised data
    let sizeStr;
    try {
      const bytes = new TextEncoder().encode(JSON.stringify(data)).length;
      sizeStr = bytes.toLocaleString() + ' bytes';
    } catch {
      sizeStr = 'unknown';
    }

    // Rich grouped log entry using [Lookups] bracket notation — consistent with
    // event bus auto-log style so the log viewer renders it in the same column.
    logGroup('[Lookups]', Status.INFO,
      `Loaded lookup ${queryRef}: ${label}`,
      [
        `${countLabel.charAt(0).toUpperCase() + countLabel.slice(1)}: ${count}`,
        `Source: ${source}`,
        `Size: ${sizeStr}`,
      ]
    );

    // Silent event — callers (e.g. login.js) still receive it but no duplicate log line
    eventBus.emitSilent(eventName, { category, count });
  });
}

// Lookup categories and their QueryRefs (reserved for future use)
// const LOOKUP_QUERYREFS = {
//   system_info: 1,      // QueryRef 001 - System Information
//   themes: 53,          // QueryRef 053 - Themes
//   icons: 54,           // QueryRef 054 - Icons
//   lookup_names: 30,    // QueryRef 030 - Names of all lookups
//   managers: 1,         // Derived from system_info
//   tabulator_schemas: 60, // QueryRef 060 - Tabulator Schemas
// };

// Categories that are "open" (available before login) (reserved for future use)
// const OPEN_CATEGORIES = ['system_info', 'themes', 'icons', 'managers', 'tabulator_schemas'];

// localStorage key for caching lookups
const STORAGE_KEY = 'lithium_lookups';
const STORAGE_TIMESTAMP_KEY = 'lithium_lookups_timestamp';
const CACHE_TTL_MS = 24 * 60 * 60 * 1000; // 24 hours

/**
 * Load lookups from localStorage cache
 * @returns {Object|null} Cached lookup data or null
 */
function loadFromLocalStorage() {
  try {
    const stored = localStorage.getItem(STORAGE_KEY);
    const timestamp = localStorage.getItem(STORAGE_TIMESTAMP_KEY);

    if (!stored || !timestamp) {
      return null;
    }

    // Check if cache is still valid
    const age = Date.now() - parseInt(timestamp, 10);
    if (age > CACHE_TTL_MS) {
      logger.info('Cache expired, will fetch fresh data');
      return null;
    }

    const data = JSON.parse(stored);
    return data;
  } catch (error) {
    logger.warn(`Failed to load from localStorage: ${error.message}`);
    return null;
  }
}

/**
 * Save lookups to localStorage cache
 * @param {Object} data - Lookup data to cache
 */
function saveToLocalStorage(data) {
  try {
    localStorage.setItem(STORAGE_KEY, JSON.stringify(data));
    localStorage.setItem(STORAGE_TIMESTAMP_KEY, String(Date.now()));
    logger.info('Saved to localStorage cache');
  } catch (error) {
    logger.warn(`Failed to save to localStorage: ${error.message}`);
  }
}

/**
 * Fetch multiple lookups from the conduit endpoint in one batch request
 * @param {number[]} queryRefs - Array of QueryRef numbers to fetch
 * @param {string} requestId - Identifier for logging this request
 * @returns {Promise<Object>} Combined lookup data
 */
async function fetchBatchQueries(queryRefs, _requestId) {
  const serverUrl = getConfigValue('server.url', 'http://localhost:8080');
  const apiPrefix = getConfigValue('server.api_prefix', '/api');
  const database = getConfigValue('auth.default_database', 'Lithium');
  const startTime = performance.now();

  // Get API key for QueryRef 001
  const apiKey = getConfigValue('auth.api_key', '');

  // Build queries array for batch request
  const queries = queryRefs.map(ref => {
    // QueryRef 001 requires APIKEY parameter (wrapped in STRING datatype)
    if (ref === 1) {
      return {
        query_ref: ref,
        params: {
          STRING: {
            APIKEY: apiKey
          }
        }
      };
    }
    // Other queries don't require parameters
    return {
      query_ref: ref,
      params: {}
    };
  });

  // Log the request using RESTAPI-style logging
  const requestPath = `conduit/queries [${queryRefs.join(', ')}]`;
  const requestNum = logHttpRequest('POST', requestPath);

  const response = await fetch(`${serverUrl}${apiPrefix}/conduit/queries`, {
    method: 'POST',
    headers: {
      'Content-Type': 'application/json',
      'Accept': 'application/json',
    },
    body: JSON.stringify({ database, queries }),
  });

  Math.round(performance.now() - startTime);

  if (!response.ok) {
    logHttpResponse(requestNum, 'POST', requestPath, response.status);
    const error = new Error(`HTTP ${response.status}`);
    error.status = response.status;
    throw error;
  }

  const data = await response.json();
  JSON.stringify(data).length;

  // Log the response with code, size, and duration
  logHttpResponse(requestNum, 'POST', requestPath, response.status);

  return data;
}

/**
 * Process batch query results into lookup categories
 * @param {Object} batchResults - Results from batch query endpoint
 * @returns {Object} Processed lookup data by category
 */
function processBatchResults(batchResults) {
  const lookups = {};

  if (!batchResults || !batchResults.results || !Array.isArray(batchResults.results)) {
    logger.warn('Invalid batch results format');
    return lookups;
  }

  // Process each result in the array
  batchResults.results.forEach(result => {
    if (!result.success) {
      logger.warn(`Query ${result.query_ref} failed: ${result.error || 'Unknown error'}`);
      return;
    }

    const queryRef = result.query_ref;
    const rows = result.rows || [];

    // Map QueryRef to lookup category
    switch (queryRef) {
      case 1:
        // QueryRef 001 - System Information (first row contains license info)
        lookups.system_info = rows[0] || {};
        break;

      case 30:
        // QueryRef 030 - Names of all lookups
        lookups.lookup_names = rows;
        break;

      case 53:
        // QueryRef 053 - Themes
        lookups.themes = rows;
        break;

      case 54:
        // QueryRef 054 - Icons
        lookups.icons = rows;
        break;

      case 46:
        // QueryRef 046 - Macro Expansion
        lookups.macros = rows;
        break;

      case 60:
        // QueryRef 060 - Tabulator Schemas
        lookups.tabulator_schemas = rows;
        break;

      default:
        logger.info(`Unhandled query_ref: ${queryRef} with ${rows.length} rows`);
    }
  });

  return lookups;
}

/**
 * Fetch lookups from the server using batch query
 * @param {Object} options - Fetch options
 * @param {boolean} options.force - Force refresh even if cached
 * @param {boolean} options.silent - Don't emit events (for background refresh)
 * @returns {Promise<Object>} Lookup data
 */
async function fetchLookups(options = {}) {
  const { force = false, silent = false } = options;

  // Return cached data if available and not forcing refresh
  if (cache && !force) {
    return cache;
  }

  // Return existing promise if fetch is in progress
  if (fetchPromise && !force) {
    return fetchPromise;
  }

  // Try loading from localStorage first (if not forcing refresh)
  if (!force && !cache) {
    const localData = loadFromLocalStorage();
    if (localData) {
      cache = localData;
      const qrefs = Object.keys(cache)
        .map(k => LOOKUP_META[k]?.queryRef)
        .filter(Boolean)
        .join(', ');
      logger.info(`Loading lookups [${qrefs}] from localStorage`);
      // Emit event that lookups are loaded from cache
      if (!silent) {
        emitLookupsLoaded('cache', Object.keys(cache), cache);
      }
      // Still fetch fresh data in background
      fetchFreshLookups(silent);
      return cache;
    }
  }

  // Fetch fresh data from server
  fetchPromise = fetchFreshLookups(silent);
  return fetchPromise;
}

/**
 * Fetch fresh lookups from the server
 * @param {boolean} silent - Don't emit events
 * @returns {Promise<Object>} Fresh lookup data
 */
async function fetchFreshLookups(silent = false) {
  try {
    if (!silent) {
      eventBus.emitSilent(Events.LOOKUPS_LOADING, {});
    }

    // Fetch all open lookups in one batch
    // Note: QueryRef 046 (Macros) is loaded separately post-login - see loadMacrosPostLogin()
    const queryRefs = [1, 30, 53, 54, 60]; // QueryRefs 001, 030, 053, 054, 060
    const qrefLabels = queryRefs.map(r => String(r).padStart(3, '0')).join(', ');
    logger.info(`Loading lookups [${qrefLabels}] from server`);
    const batchResults = await fetchBatchQueries(queryRefs, '001');

    // Process results into lookup categories
    const freshLookups = processBatchResults(batchResults);

    // Track timestamp for tabulator_schemas to enable version comparison
    if (freshLookups.tabulator_schemas) {
      tabulatorSchemasTimestamp = Date.now();
      logger.info(`Tabulator schemas fetched at ${new Date(tabulatorSchemasTimestamp).toISOString()}`);
    }

    // Merge with existing cache if present
    cache = { ...cache, ...freshLookups };

    // Save to localStorage
    saveToLocalStorage(cache);

    // Emit server-loaded event
    if (!silent) {
      emitLookupsLoaded('server', Object.keys(freshLookups), freshLookups);
    }

    fetchPromise = null;
    return cache;
  } catch (error) {
    fetchPromise = null;
    logger.warn(`Failed to fetch from server: ${error.message}`);

    // If we have cache, return it even on error
    if (cache) {
      if (!silent) {
        emitLookupsLoaded('cache_fallback', Object.keys(cache), cache);
      }
      return cache;
    }

    // Emit error event
    eventBus.emit(Events.LOOKUPS_ERROR, { error: error.message });

    // Return empty object on error
    return {};
  }
}

/**
 * Check if a lookup category is available (loaded from any source)
 * @param {string} category - Lookup category name
 * @returns {boolean} True if available
 */
function hasLookup(category) {
  return cache !== null && cache[category] !== undefined;
}

/**
 * Get Tabulator schemas lookup data
 * @returns {Object|null} Tabulator schemas data or null
 */
function getTabulatorSchemas() {
  return cache?.tabulator_schemas || null;
}

/**
 * Force-refresh tabulator schemas from server, bypassing cache.
 * Returns the fresh data.
 * @returns {Promise<Object|null>} Fresh tabulator schemas data
 */
async function refreshTabulatorSchemas() {
  if (!cache?.tabulator_schemas) {
    // Not cached yet, fetch fresh
    await fetchLookups({ force: true, silent: false });
    return cache?.tabulator_schemas || null;
  }

  logger.info('Force-refreshing tabulator schemas from server...');

  try {
    // Use authQuery to ensure JWT authentication is included
    const { authQuery } = await __vitePreload(async () => { const { authQuery } = await import('./conduit-B8Ld7ghl.js').then(n => n.h);return { authQuery }},true              ?[]:void 0,import.meta.url);
    const freshSchemas = await authQuery(null, 60);

    if (freshSchemas && Array.isArray(freshSchemas)) {
      cache.tabulator_schemas = freshSchemas;
      tabulatorSchemasTimestamp = Date.now();

      // Update localStorage without overwriting other lookups
      saveToLocalStorage(cache);

      logger.info(`Tabulator schemas refreshed: ${freshSchemas.length} entries`);

      // Emit event so LithiumTable can clear its internal cache
      eventBus.emitSilent(Events.LOOKUPS_REFRESHED, { category: 'tabulator_schemas' });

      return freshSchemas;
    } else {
      logger.warn('Tabulator schemas refresh returned invalid data');
      return cache?.tabulator_schemas || null;
    }
  } catch (error) {
    logger.warn(`Failed to refresh tabulator schemas: ${error.message}`);
    return cache?.tabulator_schemas || null;
  }
}

/**
 * Get all lookups for a category
 * @param {string} category - Lookup category
 * @returns {Object|null} Category data or null
 */
function getLookupCategory(category) {
  if (!cache) {
    logger.warn('Lookups not loaded yet');
    return null;
  }
  return cache[category] || null;
}

/**
 * Load macros lookup post-login (requires authentication)
 * This should be called after successful login when JWT is available
 * Checks cache first, then fetches from server if needed
 * @param {Object} api - The API/conduit instance with auth
 * @param {Object} options - Options
 * @param {boolean} options.force - Force refresh even if cached
 * @returns {Promise<boolean>} True if macros loaded successfully
 */
async function loadMacrosPostLogin(api, options = {}) {
  const { force = false } = options;

  // DEBUG: Always log when this function is called
  logger.info(`[Lookups] loadMacrosPostLogin called, force=${force}`);

  // Load macros if not already loaded
  if (!force && cache?.macros && Object.keys(cache.macros).length > 0) {
    logger.info(`[Lookups] Macros already loaded (${Object.keys(cache.macros).length} entries)`);
  } else {
    await fetchMacrosFromServer(api);
  }

  // Also fetch tabulator_schemas (QueryRef 060) - needed for table column definitions
  // This is separate from the main fetchLookups because it needs to run post-login
  // when the full auth context is available
  const schemas = cache?.tabulator_schemas;
  if (!force && schemas && schemas.length > 0) {
    logger.info(`[Lookups] Tabulator schemas already loaded (${schemas.length} entries)`);
  } else {
    logger.info('[Lookups] Loading tabulator schemas from server (QueryRef 060)');
    try {
      const { authQuery } = await __vitePreload(async () => { const { authQuery } = await import('./conduit-B8Ld7ghl.js').then(n => n.h);return { authQuery }},true              ?[]:void 0,import.meta.url);
      const schemaRows = await authQuery(api, 60);
      if (schemaRows && Array.isArray(schemaRows)) {
        cache.tabulator_schemas = schemaRows;
        logger.info(`[Lookups] Tabulator schemas loaded (${schemaRows.length} entries)`);
      }
    } catch (err) {
      logger.warn(`[Lookups] Failed to load tabulator schemas: ${err.message}`);
    }
  }

  return true;
}

/**
 * Fetch macros from server (internal)
 * Uses QueryRef 26 with LOOKUPID=46 (same as Version Manager)
 * @private
 */
async function fetchMacrosFromServer(api) {
  try {
    logger.info('[Lookups] Loading macros from server (QueryRef 026, LOOKUPID=46)');

    // Import authQuery dynamically to avoid circular dependency
    const { authQuery } = await __vitePreload(async () => { const { authQuery } = await import('./conduit-B8Ld7ghl.js').then(n => n.h);return { authQuery }},true              ?[]:void 0,import.meta.url);

    // QueryRef 026 with LOOKUPID=46 returns macro definitions from lookup table
    const rows = await authQuery(api, 26, { INTEGER: { LOOKUPID: 46 } });

    // Process macros into a simple key-value map
    // Each row has: key_idx, value_txt (macro name like "{ACZ.CLIENT}"), code (macro value), collection (locale variants)
    const macros = {};
    if (Array.isArray(rows)) {
      rows.forEach(row => {
        // value_txt contains the macro name like "{ACZ.CLIENT}"
        // Strip the braces to get the clean key for lookup
        let key = row.value_txt;
        if (!key) return;
        
        // Remove { and } from key if present
        key = key.replace(/^\{/, '').replace(/\}$/, '');

        // Try to get value from code first, then parse collection for locale
        let value = row.code || '';

        // If collection exists, try to get locale-specific value
        if (row.collection) {
          let collection = row.collection;
          if (typeof collection === 'string') {
            try {
              collection = JSON.parse(collection);
            } catch (_e) {
              collection = {};
            }
          }
          if (collection && typeof collection === 'object') {
            const currentLocale = navigator.language || 'en-US';
            // Priority: exact locale > language match > Default
            if (collection[currentLocale]) {
              value = collection[currentLocale];
            } else if (currentLocale.includes('-')) {
              const langOnly = currentLocale.split('-')[0];
              for (const [k, v] of Object.entries(collection)) {
                if (k.toLowerCase().startsWith(langOnly.toLowerCase())) {
                  value = v;
                  break;
                }
              }
            } else if (collection.Default) {
              value = collection.Default;
            }
          }
        }

        macros[key] = value;
        logger.info(`[Lookups] Macro loaded: "${key}" = "${value}"`);
      });
    }

    // Store in cache
    if (!cache) cache = {};
    cache.macros = macros;

    // Save to localStorage
    saveToLocalStorage(cache);

    logger.info(`[Lookups] Loaded ${Object.keys(macros).length} macros from server`);
    logger.info(`[Lookups] Macro keys: ${Object.keys(macros).join(', ')}`);
    eventBus.emitSilent('lookups:macros:loaded', { count: Object.keys(macros).length, source: 'server' });

    return true;
  } catch (error) {
    logger.error(`[Lookups] Failed to load macros: ${error.message}`);
    return false;
  }
}

/**
 * Get all macros as a key-value object
 * @returns {Object|null} Macros object or null if not loaded
 */
function getMacros() {
  return cache?.macros || null;
}

/**
 * Initialize lookups module
 * Sets up event listeners for auto-refresh
 */
function init$1() {
  // Note: We intentionally do NOT refresh lookups when locale changes.
  // The lookup data (themes, icons, system_info, lookup_names) is not
  // locale-dependent, so there's no need for a server round-trip.
  // If locale-specific lookups are needed in the future, add a
  // filtered getter that applies Intl to the cached data instead.

  // Note: We intentionally do NOT clear cache on logout.
  // Lookups contain "open" data (themes, system_info, icons, lookup_names)
  // that don't require authentication and should persist for the login page.
  // The in-memory cache is naturally cleared on page reload.
}

/**
 * Icons - Font Awesome icon system with configurable sets and lookup integration
 *
 * Provides a mutation observer that replaces <fa> tags with proper Font Awesome
 * icon elements based on configuration. Supports:
 * - Direct icons: <fa fa-palette> → <i class="fa-duotone fa-thin fa-palette">
 * - Lookup-based icons: <fa Status> → <i class="fa-duotone fa-thin fa-flag">
 * - Preserved classes: fa-fw, fa-spin, fa-pulse, etc. are retained
 *
 * QueryRef 054 (Icons lookup) provides server-side icon overrides.
 */


// Module state
let observer$1 = null;
let iconConfig = null;
let iconLookups = null;
let isInitialized = false;

/**
 * Decode any HTML entities in a string.
 * @param {string} value
 * @returns {string}
 */
function decodeHtmlEntities(value) {
  if (typeof value !== 'string' || !value) return value;

  const textarea = document.createElement('textarea');
  textarea.innerHTML = value;
  return textarea.value;
}

/**
 * Normalize icon markup so downstream code always receives real HTML.
 * This handles server/cache values that arrive entity-escaped and ensures
 * custom <fa> tags have a closing tag.
 * @param {string} value
 * @param {string} fallback
 * @returns {string}
 */
function normalizeIconHtml(value, fallback = '') {
  if (typeof value !== 'string') return fallback;

  let html = value.trim();
  if (!html) return fallback;

  if (html.includes('&lt;') || html.includes('&gt;') || html.includes('&#')) {
    html = decodeHtmlEntities(html).trim();
  }

  if (/^<fa\s+.+?>$/i.test(html) && !html.includes('</fa>')) {
    const match = html.match(/^<fa\s+(.+?)\s*\/?>$/i);
    if (match) {
      html = `<fa ${match[1]}></fa>`;
    }
  }

  return html || fallback;
}

// Font Awesome class mappings
const FA_PREFIX_MAP = {
  'solid': 'fas',
  'regular': 'far',
  'light': 'fal',
  'thin': 'fat',
  'duotone': 'fad',
  'sharp-solid': 'fass',
  'sharp-regular': 'fasr',
  'sharp-light': 'fasl',
  'sharp-thin': 'fast',
  'brands': 'fab'
};

// Classes that should be preserved from the original fa-* classes
const PRESERVED_CLASSES = ['fa-fw', 'fa-spin', 'fa-pulse', 'fa-border', 'fa-pull-left', 'fa-pull-right'];

// Size classes
const SIZE_CLASSES = ['fa-xs', 'fa-sm', 'fa-lg', 'fa-xl', 'fa-2x', 'fa-2xs', 'fa-3x', 'fa-4x', 'fa-5x', 'fa-6x', 'fa-7x', 'fa-8x', 'fa-9x', 'fa-10x'];

// Animation classes
const ANIMATION_CLASSES = ['fa-spin', 'fa-pulse', 'fa-fade', 'fa-beat', 'fa-beat-fade', 'fa-bounce', 'fa-flip', 'fa-shake'];

// Utility classes (all preserved modifiers)
const UTILITY_CLASSES = [
  ...PRESERVED_CLASSES,
  ...SIZE_CLASSES,
  ...ANIMATION_CLASSES,
  'fa-rotate-90', 'fa-rotate-180', 'fa-rotate-270',
  'fa-flip-horizontal', 'fa-flip-vertical', 'fa-flip-both',
  'fa-stack', 'fa-stack-1x', 'fa-stack-2x', 'fa-inverse'
];

/**
 * Get icon configuration from config file
 * @returns {Object} Icon configuration
 */
function getIconConfig() {
  if (!iconConfig) {
    iconConfig = {
      set: getConfigValue('icons.set', 'solid'),
      weight: getConfigValue('icons.weight', ''),
      prefix: getConfigValue('icons.prefix', ''),
      fallback: getConfigValue('icons.fallback', 'solid')
    };
  }
  return iconConfig;
}

/**
 * Build the Font Awesome prefix based on configuration
 * @returns {string} The Font Awesome class prefix (e.g., 'fad', 'fas', 'fal')
 */
function buildPrefix() {
  const config = getIconConfig();

  // If explicit prefix is set, use it
  if (config.prefix) {
    return config.prefix;
  }

  // Build prefix from set and weight
  const setPrefix = FA_PREFIX_MAP[config.set] || 'fas';

  // For duotone with weight, we use the base duotone prefix
  // Font Awesome 6 Pro handles weight via separate classes
  return setPrefix;
}

/**
 * Build weight class if applicable
 * @returns {string|null} Weight class or null
 */
function buildWeightClass() {
  const config = getIconConfig();

  // Only certain sets support weight classes
  if (config.weight && ['duotone', 'sharp-solid', 'sharp-regular', 'sharp-light', 'sharp-thin'].includes(config.set)) {
    return `fa-${config.weight}`;
  }

  return null;
}

/**
 * Parse icon name from fa-* class list
 * @param {string[]} classes - Array of class names
 * @returns {string|null} Icon name or null
 */
function parseIconName(classes) {
  for (const cls of classes) {
    if (cls.startsWith('fa-') && !UTILITY_CLASSES.includes(cls)) {
      // This is likely the icon name
      return cls.slice(3); // Remove 'fa-' prefix
    }
  }
  return null;
}

/**
 * Extract utility classes from class list
 * @param {string[]} classes - Array of class names
 * @returns {string[]} Array of utility classes
 */
function extractUtilityClasses(classes) {
  return classes.filter(cls => UTILITY_CLASSES.includes(cls));
}

/**
 * Get icon from lookups by name
 * @param {string} name - Icon lookup name (e.g., 'Status')
 * @returns {Object|null} Icon data with icon class, or null
 */
function getLookupIcon(name) {
  if (!iconLookups) {
    iconLookups = getLookupCategory('icons');
  }

  if (!iconLookups) {
    return null;
  }

  // Lookups can be an array or object depending on structure
  const lookupArray = Array.isArray(iconLookups) ? iconLookups : Object.values(iconLookups);

  // Find the lookup entry by value/lookup_key
  const lookup = lookupArray.find(item => {
    const key = item.lookup_value || item.value || item.name || item.lookup_key;
    return key === name;
  });

  if (!lookup) {
    return null;
  }

  // Parse the icon HTML from value_txt or icon field
  const iconHtml = normalizeIconHtml(lookup.value_txt || lookup.icon || lookup.html);

  if (!iconHtml) {
    return null;
  }

  // Extract icon class from HTML like "<i class='fa fa-fw fa-flag'></i>"
  const match = iconHtml.match(/class=['"]([^'"]*)['"]/);
  if (match) {
    const classes = match[1].split(/\s+/);
    const iconName = parseIconName(classes);
    const utilities = extractUtilityClasses(classes);

    return {
      icon: iconName,
      utilities: utilities
    };
  }

  return null;
}

/**
 * Process a single <fa> element and replace it with an <i> element
 * @param {HTMLElement} faElement - The <fa> element to process
 */
function processIconElement(faElement) {
  const attributes = Array.from(faElement.attributes);
  const parseResult = _parseIconAttributes(attributes, faElement.textContent);
  
  const finalIconName = _resolveIconName(parseResult);
  const utilityClasses = _resolveUtilityClasses(parseResult);
  
  const iElement = _createIconElement(finalIconName, utilityClasses, attributes, faElement);
  
  faElement.parentNode.replaceChild(iElement, faElement);
  return iElement;
}

/**
 * Parse icon attributes from element
 * @param {Array} attributes - Element attributes
 * @param {string} textContent - Element text content
 * @returns {Object} Parsed result
 * @private
 */
function _parseIconAttributes(attributes, textContent) {
  const classes = [];
  let lookupName = null;
  let iconName = null;

  for (const attr of attributes) {
    const { name, value } = attr;

    if (name === 'class') {
      classes.push(...value.split(/\s+/).filter(Boolean));
      continue;
    }
    
    if (name === 'icon') {
      iconName = value;
      continue;
    }
    
    if (name.startsWith('fa-')) {
      classes.push(name);
      continue;
    }
    
    if (name.startsWith('data-')) {
      continue;
    }
    
    lookupName = _extractLookupName(name, classes);
  }

  // Check text content for icon name (legacy support)
  if (!lookupName && !iconName) {
    const result = _parseTextContent(textContent, classes);
    lookupName = result.lookupName;
  }

  return { classes, lookupName, iconName };
}

/**
 * Extract lookup name from attribute name
 * @param {string} name - Attribute name
 * @param {string[]} classes - Classes array (modified if fa-prefixed)
 * @returns {string|null} Lookup name or null
 * @private
 */
function _extractLookupName(name, classes) {
  const cleanName = name.toLowerCase();
  if (cleanName.startsWith('fa')) {
    classes.push(cleanName);
    return null;
  }
  return name;
}

/**
 * Parse text content for icon name (legacy support)
 * @param {string} textContent - Element text content
 * @param {string[]} classes - Classes array
 * @returns {Object} Result with lookupName
 * @private
 */
function _parseTextContent(textContent, classes) {
  const text = textContent.trim();
  
  if (text.startsWith('fa-')) {
    classes.push(text);
    return { lookupName: null };
  }
  
  if (/^[a-z-]+$/i.test(text)) {
    return { lookupName: text };
  }
  
  return { lookupName: null };
}

/**
 * Resolve final icon name from parsed result
 * @param {Object} parseResult - Parsed attributes
 * @returns {string|null} Final icon name
 * @private
 */
function _resolveIconName(parseResult) {
  if (parseResult.lookupName) {
    const lookupData = getLookupIcon(parseResult.lookupName);
    if (lookupData?.icon) return lookupData.icon;
  }
  return parseIconName(parseResult.classes);
}

/**
 * Resolve utility classes from parsed result
 * @param {Object} parseResult - Parsed attributes
 * @param {string} finalIconName - Resolved icon name
 * @returns {Array} Utility classes
 * @private
 */
function _resolveUtilityClasses(parseResult, _finalIconName) {
  let utilityClasses = [];
  
  if (parseResult.lookupName) {
    const lookupData = getLookupIcon(parseResult.lookupName);
    if (lookupData?.utilities) utilityClasses = lookupData.utilities;
  }
  
  const elementUtilities = extractUtilityClasses(parseResult.classes);
  return [...new Set([...utilityClasses, ...elementUtilities])];
}

/**
 * Create the icon element
 * @param {string} finalIconName - Icon name
 * @param {Array} utilityClasses - Utility classes
 * @param {Array} attributes - Original attributes
 * @param {HTMLElement} faElement - Original fa element
 * @returns {HTMLElement} Created icon element
 * @private
 */
function _createIconElement(finalIconName, utilityClasses, attributes, faElement) {
  const iElement = document.createElement('i');
  
  // Build class list
  const prefix = buildPrefix();
  const weightClass = buildWeightClass();
  const classList = [prefix];
  
  if (weightClass) classList.push(weightClass);
  if (finalIconName) classList.push(`fa-${finalIconName}`);
  classList.push(...utilityClasses);
  
  iElement.className = classList.join(' ');
  
  // Copy attributes
  _copyIconAttributes(iElement, attributes, faElement);
  
  return iElement;
}

/**
 * Copy attributes from fa element to icon element
 * @param {HTMLElement} iElement - Icon element
 * @param {Array} attributes - Original attributes
 * @param {HTMLElement} faElement - Original fa element
 * @private
 */
function _copyIconAttributes(iElement, attributes, faElement) {
  // Copy data and aria attributes
  for (const attr of attributes) {
    if (attr.name.startsWith('data-') || attr.name.startsWith('aria-')) {
      iElement.setAttribute(attr.name, attr.value);
    }
  }
  
  // Copy inline styles
  if (faElement.style.cssText) {
    iElement.style.cssText = faElement.style.cssText;
  }
  
  // Copy id and title
  if (faElement.id) iElement.id = faElement.id;
  if (faElement.title) iElement.title = faElement.title;
}

/**
 * Process all <fa> elements within a container
 * @param {HTMLElement} container - Container to search for <fa> elements
 */
function processIcons(container = document) {
  const faElements = container.querySelectorAll('fa');

  faElements.forEach((element) => {
    try {
      processIconElement(element);
    } catch (_error) {
      // Silently ignore icon processing errors
    }
  });

  return faElements.length;
}

/**
 * Set up mutation observer to watch for new <fa> elements
 */
function setupMutationObserver() {
  if (observer$1) {
    return; // Already set up
  }

  observer$1 = new MutationObserver((mutations) => {
    let shouldProcess = false;

    for (const mutation of mutations) {
      // Check if any added nodes contain <fa> elements
      for (const node of mutation.addedNodes) {
        if (node.nodeType === Node.ELEMENT_NODE) {
          if (node.tagName.toLowerCase() === 'fa' || node.querySelector?.('fa')) {
            shouldProcess = true;
            break;
          }
        }
      }

      if (shouldProcess) break;
    }

    if (shouldProcess) {
      // Process any new <fa> elements
      processIcons();
    }
  });

  // Start observing the document body
  observer$1.observe(document.body, {
    childList: true,
    subtree: true
  });
}

/**
 * Refresh icon lookups from the server
 */
function refreshIconLookups() {
  iconLookups = getLookupCategory('icons');
}

/**
 * Initialize the icon system
 * @param {Object} options - Initialization options
 * @param {boolean} options.observe - Whether to set up mutation observer
 */
function init(options = {}) {
  if (isInitialized) {
    return;
  }

  const { observe = true } = options;

  // Process any existing <fa> elements
  if (document.body) {
    processIcons();
  }

  // Set up mutation observer if requested
  if (observe) {
    setupMutationObserver();
  }

  // Listen for lookups loaded events to refresh icon data
  eventBus.on(Events.LOOKUPS_LOADED, () => {
    refreshIconLookups();
    // Re-process icons in case lookups changed any icon mappings
    processIcons();
  });

  // Also listen for specific icon lookup events
  eventBus.on('lookups:icons:loaded', () => {
    refreshIconLookups();
    processIcons();
  });

  isInitialized = true;
}

/**
 * Load icon names from a text file (one icon name per line).
 * @param {string} path - Path to the icons file
 * @returns {Promise<string[]>} Array of icon names
 */
async function loadIconFile(path) {
  try {
    const response = await fetch(path);
    if (!response.ok) return [];
    const text = await response.text();
    return text
      .split('\n')
      .map(line => line.trim())
      .filter(line => line && !line.startsWith('#'));
  } catch (_e) {
    return [];
  }
}

/**
 * Preload icons by inserting hidden icon elements into the DOM.
 * This forces Font Awesome to download and cache the icon SVGs
 * before they are needed visually.
 * Supports: <fa fa-square></fa>, <i class="fa-solid fa-square"></i>, <img src="...">
 * @param {string[]} markupArr - Array of icon markup strings
 * @param {number} delayMs - Delay before removing the elements (default 10000ms)
 */
function preloadIcons(markupArr, delayMs = 10000) {
  if (!markupArr || markupArr.length === 0) return;
  if (!document.body) return;

  const container = document.createElement('div');
  container.id = 'li-icon-preload';
  container.style.display = 'none';
  container.setAttribute('aria-hidden', 'true');

  for (const markup of markupArr) {
    container.insertAdjacentHTML('beforeend', markup);
  }

  document.body.appendChild(container);

  setTimeout(() => {
    if (container.parentNode) {
      container.parentNode.removeChild(container);
    }
  }, delayMs);
}

/**
 * Preload icons from config files (icons-dev.txt and icons-usr.txt).
 * Called during app initialization to cache icons early.
 * File format: one icon element per line, e.g.:
 *   <fa fa-user></fa>
 *   <fa-brands fa-css3></fa>
 *   <i class="fa-solid fa-square-full"></i>
 *   <img src="assets/icons/custom.png">
 */
async function preloadIconsFromConfig() {
  log(Subsystems.STARTUP, Status.INFO, 'Submitting icon cache requests');

  const devIcons = await loadIconFile('/config/icons-dev.txt');
  const usrIcons = await loadIconFile('/config/icons-usr.txt');

  const devCount = devIcons.length;
  const usrCount = usrIcons.length;
  const allIcons = [...new Set([...devIcons, ...usrIcons])].filter(Boolean);

  if (allIcons.length > 0) {
    preloadIcons(allIcons);
    log(Subsystems.STARTUP, Status.INFO, `Cached ${allIcons.length} icons (${devCount} from icons-dev.txt, ${usrCount} from icons-usr.txt)`);
  } else {
    log(Subsystems.STARTUP, Status.INFO, 'No icons to cache');
  }
}

/**
 * Custom positioning reference element.
 * @see https://floating-ui.com/docs/virtual-elements
 */

const min = Math.min;
const max = Math.max;
const round = Math.round;
const floor = Math.floor;
const createCoords = v => ({
  x: v,
  y: v
});
const oppositeSideMap = {
  left: 'right',
  right: 'left',
  bottom: 'top',
  top: 'bottom'
};
function clamp(start, value, end) {
  return max(start, min(value, end));
}
function evaluate(value, param) {
  return typeof value === 'function' ? value(param) : value;
}
function getSide(placement) {
  return placement.split('-')[0];
}
function getAlignment(placement) {
  return placement.split('-')[1];
}
function getOppositeAxis(axis) {
  return axis === 'x' ? 'y' : 'x';
}
function getAxisLength(axis) {
  return axis === 'y' ? 'height' : 'width';
}
function getSideAxis(placement) {
  const firstChar = placement[0];
  return firstChar === 't' || firstChar === 'b' ? 'y' : 'x';
}
function getAlignmentAxis(placement) {
  return getOppositeAxis(getSideAxis(placement));
}
function getAlignmentSides(placement, rects, rtl) {
  if (rtl === void 0) {
    rtl = false;
  }
  const alignment = getAlignment(placement);
  const alignmentAxis = getAlignmentAxis(placement);
  const length = getAxisLength(alignmentAxis);
  let mainAlignmentSide = alignmentAxis === 'x' ? alignment === (rtl ? 'end' : 'start') ? 'right' : 'left' : alignment === 'start' ? 'bottom' : 'top';
  if (rects.reference[length] > rects.floating[length]) {
    mainAlignmentSide = getOppositePlacement(mainAlignmentSide);
  }
  return [mainAlignmentSide, getOppositePlacement(mainAlignmentSide)];
}
function getExpandedPlacements(placement) {
  const oppositePlacement = getOppositePlacement(placement);
  return [getOppositeAlignmentPlacement(placement), oppositePlacement, getOppositeAlignmentPlacement(oppositePlacement)];
}
function getOppositeAlignmentPlacement(placement) {
  return placement.includes('start') ? placement.replace('start', 'end') : placement.replace('end', 'start');
}
const lrPlacement = ['left', 'right'];
const rlPlacement = ['right', 'left'];
const tbPlacement = ['top', 'bottom'];
const btPlacement = ['bottom', 'top'];
function getSideList(side, isStart, rtl) {
  switch (side) {
    case 'top':
    case 'bottom':
      if (rtl) return isStart ? rlPlacement : lrPlacement;
      return isStart ? lrPlacement : rlPlacement;
    case 'left':
    case 'right':
      return isStart ? tbPlacement : btPlacement;
    default:
      return [];
  }
}
function getOppositeAxisPlacements(placement, flipAlignment, direction, rtl) {
  const alignment = getAlignment(placement);
  let list = getSideList(getSide(placement), direction === 'start', rtl);
  if (alignment) {
    list = list.map(side => side + "-" + alignment);
    if (flipAlignment) {
      list = list.concat(list.map(getOppositeAlignmentPlacement));
    }
  }
  return list;
}
function getOppositePlacement(placement) {
  const side = getSide(placement);
  return oppositeSideMap[side] + placement.slice(side.length);
}
function expandPaddingObject(padding) {
  return {
    top: 0,
    right: 0,
    bottom: 0,
    left: 0,
    ...padding
  };
}
function getPaddingObject(padding) {
  return typeof padding !== 'number' ? expandPaddingObject(padding) : {
    top: padding,
    right: padding,
    bottom: padding,
    left: padding
  };
}
function rectToClientRect(rect) {
  const {
    x,
    y,
    width,
    height
  } = rect;
  return {
    width,
    height,
    top: y,
    left: x,
    right: x + width,
    bottom: y + height,
    x,
    y
  };
}

function computeCoordsFromPlacement(_ref, placement, rtl) {
  let {
    reference,
    floating
  } = _ref;
  const sideAxis = getSideAxis(placement);
  const alignmentAxis = getAlignmentAxis(placement);
  const alignLength = getAxisLength(alignmentAxis);
  const side = getSide(placement);
  const isVertical = sideAxis === 'y';
  const commonX = reference.x + reference.width / 2 - floating.width / 2;
  const commonY = reference.y + reference.height / 2 - floating.height / 2;
  const commonAlign = reference[alignLength] / 2 - floating[alignLength] / 2;
  let coords;
  switch (side) {
    case 'top':
      coords = {
        x: commonX,
        y: reference.y - floating.height
      };
      break;
    case 'bottom':
      coords = {
        x: commonX,
        y: reference.y + reference.height
      };
      break;
    case 'right':
      coords = {
        x: reference.x + reference.width,
        y: commonY
      };
      break;
    case 'left':
      coords = {
        x: reference.x - floating.width,
        y: commonY
      };
      break;
    default:
      coords = {
        x: reference.x,
        y: reference.y
      };
  }
  switch (getAlignment(placement)) {
    case 'start':
      coords[alignmentAxis] -= commonAlign * (rtl && isVertical ? -1 : 1);
      break;
    case 'end':
      coords[alignmentAxis] += commonAlign * (rtl && isVertical ? -1 : 1);
      break;
  }
  return coords;
}

/**
 * Resolves with an object of overflow side offsets that determine how much the
 * element is overflowing a given clipping boundary on each side.
 * - positive = overflowing the boundary by that number of pixels
 * - negative = how many pixels left before it will overflow
 * - 0 = lies flush with the boundary
 * @see https://floating-ui.com/docs/detectOverflow
 */
async function detectOverflow(state, options) {
  var _await$platform$isEle;
  if (options === void 0) {
    options = {};
  }
  const {
    x,
    y,
    platform,
    rects,
    elements,
    strategy
  } = state;
  const {
    boundary = 'clippingAncestors',
    rootBoundary = 'viewport',
    elementContext = 'floating',
    altBoundary = false,
    padding = 0
  } = evaluate(options, state);
  const paddingObject = getPaddingObject(padding);
  const altContext = elementContext === 'floating' ? 'reference' : 'floating';
  const element = elements[altBoundary ? altContext : elementContext];
  const clippingClientRect = rectToClientRect(await platform.getClippingRect({
    element: ((_await$platform$isEle = await (platform.isElement == null ? void 0 : platform.isElement(element))) != null ? _await$platform$isEle : true) ? element : element.contextElement || (await (platform.getDocumentElement == null ? void 0 : platform.getDocumentElement(elements.floating))),
    boundary,
    rootBoundary,
    strategy
  }));
  const rect = elementContext === 'floating' ? {
    x,
    y,
    width: rects.floating.width,
    height: rects.floating.height
  } : rects.reference;
  const offsetParent = await (platform.getOffsetParent == null ? void 0 : platform.getOffsetParent(elements.floating));
  const offsetScale = (await (platform.isElement == null ? void 0 : platform.isElement(offsetParent))) ? (await (platform.getScale == null ? void 0 : platform.getScale(offsetParent))) || {
    x: 1,
    y: 1
  } : {
    x: 1,
    y: 1
  };
  const elementClientRect = rectToClientRect(platform.convertOffsetParentRelativeRectToViewportRelativeRect ? await platform.convertOffsetParentRelativeRectToViewportRelativeRect({
    elements,
    rect,
    offsetParent,
    strategy
  }) : rect);
  return {
    top: (clippingClientRect.top - elementClientRect.top + paddingObject.top) / offsetScale.y,
    bottom: (elementClientRect.bottom - clippingClientRect.bottom + paddingObject.bottom) / offsetScale.y,
    left: (clippingClientRect.left - elementClientRect.left + paddingObject.left) / offsetScale.x,
    right: (elementClientRect.right - clippingClientRect.right + paddingObject.right) / offsetScale.x
  };
}

// Maximum number of resets that can occur before bailing to avoid infinite reset loops.
const MAX_RESET_COUNT = 50;

/**
 * Computes the `x` and `y` coordinates that will place the floating element
 * next to a given reference element.
 *
 * This export does not have any `platform` interface logic. You will need to
 * write one for the platform you are using Floating UI with.
 */
const computePosition$1 = async (reference, floating, config) => {
  const {
    placement = 'bottom',
    strategy = 'absolute',
    middleware = [],
    platform
  } = config;
  const platformWithDetectOverflow = platform.detectOverflow ? platform : {
    ...platform,
    detectOverflow
  };
  const rtl = await (platform.isRTL == null ? void 0 : platform.isRTL(floating));
  let rects = await platform.getElementRects({
    reference,
    floating,
    strategy
  });
  let {
    x,
    y
  } = computeCoordsFromPlacement(rects, placement, rtl);
  let statefulPlacement = placement;
  let resetCount = 0;
  const middlewareData = {};
  for (let i = 0; i < middleware.length; i++) {
    const currentMiddleware = middleware[i];
    if (!currentMiddleware) {
      continue;
    }
    const {
      name,
      fn
    } = currentMiddleware;
    const {
      x: nextX,
      y: nextY,
      data,
      reset
    } = await fn({
      x,
      y,
      initialPlacement: placement,
      placement: statefulPlacement,
      strategy,
      middlewareData,
      rects,
      platform: platformWithDetectOverflow,
      elements: {
        reference,
        floating
      }
    });
    x = nextX != null ? nextX : x;
    y = nextY != null ? nextY : y;
    middlewareData[name] = {
      ...middlewareData[name],
      ...data
    };
    if (reset && resetCount < MAX_RESET_COUNT) {
      resetCount++;
      if (typeof reset === 'object') {
        if (reset.placement) {
          statefulPlacement = reset.placement;
        }
        if (reset.rects) {
          rects = reset.rects === true ? await platform.getElementRects({
            reference,
            floating,
            strategy
          }) : reset.rects;
        }
        ({
          x,
          y
        } = computeCoordsFromPlacement(rects, statefulPlacement, rtl));
      }
      i = -1;
    }
  }
  return {
    x,
    y,
    placement: statefulPlacement,
    strategy,
    middlewareData
  };
};

/**
 * Provides data to position an inner element of the floating element so that it
 * appears centered to the reference element.
 * @see https://floating-ui.com/docs/arrow
 */
const arrow$1 = options => ({
  name: 'arrow',
  options,
  async fn(state) {
    const {
      x,
      y,
      placement,
      rects,
      platform,
      elements,
      middlewareData
    } = state;
    // Since `element` is required, we don't Partial<> the type.
    const {
      element,
      padding = 0
    } = evaluate(options, state) || {};
    if (element == null) {
      return {};
    }
    const paddingObject = getPaddingObject(padding);
    const coords = {
      x,
      y
    };
    const axis = getAlignmentAxis(placement);
    const length = getAxisLength(axis);
    const arrowDimensions = await platform.getDimensions(element);
    const isYAxis = axis === 'y';
    const minProp = isYAxis ? 'top' : 'left';
    const maxProp = isYAxis ? 'bottom' : 'right';
    const clientProp = isYAxis ? 'clientHeight' : 'clientWidth';
    const endDiff = rects.reference[length] + rects.reference[axis] - coords[axis] - rects.floating[length];
    const startDiff = coords[axis] - rects.reference[axis];
    const arrowOffsetParent = await (platform.getOffsetParent == null ? void 0 : platform.getOffsetParent(element));
    let clientSize = arrowOffsetParent ? arrowOffsetParent[clientProp] : 0;

    // DOM platform can return `window` as the `offsetParent`.
    if (!clientSize || !(await (platform.isElement == null ? void 0 : platform.isElement(arrowOffsetParent)))) {
      clientSize = elements.floating[clientProp] || rects.floating[length];
    }
    const centerToReference = endDiff / 2 - startDiff / 2;

    // If the padding is large enough that it causes the arrow to no longer be
    // centered, modify the padding so that it is centered.
    const largestPossiblePadding = clientSize / 2 - arrowDimensions[length] / 2 - 1;
    const minPadding = min(paddingObject[minProp], largestPossiblePadding);
    const maxPadding = min(paddingObject[maxProp], largestPossiblePadding);

    // Make sure the arrow doesn't overflow the floating element if the center
    // point is outside the floating element's bounds.
    const min$1 = minPadding;
    const max = clientSize - arrowDimensions[length] - maxPadding;
    const center = clientSize / 2 - arrowDimensions[length] / 2 + centerToReference;
    const offset = clamp(min$1, center, max);

    // If the reference is small enough that the arrow's padding causes it to
    // to point to nothing for an aligned placement, adjust the offset of the
    // floating element itself. To ensure `shift()` continues to take action,
    // a single reset is performed when this is true.
    const shouldAddOffset = !middlewareData.arrow && getAlignment(placement) != null && center !== offset && rects.reference[length] / 2 - (center < min$1 ? minPadding : maxPadding) - arrowDimensions[length] / 2 < 0;
    const alignmentOffset = shouldAddOffset ? center < min$1 ? center - min$1 : center - max : 0;
    return {
      [axis]: coords[axis] + alignmentOffset,
      data: {
        [axis]: offset,
        centerOffset: center - offset - alignmentOffset,
        ...(shouldAddOffset && {
          alignmentOffset
        })
      },
      reset: shouldAddOffset
    };
  }
});

/**
 * Optimizes the visibility of the floating element by flipping the `placement`
 * in order to keep it in view when the preferred placement(s) will overflow the
 * clipping boundary. Alternative to `autoPlacement`.
 * @see https://floating-ui.com/docs/flip
 */
const flip$1 = function (options) {
  if (options === void 0) {
    options = {};
  }
  return {
    name: 'flip',
    options,
    async fn(state) {
      var _middlewareData$arrow, _middlewareData$flip;
      const {
        placement,
        middlewareData,
        rects,
        initialPlacement,
        platform,
        elements
      } = state;
      const {
        mainAxis: checkMainAxis = true,
        crossAxis: checkCrossAxis = true,
        fallbackPlacements: specifiedFallbackPlacements,
        fallbackStrategy = 'bestFit',
        fallbackAxisSideDirection = 'none',
        flipAlignment = true,
        ...detectOverflowOptions
      } = evaluate(options, state);

      // If a reset by the arrow was caused due to an alignment offset being
      // added, we should skip any logic now since `flip()` has already done its
      // work.
      // https://github.com/floating-ui/floating-ui/issues/2549#issuecomment-1719601643
      if ((_middlewareData$arrow = middlewareData.arrow) != null && _middlewareData$arrow.alignmentOffset) {
        return {};
      }
      const side = getSide(placement);
      const initialSideAxis = getSideAxis(initialPlacement);
      const isBasePlacement = getSide(initialPlacement) === initialPlacement;
      const rtl = await (platform.isRTL == null ? void 0 : platform.isRTL(elements.floating));
      const fallbackPlacements = specifiedFallbackPlacements || (isBasePlacement || !flipAlignment ? [getOppositePlacement(initialPlacement)] : getExpandedPlacements(initialPlacement));
      const hasFallbackAxisSideDirection = fallbackAxisSideDirection !== 'none';
      if (!specifiedFallbackPlacements && hasFallbackAxisSideDirection) {
        fallbackPlacements.push(...getOppositeAxisPlacements(initialPlacement, flipAlignment, fallbackAxisSideDirection, rtl));
      }
      const placements = [initialPlacement, ...fallbackPlacements];
      const overflow = await platform.detectOverflow(state, detectOverflowOptions);
      const overflows = [];
      let overflowsData = ((_middlewareData$flip = middlewareData.flip) == null ? void 0 : _middlewareData$flip.overflows) || [];
      if (checkMainAxis) {
        overflows.push(overflow[side]);
      }
      if (checkCrossAxis) {
        const sides = getAlignmentSides(placement, rects, rtl);
        overflows.push(overflow[sides[0]], overflow[sides[1]]);
      }
      overflowsData = [...overflowsData, {
        placement,
        overflows
      }];

      // One or more sides is overflowing.
      if (!overflows.every(side => side <= 0)) {
        var _middlewareData$flip2, _overflowsData$filter;
        const nextIndex = (((_middlewareData$flip2 = middlewareData.flip) == null ? void 0 : _middlewareData$flip2.index) || 0) + 1;
        const nextPlacement = placements[nextIndex];
        if (nextPlacement) {
          const ignoreCrossAxisOverflow = checkCrossAxis === 'alignment' ? initialSideAxis !== getSideAxis(nextPlacement) : false;
          if (!ignoreCrossAxisOverflow ||
          // We leave the current main axis only if every placement on that axis
          // overflows the main axis.
          overflowsData.every(d => getSideAxis(d.placement) === initialSideAxis ? d.overflows[0] > 0 : true)) {
            // Try next placement and re-run the lifecycle.
            return {
              data: {
                index: nextIndex,
                overflows: overflowsData
              },
              reset: {
                placement: nextPlacement
              }
            };
          }
        }

        // First, find the candidates that fit on the mainAxis side of overflow,
        // then find the placement that fits the best on the main crossAxis side.
        let resetPlacement = (_overflowsData$filter = overflowsData.filter(d => d.overflows[0] <= 0).sort((a, b) => a.overflows[1] - b.overflows[1])[0]) == null ? void 0 : _overflowsData$filter.placement;

        // Otherwise fallback.
        if (!resetPlacement) {
          switch (fallbackStrategy) {
            case 'bestFit':
              {
                var _overflowsData$filter2;
                const placement = (_overflowsData$filter2 = overflowsData.filter(d => {
                  if (hasFallbackAxisSideDirection) {
                    const currentSideAxis = getSideAxis(d.placement);
                    return currentSideAxis === initialSideAxis ||
                    // Create a bias to the `y` side axis due to horizontal
                    // reading directions favoring greater width.
                    currentSideAxis === 'y';
                  }
                  return true;
                }).map(d => [d.placement, d.overflows.filter(overflow => overflow > 0).reduce((acc, overflow) => acc + overflow, 0)]).sort((a, b) => a[1] - b[1])[0]) == null ? void 0 : _overflowsData$filter2[0];
                if (placement) {
                  resetPlacement = placement;
                }
                break;
              }
            case 'initialPlacement':
              resetPlacement = initialPlacement;
              break;
          }
        }
        if (placement !== resetPlacement) {
          return {
            reset: {
              placement: resetPlacement
            }
          };
        }
      }
      return {};
    }
  };
};

const originSides = /*#__PURE__*/new Set(['left', 'top']);

// For type backwards-compatibility, the `OffsetOptions` type was also
// Derivable.

async function convertValueToCoords(state, options) {
  const {
    placement,
    platform,
    elements
  } = state;
  const rtl = await (platform.isRTL == null ? void 0 : platform.isRTL(elements.floating));
  const side = getSide(placement);
  const alignment = getAlignment(placement);
  const isVertical = getSideAxis(placement) === 'y';
  const mainAxisMulti = originSides.has(side) ? -1 : 1;
  const crossAxisMulti = rtl && isVertical ? -1 : 1;
  const rawValue = evaluate(options, state);

  // eslint-disable-next-line prefer-const
  let {
    mainAxis,
    crossAxis,
    alignmentAxis
  } = typeof rawValue === 'number' ? {
    mainAxis: rawValue,
    crossAxis: 0,
    alignmentAxis: null
  } : {
    mainAxis: rawValue.mainAxis || 0,
    crossAxis: rawValue.crossAxis || 0,
    alignmentAxis: rawValue.alignmentAxis
  };
  if (alignment && typeof alignmentAxis === 'number') {
    crossAxis = alignment === 'end' ? alignmentAxis * -1 : alignmentAxis;
  }
  return isVertical ? {
    x: crossAxis * crossAxisMulti,
    y: mainAxis * mainAxisMulti
  } : {
    x: mainAxis * mainAxisMulti,
    y: crossAxis * crossAxisMulti
  };
}

/**
 * Modifies the placement by translating the floating element along the
 * specified axes.
 * A number (shorthand for `mainAxis` or distance), or an axes configuration
 * object may be passed.
 * @see https://floating-ui.com/docs/offset
 */
const offset$1 = function (options) {
  if (options === void 0) {
    options = 0;
  }
  return {
    name: 'offset',
    options,
    async fn(state) {
      var _middlewareData$offse, _middlewareData$arrow;
      const {
        x,
        y,
        placement,
        middlewareData
      } = state;
      const diffCoords = await convertValueToCoords(state, options);

      // If the placement is the same and the arrow caused an alignment offset
      // then we don't need to change the positioning coordinates.
      if (placement === ((_middlewareData$offse = middlewareData.offset) == null ? void 0 : _middlewareData$offse.placement) && (_middlewareData$arrow = middlewareData.arrow) != null && _middlewareData$arrow.alignmentOffset) {
        return {};
      }
      return {
        x: x + diffCoords.x,
        y: y + diffCoords.y,
        data: {
          ...diffCoords,
          placement
        }
      };
    }
  };
};

/**
 * Optimizes the visibility of the floating element by shifting it in order to
 * keep it in view when it will overflow the clipping boundary.
 * @see https://floating-ui.com/docs/shift
 */
const shift$1 = function (options) {
  if (options === void 0) {
    options = {};
  }
  return {
    name: 'shift',
    options,
    async fn(state) {
      const {
        x,
        y,
        placement,
        platform
      } = state;
      const {
        mainAxis: checkMainAxis = true,
        crossAxis: checkCrossAxis = false,
        limiter = {
          fn: _ref => {
            let {
              x,
              y
            } = _ref;
            return {
              x,
              y
            };
          }
        },
        ...detectOverflowOptions
      } = evaluate(options, state);
      const coords = {
        x,
        y
      };
      const overflow = await platform.detectOverflow(state, detectOverflowOptions);
      const crossAxis = getSideAxis(getSide(placement));
      const mainAxis = getOppositeAxis(crossAxis);
      let mainAxisCoord = coords[mainAxis];
      let crossAxisCoord = coords[crossAxis];
      if (checkMainAxis) {
        const minSide = mainAxis === 'y' ? 'top' : 'left';
        const maxSide = mainAxis === 'y' ? 'bottom' : 'right';
        const min = mainAxisCoord + overflow[minSide];
        const max = mainAxisCoord - overflow[maxSide];
        mainAxisCoord = clamp(min, mainAxisCoord, max);
      }
      if (checkCrossAxis) {
        const minSide = crossAxis === 'y' ? 'top' : 'left';
        const maxSide = crossAxis === 'y' ? 'bottom' : 'right';
        const min = crossAxisCoord + overflow[minSide];
        const max = crossAxisCoord - overflow[maxSide];
        crossAxisCoord = clamp(min, crossAxisCoord, max);
      }
      const limitedCoords = limiter.fn({
        ...state,
        [mainAxis]: mainAxisCoord,
        [crossAxis]: crossAxisCoord
      });
      return {
        ...limitedCoords,
        data: {
          x: limitedCoords.x - x,
          y: limitedCoords.y - y,
          enabled: {
            [mainAxis]: checkMainAxis,
            [crossAxis]: checkCrossAxis
          }
        }
      };
    }
  };
};

function hasWindow() {
  return typeof window !== 'undefined';
}
function getNodeName(node) {
  if (isNode(node)) {
    return (node.nodeName || '').toLowerCase();
  }
  // Mocked nodes in testing environments may not be instances of Node. By
  // returning `#document` an infinite loop won't occur.
  // https://github.com/floating-ui/floating-ui/issues/2317
  return '#document';
}
function getWindow(node) {
  var _node$ownerDocument;
  return (node == null || (_node$ownerDocument = node.ownerDocument) == null ? void 0 : _node$ownerDocument.defaultView) || window;
}
function getDocumentElement(node) {
  var _ref;
  return (_ref = (isNode(node) ? node.ownerDocument : node.document) || window.document) == null ? void 0 : _ref.documentElement;
}
function isNode(value) {
  if (!hasWindow()) {
    return false;
  }
  return value instanceof Node || value instanceof getWindow(value).Node;
}
function isElement(value) {
  if (!hasWindow()) {
    return false;
  }
  return value instanceof Element || value instanceof getWindow(value).Element;
}
function isHTMLElement(value) {
  if (!hasWindow()) {
    return false;
  }
  return value instanceof HTMLElement || value instanceof getWindow(value).HTMLElement;
}
function isShadowRoot(value) {
  if (!hasWindow() || typeof ShadowRoot === 'undefined') {
    return false;
  }
  return value instanceof ShadowRoot || value instanceof getWindow(value).ShadowRoot;
}
function isOverflowElement(element) {
  const {
    overflow,
    overflowX,
    overflowY,
    display
  } = getComputedStyle$1(element);
  return /auto|scroll|overlay|hidden|clip/.test(overflow + overflowY + overflowX) && display !== 'inline' && display !== 'contents';
}
function isTableElement(element) {
  return /^(table|td|th)$/.test(getNodeName(element));
}
function isTopLayer(element) {
  try {
    if (element.matches(':popover-open')) {
      return true;
    }
  } catch (_e) {
    // no-op
  }
  try {
    return element.matches(':modal');
  } catch (_e) {
    return false;
  }
}
const willChangeRe = /transform|translate|scale|rotate|perspective|filter/;
const containRe = /paint|layout|strict|content/;
const isNotNone = value => !!value && value !== 'none';
let isWebKitValue;
function isContainingBlock(elementOrCss) {
  const css = isElement(elementOrCss) ? getComputedStyle$1(elementOrCss) : elementOrCss;

  // https://developer.mozilla.org/en-US/docs/Web/CSS/Containing_block#identifying_the_containing_block
  // https://drafts.csswg.org/css-transforms-2/#individual-transforms
  return isNotNone(css.transform) || isNotNone(css.translate) || isNotNone(css.scale) || isNotNone(css.rotate) || isNotNone(css.perspective) || !isWebKit() && (isNotNone(css.backdropFilter) || isNotNone(css.filter)) || willChangeRe.test(css.willChange || '') || containRe.test(css.contain || '');
}
function getContainingBlock(element) {
  let currentNode = getParentNode(element);
  while (isHTMLElement(currentNode) && !isLastTraversableNode(currentNode)) {
    if (isContainingBlock(currentNode)) {
      return currentNode;
    } else if (isTopLayer(currentNode)) {
      return null;
    }
    currentNode = getParentNode(currentNode);
  }
  return null;
}
function isWebKit() {
  if (isWebKitValue == null) {
    isWebKitValue = typeof CSS !== 'undefined' && CSS.supports && CSS.supports('-webkit-backdrop-filter', 'none');
  }
  return isWebKitValue;
}
function isLastTraversableNode(node) {
  return /^(html|body|#document)$/.test(getNodeName(node));
}
function getComputedStyle$1(element) {
  return getWindow(element).getComputedStyle(element);
}
function getNodeScroll(element) {
  if (isElement(element)) {
    return {
      scrollLeft: element.scrollLeft,
      scrollTop: element.scrollTop
    };
  }
  return {
    scrollLeft: element.scrollX,
    scrollTop: element.scrollY
  };
}
function getParentNode(node) {
  if (getNodeName(node) === 'html') {
    return node;
  }
  const result =
  // Step into the shadow DOM of the parent of a slotted node.
  node.assignedSlot ||
  // DOM Element detected.
  node.parentNode ||
  // ShadowRoot detected.
  isShadowRoot(node) && node.host ||
  // Fallback.
  getDocumentElement(node);
  return isShadowRoot(result) ? result.host : result;
}
function getNearestOverflowAncestor(node) {
  const parentNode = getParentNode(node);
  if (isLastTraversableNode(parentNode)) {
    return node.ownerDocument ? node.ownerDocument.body : node.body;
  }
  if (isHTMLElement(parentNode) && isOverflowElement(parentNode)) {
    return parentNode;
  }
  return getNearestOverflowAncestor(parentNode);
}
function getOverflowAncestors(node, list, traverseIframes) {
  var _node$ownerDocument2;
  if (list === void 0) {
    list = [];
  }
  if (traverseIframes === void 0) {
    traverseIframes = true;
  }
  const scrollableAncestor = getNearestOverflowAncestor(node);
  const isBody = scrollableAncestor === ((_node$ownerDocument2 = node.ownerDocument) == null ? void 0 : _node$ownerDocument2.body);
  const win = getWindow(scrollableAncestor);
  if (isBody) {
    const frameElement = getFrameElement(win);
    return list.concat(win, win.visualViewport || [], isOverflowElement(scrollableAncestor) ? scrollableAncestor : [], frameElement && traverseIframes ? getOverflowAncestors(frameElement) : []);
  } else {
    return list.concat(scrollableAncestor, getOverflowAncestors(scrollableAncestor, [], traverseIframes));
  }
}
function getFrameElement(win) {
  return win.parent && Object.getPrototypeOf(win.parent) ? win.frameElement : null;
}

function getCssDimensions(element) {
  const css = getComputedStyle$1(element);
  // In testing environments, the `width` and `height` properties are empty
  // strings for SVG elements, returning NaN. Fallback to `0` in this case.
  let width = parseFloat(css.width) || 0;
  let height = parseFloat(css.height) || 0;
  const hasOffset = isHTMLElement(element);
  const offsetWidth = hasOffset ? element.offsetWidth : width;
  const offsetHeight = hasOffset ? element.offsetHeight : height;
  const shouldFallback = round(width) !== offsetWidth || round(height) !== offsetHeight;
  if (shouldFallback) {
    width = offsetWidth;
    height = offsetHeight;
  }
  return {
    width,
    height,
    $: shouldFallback
  };
}

function unwrapElement(element) {
  return !isElement(element) ? element.contextElement : element;
}

function getScale(element) {
  const domElement = unwrapElement(element);
  if (!isHTMLElement(domElement)) {
    return createCoords(1);
  }
  const rect = domElement.getBoundingClientRect();
  const {
    width,
    height,
    $
  } = getCssDimensions(domElement);
  let x = ($ ? round(rect.width) : rect.width) / width;
  let y = ($ ? round(rect.height) : rect.height) / height;

  // 0, NaN, or Infinity should always fallback to 1.

  if (!x || !Number.isFinite(x)) {
    x = 1;
  }
  if (!y || !Number.isFinite(y)) {
    y = 1;
  }
  return {
    x,
    y
  };
}

const noOffsets = /*#__PURE__*/createCoords(0);
function getVisualOffsets(element) {
  const win = getWindow(element);
  if (!isWebKit() || !win.visualViewport) {
    return noOffsets;
  }
  return {
    x: win.visualViewport.offsetLeft,
    y: win.visualViewport.offsetTop
  };
}
function shouldAddVisualOffsets(element, isFixed, floatingOffsetParent) {
  if (isFixed === void 0) {
    isFixed = false;
  }
  if (!floatingOffsetParent || isFixed && floatingOffsetParent !== getWindow(element)) {
    return false;
  }
  return isFixed;
}

function getBoundingClientRect(element, includeScale, isFixedStrategy, offsetParent) {
  if (includeScale === void 0) {
    includeScale = false;
  }
  if (isFixedStrategy === void 0) {
    isFixedStrategy = false;
  }
  const clientRect = element.getBoundingClientRect();
  const domElement = unwrapElement(element);
  let scale = createCoords(1);
  if (includeScale) {
    if (offsetParent) {
      if (isElement(offsetParent)) {
        scale = getScale(offsetParent);
      }
    } else {
      scale = getScale(element);
    }
  }
  const visualOffsets = shouldAddVisualOffsets(domElement, isFixedStrategy, offsetParent) ? getVisualOffsets(domElement) : createCoords(0);
  let x = (clientRect.left + visualOffsets.x) / scale.x;
  let y = (clientRect.top + visualOffsets.y) / scale.y;
  let width = clientRect.width / scale.x;
  let height = clientRect.height / scale.y;
  if (domElement) {
    const win = getWindow(domElement);
    const offsetWin = offsetParent && isElement(offsetParent) ? getWindow(offsetParent) : offsetParent;
    let currentWin = win;
    let currentIFrame = getFrameElement(currentWin);
    while (currentIFrame && offsetParent && offsetWin !== currentWin) {
      const iframeScale = getScale(currentIFrame);
      const iframeRect = currentIFrame.getBoundingClientRect();
      const css = getComputedStyle$1(currentIFrame);
      const left = iframeRect.left + (currentIFrame.clientLeft + parseFloat(css.paddingLeft)) * iframeScale.x;
      const top = iframeRect.top + (currentIFrame.clientTop + parseFloat(css.paddingTop)) * iframeScale.y;
      x *= iframeScale.x;
      y *= iframeScale.y;
      width *= iframeScale.x;
      height *= iframeScale.y;
      x += left;
      y += top;
      currentWin = getWindow(currentIFrame);
      currentIFrame = getFrameElement(currentWin);
    }
  }
  return rectToClientRect({
    width,
    height,
    x,
    y
  });
}

// If <html> has a CSS width greater than the viewport, then this will be
// incorrect for RTL.
function getWindowScrollBarX(element, rect) {
  const leftScroll = getNodeScroll(element).scrollLeft;
  if (!rect) {
    return getBoundingClientRect(getDocumentElement(element)).left + leftScroll;
  }
  return rect.left + leftScroll;
}

function getHTMLOffset(documentElement, scroll) {
  const htmlRect = documentElement.getBoundingClientRect();
  const x = htmlRect.left + scroll.scrollLeft - getWindowScrollBarX(documentElement, htmlRect);
  const y = htmlRect.top + scroll.scrollTop;
  return {
    x,
    y
  };
}

function convertOffsetParentRelativeRectToViewportRelativeRect(_ref) {
  let {
    elements,
    rect,
    offsetParent,
    strategy
  } = _ref;
  const isFixed = strategy === 'fixed';
  const documentElement = getDocumentElement(offsetParent);
  const topLayer = elements ? isTopLayer(elements.floating) : false;
  if (offsetParent === documentElement || topLayer && isFixed) {
    return rect;
  }
  let scroll = {
    scrollLeft: 0,
    scrollTop: 0
  };
  let scale = createCoords(1);
  const offsets = createCoords(0);
  const isOffsetParentAnElement = isHTMLElement(offsetParent);
  if (isOffsetParentAnElement || !isOffsetParentAnElement && !isFixed) {
    if (getNodeName(offsetParent) !== 'body' || isOverflowElement(documentElement)) {
      scroll = getNodeScroll(offsetParent);
    }
    if (isOffsetParentAnElement) {
      const offsetRect = getBoundingClientRect(offsetParent);
      scale = getScale(offsetParent);
      offsets.x = offsetRect.x + offsetParent.clientLeft;
      offsets.y = offsetRect.y + offsetParent.clientTop;
    }
  }
  const htmlOffset = documentElement && !isOffsetParentAnElement && !isFixed ? getHTMLOffset(documentElement, scroll) : createCoords(0);
  return {
    width: rect.width * scale.x,
    height: rect.height * scale.y,
    x: rect.x * scale.x - scroll.scrollLeft * scale.x + offsets.x + htmlOffset.x,
    y: rect.y * scale.y - scroll.scrollTop * scale.y + offsets.y + htmlOffset.y
  };
}

function getClientRects(element) {
  return Array.from(element.getClientRects());
}

// Gets the entire size of the scrollable document area, even extending outside
// of the `<html>` and `<body>` rect bounds if horizontally scrollable.
function getDocumentRect(element) {
  const html = getDocumentElement(element);
  const scroll = getNodeScroll(element);
  const body = element.ownerDocument.body;
  const width = max(html.scrollWidth, html.clientWidth, body.scrollWidth, body.clientWidth);
  const height = max(html.scrollHeight, html.clientHeight, body.scrollHeight, body.clientHeight);
  let x = -scroll.scrollLeft + getWindowScrollBarX(element);
  const y = -scroll.scrollTop;
  if (getComputedStyle$1(body).direction === 'rtl') {
    x += max(html.clientWidth, body.clientWidth) - width;
  }
  return {
    width,
    height,
    x,
    y
  };
}

// Safety check: ensure the scrollbar space is reasonable in case this
// calculation is affected by unusual styles.
// Most scrollbars leave 15-18px of space.
const SCROLLBAR_MAX = 25;
function getViewportRect(element, strategy) {
  const win = getWindow(element);
  const html = getDocumentElement(element);
  const visualViewport = win.visualViewport;
  let width = html.clientWidth;
  let height = html.clientHeight;
  let x = 0;
  let y = 0;
  if (visualViewport) {
    width = visualViewport.width;
    height = visualViewport.height;
    const visualViewportBased = isWebKit();
    if (!visualViewportBased || visualViewportBased && strategy === 'fixed') {
      x = visualViewport.offsetLeft;
      y = visualViewport.offsetTop;
    }
  }
  const windowScrollbarX = getWindowScrollBarX(html);
  // <html> `overflow: hidden` + `scrollbar-gutter: stable` reduces the
  // visual width of the <html> but this is not considered in the size
  // of `html.clientWidth`.
  if (windowScrollbarX <= 0) {
    const doc = html.ownerDocument;
    const body = doc.body;
    const bodyStyles = getComputedStyle(body);
    const bodyMarginInline = doc.compatMode === 'CSS1Compat' ? parseFloat(bodyStyles.marginLeft) + parseFloat(bodyStyles.marginRight) || 0 : 0;
    const clippingStableScrollbarWidth = Math.abs(html.clientWidth - body.clientWidth - bodyMarginInline);
    if (clippingStableScrollbarWidth <= SCROLLBAR_MAX) {
      width -= clippingStableScrollbarWidth;
    }
  } else if (windowScrollbarX <= SCROLLBAR_MAX) {
    // If the <body> scrollbar is on the left, the width needs to be extended
    // by the scrollbar amount so there isn't extra space on the right.
    width += windowScrollbarX;
  }
  return {
    width,
    height,
    x,
    y
  };
}

// Returns the inner client rect, subtracting scrollbars if present.
function getInnerBoundingClientRect(element, strategy) {
  const clientRect = getBoundingClientRect(element, true, strategy === 'fixed');
  const top = clientRect.top + element.clientTop;
  const left = clientRect.left + element.clientLeft;
  const scale = isHTMLElement(element) ? getScale(element) : createCoords(1);
  const width = element.clientWidth * scale.x;
  const height = element.clientHeight * scale.y;
  const x = left * scale.x;
  const y = top * scale.y;
  return {
    width,
    height,
    x,
    y
  };
}
function getClientRectFromClippingAncestor(element, clippingAncestor, strategy) {
  let rect;
  if (clippingAncestor === 'viewport') {
    rect = getViewportRect(element, strategy);
  } else if (clippingAncestor === 'document') {
    rect = getDocumentRect(getDocumentElement(element));
  } else if (isElement(clippingAncestor)) {
    rect = getInnerBoundingClientRect(clippingAncestor, strategy);
  } else {
    const visualOffsets = getVisualOffsets(element);
    rect = {
      x: clippingAncestor.x - visualOffsets.x,
      y: clippingAncestor.y - visualOffsets.y,
      width: clippingAncestor.width,
      height: clippingAncestor.height
    };
  }
  return rectToClientRect(rect);
}
function hasFixedPositionAncestor(element, stopNode) {
  const parentNode = getParentNode(element);
  if (parentNode === stopNode || !isElement(parentNode) || isLastTraversableNode(parentNode)) {
    return false;
  }
  return getComputedStyle$1(parentNode).position === 'fixed' || hasFixedPositionAncestor(parentNode, stopNode);
}

// A "clipping ancestor" is an `overflow` element with the characteristic of
// clipping (or hiding) child elements. This returns all clipping ancestors
// of the given element up the tree.
function getClippingElementAncestors(element, cache) {
  const cachedResult = cache.get(element);
  if (cachedResult) {
    return cachedResult;
  }
  let result = getOverflowAncestors(element, [], false).filter(el => isElement(el) && getNodeName(el) !== 'body');
  let currentContainingBlockComputedStyle = null;
  const elementIsFixed = getComputedStyle$1(element).position === 'fixed';
  let currentNode = elementIsFixed ? getParentNode(element) : element;

  // https://developer.mozilla.org/en-US/docs/Web/CSS/Containing_block#identifying_the_containing_block
  while (isElement(currentNode) && !isLastTraversableNode(currentNode)) {
    const computedStyle = getComputedStyle$1(currentNode);
    const currentNodeIsContaining = isContainingBlock(currentNode);
    if (!currentNodeIsContaining && computedStyle.position === 'fixed') {
      currentContainingBlockComputedStyle = null;
    }
    const shouldDropCurrentNode = elementIsFixed ? !currentNodeIsContaining && !currentContainingBlockComputedStyle : !currentNodeIsContaining && computedStyle.position === 'static' && !!currentContainingBlockComputedStyle && (currentContainingBlockComputedStyle.position === 'absolute' || currentContainingBlockComputedStyle.position === 'fixed') || isOverflowElement(currentNode) && !currentNodeIsContaining && hasFixedPositionAncestor(element, currentNode);
    if (shouldDropCurrentNode) {
      // Drop non-containing blocks.
      result = result.filter(ancestor => ancestor !== currentNode);
    } else {
      // Record last containing block for next iteration.
      currentContainingBlockComputedStyle = computedStyle;
    }
    currentNode = getParentNode(currentNode);
  }
  cache.set(element, result);
  return result;
}

// Gets the maximum area that the element is visible in due to any number of
// clipping ancestors.
function getClippingRect(_ref) {
  let {
    element,
    boundary,
    rootBoundary,
    strategy
  } = _ref;
  const elementClippingAncestors = boundary === 'clippingAncestors' ? isTopLayer(element) ? [] : getClippingElementAncestors(element, this._c) : [].concat(boundary);
  const clippingAncestors = [...elementClippingAncestors, rootBoundary];
  const firstRect = getClientRectFromClippingAncestor(element, clippingAncestors[0], strategy);
  let top = firstRect.top;
  let right = firstRect.right;
  let bottom = firstRect.bottom;
  let left = firstRect.left;
  for (let i = 1; i < clippingAncestors.length; i++) {
    const rect = getClientRectFromClippingAncestor(element, clippingAncestors[i], strategy);
    top = max(rect.top, top);
    right = min(rect.right, right);
    bottom = min(rect.bottom, bottom);
    left = max(rect.left, left);
  }
  return {
    width: right - left,
    height: bottom - top,
    x: left,
    y: top
  };
}

function getDimensions(element) {
  const {
    width,
    height
  } = getCssDimensions(element);
  return {
    width,
    height
  };
}

function getRectRelativeToOffsetParent(element, offsetParent, strategy) {
  const isOffsetParentAnElement = isHTMLElement(offsetParent);
  const documentElement = getDocumentElement(offsetParent);
  const isFixed = strategy === 'fixed';
  const rect = getBoundingClientRect(element, true, isFixed, offsetParent);
  let scroll = {
    scrollLeft: 0,
    scrollTop: 0
  };
  const offsets = createCoords(0);

  // If the <body> scrollbar appears on the left (e.g. RTL systems). Use
  // Firefox with layout.scrollbar.side = 3 in about:config to test this.
  function setLeftRTLScrollbarOffset() {
    offsets.x = getWindowScrollBarX(documentElement);
  }
  if (isOffsetParentAnElement || !isOffsetParentAnElement && !isFixed) {
    if (getNodeName(offsetParent) !== 'body' || isOverflowElement(documentElement)) {
      scroll = getNodeScroll(offsetParent);
    }
    if (isOffsetParentAnElement) {
      const offsetRect = getBoundingClientRect(offsetParent, true, isFixed, offsetParent);
      offsets.x = offsetRect.x + offsetParent.clientLeft;
      offsets.y = offsetRect.y + offsetParent.clientTop;
    } else if (documentElement) {
      setLeftRTLScrollbarOffset();
    }
  }
  if (isFixed && !isOffsetParentAnElement && documentElement) {
    setLeftRTLScrollbarOffset();
  }
  const htmlOffset = documentElement && !isOffsetParentAnElement && !isFixed ? getHTMLOffset(documentElement, scroll) : createCoords(0);
  const x = rect.left + scroll.scrollLeft - offsets.x - htmlOffset.x;
  const y = rect.top + scroll.scrollTop - offsets.y - htmlOffset.y;
  return {
    x,
    y,
    width: rect.width,
    height: rect.height
  };
}

function isStaticPositioned(element) {
  return getComputedStyle$1(element).position === 'static';
}

function getTrueOffsetParent(element, polyfill) {
  if (!isHTMLElement(element) || getComputedStyle$1(element).position === 'fixed') {
    return null;
  }
  if (polyfill) {
    return polyfill(element);
  }
  let rawOffsetParent = element.offsetParent;

  // Firefox returns the <html> element as the offsetParent if it's non-static,
  // while Chrome and Safari return the <body> element. The <body> element must
  // be used to perform the correct calculations even if the <html> element is
  // non-static.
  if (getDocumentElement(element) === rawOffsetParent) {
    rawOffsetParent = rawOffsetParent.ownerDocument.body;
  }
  return rawOffsetParent;
}

// Gets the closest ancestor positioned element. Handles some edge cases,
// such as table ancestors and cross browser bugs.
function getOffsetParent(element, polyfill) {
  const win = getWindow(element);
  if (isTopLayer(element)) {
    return win;
  }
  if (!isHTMLElement(element)) {
    let svgOffsetParent = getParentNode(element);
    while (svgOffsetParent && !isLastTraversableNode(svgOffsetParent)) {
      if (isElement(svgOffsetParent) && !isStaticPositioned(svgOffsetParent)) {
        return svgOffsetParent;
      }
      svgOffsetParent = getParentNode(svgOffsetParent);
    }
    return win;
  }
  let offsetParent = getTrueOffsetParent(element, polyfill);
  while (offsetParent && isTableElement(offsetParent) && isStaticPositioned(offsetParent)) {
    offsetParent = getTrueOffsetParent(offsetParent, polyfill);
  }
  if (offsetParent && isLastTraversableNode(offsetParent) && isStaticPositioned(offsetParent) && !isContainingBlock(offsetParent)) {
    return win;
  }
  return offsetParent || getContainingBlock(element) || win;
}

const getElementRects = async function (data) {
  const getOffsetParentFn = this.getOffsetParent || getOffsetParent;
  const getDimensionsFn = this.getDimensions;
  const floatingDimensions = await getDimensionsFn(data.floating);
  return {
    reference: getRectRelativeToOffsetParent(data.reference, await getOffsetParentFn(data.floating), data.strategy),
    floating: {
      x: 0,
      y: 0,
      width: floatingDimensions.width,
      height: floatingDimensions.height
    }
  };
};

function isRTL(element) {
  return getComputedStyle$1(element).direction === 'rtl';
}

const platform = {
  convertOffsetParentRelativeRectToViewportRelativeRect,
  getDocumentElement,
  getClippingRect,
  getOffsetParent,
  getElementRects,
  getClientRects,
  getDimensions,
  getScale,
  isElement,
  isRTL
};

function rectsAreEqual(a, b) {
  return a.x === b.x && a.y === b.y && a.width === b.width && a.height === b.height;
}

// https://samthor.au/2021/observing-dom/
function observeMove(element, onMove) {
  let io = null;
  let timeoutId;
  const root = getDocumentElement(element);
  function cleanup() {
    var _io;
    clearTimeout(timeoutId);
    (_io = io) == null || _io.disconnect();
    io = null;
  }
  function refresh(skip, threshold) {
    if (skip === void 0) {
      skip = false;
    }
    if (threshold === void 0) {
      threshold = 1;
    }
    cleanup();
    const elementRectForRootMargin = element.getBoundingClientRect();
    const {
      left,
      top,
      width,
      height
    } = elementRectForRootMargin;
    if (!skip) {
      onMove();
    }
    if (!width || !height) {
      return;
    }
    const insetTop = floor(top);
    const insetRight = floor(root.clientWidth - (left + width));
    const insetBottom = floor(root.clientHeight - (top + height));
    const insetLeft = floor(left);
    const rootMargin = -insetTop + "px " + -insetRight + "px " + -insetBottom + "px " + -insetLeft + "px";
    const options = {
      rootMargin,
      threshold: max(0, min(1, threshold)) || 1
    };
    let isFirstUpdate = true;
    function handleObserve(entries) {
      const ratio = entries[0].intersectionRatio;
      if (ratio !== threshold) {
        if (!isFirstUpdate) {
          return refresh();
        }
        if (!ratio) {
          // If the reference is clipped, the ratio is 0. Throttle the refresh
          // to prevent an infinite loop of updates.
          timeoutId = setTimeout(() => {
            refresh(false, 1e-7);
          }, 1000);
        } else {
          refresh(false, ratio);
        }
      }
      if (ratio === 1 && !rectsAreEqual(elementRectForRootMargin, element.getBoundingClientRect())) {
        // It's possible that even though the ratio is reported as 1, the
        // element is not actually fully within the IntersectionObserver's root
        // area anymore. This can happen under performance constraints. This may
        // be a bug in the browser's IntersectionObserver implementation. To
        // work around this, we compare the element's bounding rect now with
        // what it was at the time we created the IntersectionObserver. If they
        // are not equal then the element moved, so we refresh.
        refresh();
      }
      isFirstUpdate = false;
    }

    // Older browsers don't support a `document` as the root and will throw an
    // error.
    try {
      io = new IntersectionObserver(handleObserve, {
        ...options,
        // Handle <iframe>s
        root: root.ownerDocument
      });
    } catch (_e) {
      io = new IntersectionObserver(handleObserve, options);
    }
    io.observe(element);
  }
  refresh(true);
  return cleanup;
}

/**
 * Automatically updates the position of the floating element when necessary.
 * Should only be called when the floating element is mounted on the DOM or
 * visible on the screen.
 * @returns cleanup function that should be invoked when the floating element is
 * removed from the DOM or hidden from the screen.
 * @see https://floating-ui.com/docs/autoUpdate
 */
function autoUpdate(reference, floating, update, options) {
  if (options === void 0) {
    options = {};
  }
  const {
    ancestorScroll = true,
    ancestorResize = true,
    elementResize = typeof ResizeObserver === 'function',
    layoutShift = typeof IntersectionObserver === 'function',
    animationFrame = false
  } = options;
  const referenceEl = unwrapElement(reference);
  const ancestors = ancestorScroll || ancestorResize ? [...(referenceEl ? getOverflowAncestors(referenceEl) : []), ...(floating ? getOverflowAncestors(floating) : [])] : [];
  ancestors.forEach(ancestor => {
    ancestorScroll && ancestor.addEventListener('scroll', update, {
      passive: true
    });
    ancestorResize && ancestor.addEventListener('resize', update);
  });
  const cleanupIo = referenceEl && layoutShift ? observeMove(referenceEl, update) : null;
  let reobserveFrame = -1;
  let resizeObserver = null;
  if (elementResize) {
    resizeObserver = new ResizeObserver(_ref => {
      let [firstEntry] = _ref;
      if (firstEntry && firstEntry.target === referenceEl && resizeObserver && floating) {
        // Prevent update loops when using the `size` middleware.
        // https://github.com/floating-ui/floating-ui/issues/1740
        resizeObserver.unobserve(floating);
        cancelAnimationFrame(reobserveFrame);
        reobserveFrame = requestAnimationFrame(() => {
          var _resizeObserver;
          (_resizeObserver = resizeObserver) == null || _resizeObserver.observe(floating);
        });
      }
      update();
    });
    if (referenceEl && !animationFrame) {
      resizeObserver.observe(referenceEl);
    }
    if (floating) {
      resizeObserver.observe(floating);
    }
  }
  let frameId;
  let prevRefRect = animationFrame ? getBoundingClientRect(reference) : null;
  if (animationFrame) {
    frameLoop();
  }
  function frameLoop() {
    const nextRefRect = getBoundingClientRect(reference);
    if (prevRefRect && !rectsAreEqual(prevRefRect, nextRefRect)) {
      update();
    }
    prevRefRect = nextRefRect;
    frameId = requestAnimationFrame(frameLoop);
  }
  update();
  return () => {
    var _resizeObserver2;
    ancestors.forEach(ancestor => {
      ancestorScroll && ancestor.removeEventListener('scroll', update);
      ancestorResize && ancestor.removeEventListener('resize', update);
    });
    cleanupIo == null || cleanupIo();
    (_resizeObserver2 = resizeObserver) == null || _resizeObserver2.disconnect();
    resizeObserver = null;
    if (animationFrame) {
      cancelAnimationFrame(frameId);
    }
  };
}

/**
 * Modifies the placement by translating the floating element along the
 * specified axes.
 * A number (shorthand for `mainAxis` or distance), or an axes configuration
 * object may be passed.
 * @see https://floating-ui.com/docs/offset
 */
const offset = offset$1;

/**
 * Optimizes the visibility of the floating element by shifting it in order to
 * keep it in view when it will overflow the clipping boundary.
 * @see https://floating-ui.com/docs/shift
 */
const shift = shift$1;

/**
 * Optimizes the visibility of the floating element by flipping the `placement`
 * in order to keep it in view when the preferred placement(s) will overflow the
 * clipping boundary. Alternative to `autoPlacement`.
 * @see https://floating-ui.com/docs/flip
 */
const flip = flip$1;

/**
 * Provides data to position an inner element of the floating element so that it
 * appears centered to the reference element.
 * @see https://floating-ui.com/docs/arrow
 */
const arrow = arrow$1;

/**
 * Computes the `x` and `y` coordinates that will place the floating element
 * next to a given reference element.
 */
const computePosition = (reference, floating, options) => {
  // This caches the expensive `getClippingElementAncestors` function so that
  // multiple lifecycle resets re-use the same result. It only lives for a
  // single call. If other functions become expensive, we can add them as well.
  const cache = new Map();
  const mergedOptions = {
    platform,
    ...options
  };
  const platformWithCache = {
    ...mergedOptions.platform,
    _c: cache
  };
  return computePosition$1(reference, floating, {
    ...mergedOptions,
    platform: platformWithCache
  });
};

/* Track keyboard navigation — focus only triggers tooltips after Tab/arrow key */
let isKeyboardNav = false;
document.addEventListener('keydown', (e) => {
  if (e.key === 'Tab' || e.key.startsWith('Arrow')) isKeyboardNav = true;
});
document.addEventListener('mousedown', () => { isKeyboardNav = false; });

/**
 * Get or create the tooltip container element
 * @returns {HTMLElement} The tooltip container
 */
function getTooltipContainer() {
  let container = document.getElementById('tooltip-container');
  if (!container) {
    container = document.createElement('div');
    container.id = 'tooltip-container';
    container.className = 'tooltip-container';
    document.body.appendChild(container);
  }
  return container;
}

const TOOLTIP_DEFAULTS = {
  placement: 'top',
  offsetDistance: 8,
  arrow: true,
  theme: 'default',
  trigger: 'hover',
  delay: { show: 1500, hide: 100 },
  maxDuration: 5000,
  maxWidth: 320,
  interactive: false,
};

class Tooltip {
  constructor(triggerEl, content, options = {}) {
    this.trigger = triggerEl;
    this.content = content;
    this.options = { ...TOOLTIP_DEFAULTS, ...options };
    this.tooltipEl = null;
    this.arrowEl = null;
    this.cleanup = null;
    this.showTimeout = null;
    this.hideTimeout = null;
    this.isVisible = false;
    this._onDocKeydown = this._onDocKeydown.bind(this);
  }

  init() {
    this._createElement();
    this._bindTriggerEvents();
    return this;
  }

  _createElement() {
    this.tooltipEl = document.createElement('div');
    this.tooltipEl.className = `li-tooltip li-tooltip--${this.options.theme}`;
    this.tooltipEl.setAttribute('role', 'tooltip');
    this.tooltipEl.setAttribute('aria-hidden', 'true');
    this.tooltipEl.style.maxWidth = `${this.options.maxWidth}px`;

    const contentEl = document.createElement('div');
    contentEl.className = 'li-tooltip__content';
    contentEl.innerHTML = this.content;
    this.tooltipEl.appendChild(contentEl);

    if (this.options.arrow) {
      this.arrowEl = document.createElement('div');
      this.arrowEl.className = 'li-tooltip__arrow';
      this.tooltipEl.appendChild(this.arrowEl);
    }

    getTooltipContainer().appendChild(this.tooltipEl);

    if (this.options.interactive) {
      this.tooltipEl.addEventListener('mouseenter', () => {
        clearTimeout(this.hideTimeout);
      });
      this.tooltipEl.addEventListener('mouseleave', () => {
        this._scheduleHide();
      });
    }
  }

  _bindTriggerEvents() {
    if (this.options.trigger === 'hover') {
      this.trigger.addEventListener('mouseenter', () => this._scheduleShow());
      this.trigger.addEventListener('mouseleave', () => this._scheduleHide());
      this.trigger.addEventListener('focus', () => {
        if (isKeyboardNav) this._scheduleShow();
      });
      this.trigger.addEventListener('blur', () => this._scheduleHide());
    } else if (this.options.trigger === 'click') {
      this.trigger.addEventListener('click', (e) => {
        e.stopPropagation();
        this.toggle();
      });
      this._clickOutsideHandler = (e) => {
        if (this.tooltipEl && !this.tooltipEl.contains(e.target) && !this.trigger.contains(e.target)) {
          this.hide();
        }
      };
      document.addEventListener('click', this._clickOutsideHandler);
    }
  }

  _scheduleShow() {
    clearTimeout(this.hideTimeout);
    this.showTimeout = setTimeout(() => this.show(), this.options.delay.show);
  }

  _scheduleHide() {
    clearTimeout(this.showTimeout);
    this.hideTimeout = setTimeout(() => this.hide(), this.options.delay.hide);
  }

  show() {
    if (this.isVisible) return;
    // Check conditional — don't show if condition fails
    if (this.options.condition && !this.options.condition()) return;
    this.isVisible = true;
    this.tooltipEl.setAttribute('aria-hidden', 'false');
    this.tooltipEl.classList.add('li-tooltip--visible');
    document.addEventListener('keydown', this._onDocKeydown);
    _trackShow(this);

    this.cleanup = autoUpdate(this.trigger, this.tooltipEl, () => {
      this._updatePosition();
    });
    this._updatePosition();

    if (this.options.maxDuration > 0) {
      this.maxDurationTimeout = setTimeout(() => this.hide(), this.options.maxDuration);
    }
  }

  hide() {
    if (!this.isVisible) return;
    this.isVisible = false;
    this.tooltipEl.setAttribute('aria-hidden', 'true');
    this.tooltipEl.classList.remove('li-tooltip--visible');
    document.removeEventListener('keydown', this._onDocKeydown);
    _trackHide(this);

    clearTimeout(this.maxDurationTimeout);
    this.maxDurationTimeout = null;

    if (this.cleanup) {
      this.cleanup();
      this.cleanup = null;
    }
  }

  toggle() {
    this.isVisible ? this.hide() : this.show();
  }

  async _updatePosition() {
    const middleware = [
      offset(this.options.offsetDistance),
      flip({ fallbackAxisSideAssignment: 'start' }),
      shift({ padding: 8 }),
    ];

    if (this.options.arrow && this.arrowEl) {
      middleware.push(arrow({ element: this.arrowEl, padding: 4 }));
    }

    const { x, y, placement, middlewareData } = await computePosition(
      this.trigger,
      this.tooltipEl,
      {
        placement: this.options.placement,
        strategy: 'fixed',
        middleware,
      }
    );

    Object.assign(this.tooltipEl.style, {
      left: `${x}px`,
      top: `${y}px`,
    });

    this.tooltipEl.setAttribute('data-placement', placement.split('-')[0]);

    if (this.options.arrow && this.arrowEl && middlewareData.arrow) {
      const { x: arrowX, y: arrowY } = middlewareData.arrow;
      const side = placement.split('-')[0];
      const staticSide = { top: 'bottom', right: 'left', bottom: 'top', left: 'right' }[side];

      Object.assign(this.arrowEl.style, {
        left: arrowX != null ? `${arrowX}px` : '',
        top: arrowY != null ? `${arrowY}px` : '',
        right: '',
        bottom: '',
        [staticSide]: '-6px',
      });
    }
  }

  _onDocKeydown(e) {
    if (e.key === 'Escape') {
      this.hide();
    }
  }

  updateContent(newContent) {
    this.content = newContent;
    const contentEl = this.tooltipEl?.querySelector('.li-tooltip__content');
    if (contentEl) contentEl.innerHTML = newContent;
  }

  destroy() {
    this.hide();
    clearTimeout(this.showTimeout);
    clearTimeout(this.hideTimeout);
    clearTimeout(this.maxDurationTimeout);
    if (this._clickOutsideHandler) {
      document.removeEventListener('click', this._clickOutsideHandler);
    }
    this.tooltipEl?.remove();
    this.tooltipEl = null;
    this.arrowEl = null;
  }
}

const tooltipRegistry = new Map();
const activeTooltips = new Set();

let observer = null;
let pendingNodes = [];
let flushScheduled = false;

/** Called by Tooltip instances when they show/hide */
function _trackShow(tooltip) { activeTooltips.add(tooltip); }
function _trackHide(tooltip) { activeTooltips.delete(tooltip); }

/**
 * Shortcut ID → key combo registry (populated by manager-ui via registerShortcut)
 * Avoids circular dependency between tooltip-api and manager-ui.
 */
const shortcutKeyMap = new Map();

function _registerShortcutKey(id, key) {
  shortcutKeyMap.set(id, key);
}

function _unregisterShortcutKey(id) {
  shortcutKeyMap.delete(id);
}

/**
 * Attach a tooltip to an element. Returns the Tooltip instance for manual control.
 * Destroys any existing tooltip on the element.
 */
function tip(triggerEl, content, options = {}) {
  const existing = tooltipRegistry.get(triggerEl);
  if (existing) existing.destroy();

  const t = new Tooltip(triggerEl, content, options).init();
  tooltipRegistry.set(triggerEl, t);
  return t;
}

/**
 * Remove tooltip from element and clean up.
 */
function untip(triggerEl) {
  const existing = tooltipRegistry.get(triggerEl);
  if (existing) {
    existing.destroy();
    tooltipRegistry.delete(triggerEl);
  }
}

/**
 * Get existing tooltip instance for an element.
 */
function getTip(triggerEl) {
  return tooltipRegistry.get(triggerEl) || null;
}

/**
 * Initialize all elements with data-tooltip or title attributes.
 * data-tooltip takes precedence over title. When title is used, the
 * attribute is removed to prevent the native browser tooltip.
 * Reads options from data-tip-* attributes.
 * Starts a MutationObserver on first call to auto-initialize dynamically added elements.
 */
function initTooltips(root = document) {
  if (root === document) _startObserver();

  root.querySelectorAll('[data-tooltip], [title]').forEach((el) => {
    _initElement(el);
  });
}

function _initElement(el) {
  if (tooltipRegistry.has(el)) return;

  // data-tooltip takes precedence over title
  let content = el.getAttribute('data-tooltip');
  if (!content) {
    content = el.getAttribute('title');
    if (!content) return;
    // Remove title to prevent native browser tooltip
    el.removeAttribute('title');
  }

  // If element has data-shortcut-id, append the key combo to the tooltip
  const shortcutId = el.getAttribute('data-shortcut-id');
  if (shortcutId) {
    const key = shortcutKeyMap.get(shortcutId);
    if (key) {
      content = `${content}<br><span class="li-tip-kbd">${key}</span>`;
      el.setAttribute('data-tooltip', content);
      el.dataset.tooltipOriginal = el.getAttribute('data-tooltip')?.replace(/<br><span class='li-tip-kbd'>.*<\/span>$/, '') || content;
    }
  }

  const options = {};
  const placement = el.getAttribute('data-tip-placement');
  const theme = el.getAttribute('data-tip-theme');
  const trigger = el.getAttribute('data-tip-trigger');
  const interactive = el.hasAttribute('data-tip-interactive');
  const maxWidth = el.getAttribute('data-tip-max-width');
  const tipWhen = el.getAttribute('data-tip-when');

  if (placement) options.placement = placement;
  if (theme) options.theme = theme;
  if (trigger) options.trigger = trigger;
  if (interactive) options.interactive = true;
  if (maxWidth) options.maxWidth = parseInt(maxWidth, 10);

  // Conditional tooltip — only show when condition is met
  if (tipWhen) {
    options.condition = _parseCondition(tipWhen);
  }

  tip(el, content, options);
}

function _parseCondition(expr) {
  // Supports: "collapsed" (sidebar collapsed), "not:collapsed" (sidebar not collapsed)
  if (expr === 'collapsed') {
    return () => document.querySelector('.main-sidebar.collapsed') !== null;
  }
  if (expr === 'not:collapsed') {
    return () => document.querySelector('.main-sidebar.collapsed') === null;
  }
  // Default: always show
  return null;
}

function _startObserver() {
  if (observer) return;

  // Hide tooltips on mousedown to prevent stale tooltips after clicks
  document.addEventListener('mousedown', () => {
    for (const t of activeTooltips) t.hide();
  }, { capture: true });

  observer = new MutationObserver((mutations) => {
    for (const mutation of mutations) {
      for (const node of mutation.addedNodes) {
        if (node.nodeType !== Node.ELEMENT_NODE) continue;
        pendingNodes.push(node);
      }
    }
    if (pendingNodes.length > 0 && !flushScheduled) {
      flushScheduled = true;
      requestAnimationFrame(_flushPending);
    }
  });

  observer.observe(document.body, { childList: true, subtree: true });
}

function _flushPending() {
  flushScheduled = false;
  const nodes = pendingNodes;
  pendingNodes = [];

  for (const node of nodes) {
    if (node.matches?.('[data-tooltip], [title]')) {
      _initElement(node);
    }
    node.querySelectorAll?.('[data-tooltip], [title]').forEach(_initElement);
  }
}

function _stopObserver() {
  if (observer) {
    observer.disconnect();
    observer = null;
  }
  pendingNodes = [];
  flushScheduled = false;
}

/**
 * Destroy all tooltips and stop the MutationObserver.
 */
function destroyAllTooltips() {
  _stopObserver();
  document.querySelectorAll('.li-tooltip').forEach((el) => el.remove());
  tooltipRegistry.clear();
}

/**
 * Toast Notification System
 *
 * A toast notification system for displaying
 * error, success, warning, and info messages to users.
 *
 * Features:
 * - Unified header button group with icon, title, subsystem, expand, close
 * - Expand button shows/hides description and keeps the toast
 * - Countdown progress bar showing time remaining before auto-dismiss
 * - Font Awesome icons (customizable per toast)
 * - Collapsible detail section for extended information
 * - Automatic error logging via the logging system
 */


const TOAST_TYPES = {
  ERROR: 'error',
  SUCCESS: 'success',
  WARNING: 'warning',
  INFO: 'info',
};

const TOAST_DEFAULTS = {
  duration: 5000,
  dismissible: true,
  type: TOAST_TYPES.INFO,
  icon: null,            // Custom icon (Font Awesome HTML), null = use default for type
  title: null,           // Title text (if not set, uses message)
  description: null,     // Description text shown when expanded
  subsystem: null,       // Subsystem label (e.g., 'Conduit', 'Auth')
  details: null,         // Optional details text/HTML for collapsible section
  detailsTitle: 'Details', // Label for the details toggle
  showDetails: false,    // Whether details section is expanded by default
  forceDescription: true, // Always show description (defaults to "No additional information provided")
};

const DEFAULT_DESCRIPTION = 'No additional information provided';

class ToastManager {
  constructor() {
    this.container = null;
    this.toasts = new Map();
    this._idCounter = 0;
    this._initContainer();
  }

  /**
   * Initialize the toast container in the DOM
   * @private
   */
  _initContainer() {
    // Check if container already exists
    this.container = document.getElementById('toast-container');

    if (!this.container) {
      this.container = document.createElement('div');
      this.container.id = 'toast-container';
      this.container.className = 'toast-container';
      document.body.appendChild(this.container);
    }
  }

  /**
   * Generate a unique ID for each toast
   * @private
   */
  _generateId() {
    return `toast-${++this._idCounter}`;
  }

  /**
   * Create a toast element
   * @private
   */
  _createToastElement(id, message, options) {
    const toast = document.createElement('div');
    toast.className = `toast toast-${options.type}`;
    toast.dataset.id = id;

    // Icon: use custom if provided, otherwise use default for type
    const icon = options.icon || this._getIcon(options.type);

    // Title: use explicit title or extract from message
    const title = options.title || message;

    // Description: use provided or default if forceDescription is enabled
    let description = options.description;
    if (options.forceDescription && (!description || description.length === 0)) {
      description = DEFAULT_DESCRIPTION;
    }

    // Content flags
    const hasDetails = options.details && options.details.length > 0;
    const hasDescription = description && description.length > 0;
    const hasExpandableContent = hasDescription || hasDetails;

    // ─── Build HTML structure ───
    let html = '';

    // Header row: unified button group style
    html += '<div class="toast-header">';
    html += '<div class="toast-header-group">';

    // Icon button (leftmost, non-clickable)
    html += `<button class="toast-header-icon" aria-label="${options.type}" tabindex="-1">${icon}</button>`;

    // Title button (flex: 1, takes remaining space)
    html += `<button class="toast-header-title" tabindex="-1">${this._escapeHtml(title)}</button>`;

    // Subsystem badge (if provided)
    if (options.subsystem) {
      html += `<button class="toast-header-subsystem" tabindex="-1">${this._escapeHtml(options.subsystem)}</button>`;
    }

    // Expand button (if there's content to expand)
    if (hasExpandableContent && options.duration > 0) {
      html += '<button class="toast-header-expand" aria-label="Expand" title="Expand to keep"><i class="fas fa-chevron-down"></i></button>';
    }

    // Close button (rightmost)
    if (options.dismissible) {
      html += '<button class="toast-header-close" aria-label="Dismiss" title="Dismiss"><i class="fas fa-times"></i></button>';
    }

    html += '</div>'; // .toast-header-group
    html += '</div>'; // .toast-header

    // Content area (description + details combined into single section)
    if (hasExpandableContent) {
      html += '<div class="toast-content">';

      // Build combined content: description followed by details (if present)
      if (hasDescription) {
        html += `<div class="toast-description">${this._escapeHtml(description)}</div>`;
      }

      // Add details directly after description (no separate toggle needed)
      if (hasDetails) {
        html += `<div class="toast-details"><div class="toast-details-content">${options.details}</div></div>`;
      }

      html += '</div>'; // .toast-content
    }

    // Countdown progress bar (only when auto-dismissing)
    if (options.duration > 0) {
      html += `
        <div class="toast-progress">
          <div class="toast-progress-bar" style="animation-duration: ${options.duration}ms"></div>
        </div>
      `;
    }

    toast.innerHTML = html;

    // ─── Event handlers ───

    // Close button
    if (options.dismissible) {
      const closeBtn = toast.querySelector('.toast-header-close');
      closeBtn?.addEventListener('click', () => this.dismiss(id));
    }

    // Expand button - toggles content visibility with smooth height animation AND keeps the toast
    const expandBtn = toast.querySelector('.toast-header-expand');
    const content = toast.querySelector('.toast-content');
    if (expandBtn && content) {
      // Initialize height for animation
      content.style.height = '0px';
      content.style.overflow = 'hidden';

      expandBtn.addEventListener('click', () => {
        const isExpanded = expandBtn.classList.contains('toast-header-expand-active');

        if (isExpanded) {
          // Collapsing - set explicit height first, then animate to 0
          const targetHeight = content.scrollHeight;
          content.style.height = targetHeight + 'px';
          // Force reflow to ensure the browser registers the height change
          content.offsetHeight;
          requestAnimationFrame(() => {
            content.style.height = '0px';
          });
          expandBtn.classList.remove('toast-header-expand-active');
          expandBtn.setAttribute('aria-expanded', 'false');
        } else {
          // Expanding - animate to full height
          expandBtn.classList.add('toast-header-expand-active');
          expandBtn.setAttribute('aria-expanded', 'true');
          content.style.height = content.scrollHeight + 'px';
        }

        // Also keep the toast when expanded
        if (!isExpanded) {
          this.keep(id);
        }
      });

      // Listen for transition end to reset overflow (use { once: true } to auto-remove)
      const handleTransitionEnd = () => {
        if (content.style.height === '0px') {
          content.style.overflow = 'hidden';
        } else {
          content.style.height = 'auto';
          content.style.overflow = 'visible';
        }
      };
      content.addEventListener('transitionend', handleTransitionEnd, { once: true });
    }

    return toast;
  }

  /**
   * Escape HTML special characters for safe insertion
   * @private
   */
  _escapeHtml(text) {
    if (typeof text !== 'string') return text;
    const div = document.createElement('div');
    div.textContent = text;
    return div.innerHTML;
  }

  /**
   * Get icon HTML for toast type
   * @private
   */
  _getIcon(type) {
    const icons = {
      [TOAST_TYPES.ERROR]: '<i class="fas fa-times-circle"></i>',
      [TOAST_TYPES.SUCCESS]: '<i class="fas fa-check-circle"></i>',
      [TOAST_TYPES.WARNING]: '<i class="fas fa-exclamation-triangle"></i>',
      [TOAST_TYPES.INFO]: '<i class="fas fa-info-circle"></i>',
    };
    return icons[type] || icons[TOAST_TYPES.INFO];
  }

  /**
   * Log a server error through the logging system
   * @private
   */
  _logServerError(message, serverError, subsystem) {
    const logSub = subsystem || Subsystems.CONDUIT;
    const items = [];
    // Support both snake_case (server) and camelCase (JS convention)
    const queryRef = serverError.query_ref ?? serverError.queryRef;
    const database = serverError.database;
    const errorMsg = serverError.error;
    
    if (queryRef != null) items.push(`Query Ref: ${queryRef}`);
    if (database) items.push(`Database: ${database}`);
    if (errorMsg) items.push(`Error: ${errorMsg}`);
    if (serverError.message && serverError.message !== message) {
      items.push(`Message: ${serverError.message}`);
    }

    if (items.length > 0) {
      logGroup(logSub, Status.ERROR, message, items);
    } else {
      log(logSub, Status.ERROR, message);
    }
  }

  /**
   * Show a toast notification
   * @param {string} message - The message to display
   * @param {Object} options - Toast options
   * @param {string} [options.type='info'] - Toast type: error, success, warning, info
   * @param {number} [options.duration=5000] - Duration in ms (0 = persistent)
   * @param {boolean} [options.dismissible=true] - Whether the toast can be dismissed
   * @param {string} [options.icon=null] - Custom Font Awesome icon HTML
   * @param {string} [options.title=null] - Optional title (if not set, uses message)
   * @param {string} [options.description=null] - Description text shown when expanded
   * @param {string} [options.subsystem=null] - Subsystem label (e.g., 'Conduit')
   * @param {string} [options.details=null] - Optional details text/HTML for collapsible section
   * @param {string} [options.detailsTitle='Details'] - Label for the details toggle button
   * @param {boolean} [options.showDetails=false] - Whether to show details expanded by default
   * @param {boolean} [options.forceDescription=true] - Always show description section (defaults to "No additional information provided")
   * @returns {string} Toast ID
   */
  show(message, options = {}) {
    const id = this._generateId();
    const config = { ...TOAST_DEFAULTS, ...options };

    // Create and append toast element
    const toastEl = this._createToastElement(id, message, config);
    this.container.appendChild(toastEl);

    // Trigger entrance animation
    requestAnimationFrame(() => {
      toastEl.classList.add('toast-visible');
    });

    // Store toast info (register before starting timer so keep() can find it)
    const toastInfo = {
      element: toastEl,
      config,
      timerId: null,
    };
    this.toasts.set(id, toastInfo);

    // Auto-dismiss if duration is set
    if (config.duration > 0) {
      toastInfo.timerId = setTimeout(() => this.dismiss(id), config.duration);
    }

    return id;
  }

  /**
   * Show an error toast with optional details and automatic logging
   * @param {string} message - The error message
   * @param {Object} options - Toast options
   * @param {string} [options.icon] - Custom icon (defaults to error icon)
   * @param {string} [options.description] - Description text shown when expanded
   * @param {string} [options.details] - Optional details to show in collapsible section
   * @param {Object} [options.serverError] - Server error object with query_ref, database, etc.
   * @param {string} [options.subsystem] - Subsystem label for badge and logging
   * @returns {string} Toast ID
   */
  error(message, options = {}) {
    // Auto-build details from server error object if provided
    let details = options.details;
    let description = options.description;
    let title = options.title ?? message;

    if (options.serverError) {
      const { serverError } = options;

      // Use serverError.error as title (e.g., "Missing Required Parameters")
      if (serverError.error) {
        title = serverError.error;
      }

      // Auto-build description from serverError.message if not provided
      // Use serverError.message as description (e.g., "Required: QUERYID...")
      if (!description && serverError.message) {
        description = serverError.message;
      }

      // Auto-build details (support both snake_case and camelCase property names)
      if (!details) {
        const detailParts = [];
        const queryRef = serverError.query_ref ?? serverError.queryRef;
        const database = serverError.database;
        
        if (queryRef != null) {
          detailParts.push(`Query Ref: ${queryRef}`);
        }
        if (database) {
          detailParts.push(`Database: ${database}`);
        }
        if (detailParts.length > 0) {
          details = detailParts.join('\n');
        }
      }
}


    return this.show(title, {
      ...options,
      type: TOAST_TYPES.ERROR,
      details,
      description,
    });
  }

  /**
   * Show a success toast
   * @param {string} message - The success message
   * @param {Object} options - Toast options
   * @returns {string} Toast ID
   */
  success(message, options = {}) {
    return this.show(message, { ...options, type: TOAST_TYPES.SUCCESS });
  }

  /**
   * Show a warning toast
   * @param {string} message - The warning message
   * @param {Object} options - Toast options
   * @returns {string} Toast ID
   */
  warning(message, options = {}) {
    return this.show(message, { ...options, type: TOAST_TYPES.WARNING });
  }

  /**
   * Show an info toast
   * @param {string} message - The info message
   * @param {Object} options - Toast options
   * @returns {string} Toast ID
   */
  info(message, options = {}) {
    return this.show(message, { ...options, type: TOAST_TYPES.INFO });
  }

  /**
   * Keep (pin) a toast — cancel auto-dismiss and freeze the countdown bar
   * @param {string} id - The toast ID
   */
  keep(id) {
    const toastInfo = this.toasts.get(id);
    if (!toastInfo) return;

    // Cancel auto-dismiss timer
    if (toastInfo.timerId) {
      clearTimeout(toastInfo.timerId);
      toastInfo.timerId = null;
    }

    // Stop and fade out progress bar
    const progress = toastInfo.element.querySelector('.toast-progress');
    if (progress) {
      progress.classList.add('toast-progress-kept');
    }

    // Mark expand button as active (kept)
    const expandBtn = toastInfo.element.querySelector('.toast-header-expand');
    if (expandBtn) {
      expandBtn.classList.add('toast-header-expand-active');
    }
  }

  /**
   * Dismiss a toast by ID
   * @param {string} id - The toast ID
   */
  dismiss(id) {
    const toastInfo = this.toasts.get(id);
    if (!toastInfo) return;

    // Clear auto-dismiss timer
    if (toastInfo.timerId) {
      clearTimeout(toastInfo.timerId);
      toastInfo.timerId = null;
    }

    const { element } = toastInfo;

    // Trigger exit animation
    element.classList.remove('toast-visible');
    element.classList.add('toast-exit');

    // Remove from DOM after animation (use CSS variable with fallback for test environment)
     let transitionDelay = 450; // Default fallback (--transition-duration / 2 = 900ms / 2)
    try {
      const computedDelay = getComputedStyle(document.documentElement).getPropertyValue('--transition-delay');
      if (computedDelay) {
        const parsed = parseFloat(computedDelay);
        if (!isNaN(parsed) && parsed > 0) {
          transitionDelay = parsed;
        }
      }
    } catch (_e) {
      // Use fallback in test environments where getComputedStyle may not work
    }
    setTimeout(() => {
      element.remove();
      this.toasts.delete(id);
    }, transitionDelay);
  }

  /**
   * Dismiss all toasts
   */
  dismissAll() {
    const ids = Array.from(this.toasts.keys());
    ids.forEach(id => this.dismiss(id));
  }
}

// Create a singleton instance
const toast = new ToastManager();

const toast$1 = /*#__PURE__*/Object.freeze(/*#__PURE__*/Object.defineProperty({
	__proto__: null,
	TOAST_TYPES,
	toast
}, Symbol.toStringTag, { value: 'Module' }));

class AuthManager {
  constructor(app) {
    this.app = app;
    this.user = null;
    this._expirationWarningTimer = null;
    this._expirationCountdownInterval = null;
    this._expirationWarningToastId = null;
    this._expirationWarningActive = false;
    this._lastUserActivity = 0;
    this._tokenScheduledAt = 0;
    this._isRenewing = false;
  }

  _saveLastManager(type, id) {
    try {
      localStorage.setItem('lithium_last_manager', `${type}:${id}`);
    } catch (_e) {
      logAuth(Status.WARN, `Failed to sync settings from server: ${_e.message}`);
    }
  }

  async checkAuthAndLoad() {
    const token = retrieveJWT();
    const validation = validateJWT(token);

    if (validation.valid) {
      logAuth(Status.INFO, `Valid JWT found for user ${validation.claims?.user_id || 'unknown'}`);
      this.user = {
        id: validation.claims.user_id,
        username: validation.claims.username,
        email: validation.claims.email,
        roles: validation.claims.roles,
      };

      if (window.lithiumSettings) {
        window.lithiumSettings.setUserContext(this.user.id);
        try {
          await window.lithiumSettings.syncFromServer(this.app.api);
        } catch (err) {
          logAuth(Status.WARN, `Failed to sync settings from server: ${err.message}`);
        }
      }

      this.scheduleTokenRenewal(token);
      await this.loadPostLoginLookups();
      // Load main manager without showing it (lithium-app.js will show it after splash is dismissed)
      await this.app.managerLoader.loadMainManager({ skipShowAnimation: true });
    } else {
      logAuth(Status.INFO, 'No valid JWT, loading login');
      await this.loadLoginManager();
    }
  }

  async loadPostLoginLookups() {
    logAuth(Status.INFO, '[App] Loading post-login lookups...');
    try {
      const result = await loadMacrosPostLogin(this.app.api);
      logAuth(Status.INFO, `[App] Post-login lookups loaded: ${result}`);
    } catch (error) {
      logAuth(Status.WARN, `Failed to load post-login lookups: ${error.message}`);
    }
  }

  scheduleTokenRenewal(token) {
    this.clearExpirationTimers();
    const timeUntilExpiry = getTimeUntilExpiry(token);
    this._tokenScheduledAt = Date.now();

    const WARNING_LEAD_TIME = 90000;
    if (timeUntilExpiry > WARNING_LEAD_TIME) {
      const checkTime = timeUntilExpiry - WARNING_LEAD_TIME;
      logAuth(Status.INFO, `Token activity check scheduled in ${Math.round(checkTime / 1000)}s (expiry in ${Math.round(timeUntilExpiry / 1000)}s)`);

      this._expirationWarningTimer = setTimeout(() => {
        if (this._lastUserActivity > this._tokenScheduledAt) {
          logAuth(Status.INFO, 'User activity detected at T-90s, attempting auto-renewal');
          this.renewToken().catch((error) => {
            logAuth(Status.WARN, `Auto-renewal failed: ${error.message}, showing expiration warning`);
            this.showExpirationWarning();
          });
        } else {
          this.showExpirationWarning();
        }
      }, checkTime);
    } else if (timeUntilExpiry > 0) {
      this.showExpirationWarning(Math.floor(timeUntilExpiry / 1000));
    }
  }

  showExpirationWarning(initialSeconds = 90) {
    if (this._expirationWarningActive) return;
    this._expirationWarningActive = true;

    let secondsRemaining = initialSeconds;
    const getMessage = () => `Session expires in ${secondsRemaining}s`;

    this._expirationWarningToastId = toast.warning(getMessage(), {
      title: 'Session Expiring',
      description: 'Your session will expire soon. Dismiss this notification or interact with the page to extend your session.',
      duration: 0,
      dismissible: true,
      subsystem: 'Auth',
    });

    // Wire the toast's close button to attempt renewal instead of just hiding.
    // Dismissing the warning is treated as an explicit "I'm still here" signal.
    if (this._expirationWarningToastId) {
      const toastEl = document.querySelector(`[data-id="${this._expirationWarningToastId}"]`);
      const closeBtn = toastEl?.querySelector('.toast-header-close');
      if (closeBtn) {
        // Capture phase so we run before toast.js's own dismiss handler removes the element.
        closeBtn.addEventListener('click', (e) => {
          e.stopImmediatePropagation();
          e.preventDefault();
          logAuth(Status.INFO, 'Expiration warning dismissed by user, attempting renewal');
          this.recordUserActivity();
          this.renewToken().catch((error) => {
            logAuth(Status.WARN, `Renewal after warning dismiss failed: ${error.message}`);
          });
        }, { capture: true });
      }
    }

    logAuth(Status.WARN, `Token expires in ${secondsRemaining}s`);

    this._expirationCountdownInterval = setInterval(() => {
      secondsRemaining--;
      if (this._expirationWarningToastId && secondsRemaining > 0) {
        const toastEl = document.querySelector(`[data-id="${this._expirationWarningToastId}"]`);
        if (toastEl) {
          const titleBtn = toastEl.querySelector('.toast-header-title');
          if (titleBtn) titleBtn.textContent = getMessage();
        }
      }
      if ([60, 30, 10].includes(secondsRemaining)) {
        logAuth(Status.WARN, `Token expires in ${secondsRemaining}s`);
      }
      if (secondsRemaining <= 0) {
        this.clearExpirationTimers();
        this.performExpiredSessionLogout();
      }
    }, 1000);
  }

  clearExpirationTimers() {
    if (this._expirationWarningTimer) {
      clearTimeout(this._expirationWarningTimer);
      this._expirationWarningTimer = null;
    }
    if (this._expirationCountdownInterval) {
      clearInterval(this._expirationCountdownInterval);
      this._expirationCountdownInterval = null;
    }
    this._expirationWarningActive = false;
  }

  recordUserActivity() {
    this._lastUserActivity = Date.now();

    // If the expiration warning is currently shown, real user activity should
    // immediately attempt renewal instead of waiting for the countdown to hit 0.
    // This is rate-limited by _isRenewing inside renewToken() and the 10s guard
    // in _renewOnActivity().
    if (this._expirationWarningActive) {
      this._renewOnActivity();
    }
  }

  async _renewOnActivity() {
    if (this._isRenewing) return;

    // Skip the 10s freshness guard when the warning is showing — at that point
    // the token is genuinely close to expiry and the user needs renewal now.
    if (!this._expirationWarningActive) {
      const tokenAge = Date.now() - this._tokenScheduledAt;
      if (tokenAge < 10000) return;
    }

    const token = retrieveJWT();
    if (!token) return;
    const validation = validateJWT(token);
    if (!validation.valid) return;

    try {
      await this.renewToken();
    } catch (error) {
      logAuth(Status.WARN, `Activity-triggered renewal failed: ${error.message}`);
    }
  }

  async performExpiredSessionLogout() {
    logAuth(Status.WARN, 'Session expired - performing logout');
    if (this._expirationWarningToastId) {
      toast.dismiss(this._expirationWarningToastId);
      this._expirationWarningToastId = null;
    }
    toast.error('Session Expired', {
      title: 'Session Expired',
      description: 'Your session has expired. You have been logged out.',
      duration: 5000,
      subsystem: 'Auth',
    });
    await this._fadeOutForLogout();

    // Call logout endpoint (even though expired, to clean up server-side)
    try {
      await this.app.api.post('auth/logout');
    } catch (error) {
      logAuth(Status.WARN, `Logout API call failed: ${error.message}`);
    }

    // Clear user state
    this.user = null;
    this.app.loadedManagers.clear();
    this.app.mainManagerInstance = null;

    // Reload the page
    window.location.reload(true);
  }

  async renewToken() {
    if (this._isRenewing) return;
    this._isRenewing = true;
    const wasWarningActive = this._expirationWarningActive;

    try {
      logAuth(Status.INFO, 'Starting token renewal');
      const start = Date.now();
      const response = await this.app.api.post('auth/renew', {});

      if (response.token) {
        this.clearExpirationTimers();
        if (this._expirationWarningToastId) {
          toast.dismiss(this._expirationWarningToastId);
          this._expirationWarningToastId = null;
        }
        storeJWT(response.token);
        this.scheduleTokenRenewal(response.token);
        eventBus.emit(Events.AUTH_RENEWED, { expiresAt: response.expires_at });
        logAuth(Status.SUCCESS, 'Token renewed successfully', Date.now() - start);

        if (wasWarningActive) {
          toast.success('Session Renewed', {
            title: 'Session Extended',
            description: 'Your session has been successfully renewed.',
            duration: 3000,
            subsystem: 'Auth',
          });
        }
      }
    } finally {
      this._isRenewing = false;
    }
  }

  async loadLoginManager() {
    try {
      const { default: LoginManager } = await __vitePreload(async () => { const { default: LoginManager } = await import('./login-CJ3GMAAP.js');return { default: LoginManager }},true              ?__vite__mapDeps([0,1,2,3,4]):void 0,import.meta.url);
      const container = document.getElementById('app');
      container.innerHTML = '';
      const loginManager = new LoginManager(this.app, container);
      await loginManager.init();
      this.app.currentManager = { name: 'login', instance: loginManager };
    } catch (error) {
      console.error('[Lithium] Failed to load login manager:', error);
      this.app.showFatalError('Failed to load login page. Please check your connection and refresh.');
    }
  }

   async performQuickLogoutCleanup() {
     logAuth(Status.INFO, 'Performing quick logout cleanup');
     await this._fadeOutForLogout();
     await this.performLogoutActions('quick');
     clearJWT();
     window.location.reload(true);
   }

   async performNormalLogoutCleanup() {
     logAuth(Status.INFO, 'Performing normal logout cleanup');
     await this._fadeOutForLogout();
     await this.performLogoutActions('normal');
     clearJWT();
     window.location.reload(true);
   }

  async performPublicLogoutCleanup() {
    logAuth(Status.INFO, 'Performing public logout cleanup');
    await this._fadeOutForLogout();
    await this.performLogoutActions('public');
    window.location.reload(true);
  }

   async performGlobalLogoutCleanup() {
     logAuth(Status.INFO, 'Performing global logout cleanup');
     // Fade out main UI (using the same fadeout as regular logout, but without logout panel)
     await this._fadeOutForLogout();
     await this.performLogoutActions('global');
     window.location.reload(true);
   }

   async handleAuthExpired() {
     logAuth(Status.WARN, 'Authentication expired, redirecting to login');
     // Clear expiration timers to prevent any further checks
     this.clearExpirationTimers();
     // Perform quick logout with fade-out transition
     await this.performQuickLogoutCleanup('quick');
   }

  async performLogoutActions(type = 'normal') {
    // For 'normal' logout, remove lithium_last_username
    if (type === 'normal') {
      localStorage.removeItem('lithium_last_username');

      // Flush any pending settings to server before logout
      if (window.lithiumSettings && typeof window.lithiumSettings.flush === 'function') {
        try {
          await window.lithiumSettings.flush();
        } catch (e) {
          logAuth(Status.WARN, `Failed to flush settings before logout: ${e.message}`);
        }
      }
    }

    // For 'public' logout, clear ALL localStorage for this site
    if (type === 'public') {
      // Flush any pending settings to server before logout
      if (window.lithiumSettings && typeof window.lithiumSettings.flush === 'function') {
        try {
          await window.lithiumSettings.flush();
        } catch (e) {
          logAuth(Status.WARN, `Failed to flush settings before logout: ${e.message}`);
        }
      }
      // Clear global settings service (in-memory state) AFTER syncing
      if (window.lithiumSettings && typeof window.lithiumSettings.clear === 'function') {
        window.lithiumSettings.clear();
      }
      // Clear ALL localStorage for this origin
      localStorage.clear();

      // Prevent saving username on next login (for shared computers)
      sessionStorage.setItem('lithium_no_save_username', 'true');

      logAuth(Status.INFO, 'Performed public logout - ALL local data cleared');
    }

    // For 'global' logout, clear ALL localStorage for this site
    if (type === 'global') {
      // Flush any pending settings to server before logout
      if (window.lithiumSettings && typeof window.lithiumSettings.flush === 'function') {
        try {
          await window.lithiumSettings.flush();
        } catch (e) {
          logAuth(Status.WARN, `Failed to flush settings before logout: ${e.message}`);
        }
      }
      // Clear global settings service (in-memory state) AFTER syncing
      if (window.lithiumSettings && typeof window.lithiumSettings.clear === 'function') {
        // Clear server settings for maximum security
        if (this.app.api) {
          try {
            await window.lithiumSettings.syncToServer(this.app.api);
          } catch (e) {
            logAuth(Status.WARN, `Failed to clear server settings: ${e.message}`);
          }
        }
        window.lithiumSettings.clear();
      }
      // Clear ALL localStorage for this origin
      localStorage.clear();

      // Clear service worker caches for a clean slate
      await this.clearCacheData();

      // Prevent saving username on next login (maximum security)
      sessionStorage.setItem('lithium_no_save_username', 'true');

      logAuth(Status.INFO, 'Performed global logout - ALL local and server data cleared');
    }

    this.user = null;
    this.app.loadedManagers.clear();
    this.app.mainManagerInstance = null;
    // No need to load login manager since we're reloading the page
  }

  /**
   * Clear service worker caches for global logout.
   * Ensures a clean cache state for first-user experience.
   */
  async clearCacheData() {
    try {
      if ('caches' in window) {
        const cacheNames = await caches.keys();
        await Promise.all(
          cacheNames.map(cacheName => caches.delete(cacheName))
        );
        logAuth(Status.INFO, `Cache data cleared: ${cacheNames.length} caches removed`);
      }
    } catch (error) {
      logAuth(Status.WARN, `Could not clear cache data: ${error.message}`);
    }
  }

   /**
    * Fade out UI elements for logout with proper JS-driven transitions.
    * For regular logout: fades out logout panel first (900ms), then main UI (900ms).
    * For profile manager global logout: fades out main UI directly (900ms).
    */
  async _fadeOutForLogout() {
    const mainMgr = this.app.mainManagerInstance;
    if (!mainMgr) return;

    // Destroy all tooltips first so they don't linger during logout
    destroyAllTooltips();

    // Disable all user interactions during logout to prevent clicks during fadeout
    document.body.style.pointerEvents = 'none';

     // Fade out logout panel first (if visible) - two-stage JS transition
     if (mainMgr.isLogoutPanelVisible && mainMgr.elements.logoutPanel) {
       const panel = mainMgr.elements.logoutPanel;
       const overlay = mainMgr.elements.logoutOverlay;

       // Stage 1: Set up transition and starting opacity
       panel.style.transition = 'opacity 900ms ease-out';
       panel.style.opacity = '1';
       if (overlay) {
         overlay.style.transition = 'opacity 900ms ease-out';
         overlay.style.opacity = '1';
       }

       // Wait for requestAnimationFrame to ensure styles are applied
       await new Promise(resolve => requestAnimationFrame(resolve));

       // Stage 2: Trigger fade out
       panel.style.opacity = '0';
       if (overlay) overlay.style.opacity = '0';

       // Wait for transition to complete
       await new Promise(resolve => setTimeout(resolve, 900));

       // Clean up inline styles
       panel.classList.remove('visible');
       panel.style.transition = '';
       panel.style.opacity = '';
       if (overlay) {
         overlay.classList.remove('visible');
         overlay.style.transition = '';
         overlay.style.opacity = '';
       }
     }

     // Fade out main layout - two-stage JS transition
     const layout = mainMgr.elements.layout;
     if (layout) {
       // Stage 1: Set up transition and starting opacity
       layout.style.transition = 'opacity 900ms ease-out';
       layout.style.opacity = '1';

       // Wait for requestAnimationFrame to ensure styles are applied
       await new Promise(resolve => requestAnimationFrame(resolve));

       // Stage 2: Trigger fade out
       layout.style.opacity = '0';

       // Wait for transition to complete
       await new Promise(resolve => setTimeout(resolve, 900));

       // Clean up inline styles
       layout.classList.remove('visible');
       layout.style.transition = '';
       layout.style.opacity = '';
     }
  }
}

/**
 * Permissions - Punchcard-based permission system
 * 
 * Handles manager access and feature permissions based on
 * JWT punchcard claim. Falls back to allow-all if no punchcard present.
 */

// Note: getClaims available for future use if needed

/**
 * Check if user can access a manager
 * @param {number|string} managerId - Manager ID
 * @param {Object} punchcard - Punchcard from JWT (optional, falls back to all access)
 * @returns {boolean} True if access allowed
 */
function canAccessManager(managerId, punchcard = null) {
  // If no punchcard provided, try to get from current JWT
  if (!punchcard) {
    // Fallback: allow all managers if no punchcard
    return true;
  }

  // If punchcard exists but has no managers array, deny all
  if (!punchcard.managers || !Array.isArray(punchcard.managers)) {
    return false;
  }

  // Check if manager ID is in the allowed list
  return punchcard.managers.includes(Number(managerId));
}

/**
 * Check if user has a specific feature for a manager
 * @param {number|string} managerId - Manager ID
 * @param {number|string} featureId - Feature ID
 * @param {Object} punchcard - Punchcard from JWT (optional)
 * @returns {boolean} True if feature allowed
 */
function hasFeature(managerId, featureId, punchcard = null) {
  // If no punchcard provided, fallback: allow all features
  if (!punchcard) {
    return true;
  }

  // Check manager access first
  if (!canAccessManager(managerId, punchcard)) {
    return false;
  }

  // If no features defined, allow none
  if (!punchcard.features || typeof punchcard.features !== 'object') {
    return false;
  }

  // Get features for this manager
  const managerFeatures = punchcard.features[String(managerId)];
  if (!Array.isArray(managerFeatures)) {
    return false;
  }

  return managerFeatures.includes(Number(featureId));
}

/**
 * Get list of permitted manager IDs
 * @param {Object} punchcard - Punchcard from JWT (optional)
 * @returns {Array} Array of manager IDs
 */
function getPermittedManagers(punchcard = null) {
  // Fallback: return all default managers if no punchcard
  if (!punchcard) {
    // Return default manager IDs from lithium.json (007-032)
    // Excludes Group 0: System (001-006) which are hidden from main menu
    return [7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32];
  }

  return punchcard.managers || [];
}

let MANAGER_CROSSFADE_MS = 2500;

class ManagerLoader {
  constructor(app) {
    this.app = app;
    this.managerRegistry = {
      // Content
      7: { id: 7, name: 'Dashboard Manager' },
      8: { id: 8, name: 'Mail Manager' },
      // User Management
      9: { id: 9, name: 'Profile Manager' },
      10: { id: 10, name: 'Session Manager' },
      11: { id: 11, name: 'Version Manager' },
      // Shared
      12: { id: 12, name: 'Calendar Manager' },
      13: { id: 13, name: 'Contact Manager' },
      14: { id: 14, name: 'File Manager' },
      15: { id: 15, name: 'Document Manager' },
      16: { id: 16, name: 'Media Manager' },
      17: { id: 17, name: 'Diagram Manager' },
      // AI
      18: { id: 18, name: 'Chat Manager' },
      // Support
      19: { id: 19, name: 'Notification Manager' },
      20: { id: 20, name: 'Annotation Manager' },
      21: { id: 21, name: 'Ticketing Manager' },
      // Configuration
      22: { id: 22, name: 'Style Manager' },
      23: { id: 23, name: 'Lookup Manager' },
      24: { id: 24, name: 'Report Manager' },
      // Security
      25: { id: 25, name: 'Role Manager' },
      26: { id: 26, name: 'Security Manager' },
      27: { id: 27, name: 'AI Auditor Manager' },
      28: { id: 28, name: 'Job Manager' },
      29: { id: 29, name: 'Query Manager' },
      30: { id: 30, name: 'Sync Manager' },
      31: { id: 31, name: 'Camera Manager' },
      32: { id: 32, name: 'Terminal' },
    };

    this.transitionOverlay = null;
  }

  getTransitionDurationFromCSS() {
    const duration = getComputedStyle(document.documentElement)
      .getPropertyValue('--transition-duration')
      .trim();
    if (duration.includes('ms')) {
      MANAGER_CROSSFADE_MS = parseInt(duration, 10);
    } else if (duration.includes('s')) {
      MANAGER_CROSSFADE_MS = parseFloat(duration) * 1000;
    }
    return MANAGER_CROSSFADE_MS;
  }

  getTransitionOverlay() {
    if (this.transitionOverlay) return this.transitionOverlay;
    this.transitionOverlay = document.createElement('div');
    this.transitionOverlay.className = 'manager-transition-overlay';
    document.body.appendChild(this.transitionOverlay);
    return this.transitionOverlay;
  }

  showTransitionOverlay() {
    const overlay = this.getTransitionOverlay();
    if (overlay) overlay.classList.add('active');
  }

  hideTransitionOverlay() {
    const overlay = this.getTransitionOverlay();
    if (overlay) overlay.classList.remove('active');
  }

  _logManagers() {
    const managers = Object.values(this.managerRegistry);
    logGroup(Subsystems.STARTUP, Status.INFO, `Managers available: ${managers.length}`,
      managers.map(m => `${String(m.id).padStart(4, '0')}: ${m.name}`));
  }

  async _importManager(managerId) {
    switch (managerId) {
      case 7: return __vitePreload(() => import('./dashboard-Dyj7k_ns.js'),true              ?__vite__mapDeps([5,6]):void 0,import.meta.url);
      case 8: return __vitePreload(() => import('./mail-manager-tqaUe5TF.js'),true              ?__vite__mapDeps([7,8]):void 0,import.meta.url);
      case 9: return __vitePreload(() => import('./server-profiles-BT5GzYq5.js'),true              ?__vite__mapDeps([9,10]):void 0,import.meta.url);
      case 10: return __vitePreload(() => import('./server-sessions-DkOUj5VY.js'),true              ?__vite__mapDeps([11,12]):void 0,import.meta.url);
      case 11: return __vitePreload(() => import('./version-history-DhTvkkD5.js'),true              ?__vite__mapDeps([13,14,15,16,2,17,18,19,20,1,21,22,23,24,25,4]):void 0,import.meta.url);
      case 12: return __vitePreload(() => import('./calendar-manager-BbwTkDp9.js'),true              ?__vite__mapDeps([26,27]):void 0,import.meta.url);
      case 13: return __vitePreload(() => import('./contact-manager-BmLpgjLX.js'),true              ?__vite__mapDeps([28,29]):void 0,import.meta.url);
      case 14: return __vitePreload(() => import('./file-manager-B4k_RVAx.js'),true              ?__vite__mapDeps([30,31]):void 0,import.meta.url);
      case 15: return __vitePreload(() => import('./document-library-CPVBtoUE.js'),true              ?__vite__mapDeps([32,33]):void 0,import.meta.url);
      case 16: return __vitePreload(() => import('./media-library-DBSIGjBk.js'),true              ?__vite__mapDeps([34,35]):void 0,import.meta.url);
      case 17: return __vitePreload(() => import('./diagram-library-Bejw-KFP.js'),true              ?__vite__mapDeps([36,37]):void 0,import.meta.url);
      case 18: return __vitePreload(() => import('./chats-DfR9kckd.js'),true              ?__vite__mapDeps([38,39]):void 0,import.meta.url);
      case 19: return __vitePreload(() => import('./notification-manager-BYbN9li5.js'),true              ?__vite__mapDeps([40,41]):void 0,import.meta.url);
      case 20: return __vitePreload(() => import('./annotation-manager-sq6PTP15.js'),true              ?__vite__mapDeps([42,43]):void 0,import.meta.url);
      case 21: return __vitePreload(() => import('./ticketing-manager-BQ7P7EkV.js'),true              ?__vite__mapDeps([44,45]):void 0,import.meta.url);
      case 22: return __vitePreload(() => import('./style-manager-C-T7mqzw.js'),true              ?__vite__mapDeps([46,18,19,14,15,16,2,17,20,1,21,22,23,24,47,48,4]):void 0,import.meta.url);
      case 23: return __vitePreload(() => import('./lookups-B_m-1PlQ.js'),true              ?__vite__mapDeps([49,14,15,16,2,17,18,19,20,1,21,22,23,50,51,52,24,47,53,4]):void 0,import.meta.url);
      case 24: return __vitePreload(() => import('./reports-oxF4xt_m.js'),true              ?__vite__mapDeps([54,55]):void 0,import.meta.url);
      case 25: return __vitePreload(() => import('./role-manager-edDk62YU.js'),true              ?__vite__mapDeps([56,57]):void 0,import.meta.url);
      case 26: return __vitePreload(() => import('./security-manager-D_d_wBKH.js'),true              ?__vite__mapDeps([58,59]):void 0,import.meta.url);
      case 27: return __vitePreload(() => import('./ai-auditor-B0PTDy_L.js'),true              ?__vite__mapDeps([60,61]):void 0,import.meta.url);
      case 28: return __vitePreload(() => import('./job-manager-DE98yyDl.js'),true              ?__vite__mapDeps([62,63]):void 0,import.meta.url);
      case 29: return __vitePreload(() => import('./queries-DWQDr50V.js'),true              ?__vite__mapDeps([64,18,19,14,15,16,2,17,20,1,21,22,23,50,51,47,24,65,4]):void 0,import.meta.url);
      case 30: return __vitePreload(() => import('./sync-manager-DGd7MsAz.js'),true              ?__vite__mapDeps([66,67]):void 0,import.meta.url);
      case 31: return __vitePreload(() => import('./camera-manager-fk-CGRVm.js'),true              ?__vite__mapDeps([68,69]):void 0,import.meta.url);
      case 32: return __vitePreload(() => import('./terminal-_IKFZm20.js'),true              ?__vite__mapDeps([70,71]):void 0,import.meta.url);
      case 1: return __vitePreload(() => import('./login-CJ3GMAAP.js'),true              ?__vite__mapDeps([0,1,2,3,4]):void 0,import.meta.url);
      case 2: return __vitePreload(() => import('./server-sessions-DkOUj5VY.js'),true              ?__vite__mapDeps([11,12]):void 0,import.meta.url);
      case 3: return __vitePreload(() => import('./version-history-DhTvkkD5.js'),true              ?__vite__mapDeps([13,14,15,16,2,17,18,19,20,1,21,22,23,24,25,4]):void 0,import.meta.url);
      case 4: return __vitePreload(() => import('./queries-DWQDr50V.js'),true              ?__vite__mapDeps([64,18,19,14,15,16,2,17,20,1,21,22,23,50,51,47,24,65,4]):void 0,import.meta.url);
      case 5: return __vitePreload(() => import('./lookups-B_m-1PlQ.js'),true              ?__vite__mapDeps([49,14,15,16,2,17,18,19,20,1,21,22,23,50,51,52,24,47,53,4]):void 0,import.meta.url);
      case 6: return __vitePreload(() => import('./chats-DfR9kckd.js'),true              ?__vite__mapDeps([38,39]):void 0,import.meta.url);
      default: throw new Error(`No import mapping for manager ID: ${managerId}`);
    }
  }

  async _importUtilityManager(managerKey) {
    switch (managerKey) {
      case 'session-log': return __vitePreload(() => import('./session-log-DxaXKH9Q.js'),true              ?__vite__mapDeps([72,1,20,16,21,22,23,2,24,47,73]):void 0,import.meta.url);
      case 'user-profile': return __vitePreload(() => import('./profile-manager-YaKVbSkH.js'),true              ?__vite__mapDeps([74,18,19,20,16,1,21,22,23,47,51,2,14,15,17,24,75,4,76]):void 0,import.meta.url);
      case 'terminal': return __vitePreload(() => import('./terminal-_IKFZm20.js'),true              ?__vite__mapDeps([70,71]):void 0,import.meta.url);
      default: throw new Error(`No import mapping for utility manager key: ${managerKey}`);
    }
  }

  _getMainManager() {
    return this.app.mainManagerInstance || null;
  }

  _saveLastManager(type, id) {
    try {
      localStorage.setItem('lithium_last_manager', `${type}:${id}`);
    } catch (_e) {
      // Non-fatal
    }
  }

  async loadMainManager(options = {}) {
    try {
      const { default: MainManager } = await __vitePreload(async () => { const { default: MainManager } = await import('./main-BzM9uq37.js').then(n => n.m);return { default: MainManager }},true              ?__vite__mapDeps([77,20,16,1,21,22,23,78,79,76]):void 0,import.meta.url);
      const container = document.getElementById('app');
      container.innerHTML = '';
      const permittedManagers = getPermittedManagers();
      const mainManager = new MainManager(this.app, container, permittedManagers);
      this.app.mainManagerInstance = mainManager;
      await mainManager.init(options);

      if (this.app.currentManager?.id == null) {
        this.app.currentManager = { name: 'main', instance: mainManager };
      }
    } catch (error) {
      console.error('[Lithium] Failed to load main manager:', error);
      this.app.showFatalError('Failed to load application. Please refresh the page.');
    }
  }

  async loadManager(managerId) {
    const managerDef = this.managerRegistry[managerId];
    if (!managerDef) {
      logManager(Status.ERROR, `Unknown manager ID: ${managerId}`);
      return;
    }

    if (this.app.loadedManagers.has(managerId)) {
      await this.showManager(managerId);
      eventBus.emit(Events.MANAGER_SWITCHED, { from: this.app.currentManager?.id, to: managerId });
      logManager(Status.INFO, `Switched to already-loaded manager: ${managerDef.name}`);
      return;
    }

    try {
      logManager(Status.INFO,'────────────────────');
      logManager(Status.INFO, `Loading manager: ${managerDef.name}`);
      const loadStart = Date.now();
      const module = await this._importManager(managerId);
      const mainMgr = this._getMainManager();
      if (!mainMgr) throw new Error('MainManager not ready');

      const iconInfo = mainMgr.managerIcons?.[managerId] || { iconHtml: '<fa fa-cube></fa>', name: managerDef.name };
      const slotId = mainMgr._slotId(managerId);
      const slotResult = mainMgr.createSlot(slotId, iconInfo.iconHtml, iconInfo.name, managerId);
      if (!slotResult) throw new Error('Failed to create manager slot');
      const { slotEl, workspaceEl } = slotResult;

      const ManagerClass = module.default;
      const managerInstance = new ManagerClass(this.app, workspaceEl);
      await managerInstance.init();

      this.app.loadedManagers.set(managerId, {
        id: managerId,
        name: managerDef.name,
        instance: managerInstance,
        slotEl,
        workspaceEl,
      });

      this.app.openManagers.add(managerId);
      mainMgr.updateOpenMenuItems();
      await this.showManager(managerId);
      eventBus.emit(Events.MANAGER_LOADED, { managerId });
      eventBus.emit(Events.MANAGER_SWITCHED, { from: this.app.currentManager?.id, to: managerId });
      logManager(Status.SUCCESS, `Manager ${managerDef.name} loaded`, Date.now() - loadStart);
      logManager(Status.INFO, '────────────────────');
    } catch (error) {
      logManager(Status.ERROR, `Failed to load manager ${managerDef?.name}: ${error.message}`);
    }
  }

  async showManager(managerId) {
    const managerData = this.app.loadedManagers.get(managerId) || this._getUtilityManagerData(managerId);
    if (!managerData) {
      logManager(Status.ERROR, `No manager data found for ID: ${managerId}`);
      return;
    }

    const mainMgr = this._getMainManager();
    if (!mainMgr) {
      logManager(Status.ERROR, 'MainManager not loaded');
      return;
    }

    const outgoingSlot = this.app.currentManager ? this._getSlotForCurrent() : null;
    const incomingSlot = managerData.slotEl || null;

    if (outgoingSlot === incomingSlot) {
      logManager(Status.INFO, `Manager ${managerData.name} is already visible`);
      return;
    }

    this.showTransitionOverlay();
    this._saveLastManager('manager', managerId);
    this.getTransitionDurationFromCSS();

    await this._crossfadeSlots(outgoingSlot, incomingSlot, () => {
      this.app.currentManager = { id: managerId, name: managerData.name, instance: managerData.instance };
      if (managerData.id) {
        this.app.openManagers.add(managerId);
        mainMgr.updateOpenMenuItems();
      }
      this.hideTransitionOverlay();
    });
  }

  _getSlotForCurrent() {
    if (!this.app.currentManager) return null;
    if (this.app.currentManager.name === 'main') return null;
    if (this.app.currentManager.name === 'login') return null;
    const data = this.app.loadedManagers.get(this.app.currentManager.id);
    return data ? data.slotEl : null;
  }

  _getUtilityManagerData(key) {
    const def = this.app.utilityManagerRegistry[key];
    if (!def) return null;
    return this.app.loadedManagers.get(def.id) || null;
  }

  async loadUtilityManager(key) {
    const def = this.app.utilityManagerRegistry[key];
    if (!def) {
      logManager(Status.ERROR, `Unknown utility manager key: ${key}`);
      return;
    }

    if (this.app.loadedManagers.has(def.id)) {
      await this.showManager(def.id);
      return;
    }

    try {
      logManager(Status.INFO, `Loading utility manager: ${def.name}`);
      const module = await this._importUtilityManager(key);
      const mainMgr = this._getMainManager();
      if (!mainMgr) throw new Error('MainManager not ready');

      const slotId = `utility-slot-${key}`;
      const slotResult = mainMgr.createSlot(slotId, '', def.name, def.id);
      if (!slotResult) throw new Error('Failed to create utility slot');
      const { slotEl, workspaceEl } = slotResult;

      const ManagerClass = module.default;
      const managerInstance = new ManagerClass(this.app, workspaceEl);
      await managerInstance.init();

      this.app.loadedManagers.set(def.id, {
        id: def.id,
        name: def.name,
        instance: managerInstance,
        slotEl,
        workspaceEl,
      });

      this.app.openManagers.add(def.id);
      mainMgr.updateOpenMenuItems();
      await this.showManager(def.id);

      logManager(Status.SUCCESS, `Utility manager ${def.name} loaded`);
    } catch (error) {
      logManager(Status.ERROR, `Failed to load utility manager ${def?.name}: ${error.message}`);
    }
  }

  async _crossfadeSlots(outgoing, incoming, onComplete) {
    const duration = MANAGER_CROSSFADE_MS;
    const steps = [];

    if (outgoing) {
      steps.push(new Promise(resolve => {
        outgoing.classList.add('slot-fading-out');
        setTimeout(() => {
          outgoing.classList.remove('slot-visible', 'slot-fading-out');
          outgoing.classList.add('slot-hidden');
          resolve();
        }, duration);
      }));
    }

    if (incoming) {
      steps.push(new Promise(resolve => {
        incoming.classList.remove('slot-hidden');
        incoming.classList.add('slot-visible');
        setTimeout(resolve, duration);
      }));
    }

    await Promise.all(steps);
    if (onComplete) onComplete();
  }

  /**
   * Close a manager by ID.
   * Removes the slot from DOM, cleans up the manager instance, and updates UI.
   * @param {number|string} managerId - Manager ID or utility key
   */
  async closeManager(managerId) {
    // Handle both numeric IDs and string keys
    let numericId = managerId;
    let managerData = this.app.loadedManagers.get(managerId);

    // If not found by direct key, try to find by numeric ID
    if (!managerData && typeof managerId === 'number') {
      managerData = this.app.loadedManagers.get(managerId);
    }

    // Try utility manager registry
    if (!managerData && this.app.utilityManagerRegistry[managerId]) {
      const def = this.app.utilityManagerRegistry[managerId];
      managerData = this.app.loadedManagers.get(def.id);
      numericId = def.id;
    }

    if (!managerData) {
      logManager(Status.WARN, `closeManager: No manager found for ID ${managerId}`);
      return;
    }

    logManager(Status.INFO, `Closing manager: ${managerData.name}`);

    // Remove slot from DOM
    if (managerData.slotEl && managerData.slotEl.parentNode) {
      managerData.slotEl.remove();
    }

    // Call destroy/cleanup on manager instance if it exists
    if (managerData.instance && typeof managerData.instance.destroy === 'function') {
      try {
        await managerData.instance.destroy();
      } catch (err) {
        logManager(Status.WARN, `Error destroying manager ${managerData.name}: ${err.message}`);
      }
    }

    // Remove from tracking
    this.app.loadedManagers.delete(numericId);
    this.app.openManagers.delete(numericId);

    // Update main manager UI
    const mainMgr = this._getMainManager();
    if (mainMgr) {
      mainMgr.updateOpenMenuItems();
    }

    // Clear currentManager if it was this manager
    if (this.app.currentManager && this.app.currentManager.id === numericId) {
      this.app.currentManager = null;
    }
  }
}

class LithiumApp {
  constructor() {
    this.version = null;
    this.build = null;
    this.config = null;
    this.api = null;
    this.currentManager = null;
    this.loadedManagers = new Map();
    this.openManagers = new Set();
    this.mainManagerInstance = null;
    this.user = null;
    this.deferredInstallPrompt = null;
    this.settingsService = new GlobalSettingsService();
    window.lithiumSettings = this.settingsService;

    // Utility manager registry (accessed by main-sidebar.js)
    this.utilityManagerRegistry = {
      'user-profile': { id: 3, key: 'user-profile', name: 'Profile' },
      'session-log': { id: 4, key: 'session-log', name: 'Session Log' },
      'terminal': { id: 5, key: 'terminal', name: 'Terminal' },
    };

    this.auth = new AuthManager(this);
    this.managerLoader = new ManagerLoader(this);
  }

  async init() {
    const initStart = Date.now();
    try {
      await this.loadVersion();
      let releaseDate = '';
      if (this.timestamp) {
        const date = new Date(this.timestamp);
        const months = ['Jan', 'Feb', 'Mar', 'Apr', 'May', 'Jun', 'Jul', 'Aug', 'Sep', 'Oct', 'Nov', 'Dec'];
        const hours = String(date.getUTCHours()).padStart(2, '0');
        const minutes = String(date.getUTCMinutes()).padStart(2, '0');
        releaseDate = `${date.getUTCFullYear()}-${months[date.getUTCMonth()]}-${String(date.getUTCDate()).padStart(2, '0')} ${hours}:${minutes} UTC`;
      }

      logGroup(Subsystems.STARTUP, Status.INFO, 'Lithium Application Started', [
        `Version: ${this.version || 'unknown'}`,
        `Release: ${releaseDate || 'unknown'}`,
      ]);
      this._logEnvironment();
      logStartup('Application initializing');

      this.config = await loadConfig();
      const serverUrl = getConfigValue('server.url', 'unknown');
      const apiPrefix = getConfigValue('server.api_prefix', '/api');
      const iconSet = getConfigValue('icons.set', 'duotone');
      const iconWeight = getConfigValue('icons.weight', 'thin');
      const iconPrefix = getConfigValue('icons.prefix', 'fad');
      const iconFallback = getConfigValue('icons.fallback', 'solid');
      let configFileSize = '';
      try {
        const configResponse = await fetch('/config/lithium.json', { method: 'HEAD' });
        if (configResponse.ok) {
          const contentLength = configResponse.headers.get('content-length');
          if (contentLength) configFileSize = ` (${parseInt(contentLength).toLocaleString()} bytes)`;
        }
      } catch (_e) { /* ignore errors */ }
      logGroup(Subsystems.STARTUP, Status.INFO, 'Application Configuration', [
        `ConfigFile: /config/lithium.json${configFileSize}`,
        `Server: ${serverUrl}${apiPrefix}`,
        `Icons: ${iconPrefix}-${iconWeight} ${iconSet} (fallback: ${iconFallback})`,
      ]);

      this.managerLoader._logManagers();
      const apiStart = Date.now();
      this.api = createRequest(this.config);
      logStartup('API client created', Date.now() - apiStart);

      const iconsStart = Date.now();
      init();
      logStartup('Icon system initialized', Date.now() - iconsStart);

      this.revealPage();
      logStartup('Page revealed');
      initTooltips();
      logStartup('Tooltip system initialized');

      // Set up event listeners BEFORE auth check so login:success handler is ready
      this.setupEventListeners();
      logStartup('Event listeners registered');

      // Track real user activity so the auth manager can refresh the JWT
      // before the inactivity timeout fires.
      this.setupActivityTracking();
      logStartup('User activity tracking active');

      await this.auth.checkAuthAndLoad();
      logStartup('Auth check complete', Date.now() - initStart);

      // Crossfade from Loading to main UI (run both transitions concurrently)
      if (this.mainManagerInstance) {
        await Promise.all([
          this.dismissSplash(),
          this._crossfadeToMainUI(),
        ]);
      } else {
        // No main manager (login form shown), just dismiss splash
        await this.dismissSplash();
      }

      this._runBackgroundStartup(initStart);
    } catch (error) {
      logStartup(`Initialization failed: ${error.message}`);
      console.error('[Lithium] Failed to initialize:', error);
      this.showFatalError('Failed to initialize application. Please refresh the page.');
    }
  }

  async _runBackgroundStartup(initStart) {
    try {
      try {
        const healthResponse = await this.api.get('system/health');
        const healthStatus = healthResponse.status || healthResponse.message || 'unknown';
        logGroup(Subsystems.STARTUP, Status.INFO, 'Server Health Check', [`Status: ${healthStatus}`]);
      } catch (error) {
        logGroup(Subsystems.STARTUP, Status.WARN, 'Server Health Check', [`Status: Failed`, `Error: ${error.message}`]);
      }

      const lookupsStart = Date.now();
      init$1();
      this.fetchLookups();
      logStartup('Lookups initialized', Date.now() - lookupsStart);

      preloadIconsFromConfig().catch(() => {});
      this.setupNetworkMonitoring();
      logStartup('Network monitoring active');

      const totalInitTime = Date.now() - initStart;
      logStartup(`Application initialized successfully in ${(totalInitTime / 1000).toFixed(2)}s`);
    } catch (error) {
      logStartup(`Background startup error: ${error.message}`);
    } finally {
      eventBus.emit(Events.STARTUP_COMPLETE);
    }
    logStartup('────────────────────');
  }

  _logEnvironment() {
    const ua = navigator.userAgent;
    const browser = this._detectBrowser(ua);
    const sessionId = getSessionId();
    logGroup(Subsystems.STARTUP, Status.INFO, 'Browser Environment', [
      `Session: ${sessionId}`,
      `Browser: ${browser.name} ${browser.version}`,
      `Platform: ${navigator.platform}`,
      `Language: ${navigator.language}`,
      `Screen: ${window.screen.width}x${window.screen.height}, Color Depth: ${window.screen.colorDepth}`,
      `Online: ${navigator.onLine}`,
    ]);
  }

  _detectBrowser(ua) {
    let name = 'Unknown', version = 'Unknown';
    if (ua.includes('Firefox/')) {
      name = 'Firefox';
      version = ua.match(/Firefox\/(\d+)/)?.[1] || '';
    } else if (ua.includes('Edg/')) {
      name = 'Edge';
      version = ua.match(/Edg\/(\d+)/)?.[1] || '';
    } else if (ua.includes('Chrome/')) {
      name = 'Chrome';
      version = ua.match(/Chrome\/(\d+)/)?.[1] || '';
    } else if (ua.includes('Safari/') && !ua.includes('Chrome')) {
      name = 'Safari';
      version = ua.match(/Version\/(\d+)/)?.[1] || '';
    }
    return { name, version };
  }

  async loadVersion() {
    try {
      if (window.__lithiumVersionData) {
        const data = window.__lithiumVersionData;
        this.version = data.version;
        this.build = data.build;
        this.timestamp = data.timestamp;
        return;
      }
      if (window.__lithiumVersionPromise) {
        const data = await window.__lithiumVersionPromise;
        if (data) {
          this.version = data.version;
          this.build = data.build;
          this.timestamp = data.timestamp;
          return;
        }
      }
      const response = await fetch('/version.json');
      if (response.ok) {
        const data = await response.json();
        this.version = data.version;
        this.build = data.build;
        this.timestamp = data.timestamp;
      }
    } catch (_error) { /* ignore */ }
  }

  revealPage() {
    requestAnimationFrame(() => {
      document.documentElement.style.visibility = 'visible';
    });
  }

  dismissSplash() {
    const el = document.getElementById('Loading');
    if (!el) return Promise.resolve();
    const currentOpacity = getComputedStyle(el).opacity;
    el.style.animation = 'none';
    el.style.opacity = currentOpacity;
    void el.offsetHeight;
    el.style.transition = 'opacity 900ms ease-out';
    el.style.opacity = '0';
    return new Promise(resolve => {
      el.addEventListener('transitionend', () => {
        el.remove();
        resolve();
      }, { once: true });
    });
  }

  async _crossfadeToMainUI() {
    // Crossfade the main UI in with a slow transition
    const mainInstance = this.mainManagerInstance;
    if (!mainInstance || !mainInstance.elements.layout) return;

    const layout = mainInstance.elements.layout;
    
    // Step 1: Start at opacity 0 WITH inline style (overrides CSS .visible class)
    layout.style.opacity = '0';
    layout.style.transition = 'none';
    
    // Step 2: Add visible class (CSS says opacity:1, but inline opacity:0 overrides)
    layout.classList.add('visible');
    
    // Step 3: Force reflow so browser registers the starting state
    void layout.offsetHeight;
    
    // Step 4: Set up the transition
    layout.style.transition = 'opacity 900ms ease-in';
    void layout.offsetHeight;
    
    // Step 5: Change to opacity 1 - this triggers the animation
    layout.style.opacity = '1';
    
    // Wait for animation to complete
    await new Promise(resolve => setTimeout(resolve, 900));
  }

  fetchLookups() {
    try {
      fetchLookups();
    } catch (error) {
      log(Subsystems.LOOKUPS, Status.WARN, `Failed to fetch lookups: ${error.message}`);
    }
  }

  setupEventListeners() {
    // Register event listeners BEFORE any async operations

    // Login success handler - proceed to main manager after successful login
    eventBus.on(Events.AUTH_LOGIN, async (event) => {
      const data = event.detail;
      logAuth(Status.INFO, `Login successful for user ${data.username}, proceeding to main manager`);
      // Set user info on auth manager
      this.auth.user = {
        id: data.userId,
        username: data.username,
        roles: data.roles,
      };
      // Sync user context and load settings from server before proceeding
      if (window.lithiumSettings) {
        window.lithiumSettings.setUserContext(this.auth.user.id);
        try {
          await window.lithiumSettings.syncFromServer(this.api);
        } catch (err) {
          logAuth(Status.WARN, `Failed to sync settings from server: ${err.message}`);
        }
      }
      // Schedule token renewal if expiresAt provided
      if (data.expiresAt) {
        const token = retrieveJWT();
        if (token) {
          this.auth.scheduleTokenRenewal(token);
        }
      }
      // Load post-login lookups
      this.auth.loadPostLoginLookups();
      // Load main manager (skip show animation - we'll crossfade after init)
      await this.managerLoader.loadMainManager({ skipShowAnimation: true });
      // Crossfade from blank screen to main UI (after login form fades out)
      await this._crossfadeToMainUI();
    });

    // Logout handler
    eventBus.on('logout:initiate', async (event) => {
      const data = event.detail;
      logAuth(Status.INFO, `Logout type received: ${data.logoutType}`);

      // Disable all user interactions immediately to prevent any clicks during logout
      document.body.style.pointerEvents = 'none';

      // Clear expiration timers immediately
      this.auth.clearExpirationTimers();

      try {
        if (data.logoutType === 'quick') {
          await this.auth.performQuickLogoutCleanup(data.logoutType);
        } else if (data.logoutType === 'normal') {
          await this.auth.performNormalLogoutCleanup(data.logoutType);
        } else if (data.logoutType === 'public') {
          await this.auth.performPublicLogoutCleanup(data.logoutType);
        } else if (data.logoutType === 'global') {
          await this.auth.performGlobalLogoutCleanup(data.logoutType);
        }
      } catch (error) {
        logAuth(Status.ERROR, `Logout cleanup failed: ${error.message}`);
        // Force reload even if cleanup fails
        window.location.reload(true);
      }
    });

    // Auth expired handler
    eventBus.on(Events.AUTH_EXPIRED, () => {
      this.auth.handleAuthExpired();
    });
  }

  /**
   * Listen for real user input and forward it to the auth manager so it can
   * decide whether to renew the JWT or let it expire. Throttled so a typing
   * burst or repeated clicks don't spam the call.
   *
   * Only discrete intentional inputs (mousedown, keydown, touchstart) count as
   * activity — passive mouse movement does not. Renewal API requests don't
   * dispatch any of these events so renewal will not self-trigger as activity.
   */
  setupActivityTracking() {
    const ACTIVITY_THROTTLE_MS = 1000;
    let lastDispatch = 0;

    const onActivity = () => {
      const now = Date.now();
      if (now - lastDispatch < ACTIVITY_THROTTLE_MS) return;
      lastDispatch = now;
      if (this.auth && typeof this.auth.recordUserActivity === 'function') {
        this.auth.recordUserActivity();
      }
    };

    // Capture phase so we see the events even if a handler stops propagation.
    // passive: true keeps us out of the way of scroll/touch performance paths.
    const opts = { capture: true, passive: true };
    document.addEventListener('mousedown', onActivity, opts);
    document.addEventListener('keydown', onActivity, opts);
    document.addEventListener('touchstart', onActivity, opts);
  }

  setupNetworkMonitoring() {
    const updateStatus = () => {
      const isOnline = navigator.onLine;
      if (isOnline) {
        eventBus.emit(Events.NETWORK_ONLINE);
        logStartup('Network: Online');
      } else {
        eventBus.emit(Events.NETWORK_OFFLINE);
        logStartup('Network: Offline');
      }
    };
    window.addEventListener('online', updateStatus);
    window.addEventListener('offline', updateStatus);
    updateStatus();
  }

  showFatalError(message) {
    const app = document.getElementById('app');
    if (app) {
      app.innerHTML = `
        <div class="fatal-error">
          <h1><fa fa-exclamation-triangle></fa> Error</h1>
          <p>${message}</p>
          <button onclick="window.location.reload(true)" class="btn btn-primary">
            <fa fa-redo></fa> Reload Page
          </button>
        </div>
      `;
    }
    document.documentElement.style.visibility = 'visible';
  }

  getState() {
    return {
      version: this.version,
      build: this.build,
      buildTimestamp: this.timestamp,
      config: this.config,
      user: this.user,
      currentManager: this.currentManager?.name || null,
      online: navigator.onLine,
      now: new Date().toISOString(),
    };
  }

  /**
   * Load a manager by ID.
   * Delegates to ManagerLoader.
   * @param {number} managerId - Manager ID from managerRegistry
   */
  async loadManager(managerId) {
    return this.managerLoader.loadManager(managerId);
  }

  /**
   * Load a utility manager by key (user-profile, session-log, terminal)
   * Delegates to ManagerLoader
   */
  async loadUtilityManager(key) {
    return this.managerLoader.loadUtilityManager(key);
  }

  /**
   * Close a manager by ID.
   * Delegates to ManagerLoader.
   * @param {number|string} managerId - Manager ID or utility key
   */
  async closeManager(managerId) {
    return this.managerLoader.closeManager(managerId);
  }

  /**
   * Handle expired authentication.
   * Delegates to AuthManager.
   */
  async handleAuthExpired() {
    return this.auth.handleAuthExpired();
  }

  /**
   * Perform logout for expired session.
   * Delegates to AuthManager.
   */
  async performExpiredSessionLogout() {
    // Clear JWT immediately
    clearJWT();
    return this.auth.performExpiredSessionLogout();
  }
}

// Console capture MUST be first - captures all subsequent console output

let app = null;

document.addEventListener('DOMContentLoaded', async () => {
  app = new LithiumApp();
  window.lithiumApp = app;

  window.lithiumLogs = {
    get raw() { return getRawLog(); },
    get display() { return getDisplayLog(); },
    recent(n = 50) { return getRecentLogs(n); },
    print(n = 50) { getRecentLogs(n).forEach(line => console.log(line)); },
    get count() { return getCounter(); },
    get sessionId() { return getSessionId(); },
    setConsoleLogging,
    flush,
    get archived() { return getArchivedSessions(); },
    removeArchived: removeArchivedSession,
  };

  window.lithiumTip = { tip, untip, initTooltips };

  await app.init();
});

export { getTabulatorSchemas as A, fetchLookups as B, tip as C, refreshTabulatorSchemas as D, Events as E, jwt as F, toast$1 as G, Status as S, Tooltip as T, __vitePreload as _, Subsystems as a, getClaims as b, getRawLog as c, getTip as d, eventBus as e, getConfigValue as f, getMacros as g, hasFeature as h, initTooltips as i, hasLookup as j, canAccessManager as k, log as l, _registerShortcutKey as m, _unregisterShortcutKey as n, offset as o, processIcons as p, flip as q, retrieveJWT as r, storeJWT as s, toast as t, shift as u, validateJWT as v, isConfigLoaded as w, loadConfig as x, normalizeIconHtml as y, createRequest as z };
