/**
 * Event Bus - Singleton EventTarget for cross-module communication
 * 
 * Provides a lightweight publish/subscribe mechanism for decoupled
 * communication between managers and core modules.
 */

class EventBus extends EventTarget {
  /**
   * Emit an event with optional detail payload
   * @param {string} name - Event name
   * @param {*} detail - Optional data payload
   */
  emit(name, detail = null) {
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
