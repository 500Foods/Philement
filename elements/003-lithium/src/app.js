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
import { validateJWT, retrieveJWT, getClaims, getRenewalTime } from './core/jwt.js';
import { getPermittedManagers } from './core/permissions.js';
import { createRequest } from './core/json-request.js';
import { fetchLookups, init as initLookups } from './shared/lookups.js';
import { init as initIcons } from './core/icons.js';
import { getTransitionDuration } from './core/transitions.js';

/**
 * Main Lithium Application Class
 */
class LithiumApp {
  constructor() {
    this.version = '1.0.0';
    this.config = null;
    this.api = null;
    this.currentManager = null;
    this.loadedManagers = new Map();
    this.user = null;
    this.deferredInstallPrompt = null;

    // Manager definitions (id -> module path)
    this.managerRegistry = {
      1: { id: 1, name: 'Style Manager', path: '/src/managers/style-manager/index.js' },
      2: { id: 2, name: 'Profile Manager', path: '/src/managers/profile-manager/index.js' },
      3: { id: 3, name: 'Dashboard', path: '/src/managers/dashboard/index.js' },
      4: { id: 4, name: 'Lookups', path: '/src/managers/lookups/index.js' },
      5: { id: 5, name: 'Queries', path: '/src/managers/queries/index.js' },
    };
  }

  /**
   * Initialize the application
   */
  async init() {
    try {
      console.log('[Lithium] Initializing application...');

      // Step 1: Load configuration
      this.config = await loadConfig();
      console.log('[Lithium] Configuration loaded:', this.config.app.name);

      // Step 2: Create API client with config
      this.api = createRequest(this.config);

      // Step 3: Initialize lookups (fetch open lookups from cache or server)
      // This happens before auth check so lookups are available for login UI
      initLookups();
      this.fetchLookups();

      // Step 4: Initialize icon system
      initIcons();

      // Step 5: Reveal the page (FOUC prevention)
      this.revealPage();

      // Step 6: Check authentication and load appropriate manager
      await this.checkAuthAndLoad();

      // Step 7: Set up global event listeners
      this.setupEventListeners();

      // Step 8: Set up network status monitoring
      this.setupNetworkMonitoring();

      console.log('[Lithium] Application initialized successfully');
    } catch (error) {
      console.error('[Lithium] Failed to initialize:', error);
      this.showFatalError('Failed to initialize application. Please refresh the page.');
    }
  }

  /**
   * Reveal the page after styles are loaded (FOUC prevention)
   */
  revealPage() {
    // Small delay to ensure CSS is parsed
    requestAnimationFrame(() => {
      document.documentElement.style.visibility = 'visible';
      console.log('[Lithium] Page revealed');
    });
  }

  /**
   * Check authentication status and load appropriate manager
   */
  async checkAuthAndLoad() {
    const token = retrieveJWT();
    const validation = validateJWT(token);

    if (validation.valid) {
      console.log('[Lithium] Valid JWT found, loading main menu');
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
      console.log('[Lithium] No valid JWT, loading login');
      await this.loadLoginManager();
    }
  }

  /**
   * Schedule automatic token renewal
   */
  scheduleTokenRenewal(token) {
    const renewalTime = getRenewalTime(token);

    if (renewalTime > 0 && renewalTime !== Infinity) {
      console.log(`[Lithium] Token renewal scheduled in ${Math.round(renewalTime / 1000)}s`);

      setTimeout(async () => {
        try {
          await this.renewToken();
        } catch (error) {
          console.error('[Lithium] Token renewal failed:', error);
          eventBus.emit(Events.AUTH_EXPIRED);
        }
      }, renewalTime);
    }
  }

  /**
   * Renew the JWT token
   */
  async renewToken() {
    console.log('[Lithium] Renewing token...');
    const response = await this.api.post('auth/renew');

    if (response.token) {
      const { storeJWT } = await import('./core/jwt.js');
      storeJWT(response.token);
      this.scheduleTokenRenewal(response.token);
      eventBus.emit(Events.AUTH_RENEWED, { expiresAt: response.expires_at });
      console.log('[Lithium] Token renewed successfully');
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
      await mainManager.init();

      this.currentManager = { name: 'main', instance: mainManager };
    } catch (error) {
      console.error('[Lithium] Failed to load main manager:', error);
      this.showFatalError('Failed to load application. Please refresh the page.');
    }
  }

