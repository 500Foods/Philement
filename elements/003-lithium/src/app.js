/**
 * Lithium PWA - Main Application Bootstrap
 * 
 * Bootstrap sequence:
 * 1. Load config
 * 2. Initialize core modules
 * 3. Check for existing JWT
 * 4. Load login or main manager based on auth state
 * 5. Set up event listeners
 */

// Suppress vendor CSS parsing warnings that flood the console
// These come from Tabulator, CodeMirror, etc. dynamically injecting CSS
(function() {
  const originalConsoleError = console.error;
  const cssWarningPattern = /Error in parsing value for '(text-align|min-height)'\. Declaration dropped\./;

  console.error = function(...args) {
    // Filter out the vendor CSS parsing warnings
    if (args.length > 0 && typeof args[0] === 'string' && cssWarningPattern.test(args[0])) {
      return; // Silently drop these warnings
    }
    originalConsoleError.apply(console, args);
  };
})();

// Monkey-patch CSSStyleDeclaration to intercept invalid values at the source
// This prevents the browser's CSS parser from ever seeing invalid values
(function() {
  const INVALID_VALUES = new Set(['', 'null', 'undefined', 'NaN', 'inherit', 'initial']);

  // Known problematic property patterns
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
      return; // Silently drop invalid values
    }
    return originalSetProperty.call(this, property, value, priority);
  };

  // Patch cssText setter
  const cssTextDescriptor = Object.getOwnPropertyDescriptor(CSSStyleDeclaration.prototype, 'cssText');
  if (cssTextDescriptor && cssTextDescriptor.set) {
    const originalCssTextSetter = cssTextDescriptor.set;
    Object.defineProperty(CSSStyleDeclaration.prototype, 'cssText', {
      set: function(value) {
        // Filter out problematic declarations from cssText
        if (typeof value === 'string') {
          let filtered = value;
          PROBLEMATIC_PROPS.forEach(prop => {
            // Remove declarations like "text-align: ;" or "text-align: null;"
            const regex = new RegExp(`${prop}\\s*:\\s*[^;]*(?:;|$)`, 'gi');
            filtered = filtered.replace(regex, (match) => {
              const val = match.split(':')[1]?.replace(';', '').trim();
              if (isInvalidValue(val)) return '';
              return match;
            });
          });
          return originalCssTextSetter.call(this, filtered);
        }
        return originalCssTextSetter.call(this, value);
      },
      get: cssTextDescriptor.get,
      configurable: true,
    });
  }

  // Patch individual property setters for camelCase properties (element.style.textAlign)
  const CAMEL_PROPS = {
    'textAlign': 'text-align',
    'minHeight': 'min-height',
  };

  Object.entries(CAMEL_PROPS).forEach(([camel, kebab]) => {
    const descriptor = Object.getOwnPropertyDescriptor(CSSStyleDeclaration.prototype, camel);
    if (descriptor && descriptor.set) {
      const originalSetter = descriptor.set;
      Object.defineProperty(CSSStyleDeclaration.prototype, camel, {
        set: function(value) {
          if (isInvalidValue(value)) {
            return; // Silently drop invalid values
          }
          return originalSetter.call(this, value);
        },
        get: descriptor.get,
        configurable: true,
      });
    }
  });

  // Patch insertRule on CSSStyleSheet to filter invalid rules
  const originalInsertRule = CSSStyleSheet.prototype.insertRule;
  CSSStyleSheet.prototype.insertRule = function(rule, index) {
    // Check for problematic declarations in the rule text
    let filtered = rule;
    PROBLEMATIC_PROPS.forEach(prop => {
      const regex = new RegExp(`${prop}\\s*:\\s*[^;{}]*`, 'gi');
      filtered = filtered.replace(regex, (match) => {
        const val = match.split(':')[1]?.trim();
        if (isInvalidValue(val)) return `${prop}: initial`; // Replace with valid default
        return match;
      });
    });
    return originalInsertRule.call(this, filtered, index);
  };

  // Patch appendRule (used by some older libraries)
  if (CSSStyleSheet.prototype.appendRule) {
    const originalAppendRule = CSSStyleSheet.prototype.appendRule;
    CSSStyleSheet.prototype.appendRule = function(rule) {
      let filtered = rule;
      PROBLEMATIC_PROPS.forEach(prop => {
        const regex = new RegExp(`${prop}\\s*:\\s*[^;{}]*`, 'gi');
        filtered = filtered.replace(regex, (match) => {
          const val = match.split(':')[1]?.trim();
          if (isInvalidValue(val)) return `${prop}: initial`;
          return match;
        });
      });
      return originalAppendRule.call(this, filtered);
    };
  }

  // Patch innerHTML setter on HTMLStyleElement to filter CSS text
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
              if (isInvalidValue(val)) return `${prop}: initial`;
              return match;
            });
          });
          return originalInnerHTMLSetter.call(this, filtered);
        }
        return originalInnerHTMLSetter.call(this, value);
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
              if (isInvalidValue(val)) return `${prop}: initial`;
              return match;
            });
          });
          return originalTextContentSetter.call(this, filtered);
        }
        return originalTextContentSetter.call(this, value);
      },
      get: function() {
        return textContentDescriptor.get.call(this);
      },
      configurable: true,
    });
  }
})();

// Core styles - imported first to establish CSS cascade layers
import './styles/base.css';
import './styles/vendor-fixes.css';
import './styles/layout.css';
import './styles/components.css';
import './styles/transitions.css';
import './styles/toast.css';
import './styles/tooltip.css';

import { loadConfig, getConfig, getConfigValue } from './core/config.js';
import { eventBus, Events } from './core/event-bus.js';
import { validateJWT, retrieveJWT, getClaims, getRenewalTime, getTimeUntilExpiry, storeJWT, clearJWT } from './core/jwt.js';
import { getPermittedManagers } from './core/permissions.js';
import { createRequest } from './core/json-request.js';
import { fetchLookups, init as initLookups, loadMacrosPostLogin } from './shared/lookups.js';
import { init as initIcons } from './core/icons.js';
import { getTransitionDuration } from './core/transitions.js';
import { initTooltips, tip, untip } from './core/tooltip-api.js';
import { toast } from './shared/toast.js';
import { log, logGroup, logStartup, logAuth, logManager, Subsystems, Status, flush, getSessionId, getRecentLogs, getRawLog, getDisplayLog, getCounter, setConsoleLogging, getArchivedSessions, removeArchivedSession } from './core/log.js';

