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

import { loadConfig, getConfig, getConfigValue } from './core/config.js';
import { eventBus, Events } from './core/event-bus.js';
import { validateJWT, retrieveJWT, getClaims, getRenewalTime, storeJWT, clearJWT } from './core/jwt.js';
import { getPermittedManagers } from './core/permissions.js';
import { createRequest } from './core/json-request.js';
import { fetchLookups, init as initLookups } from './shared/lookups.js';
import { init as initIcons } from './core/icons.js';
import { getTransitionDuration } from './core/transitions.js';
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
    this.managerRegistry = {
      1: { id: 1, name: 'Style Manager' },
      2: { id: 2, name: 'Profile Manager' },
      3: { id: 3, name: 'Dashboard' },
      4: { id: 4, name: 'Lookups' },
      5: { id: 5, name: 'Queries' },
    };

    // Utility manager definitions — key -> name only (module loaded via _importUtilityManager)
    this.utilityManagerRegistry = {
      'session-log': { key: 'session-log', name: 'Session Log' },
      'user-profile': { key: 'user-profile', name: 'User Profile' },
    };

    // Transition overlay element (created lazily)
    this.transitionOverlay = null;
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

      // Step 6: Check authentication and load appropriate manager
      // The login panel starts with its logs button disabled; it will be enabled
      // once background startup tasks complete and STARTUP_COMPLETE is emitted.
      const authStart = Date.now();
      await this.checkAuthAndLoad();
      logStartup('Auth check complete', Date.now() - authStart);

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
   */
  async loadVersion() {
    try {
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

      // Load main menu manager
      await this.loadMainManager();
    } else {
      logAuth(Status.INFO, 'No valid JWT, loading login');
      await this.loadLoginManager();
    }
  }

  /**
   * Schedule automatic token renewal
   */
  scheduleTokenRenewal(token) {
    const renewalTime = getRenewalTime(token);

    if (renewalTime > 0 && renewalTime !== Infinity) {
      logAuth(Status.INFO, `Token renewal scheduled in ${Math.round(renewalTime / 1000)}s`);

      setTimeout(async () => {
        try {
          await this.renewToken();
        } catch (error) {
          logAuth(Status.ERROR, `Token renewal failed: ${error.message}`);
          eventBus.emit(Events.AUTH_EXPIRED);
        }
      }, renewalTime);
    }
  }

  /**
   * Renew the JWT token
   */
  async renewToken() {
    logAuth(Status.INFO, 'Starting token renewal');
    const start = Date.now();
    const response = await this.api.post('auth/renew');

    if (response.token) {
      storeJWT(response.token);
      this.scheduleTokenRenewal(response.token);
      eventBus.emit(Events.AUTH_RENEWED, { expiresAt: response.expires_at });
      logAuth(Status.SUCCESS, 'Token renewed successfully', Date.now() - start);
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

      this.currentManager = { name: 'main', instance: mainManager };
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
      case 1: return import('./managers/style-manager/index.js');
      case 2: return import('./managers/profile-manager/index.js');
      case 3: return import('./managers/dashboard/index.js');
      case 4: return import('./managers/lookups/index.js');
      case 5: return import('./managers/queries/index.js');
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
      case 'session-log': return import('./managers/session-log/index.js');
      case 'user-profile': return import('./managers/profile-manager/index.js');
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
      logManager(Status.INFO, `Loading manager: ${managerDef.name}`);
      const loadStart = Date.now();
      const module = await this._importManager(managerId);

      // Get the MainManager to create a slot
      const mainMgr = this._getMainManager();
      if (!mainMgr) {
        throw new Error('MainManager not ready');
      }

      // Look up icon for this manager
      const iconInfo = mainMgr.managerIcons?.[managerId] || { icon: 'fa-cube', name: managerDef.name };
      const slotId = mainMgr._slotId(managerId);
      const slotResult = mainMgr.createSlot(slotId, iconInfo.icon, iconInfo.name);
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
        instance: managerInstance,
        slotEl,
        workspaceEl,
      });

      await this.showManager(managerId);
      eventBus.emit(Events.MANAGER_LOADED, { managerId });
      eventBus.emit(Events.MANAGER_SWITCHED, { from: this.currentManager?.id, to: managerId });

      logManager(Status.SUCCESS, `Manager ${managerDef.name} loaded`, Date.now() - loadStart);
    } catch (error) {
      logManager(Status.ERROR, `Failed to load manager ${managerDef.name}: ${error.message}`);
    }
  }

  /**
   * Load a utility manager into the workspace.
   * Uses the slot-based system: each utility manager gets a slot.
   * @param {string} managerKey - The utility manager key (e.g. 'session-log')
   */
  async loadUtilityManager(managerKey) {
    const managerDef = this.utilityManagerRegistry[managerKey];

    if (!managerDef) {
      logManager(Status.ERROR, `Unknown utility manager key: ${managerKey}`);
      return;
    }

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

      const iconInfo = mainMgr.utilityManagerIcons?.[managerKey] || { icon: 'fa-puzzle-piece', name: managerDef.name };
      const slotId = mainMgr._utilitySlotId(managerKey);
      const slotResult = mainMgr.createSlot(slotId, iconInfo.icon, iconInfo.name);
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
    this.currentManager = { id: loadedKey, instance: incoming.instance };

    // Notify main manager to update sidebar active state
    const mainMgr = this._getMainManager();
    if (mainMgr) {
      const utilityKey = loadedKey.replace('utility:', '');
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
    this.currentManager = { id: managerId, instance: incoming.instance };

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

    // If no outgoing, just show incoming immediately with fade-in
    if (!outgoingSlot || outgoingSlot === incomingSlot) {
      incomingSlot.classList.remove('slot-hidden');
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

    // Force reflow
    void incomingSlot.offsetHeight;

    // Step 2: Fade in incoming, fade out outgoing simultaneously
    incomingSlot.classList.add('slot-visible');     // opacity → 1
    outgoingSlot.classList.remove('slot-visible');  // opacity → 0

    // Step 3: Wait for CSS transitions to complete
    await new Promise(resolve => setTimeout(resolve, duration + 50));

    // Step 4: Fully hide outgoing (visibility:hidden after opacity transition)
    outgoingSlot.classList.add('slot-hidden');

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

      // Update current manager reference
      this.currentManager = { name: 'main', instance: mainManager };

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
    clearJWT();
    this.user = null;
    this.loadedManagers.clear();

    // Show expired message and redirect to login
    alert('Your session has expired. Please log in again.');
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
      config: this.config,
      user: this.user,
      currentManager: this.currentManager?.name || null,
      online: navigator.onLine,
      timestamp: new Date().toISOString(),
    };
  }
}

// Create and initialize app when DOM is ready
let app = null;

document.addEventListener('DOMContentLoaded', async () => {
  app = new LithiumApp();

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

  await app.init();
});

// Export for potential module use
export { LithiumApp };
export default app;
