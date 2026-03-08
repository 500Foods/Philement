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

import { log, Status } from './log.js';

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
export const eventBus = new EventBus();

// Standard event names for consistency
export const Events = {
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

  // Lookup-specific events (for monitoring specific lookup categories)
  // Pattern: lookups:{category}:loaded - e.g., lookups:themes:loaded
  LOOKUPS_THEMES_LOADED: 'lookups:themes:loaded',
  LOOKUPS_ICONS_LOADED: 'lookups:icons:loaded',
  LOOKUPS_SYSTEM_INFO_LOADED: 'lookups:system_info:loaded',
  LOOKUPS_MANAGERS_LOADED: 'lookups:managers:loaded',
  LOOKUPS_LOOKUP_NAMES_LOADED: 'lookups:lookup_names:loaded',
};
