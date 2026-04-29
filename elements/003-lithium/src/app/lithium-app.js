import './../core/vendor-patches.js';
import GlobalSettingsService from '../core/global-settings-service.js';
import { loadConfig, getConfigValue } from '../core/config.js';
import { eventBus, Events } from '../core/event-bus.js';
import { retrieveJWT, clearJWT } from '../core/jwt.js';
import { createRequest } from '../core/json-request.js';
import { fetchLookups, init as initLookups } from '../shared/lookups.js';
import { init as initIcons, preloadIconsFromConfig } from '../core/icons.js';
import { initTooltips } from '../core/tooltip-api.js';
import { log, logAuth, logGroup, logStartup, Subsystems, Status, getSessionId } from '../core/log.js';
import { AuthManager } from './auth-manager.js';
import { ManagerLoader } from './manager-loader.js';

export class LithiumApp {
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
      initIcons();
      logStartup('Icon system initialized', Date.now() - iconsStart);

      this.revealPage();
      logStartup('Page revealed');
      initTooltips();
      logStartup('Tooltip system initialized');

      // Set up event listeners BEFORE auth check so login:success handler is ready
      this.setupEventListeners();
      logStartup('Event listeners registered');

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
      initLookups();
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