// Manager crossfade duration will be read from CSS --transition-duration
let MANAGER_CROSSFADE_MS = 2500; // Default fallback, updated at runtime

/**
 * Main Lithium Application Class
 */
class LithiumApp {
  constructor() {
    this.version = null;
    this.build = null;
    this.config = null;
    this.api = null;
    this.currentManager = null;
    this.loadedManagers = new Map();
    this.mainManagerInstance = null;  // Reference to the MainManager for header updates
    this.user = null;
    this.deferredInstallPrompt = null;

    // Manager definitions — id -> name only (module loaded via _importManager)
    // Manager IDs correspond to lithium.json managers section (007-032)
    // Group 0: System (001-006) - hidden from main menu (Login, Menu, Profile, Session, Crimson, Tour)
    // Groups 1-3: Visible menu managers (007-032)
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

    // Utility manager definitions — key -> definition (matches lithium.json 001-006 range)
    this.utilityManagerRegistry = {
      'user-profile': { id: 3, key: 'user-profile', name: 'User Profile' },
      'session-log': { id: 4, key: 'session-log', name: 'Session Log' },
    };

    // Transition overlay element (created lazily)
    this.transitionOverlay = null;

    // JWT expiration warning
    this._expirationWarningTimer = null;
    this._expirationCountdownInterval = null;
    this._expirationWarningToastId = null;
    this._expirationWarningActive = false;

    // JWT renewal tracking
    this._lastUserActivity = 0;
    this._tokenScheduledAt = 0;
    this._isRenewing = false;
  }

  /**
   * Get the CSS transition duration from the root element.
   * Updates MANAGER_CROSSFADE_MS global for use by _crossfadeSlots.
   * @returns {number} Duration in milliseconds
   */
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

  /**
   * Get or create the transition overlay element.
   * The overlay blocks mouse clicks during manager crossfades.
   * Appended to document.body to cover the entire viewport including sidebar.
   * @returns {HTMLElement} The overlay element
   */
  getTransitionOverlay() {
    if (this.transitionOverlay) {
      return this.transitionOverlay;
    }

    this.transitionOverlay = document.createElement('div');
    this.transitionOverlay.className = 'manager-transition-overlay';
    document.body.appendChild(this.transitionOverlay);

    return this.transitionOverlay;
  }

  /**
   * Show the transition overlay to block clicks during crossfade.
   */
  showTransitionOverlay() {
    const overlay = this.getTransitionOverlay();
    if (overlay) {
      overlay.classList.add('active');
    }
  }

  /**
   * Hide the transition overlay after crossfade completes.
   */
  hideTransitionOverlay() {
    const overlay = this.getTransitionOverlay();
    if (overlay) {
      overlay.classList.remove('active');
    }
  }

  /**
   * Initialize the application
   */
  async init() {
    // Track total initialization time
    const initStart = Date.now();

    try {
      // Step 0: Load version info first so we can include it in the startup log
      await this.loadVersion();

      // Format timestamp for display
      let releaseDate = '';
      if (this.timestamp) {
        const date = new Date(this.timestamp);
        const months = ['Jan', 'Feb', 'Mar', 'Apr', 'May', 'Jun', 'Jul', 'Aug', 'Sep', 'Oct', 'Nov', 'Dec'];
        const hours = String(date.getUTCHours()).padStart(2, '0');
        const minutes = String(date.getUTCMinutes()).padStart(2, '0');
        releaseDate = `${date.getUTCFullYear()}-${months[date.getUTCMonth()]}-${String(date.getUTCDate()).padStart(2, '0')} ${hours}:${minutes} UTC`;
      }

      // First log entry: headline with version info
      logGroup(Subsystems.STARTUP, Status.INFO, 'Lithium Application Started', [
        `Version: ${this.version || 'unknown'}`,
        `Release: ${releaseDate || 'unknown'}`,
      ]);

      // Log environment info immediately
      this._logEnvironment();

      logStartup('Application initializing');

      // Step 1: Load configuration
      this.config = await loadConfig();
      const serverUrl = getConfigValue('server.url', 'unknown');
      const apiPrefix = getConfigValue('server.api_prefix', '/api');
      const iconSet = getConfigValue('icons.set', 'duotone');
      const iconWeight = getConfigValue('icons.weight', 'thin');
      const iconPrefix = getConfigValue('icons.prefix', 'fad');
      const iconFallback = getConfigValue('icons.fallback', 'solid');
      // Get config file size from response headers if available
      let configFileSize = '';
      try {
        const configResponse = await fetch('/config/lithium.json', { method: 'HEAD' });
        if (configResponse.ok) {
          const contentLength = configResponse.headers.get('content-length');
          if (contentLength) {
            configFileSize = ` (${parseInt(contentLength).toLocaleString()} bytes)`;
          }
        }
      } catch (e) {
        // Ignore errors, size is optional
      }
      logGroup(Subsystems.STARTUP, Status.INFO, 'Application Configuration', [
        `ConfigFile: /config/lithium.json${configFileSize}`,
        `Server: ${serverUrl}${apiPrefix}`,
        `Icons: ${iconPrefix}-${iconWeight} ${iconSet} (fallback: ${iconFallback})`,
      ]);

      // Step 2: Log available managers
      this._logManagers();

      // Step 3: Create API client with config
      const apiStart = Date.now();
      this.api = createRequest(this.config);
      logStartup('API client created', Date.now() - apiStart);

      // Step 4: Initialize icon system (requires config for icon set selection)
      const iconsStart = Date.now();
      initIcons();
      logStartup('Icon system initialized', Date.now() - iconsStart);

      // Step 5: Reveal the page (FOUC prevention) and load login/main UI immediately
      this.revealPage();
      logStartup('Page revealed');

      // Step 5b: Initialize tooltip system (requires revealed DOM)
      initTooltips();
      logStartup('Tooltip system initialized');

      // Step 6: Check authentication and load appropriate manager
      // The login panel starts with its logs button disabled; it will be enabled
      // once background startup tasks complete and STARTUP_COMPLETE is emitted.
      const authStart = Date.now();
      await this.checkAuthAndLoad();
      logStartup('Auth check complete', Date.now() - authStart);

      // Step 6b: Fade out and remove the loading splash now that the UI is ready
      this.dismissSplash();

      // Step 7: Set up global event listeners
      this.setupEventListeners();
      logStartup('Event listeners registered');

      // Step 8: Run remaining startup tasks in the background so the UI is
      // already visible and interactive while these complete.
      this._runBackgroundStartup(initStart);
    } catch (error) {
      logStartup(`Initialization failed: ${error.message}`);
      console.error('[Lithium] Failed to initialize:', error);
      this.showFatalError('Failed to initialize application. Please refresh the page.');
    }
  }