  /**
   * Load a manager into the workspace
   */
  async loadManager(managerId) {
    const managerDef = this.managerRegistry[managerId];

    if (!managerDef) {
      console.error(`[Lithium] Unknown manager ID: ${managerId}`);
      return;
    }

    // Check if already loaded
    if (this.loadedManagers.has(managerId)) {
      const manager = this.loadedManagers.get(managerId);
      this.showManager(managerId);
      eventBus.emit(Events.MANAGER_SWITCHED, { from: this.currentManager?.id, to: managerId });
      return;
    }

    try {
      console.log(`[Lithium] Loading manager: ${managerDef.name}`);
      const module = await import(/* @vite-ignore */ managerDef.path);

      // Create workspace container
      const workspace = document.getElementById('workspace');
      if (!workspace) {
        throw new Error('Workspace container not found');
      }

      const container = document.createElement('div');
      container.id = `manager-${managerId}`;
      container.className = 'manager-container';
      container.style.display = 'none';
      workspace.appendChild(container);

      // Initialize manager
      const ManagerClass = module.default;
      const managerInstance = new ManagerClass(this, container);
      await managerInstance.init();

      // Store and show
      this.loadedManagers.set(managerId, {
        id: managerId,
        instance: managerInstance,
        container,
      });

      this.showManager(managerId);
      eventBus.emit(Events.MANAGER_LOADED, { managerId });
      eventBus.emit(Events.MANAGER_SWITCHED, { from: this.currentManager?.id, to: managerId });

      console.log(`[Lithium] Manager ${managerDef.name} loaded successfully`);
    } catch (error) {
      console.error(`[Lithium] Failed to load manager ${managerId}:`, error);
    }
  }

  /**
   * Show a previously loaded manager
   */
  showManager(managerId) {
    // Hide current manager
    if (this.currentManager && this.currentManager.id) {
      const current = this.loadedManagers.get(this.currentManager.id);
      if (current) {
        current.container.classList.add('manager-hidden');
        current.container.style.display = 'none';
      }
    }

    // Show new manager
    const manager = this.loadedManagers.get(managerId);
    if (manager) {
      manager.container.classList.remove('manager-hidden');
      manager.container.classList.add('manager-visible');
      manager.container.style.display = 'block';
      this.currentManager = { id: managerId, instance: manager.instance };
    }
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
      console.log('[Lithium] PWA install prompt available');
    });

    window.addEventListener('appinstalled', () => {
      console.log('[Lithium] PWA was installed');
      this.deferredInstallPrompt = null;
    });
  }

  /**
   * Handle successful login
   */
  async handleLogin(detail) {
    console.log('[Lithium] Login successful, loading main menu');
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

      console.log('[Lithium] Main manager loaded with crossfade transition');
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
    console.log(`[Lithium] Initiating ${logoutType} logout with fade transition...`);

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
    console.log('[Lithium] Reloading page...');
    window.location.reload();
  }

  /**
   * Perform logout actions based on the selected logout type
   * @param {string} logoutType - 'quick', 'normal', 'public', or 'global'
   */
  async performLogoutActions(logoutType) {
    // Call logout API (for all types)
    try {
      await this.api.post('auth/logout');
    } catch (error) {
      console.warn('[Lithium] Logout API call failed:', error);
    }

    // Import JWT module dynamically
    const { clearJWT } = await import('./core/jwt.js');

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
          console.warn('[Lithium] Could not clear remembered username:', error);
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
        console.log('[Lithium] Global logout - all sessions should be invalidated');
        break;

      default:
        // Default to quick logout
        clearJWT();
    }
  }

  /**
   * Clear local data for public/global logout
   */
  clearLocalData() {
    console.log('[Lithium] Clearing local data...');

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

      console.log('[Lithium] Local data cleared');
    } catch (error) {
      console.warn('[Lithium] Could not clear local data:', error);
    }
  }

  /**
   * Handle logout (used for auth expiration, no transition)
   */
  async handleLogout() {
    console.log('[Lithium] Logging out...');

    // Call logout API
    try {
      await this.api.post('auth/logout');
    } catch (error) {
      console.warn('[Lithium] Logout API call failed:', error);
    }

    // Clear local state
    const { clearJWT } = await import('./core/jwt.js');
    clearJWT();
    this.user = null;
    this.loadedManagers.clear();

    // Return to login
    await this.loadLoginManager();
  }

  /**
   * Handle expired authentication
   */
  async handleAuthExpired() {
    console.log('[Lithium] Authentication expired');
    const { clearJWT } = await import('./core/jwt.js');
    clearJWT();
    this.user = null;
    this.loadedManagers.clear();

    // Show expired message and redirect to login
    alert('Your session has expired. Please log in again.');
    await this.loadLoginManager();
  }

  /**
   * Fetch lookups from server or cache
   * This is called during initialization to ensure lookups are available
   */
  async fetchLookups() {
    try {
      console.log('[Lithium] Fetching lookups...');
      await fetchLookups();
    } catch (error) {
      console.warn('[Lithium] Failed to fetch lookups:', error.message);
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
        console.log('[Lithium] Network: Online');
      } else {
        eventBus.emit(Events.NETWORK_OFFLINE);
        console.log('[Lithium] Network: Offline');
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
  await app.init();
});

// Export for potential module use
export { LithiumApp };
export default app;
