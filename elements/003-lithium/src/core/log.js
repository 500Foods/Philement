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
export const Subsystems = {
  STARTUP: 'Startup',
  JWT: 'JWT',
  EVENTBUS: 'EventBus',
  HTTP: 'HTTP',
  RESTAPI: 'RESTAPI',
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
};

// Status constants
export const Status = {
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
export function setConsoleLogging(enabled) {
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
    const random1 = Math.random().toString(36).substring(2, 10);
    const random2 = Math.random().toString(36).substring(2, 10);
    _sessionId = `${timestamp}-${random1}-${random2}`;
  }
  return _sessionId;
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
  } catch (e) {
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
    } catch (e) {
      // Ignore
    }
  } catch (e) {
    // Ignore storage errors
  }
}

/**
 * Save buffer to sessionStorage (for cross-chunk persistence)
 */
function _saveToStorage() {
  try {
    sessionStorage.setItem(LOG_STORAGE_KEY, JSON.stringify(_buffer));
  } catch (e) {
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
export function log(subsystem, status, description, duration = 0) {
  // Create JSON entry with raw values
  const entry = _createEntry(subsystem, status, duration, description);
  
  // Output to console only if enabled
  if (_consoleLogging) {
    console.log(`[LOG] ${_formatEntry(entry)}`);
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
export function logStartup(description, duration = 0) {
  return log(Subsystems.STARTUP, Status.INFO, description, duration);
}

// Shortcut for JWT/auth logs
export function logAuth(status, description, duration = 0) {
  return log(Subsystems.AUTH, status, description, duration);
}

// Shortcut for HTTP logs
export function logHttp(description, duration = 0) {
  return log(Subsystems.HTTP, Status.INFO, description, duration);
}

/**
 * Get next HTTP request number (for Request 001, Request 002, etc.)
 * @returns {string} Zero-padded request number (e.g., "001")
 */
export function getNextHttpRequestNum() {
  return String(++_httpRequestCounter).padStart(3, '0');
}

/**
 * Log an HTTP request (RESTAPI subsystem)
 * @param {string} method - HTTP method (GET, POST, PUT, DELETE, PATCH)
 * @param {string} path - API path
 * @returns {string} Request number for matching with response
 */
export function logHttpRequest(method, path) {
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
export function logHttpResponse(requestNum, method, path, code = null, size = null, duration = null) {
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
export function logManager(status, description, duration = 0) {
  return log(Subsystems.MANAGER, status, description, duration);
}

// Shortcut for errors
export function logError(subsystem, description, duration = 0) {
  return log(subsystem, Status.ERROR, description, duration);
}

// Shortcut for warnings
export function logWarn(subsystem, description, duration = 0) {
  return log(subsystem, Status.WARN, description, duration);
}

// Shortcut for success
export function logSuccess(subsystem, description, duration = 0) {
  return log(subsystem, Status.SUCCESS, description, duration);
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
export function logGroup(subsystem, status, title, items, duration = 0) {
  return log(subsystem, status, { title, items }, duration);
}

/**
 * Get current buffer (for debugging)
 * @returns {Array} Current log buffer (JSON entries with raw values)
 */
export function getBuffer() {
  return [..._buffer];
}

/**
 * Get all log entries as formatted display strings
 * @returns {Array} Array of formatted log lines
 */
export function getDisplayLog() {
  return _buffer.map(entry => _formatEntry(entry));
}

/**
 * Get recent logs formatted for UI display
 * Format: local time (HH:MM:SS.ZZZ) + subsystem + description
 * Grouped entries ({ title, items }) are expanded to multiple lines with ― prefix.
 * @param {number} maxEntries - Maximum number of entries to return (default 50)
 * @returns {Array} Array of formatted log strings (may include multi-line entries)
 */
export function getRecentLogs(maxEntries = 50) {
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
export function getRawLog() {
  return [..._buffer];
}

/**
 * Get current session ID
 * @returns {string|null} Current session ID
 */
export function getSessionId() {
  return _sessionId;
}

/**
 * Get current counter value
 * @returns {number} Current counter (integer)
 */
export function getCounter() {
  return _counter;
}

/**
 * Force flush buffer to server (for cleanup on logout, etc.)
 */
export async function flush() {
  if (_flushTimer) {
    clearTimeout(_flushTimer);
    _flushTimer = null;
  }
  await _flushToServer();
}

/**
 * Get all archived session log entries from localStorage.
 * Keys are of the form: lithium_log_archive_<shortId>_<timestamp>
 * Returns an array of { key, sessionId, entries } objects sorted by key (oldest first).
 * @returns {Array} Archived session records
 */
export function getArchivedSessions() {
  const results = [];
  try {
    for (let i = 0; i < localStorage.length; i++) {
      const key = localStorage.key(i);
      if (key && key.startsWith(LOG_ARCHIVE_PREFIX)) {
        try {
          const value = localStorage.getItem(key);
          const parsed = JSON.parse(value);
          results.push({ key, ...parsed });
        } catch (e) {
          // Skip malformed entries
        }
      }
    }
  } catch (e) {
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
export function removeArchivedSession(key) {
  try {
    localStorage.removeItem(key);
  } catch (e) {
    // Ignore storage errors
  }
}

// Default export for convenience
export default {
  log,
  logStartup,
  logAuth,
  logHttp,
  logHttpRequest,
  logHttpResponse,
  getNextHttpRequestNum,
  logManager,
  logError,
  logWarn,
  logSuccess,
  logGroup,
  Subsystems,
  Status,
  getBuffer,
  getDisplayLog,
  getRecentLogs,
  getRawLog,
  getSessionId,
  getCounter,
  flush,
  setConsoleLogging,
  getArchivedSessions,
  removeArchivedSession,
};