  /**
   * Run background startup tasks after the UI is already visible.
   * Emits STARTUP_COMPLETE when finished so dependent UI (e.g. logs button) can enable.
   * @param {number} initStart - Timestamp when init() began, for total-time reporting
   */
  async _runBackgroundStartup(initStart) {
    try {
      // Step 9: Check server health
      try {
        const healthResponse = await this.api.get('system/health');
        const healthStatus = healthResponse.status || healthResponse.message || 'unknown';
        logGroup(Subsystems.STARTUP, Status.INFO, 'Server Health Check', [
          `Status: ${healthStatus}`,
        ]);
      } catch (error) {
        logGroup(Subsystems.STARTUP, Status.WARN, 'Server Health Check', [
          `Status: Failed`,
          `Error: ${error.message}`,
        ]);
      }

      // Step 10: Initialize lookups (fetch open lookups from cache or server)
      const lookupsStart = Date.now();
      initLookups();
      this.fetchLookups();
      logStartup('Lookups initialized', Date.now() - lookupsStart);

      // Step 11: Set up network status monitoring
      this.setupNetworkMonitoring();
      logStartup('Network monitoring active');

      const totalInitTime = Date.now() - initStart;
      logStartup(`Application initialized successfully in ${(totalInitTime / 1000).toFixed(2)}s`);
    } catch (error) {
      logStartup(`Background startup error: ${error.message}`);
    } finally {
      // Always emit STARTUP_COMPLETE so the logs button is enabled even on error
      eventBus.emit(Events.STARTUP_COMPLETE);
    }
    logStartup('────────────────────');
  }

  /**
   * Log environment information as a single grouped entry
   */
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

