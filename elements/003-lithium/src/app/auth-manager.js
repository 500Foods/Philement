import { logAuth, Status } from '../core/log.js';
import { retrieveJWT, validateJWT, storeJWT, clearJWT, getTimeUntilExpiry } from '../core/jwt.js';
import { loadMacrosPostLogin } from '../shared/lookups.js';
import { toast } from '../shared/toast.js';
import { eventBus, Events } from '../core/event-bus.js';
import { destroyAllTooltips } from '../core/tooltip-api.js';

export class AuthManager {
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
      description: 'Your session will expire soon. Please save your work.',
      duration: 0,
      dismissible: true,
      subsystem: 'Auth',
    });

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
  }

  async _renewOnActivity() {
    if (this._isRenewing) return;
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
      const { default: LoginManager } = await import('../managers/login/login.js');
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

  async performQuickLogoutCleanup(logoutType) {
    logAuth(Status.INFO, 'Performing quick logout cleanup');
    await this._fadeOutForLogout();
    await this.performLogoutActions('quick');
    window.location.reload(true);
  }

  async performNormalLogoutCleanup(logoutType) {
    logAuth(Status.INFO, 'Performing normal logout cleanup');
    await this._fadeOutForLogout();
    await this.performLogoutActions('normal');
    window.location.reload(true);
  }

  async performPublicLogoutCleanup(logoutType) {
    logAuth(Status.INFO, 'Performing public logout cleanup');
    await this._fadeOutForLogout();
    await this.performLogoutActions('public');
    window.location.reload(true);
  }

  async performGlobalLogoutCleanup(logoutType) {
    logAuth(Status.INFO, 'Performing global logout cleanup');
    // Immediately hide the main UI to prevent any flash during reload
    const mainMgr = this.app.mainManagerInstance;
    if (mainMgr?.elements?.layout) {
      mainMgr.elements.layout.style.opacity = '0';
      mainMgr.elements.layout.style.visibility = 'hidden';
    }
    // Destroy tooltips to prevent lingering
    destroyAllTooltips();
    await this.performLogoutActions('global');
    window.location.reload(true);
  }

  async handleAuthExpired() {
    logAuth(Status.WARN, 'Authentication expired, redirecting to login');
    this.clearExpirationTimers();
    clearJWT();
    this.user = null;
    this.app.loadedManagers.clear();
    await this.loadLoginManager();
  }

  async performLogoutActions(type = 'normal') {
    // For 'normal' logout, remove lithium_last_username
    if (type === 'normal') {
      localStorage.removeItem('lithium_last_username');
    }

    // For 'public' logout, clear ALL localStorage for this site
    if (type === 'public') {
      // Clear global settings service (in-memory state) BEFORE clearing localStorage
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
      // Clear global settings service (in-memory state) BEFORE clearing localStorage
      if (window.lithiumSettings && typeof window.lithiumSettings.clear === 'function') {
        window.lithiumSettings.clear();
        // Clear server settings for maximum security
        if (this.app.api) {
          try {
            await window.lithiumSettings.syncToServer(this.app.api);
          } catch (e) {
            logAuth(Status.WARN, `Failed to clear server settings: ${e.message}`);
          }
        }
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
   * For regular logout: fades out logout panel first (750ms), then main UI (750ms).
   * For profile manager global logout: fades out main UI directly (750ms).
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
      panel.style.transition = 'opacity 750ms ease-out';
      panel.style.opacity = '1';
      if (overlay) {
        overlay.style.transition = 'opacity 750ms ease-out';
        overlay.style.opacity = '1';
      }

      // Wait for requestAnimationFrame to ensure styles are applied
      await new Promise(resolve => requestAnimationFrame(resolve));

      // Stage 2: Trigger fade out
      panel.style.opacity = '0';
      if (overlay) overlay.style.opacity = '0';

      // Wait for transition to complete
      await new Promise(resolve => setTimeout(resolve, 750));

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
      layout.style.transition = 'opacity 750ms ease-out';
      layout.style.opacity = '1';

      // Wait for requestAnimationFrame to ensure styles are applied
      await new Promise(resolve => requestAnimationFrame(resolve));

      // Stage 2: Trigger fade out
      layout.style.opacity = '0';

      // Wait for transition to complete
      await new Promise(resolve => setTimeout(resolve, 750));

      // Clean up inline styles
      layout.classList.remove('visible');
      layout.style.transition = '';
      layout.style.opacity = '';
    }
  }
}