  /**
   * Detect browser from user agent
   */
  _detectBrowser(ua) {
    let name = 'Unknown';
    let version = 'Unknown';

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

  /**
   * Log available managers as a single grouped entry.
   * Each manager is displayed as a 4-digit zero-padded ID followed by its name.
   * The first two digits are reserved for client namespacing (00 = core).
   */
  _logManagers() {
    const managers = Object.values(this.managerRegistry);
    logGroup(Subsystems.STARTUP, Status.INFO, `Managers available: ${managers.length}`,
      managers.map(m => `${String(m.id).padStart(4, '0')}: ${m.name}`));
  }

  /**
   * Load version info from version.json
   * Uses cached data from early fetch in index.html if available
   */
  async loadVersion() {
    try {
      // Check if we have cached version data from the early fetch in index.html
      if (window.__lithiumVersionData) {
        const data = window.__lithiumVersionData;
        this.version = data.version;
        this.build = data.build;
        this.timestamp = data.timestamp;
        return;
      }
      
      // Check if there's a pending promise from the early fetch
      if (window.__lithiumVersionPromise) {
        const data = await window.__lithiumVersionPromise;
        if (data) {
          this.version = data.version;
          this.build = data.build;
          this.timestamp = data.timestamp;
          return;
        }
      }
      
      // Fallback: fetch version.json directly
      const response = await fetch('/version.json');
      if (response.ok) {
        const data = await response.json();
        this.version = data.version;
        this.build = data.build;
        this.timestamp = data.timestamp;
      }
    } catch (error) {
      // Non-fatal: version info is optional
    }
  }

  /**
   * Reveal the page after styles are loaded (FOUC prevention)
   */
  revealPage() {
    // Small delay to ensure CSS is parsed
    requestAnimationFrame(() => {
      document.documentElement.style.visibility = 'visible';
    });
  }

  /**
   * Fade out and remove the loading splash screen.
   *
   * The #Loading element fades in on page load via a 1000ms CSS animation.
   * This method mirrors that: it captures the current opacity (in case the
   * fade-in hasn't finished yet), then transitions to opacity 0 over 1000ms
   * and removes the element from the DOM when the transition completes.
   */
  dismissSplash() {
    const el = document.getElementById('Loading');
    if (!el) return;

    // Capture current opacity (fadeIn may still be running)
    const currentOpacity = getComputedStyle(el).opacity;

    // Stop any running animation and lock the current opacity
    el.style.animation = 'none';
    el.style.opacity = currentOpacity;

    // Force reflow so the browser registers the static opacity
    void el.offsetHeight;

    // Fade out over 1000ms (mirrors the 1000ms fadeIn)
    el.style.transition = 'opacity 1000ms ease-out';
    el.style.opacity = '0';

    // Remove from DOM entirely after the transition completes
    el.addEventListener('transitionend', () => el.remove(), { once: true });
  }

  /**
   * Check authentication status and load appropriate manager
   */
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

      // Set up token renewal
      this.scheduleTokenRenewal(token);

      // Load macros and other post-login lookups (async, non-blocking)
      this.loadPostLoginLookups();

      // Load main menu manager
      await this.loadMainManager();
    } else {
      logAuth(Status.INFO, 'No valid JWT, loading login');
      await this.loadLoginManager();
    }
  }

  /**
   * Load lookups that require authentication (post-login)
   * Called when JWT is available - loads from cache first, then refreshes from server
   */
  async loadPostLoginLookups() {
    logAuth(Status.INFO, '[App] Loading post-login lookups...');
    try {
      // Load macros lookup (QueryRef 046) - requires auth
      const result = await loadMacrosPostLogin(this.api);
      logAuth(Status.INFO, `[App] Post-login lookups loaded: ${result}`);
    } catch (error) {
      logAuth(Status.WARN, `Failed to load post-login lookups: ${error.message}`);
    }
  }

  /**
   * Schedule automatic token renewal and expiration warning.
   *
   * At T-90s before expiry, checks whether the user has been active since
   * the token was issued/renewed.  If active → renews silently.  If idle →
   * shows the 90-second countdown toast (and eventually logs out).
   *
   * Managers also trigger immediate renewal on open via _renewOnActivity(),
   * which serves as a test mechanism for now. The 80% scheduled renewal
   * has been replaced by this activity-based approach.
   */
  scheduleTokenRenewal(token) {
    // Clear any existing timers before scheduling new ones
    this.clearExpirationTimers();

    const timeUntilExpiry = getTimeUntilExpiry(token);
    this._tokenScheduledAt = Date.now();

    // Schedule activity check / expiration warning (90 seconds before expiry)
    const WARNING_LEAD_TIME = 90000; // 90 seconds
    if (timeUntilExpiry > WARNING_LEAD_TIME) {
      const checkTime = timeUntilExpiry - WARNING_LEAD_TIME;
      logAuth(Status.INFO, `Token activity check scheduled in ${Math.round(checkTime / 1000)}s (expiry in ${Math.round(timeUntilExpiry / 1000)}s)`);

      this._expirationWarningTimer = setTimeout(() => {
        // At T-90s: check if user has been active since last renewal
        if (this._lastUserActivity > this._tokenScheduledAt) {
          // User was active — renew silently
          logAuth(Status.INFO, 'User activity detected at T-90s, attempting auto-renewal');
          this.renewToken().catch((error) => {
            // Renewal failed — fall back to showing warning
            logAuth(Status.WARN, `Auto-renewal failed: ${error.message}, showing expiration warning`);
            this.showExpirationWarning();
          });
        } else {
          // No activity — show expiration warning
          this.showExpirationWarning();
        }
      }, checkTime);
    } else if (timeUntilExpiry > 0) {
      // Less than 90s remaining, show warning immediately
      this.showExpirationWarning(Math.floor(timeUntilExpiry / 1000));
    }
  }

  /**
   * Show JWT expiration warning toast with countdown
   * @param {number} initialSeconds - Initial countdown seconds (defaults to 90)
   */
  showExpirationWarning(initialSeconds = 90) {
    if (this._expirationWarningActive) return;
    this._expirationWarningActive = true;

    let secondsRemaining = initialSeconds;

    // Create warning message with countdown
    const getMessage = () => `Session expires in ${secondsRemaining}s`;

    // Show initial warning toast
    this._expirationWarningToastId = toast.warning(getMessage(), {
      title: 'Session Expiring',
      description: 'Your session will expire soon. Please save your work.',
      duration: 0, // Persistent until dismissed or expired
      dismissible: true,
      subsystem: 'Auth',
    });

    logAuth(Status.WARN, `Token expires in ${secondsRemaining}s`);

    // Update countdown every second
    this._expirationCountdownInterval = setInterval(() => {
      secondsRemaining--;

      // Update toast message if still showing
      if (this._expirationWarningToastId && secondsRemaining > 0) {
        const toastEl = document.querySelector(`[data-id="${this._expirationWarningToastId}"]`);
        if (toastEl) {
          const titleBtn = toastEl.querySelector('.toast-header-title');
          if (titleBtn) {
            titleBtn.textContent = getMessage();
          }
        }
      }

      // Log at 60s, 30s, and 10s remaining
      if ([60, 30, 10].includes(secondsRemaining)) {
        logAuth(Status.WARN, `Token expires in ${secondsRemaining}s`);
      }

      // When countdown reaches 0, perform quick logout
      if (secondsRemaining <= 0) {
        this.clearExpirationTimers();
        this.performQuickLogout();
      }
    }, 1000);
  }

  /**
   * Clear all expiration warning timers
   */
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

  /**
   * Record user activity for JWT renewal decisions.
   * Currently triggered when a manager is opened (menu or utility).
   */
  recordUserActivity() {
    this._lastUserActivity = Date.now();
  }

  /**
   * Save the last-used manager to localStorage for session restoration.
   * @param {string} type - 'manager' or 'utility'
   * @param {string|number} id - Manager ID or utility key
   */
  _saveLastManager(type, id) {
    try {
      localStorage.setItem('lithium_last_manager', `${type}:${id}`);
    } catch (e) {
      // Non-fatal — localStorage may be unavailable
    }
  }

  /**
   * Attempt JWT renewal triggered by user activity (e.g., opening a manager).
   * Skips renewal if the token was recently obtained (within 10s) or if
   * a renewal is already in progress.
   */
  async _renewOnActivity() {
    if (this._isRenewing) return;

    // Don't renew tokens that were just obtained
    const tokenAge = Date.now() - this._tokenScheduledAt;
    if (tokenAge < 10000) return;

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

  /**
   * Perform quick logout when session expires
   */
  async performQuickLogout() {
    logAuth(Status.WARN, 'Session expired - performing quick logout');

    // Dismiss the warning toast
    if (this._expirationWarningToastId) {
      toast.dismiss(this._expirationWarningToastId);
      this._expirationWarningToastId = null;
    }

    // Show expired toast
    toast.error('Session Expired', {
      title: 'Session Expired',
      description: 'Your session has expired. You have been logged out.',
      duration: 5000,
      subsystem: 'Auth',
    });

    // Perform quick logout
    await this.performLogoutActions('quick');

    // Reload page to return to login
    window.location.reload();
  }

  /**
   * Renew the JWT token.
   *
   * Includes a concurrency guard (_isRenewing) to prevent overlapping
   * renewal calls.  Only shows a success toast if the expiration warning
   * countdown was active — silent renewals triggered by manager opens
   * do not produce user-visible toasts (only log lines).
   */
  async renewToken() {
    if (this._isRenewing) return;
    this._isRenewing = true;
    const wasWarningActive = this._expirationWarningActive;

    try {
      logAuth(Status.INFO, 'Starting token renewal');
      const start = Date.now();
      const response = await this.api.post('auth/renew', {});

      if (response.token) {
        // Clear any existing expiration warning
        this.clearExpirationTimers();

        // Dismiss warning toast if showing
        if (this._expirationWarningToastId) {
          toast.dismiss(this._expirationWarningToastId);
          this._expirationWarningToastId = null;
        }

        storeJWT(response.token);
        this.scheduleTokenRenewal(response.token);
        eventBus.emit(Events.AUTH_RENEWED, { expiresAt: response.expires_at });
        logAuth(Status.SUCCESS, 'Token renewed successfully', Date.now() - start);

        // Only show success toast if the expiration warning was active
        // (avoids noisy toasts on every manager switch)
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

  /**
   * Load the login manager
   */
  async loadLoginManager() {
    try {
      const { default: LoginManager } = await import('./managers/login/login.js');
      const container = document.getElementById('app');

      // Clear any existing content
      container.innerHTML = '';

      const loginManager = new LoginManager(this, container);
      await loginManager.init();

      this.currentManager = { name: 'login', instance: loginManager };
    } catch (error) {
      console.error('[Lithium] Failed to load login manager:', error);
      this.showFatalError('Failed to load login page. Please check your connection and refresh.');
    }
  }

  /**
   * Load the main menu manager
   */
  async loadMainManager() {
    try {
      const { default: MainManager } = await import('./managers/main/main.js');
      const container = document.getElementById('app');

      // Clear any existing content
      container.innerHTML = '';

      // Get permitted managers
      const permittedManagers = getPermittedManagers();

      const mainManager = new MainManager(this, container, permittedManagers);
      // Set reference BEFORE init() so that manager loading within init() can use it
      this.mainManagerInstance = mainManager;
      await mainManager.init();

      // NOTE: Do NOT unconditionally overwrite this.currentManager here.
      // mainManager.init() loads the first permitted manager, which calls
      // app.loadManager() → showManager() → sets this.currentManager = { id: N }.
      // Overwriting it would erase that slot ID and break subsequent crossfades —
      // the outgoing slot would not be found, leaving ghost slot-visible elements on screen.
      // Only provide a fallback when no manager slot was loaded (e.g. zero permitted managers).
      if (this.currentManager?.id == null) {
        this.currentManager = { name: 'main', instance: mainManager };
      }
    } catch (error) {
      console.error('[Lithium] Failed to load main manager:', error);
      this.showFatalError('Failed to load application. Please refresh the page.');
    }
  }

  /**
   * Static-import helper for menu managers.
   * Uses literal string literals inside import() so Vite/Rollup can analyse the
   * module graph, create proper code-split chunks, and rewrite paths at build time.
   * A /* @vite-ignore *\/ dynamic import with a runtime variable would bypass this.
   * @param {number} managerId
   * @returns {Promise<module>}
   */
  async _importManager(managerId) {
    switch (managerId) {
      // Content
      case 7: return import('./managers/dashboard/dashboard.js');
      case 8: return import('./managers/mail-manager/mail-manager.js');
      
      // User Management
      case 9: return import('./managers/user-profiles/user-profiles.js');
      case 10: return import('./managers/session-manager/session-manager.js');
      case 11: return import('./managers/version-history/version-history.js');
      
      // Shared
      case 12: return import('./managers/calendar-manager/calendar-manager.js');
      case 13: return import('./managers/contact-manager/contact-manager.js');
      case 14: return import('./managers/file-manager/file-manager.js');
      case 15: return import('./managers/document-library/document-library.js');
      case 16: return import('./managers/media-library/media-library.js');
      case 17: return import('./managers/diagram-library/diagram-library.js');
      
      // AI
      case 18: return import('./managers/chats/chats.js');
      
      // Support
      case 19: return import('./managers/notification-manager/notification-manager.js');
      case 20: return import('./managers/annotation-manager/annotation-manager.js');
      case 21: return import('./managers/ticketing-manager/ticketing-manager.js');
      
      // Configuration
      case 22: return import('./managers/style-manager/style-manager.js');
      case 23: return import('./managers/lookups/lookups.js');
      case 24: return import('./managers/reports/reports.js');
      
      // Security
      case 25: return import('./managers/role-manager/role-manager.js');
      case 26: return import('./managers/security-manager/security-manager.js');
      case 27: return import('./managers/ai-auditor/ai-auditor.js');
      case 28: return import('./managers/job-manager/job-manager.js');
      case 29: return import('./managers/queries/queries.js');
      case 30: return import('./managers/sync-manager/sync-manager.js');
      case 31: return import('./managers/camera-manager/camera-manager.js');
      case 32: return import('./managers/terminal/terminal.js');
      
      // Legacy/deprecated IDs (for backward compatibility)
      case 1: return import('./managers/login/login.js');
      case 2: return import('./managers/session-logs/session-logs.js');
      case 3: return import('./managers/version-history/version-history.js');
      case 4: return import('./managers/queries/queries.js');
      case 5: return import('./managers/lookups/lookups.js');
      case 6: return import('./managers/chats/chats.js');
      
      default: throw new Error(`No import mapping for manager ID: ${managerId}`);
    }
  }

  /**
   * Static-import helper for utility managers.
   * Same rationale as _importManager — literal paths required for Vite analysis.
   * @param {string} managerKey
   * @returns {Promise<module>}
   */
  async _importUtilityManager(managerKey) {
    switch (managerKey) {
      case 'session-log': return import('./managers/session-log/session-log.js');
      case 'user-profile': return import('./managers/profile-manager/profile-manager.js');
      default: throw new Error(`No import mapping for utility manager key: ${managerKey}`);
    }
  }

  /**
   * Get the MainManager instance (if loaded) for updating UI like the manager header.
   * Returns null if main manager has not been loaded yet.
   * @returns {Object|null}
   */
  _getMainManager() {
    return this.mainManagerInstance || null;
  }

  /**
   * Load a manager into the workspace.
   * Uses the slot-based system: each manager gets a slot (header+workspace+footer).
   */
  async loadManager(managerId) {
    const managerDef = this.managerRegistry[managerId];

    if (!managerDef) {
      logManager(Status.ERROR, `Unknown manager ID: ${managerId}`);
      return;
    }

    // Check if already loaded
    if (this.loadedManagers.has(managerId)) {
      await this.showManager(managerId);
      eventBus.emit(Events.MANAGER_SWITCHED, { from: this.currentManager?.id, to: managerId });
      logManager(Status.INFO, `Switched to already-loaded manager: ${managerDef.name}`);
      return;
    }

    try {
      logManager(Status.INFO,'────────────────────');
      logManager(Status.INFO, `Loading manager: ${managerDef.name}`);
      const loadStart = Date.now();
      const module = await this._importManager(managerId);

      // Get the MainManager to create a slot
      const mainMgr = this._getMainManager();
      if (!mainMgr) {
        throw new Error('MainManager not ready');
      }

      // Look up icon for this manager
      const iconInfo = mainMgr.managerIcons?.[managerId] || { iconHtml: '<fa fa-cube></fa>', name: managerDef.name };
      const slotId = mainMgr._slotId(managerId);
      const slotResult = mainMgr.createSlot(slotId, iconInfo.iconHtml, iconInfo.name, managerId);
      if (!slotResult) {
        throw new Error('Failed to create manager slot');
      }
      const { slotEl, workspaceEl } = slotResult;

      // Initialize manager inside the slot's workspace
      const ManagerClass = module.default;
      const managerInstance = new ManagerClass(this, workspaceEl);
      await managerInstance.init();

      // Store and show with crossfade
      this.loadedManagers.set(managerId, {
        id: managerId,
        name: managerDef.name,
        instance: managerInstance,
        slotEl,
        workspaceEl,
      });

      await this.showManager(managerId);
      eventBus.emit(Events.MANAGER_LOADED, { managerId });
      eventBus.emit(Events.MANAGER_SWITCHED, { from: this.currentManager?.id, to: managerId });

      logManager(Status.SUCCESS, `Manager ${managerDef.name} loaded`, Date.now() - loadStart);
      logManager(Status.INFO, '────────────────────');
    } catch (error) {
      logManager(Status.ERROR, `Failed to load manager ${managerDef.name}: ${error.message}`);
      logManager(Status.INFO, '────────────────────');
    }
    
  }

  /**
   * Load a utility manager into the workspace.
   * Uses the slot-based system: each utility manager gets a slot.
   * @param {number|string} managerIdOrKey - The utility manager ID (e.g., 3) or key (e.g. 'user-profile')
   */
  async loadUtilityManager(managerIdOrKey) {
    // Look up by string key directly
    let managerDef = this.utilityManagerRegistry[managerIdOrKey];

    // If not found and it's a number, look up by ID
    if (!managerDef && typeof managerIdOrKey === 'number') {
      managerDef = Object.values(this.utilityManagerRegistry).find(u => u.id === managerIdOrKey);
    }

    // If still not found and it's a numeric string, look up by ID
    if (!managerDef && typeof managerIdOrKey === 'string' && /^\d+$/.test(managerIdOrKey)) {
      const id = parseInt(managerIdOrKey, 10);
      managerDef = Object.values(this.utilityManagerRegistry).find(u => u.id === id);
    }

    if (!managerDef) {
      logManager(Status.ERROR, `Unknown utility manager: ${managerIdOrKey}`);
      return;
    }

    const managerKey = managerDef.key;

    // Check if already loaded — if so, just show it with crossfade
    const loadedKey = `utility:${managerKey}`;
    if (this.loadedManagers.has(loadedKey)) {
      await this.showUtilityManager(loadedKey);
      logManager(Status.INFO, `Switched to already-loaded utility manager: ${managerDef.name}`);
      return;
    }

    try {
      logManager(Status.INFO, `Loading utility manager: ${managerDef.name}`);
      const loadStart = Date.now();
      const module = await this._importUtilityManager(managerKey);

      // Get the MainManager to create a slot
      const mainMgr = this._getMainManager();
      if (!mainMgr) {
        throw new Error('MainManager not ready');
      }

      const iconInfo = mainMgr.utilityManagerIcons?.[managerKey] || { iconHtml: '<fa fa-puzzle-piece></fa>', name: managerDef.name };
      const slotId = mainMgr._utilitySlotId(managerKey);
      const slotResult = mainMgr.createSlot(slotId, iconInfo.iconHtml, iconInfo.name, managerKey);
      if (!slotResult) {
        throw new Error('Failed to create utility manager slot');
      }
      const { slotEl, workspaceEl } = slotResult;

      // Initialize manager inside the slot's workspace
      const ManagerClass = module.default;
      const managerInstance = new ManagerClass(this, workspaceEl);
      await managerInstance.init();

      // Store and show with crossfade
      this.loadedManagers.set(loadedKey, {
        id: loadedKey,
        name: iconInfo.name,
        instance: managerInstance,
        slotEl,
        workspaceEl,
      });

      await this.showUtilityManager(loadedKey);
      logManager(Status.SUCCESS, `Utility manager ${managerDef.name} loaded`, Date.now() - loadStart);
    } catch (error) {
      logManager(Status.ERROR, `Failed to load utility manager ${managerDef.name}: ${error.message}`);
    }
  }

  /**
   * Show a previously loaded utility manager with crossfade.
   * @param {string} loadedKey - The internal loaded key for the utility manager
   */
  async showUtilityManager(loadedKey) {
    const incoming = this.loadedManagers.get(loadedKey);
    if (!incoming) return;

    await this._crossfadeSlots(loadedKey);
    this.currentManager = { id: loadedKey, name: incoming.name || null, instance: incoming.instance };

    const utilityKey = loadedKey.replace('utility:', '');

    // Save as last-used manager and record activity
    this._saveLastManager('utility', utilityKey);
    this.recordUserActivity();
    this._renewOnActivity();

    // Notify main manager to update sidebar active state
    const mainMgr = this._getMainManager();
    if (mainMgr) {
      mainMgr.setActiveUtilityButton(utilityKey);
    }
  }

  /**
   * Show a previously loaded manager with crossfade.
   * @param {number} managerId
   */
  async showManager(managerId) {
    const incoming = this.loadedManagers.get(managerId);
    if (!incoming) return;

    await this._crossfadeSlots(managerId);
    this.currentManager = { id: managerId, name: incoming.name || null, instance: incoming.instance };

    // Save as last-used manager and record activity
    this._saveLastManager('manager', managerId);
    this.recordUserActivity();
    this._renewOnActivity();

    // Notify main manager to clear utility button state
    const mainMgr = this._getMainManager();
    if (mainMgr) {
      mainMgr.clearActiveUtilityButtons();
    }
  }

  /**
   * Perform a crossfade transition between the current slot and an incoming one.
   * The outgoing slot fades out simultaneously with the incoming fading in.
   * Slots use .slot-visible / .slot-hidden CSS classes.
   * @param {number|string} incomingId - The ID/key of the manager to show
   */
  async _crossfadeSlots(incomingId) {
    const incoming = this.loadedManagers.get(incomingId);
    if (!incoming) return;

    const outgoingId = this.currentManager?.id;
    const outgoing = outgoingId ? this.loadedManagers.get(outgoingId) : null;

    const incomingSlot = incoming.slotEl;
    const outgoingSlot = outgoing?.slotEl;

    if (!incomingSlot) return;

    // Hide all other slots immediately before showing the incoming one.
    // This prevents any stale slot-visible state from a previous transition
    // leaving ghost headers visible behind the active one.
    for (const [, entry] of this.loadedManagers) {
      if (entry.slotEl && entry.slotEl !== incomingSlot && entry.slotEl !== outgoingSlot) {
        entry.slotEl.classList.remove('slot-visible');
        entry.slotEl.classList.add('slot-hidden');
        entry.slotEl.style.display = 'none';
      }
    }

    // If no outgoing, just show incoming immediately with fade-in
    if (!outgoingSlot || outgoingSlot === incomingSlot) {
      incomingSlot.classList.remove('slot-hidden');
      // Reset display if it was set to none during a previous hide
      incomingSlot.style.display = '';
      void incomingSlot.offsetHeight;  // force reflow
      incomingSlot.classList.add('slot-visible');
      return;
    }

    // Get the current transition duration from CSS
    const duration = this.getTransitionDurationFromCSS();

    // Show overlay to block clicks during transition
    this.showTransitionOverlay();

    // Both slots visible simultaneously during crossfade (absolute positioning handles overlap)
    // Step 1: Show incoming (initially opacity 0 per CSS), make it active
    incomingSlot.classList.remove('slot-hidden');
    // Reset display if it was set to none during a previous hide
    incomingSlot.style.display = '';

    // Force reflow
    void incomingSlot.offsetHeight;

    // Step 2: Fade in incoming, fade out outgoing simultaneously
    incomingSlot.classList.add('slot-visible');     // opacity → 1
    outgoingSlot.classList.remove('slot-visible');  // opacity → 0

    // Step 3: Wait for CSS transitions to complete
    await new Promise(resolve => setTimeout(resolve, duration + 50));

    // Step 4: Fully hide outgoing (visibility:hidden after opacity transition)
    outgoingSlot.classList.add('slot-hidden');
    // Also set display:none to ensure no mouse events are captured by the hidden slot
    outgoingSlot.style.display = 'none';

    // Step 5: Hide overlay
    this.hideTransitionOverlay();
  }

  /**
   * Set up global event listeners
   */
  setupEventListeners() {
    // Auth events
    eventBus.on(Events.AUTH_LOGIN, (event) => this.handleLogin(event.detail));
    eventBus.on('logout:initiate', (event) => this.handleLogoutTransition(event.detail));
    eventBus.on(Events.AUTH_LOGOUT, () => this.handleLogout());
    eventBus.on(Events.AUTH_EXPIRED, () => this.handleAuthExpired());

    // PWA install prompt
    window.addEventListener('beforeinstallprompt', (e) => {
      e.preventDefault();
      this.deferredInstallPrompt = e;
      logStartup('PWA install prompt available');
    });

    window.addEventListener('appinstalled', () => {
      logStartup('PWA was installed');
      this.deferredInstallPrompt = null;
    });
  }

  /**
   * Handle successful login
   */
  async handleLogin(detail) {
    logAuth(Status.SUCCESS, `Login successful for user ${detail.username || detail.userId}`);
    this.user = {
      id: detail.userId,
      username: detail.username,
      roles: detail.roles,
    };

    // Schedule token renewal
    const token = retrieveJWT();
    if (token) {
      this.scheduleTokenRenewal(token);
    }

    // Load macros lookup post-login (requires authentication)
    logAuth(Status.INFO, '[App] Login complete, loading post-login lookups...');
    loadMacrosPostLogin(this.api).then(result => {
      logAuth(Status.INFO, `[App] Post-login lookups loaded: ${result}`);
    }).catch(err => {
      logAuth(Status.WARN, `Failed to load macros post-login: ${err.message}`);
    });

    // Crossfade transition from login to main manager
    await this.transitionToMainManager();
  }

  /**
   * Perform crossfade transition from login to main manager
   */
  async transitionToMainManager() {
    const container = document.getElementById('app');
    const loginElement = container.firstElementChild;
    
    if (!loginElement) {
      // Fallback: just load main manager normally
      await this.loadMainManager();
      return;
    }

    const duration = getTransitionDuration();

    try {
      // Create and load main manager in a new container
      const { default: MainManager } = await import('./managers/main/main.js');
      const permittedManagers = getPermittedManagers();

      // Create wrapper for main manager with initial opacity 0
      const mainWrapper = document.createElement('div');
      mainWrapper.style.cssText = `
        position: absolute;
        top: 0;
        left: 0;
        right: 0;
        bottom: 0;
        opacity: 0;
        transition: opacity ${duration}ms ease-in-out;
        z-index: 10;
      `;
      container.appendChild(mainWrapper);

      // Initialize main manager (skip internal show animation, we handle it via crossfade)
      const mainManager = new MainManager(this, mainWrapper, permittedManagers);
      // Set reference BEFORE init() so that manager loading within init() can use it
      this.mainManagerInstance = mainManager;
      await mainManager.init({ skipShowAnimation: true });

      // Start crossfade: fade out login (if not already faded), fade in main
      requestAnimationFrame(() => {
        // Fade out login (it may already be at opacity 0 from login.hide(), but ensure it)
        if (loginElement) {
          loginElement.style.transition = `opacity ${duration}ms ease-in-out`;
          loginElement.style.opacity = '0';
        }
        // Fade in main (with slight delay for overlap effect)
        setTimeout(() => {
          mainWrapper.style.opacity = '1';
        }, duration / 4);
      });

      // Wait for transition to complete
      await new Promise(resolve => setTimeout(resolve, duration + 50));

      // Remove login element
      if (loginElement) {
        loginElement.remove();
      }
      
      // Clean up transition styles but keep opacity at 1
      mainWrapper.style.transition = '';
      mainWrapper.style.opacity = '';
      mainWrapper.style.position = '';
      mainWrapper.style.top = '';
      mainWrapper.style.left = '';
      mainWrapper.style.right = '';
      mainWrapper.style.bottom = '';
      mainWrapper.style.zIndex = '';

      // NOTE: Do NOT unconditionally overwrite this.currentManager here.
      // mainManager.init() loads the first permitted manager slot; preserve that state.
      if (this.currentManager?.id == null) {
        this.currentManager = { name: 'main', instance: mainManager };
      }

      logStartup('Main manager loaded with crossfade transition');
    } catch (error) {
      console.error('[Lithium] Failed to transition to main manager:', error);
      // Fallback: clear and load normally
      container.innerHTML = '';
      await this.loadMainManager();
    }
  }

  /**
   * Handle logout with fade out transition and page reload
   * @param {Object} eventDetail - Optional event detail with logoutType
   */
  async handleLogoutTransition(eventDetail = {}) {
    const logoutType = eventDetail?.logoutType || 'quick';
    logAuth(Status.INFO, `Initiating ${logoutType} logout with fade transition`);

    const container = document.getElementById('app');
    const mainElement = container?.firstElementChild;
    const duration = getTransitionDuration();

    // Wait for logout panel fade out (already handled by main.js)
    // Just wait a bit for the panel to close
    await new Promise(resolve => setTimeout(resolve, duration / 2));

    // Fade out main content
    if (mainElement) {
      mainElement.style.transition = `opacity ${duration}ms ease-in-out`;
      mainElement.style.opacity = '0';

      // Wait for fade out
      await new Promise(resolve => setTimeout(resolve, duration));
    }

    // Perform logout actions based on type
    await this.performLogoutActions(logoutType);

    // Reload page for clean state
    window.location.reload();
  }

  /**
   * Perform logout actions based on the selected logout type
   * @param {string} logoutType - 'quick', 'normal', 'public', or 'global'
   */
  async performLogoutActions(logoutType) {
    logAuth(Status.INFO, `Logout initiated: ${logoutType}`);

    // Call logout API (for all types)
    try {
      await this.api.post('auth/logout');
    } catch (error) {
      logAuth(Status.WARN, `Logout API call failed: ${error.message}`);
    }

    switch (logoutType) {
      case 'quick':
        // Normal logout but don't remove any cached data
        // Just clear the JWT
        clearJWT();
        break;

      case 'normal':
        // Logout and don't remember the username for logging in again
        clearJWT();
        try {
          localStorage.removeItem('lithium_last_username');
        } catch (error) {
          logAuth(Status.WARN, `Could not clear remembered username: ${error.message}`);
        }
        break;

      case 'public':
        // Logout and clear all local history/content/etc.
        clearJWT();
        this.clearLocalData();
        break;

      case 'global':
        // Logout Public + remove all server JWTs so no other sessions are active
        clearJWT();
        this.clearLocalData();
        // TODO: Call endpoint to invalidate all server sessions
        // await this.api.post('auth/logout/all');
        logAuth(Status.INFO, 'Global logout - all sessions should be invalidated');
        break;

      default:
        // Default to quick logout
        clearJWT();
    }

    // Flush logs before page reload
    flush();
  }

  /**
   * Clear local data for public/global logout
   */
  clearLocalData() {
    try {
      // Clear remembered username
      localStorage.removeItem('lithium_last_username');

      // Clear any cached lookups or preferences
      localStorage.removeItem('lithium_lookups');
      localStorage.removeItem('lithium_preferences');

      // Clear any session-specific data (but keep config)
      const keysToRemove = [];
      for (let i = 0; i < localStorage.length; i++) {
        const key = localStorage.key(i);
        if (key && key.startsWith('lithium_') && !key.includes('config')) {
          keysToRemove.push(key);
        }
      }
      keysToRemove.forEach(key => localStorage.removeItem(key));

      logAuth(Status.INFO, 'Local data cleared');
    } catch (error) {
      logAuth(Status.WARN, `Could not clear local data: ${error.message}`);
    }
  }

  /**
   * Handle logout (used for auth expiration, no transition)
   */
  async handleLogout() {
    logAuth(Status.INFO, 'Logging out');

    // Clear JWT timers before anything else
    this.clearExpirationTimers();

    // Call logout API
    try {
      await this.api.post('auth/logout');
    } catch (error) {
      logAuth(Status.WARN, `Logout API call failed: ${error.message}`);
    }

    // Clear local state
    clearJWT();
    this.user = null;
    this.loadedManagers.clear();
    this.mainManagerInstance = null;

    // Return to login
    await this.loadLoginManager();
  }

  /**
   * Handle expired authentication
   */
  async handleAuthExpired() {
    logAuth(Status.WARN, 'Authentication expired, redirecting to login');
    this.clearExpirationTimers();
    clearJWT();
    this.user = null;
    this.loadedManagers.clear();

    // Show expired message and redirect to login
    await this.loadLoginManager();
  }

  /**
   * Fetch lookups from server or cache
   * This is called during initialization to ensure lookups are available.
   * logging is handled inside lookups.js — no duplicate log lines here.
   */
  async fetchLookups() {
    try {
      await fetchLookups();
    } catch (error) {
      log(Subsystems.LOOKUPS, Status.WARN, `Failed to fetch lookups: ${error.message}`);
      // Non-fatal error, continue without lookups
    }
  }

  /**
   * Set up network status monitoring
   */
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

    // Initial status
    updateStatus();
  }

  /**
   * Show a fatal error message
   */
  showFatalError(message) {
    const app = document.getElementById('app');
    if (app) {
      app.innerHTML = `
        <div class="fatal-error">
          <h1><fa fa-exclamation-triangle></fa> Error</h1>
          <p>${message}</p>
          <button onclick="window.location.reload()" class="btn btn-primary">
            <fa fa-redo></fa> Reload Page
          </button>
        </div>
      `;
    }
    document.documentElement.style.visibility = 'visible';
  }

  /**
   * Get current application state
   */
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
}

// Create and initialize app when DOM is ready
let app = null;

document.addEventListener('DOMContentLoaded', async () => {
  app = new LithiumApp();

  // Expose app instance on window for module access (e.g., Crimson context)
  window.lithiumApp = app;

  // Expose log inspection API on window for console debugging
  window.lithiumLogs = {
    /** Get all log entries as raw JSON objects */
    get raw() { return getRawLog(); },
    /** Get all log entries as formatted display strings */
    get display() { return getDisplayLog(); },
    /** Get the most recent N log entries formatted for display (default 50) */
    recent(n = 50) { return getRecentLogs(n); },
    /** Print the most recent N log entries to the console (default 50) */
    print(n = 50) { getRecentLogs(n).forEach(line => console.log(line)); },
    /** Get current counter */
    get count() { return getCounter(); },
    /** Get current session ID */
    get sessionId() { return getSessionId(); },
    /** Enable or disable console logging */
    setConsoleLogging,
    /** Force flush to server */
    flush,
    /** Get all archived session logs from localStorage (for future server upload) */
    get archived() { return getArchivedSessions(); },
    /** Remove an archived session by key (after successful server upload) */
    removeArchived: removeArchivedSession,
  };

  // Expose tooltip API on window for dynamic tooltip management
  window.lithiumTip = { tip, untip, initTooltips };

  await app.init();
});

// Export for potential module use
export { LithiumApp };
export default app;
