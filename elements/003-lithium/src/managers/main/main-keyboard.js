/**
 * Keyboard shortcuts and event listeners for MainManager
 * Handles global keyboard shortcuts, event listeners, and mobile sidebar
 */

import { eventBus, Events } from '../../core/event-bus.js';
import { registerShortcut } from '../../core/manager-ui.js';
import { canAccessManager } from '../../core/permissions.js';
import { log, Subsystems, Status } from '../../core/log.js';

/**
 * Methods to be mixed into MainManager prototype
 */
export const keyboardMethods = {
  /**
   * Set up event listeners
   */
  setupEventListeners() {
    // Sidebar collapse/expand button
    this.elements.sidebarCollapseBtn?.addEventListener('click', () => {
      this.toggleSidebarCollapse();
    });

    // Sidebar footer icon buttons
    this.elements.sidebarThemeBtn?.addEventListener('click', () => {
      if (canAccessManager(1)) {
        this.loadManager(1);
      }
    });

    this.elements.sidebarProfileBtn?.addEventListener('click', () => {
      this.setActiveUtilityButton('user-profile');
      this.app.loadUtilityManager('user-profile');
    });

    this.elements.sidebarLogsBtn?.addEventListener('click', () => {
      this.setActiveUtilityButton('session-log');
      this.app.loadUtilityManager('session-log');
    });

    // Sidebar logout button (icon-only in footer)
    this.elements.sidebarLogoutBtn?.addEventListener('click', () => {
      this.showLogoutPanel();
    });

    // Legacy logout button (if still in template)
    this.container.querySelector('#logout-btn')?.addEventListener('click', () => {
      this.showLogoutPanel();
    });

    // Logout panel close button
    this.elements.logoutCloseBtn?.addEventListener('click', () => {
      this.hideLogoutPanel();
    });

    // Logout overlay click to dismiss (clicking backdrop closes panel)
    this.elements.logoutOverlay?.addEventListener('click', () => {
      this.hideLogoutPanel();
    });

    // Logout option buttons
    this.elements.logoutOptions?.forEach((option) => {
      option.addEventListener('click', () => {
        const logoutType = option.getAttribute('data-logout-type');
        this.executeLogout(logoutType);
      });
    });

    // Mobile menu button
    this.elements.mobileMenuBtn?.addEventListener('click', () => {
      this.openMobileSidebar();
    });

    // Sidebar overlay (close on click)
    this.elements.sidebarOverlay?.addEventListener('click', () => {
      this.closeMobileSidebar();
    });

    // Network status events
    eventBus.on(Events.NETWORK_ONLINE, () => {
      this._setOfflineIndicators(false);
    });

    eventBus.on(Events.NETWORK_OFFLINE, () => {
      this._setOfflineIndicators(true);
    });

    // Radar icon click - toggle between header and sidebar
    this.elements.radarIcon?.addEventListener('click', () => {
      this._toggleRadarPosition();
    });

    this.elements.radarIconSidebar?.addEventListener('click', () => {
      this._toggleRadarPosition();
    });

    // Logout event from keyboard shortcut
    eventBus.on('logout:show', () => {
      this.showLogoutPanel();
    });

    // Handle window resize to reset sidebar on mobile/desktop transition
    window.addEventListener('resize', () => {
      this.handleWindowResize();
    });
  },

  /**
   * Set up global keyboard shortcuts
   * Registers all shortcuts with the manager-ui system for tooltip display
   */
  setupGlobalKeyboardShortcuts() {
    // Register all global shortcuts with manager-ui for tooltip display
    registerShortcut('close', 'Ctrl+Shift+X', 'Close manager', () => this.handleCloseManager());
    registerShortcut('zoom', 'Ctrl+Shift+Z', 'Toggle zoom', () => this._handleZoomShortcut());
    registerShortcut('shortcuts', 'Ctrl+Shift+?', 'Show keyboard shortcuts', () => this._handleKeyboardShortcutsShortcut());
    registerShortcut('settings', 'Ctrl+Shift+.', 'Open settings', () => this._handleSettingsShortcut());
    registerShortcut('search', 'Ctrl+Shift+F', 'Focus search', () => this._handleSearchShortcut());
    
    // Set up global keydown listener in CAPTURE mode so it fires first
    // This ensures shortcuts work regardless of other keydown handlers
    document.addEventListener('keydown', this.handleGlobalKeyDown, true);
  },

  /**
   * Handle global keyboard shortcuts
   */
  handleGlobalKeyDown(event) {
    const isCtrlOrCmd = event.ctrlKey || event.metaKey;
    
    // Don't trigger in input fields (except for search focus which needs it)
    const target = event.target;
    const isInput = target.tagName === 'INPUT' || target.tagName === 'TEXTAREA' || target.isContentEditable;
    
    // Always allow search shortcut even in inputs
    if (isCtrlOrCmd && event.shiftKey && event.key === 'F') {
      // For search, we want it even in inputs
      event.preventDefault();
      this._handleSearchShortcut();
      event.stopPropagation();
      return;
    }
    
    // Don't trigger other shortcuts in input fields
    if (isInput) return;

    // Ctrl+Shift+L or Cmd+Shift+L to open logout panel
    if (isCtrlOrCmd && event.shiftKey && event.key === 'L') {
      event.preventDefault();
      event.stopPropagation();
      if (!this.isLogoutPanelVisible) {
        this.showLogoutPanel();
      }
      return;
    }

    // Ctrl+Shift+X or Cmd+Shift+X to close current manager
    if (isCtrlOrCmd && event.shiftKey && event.key === 'X') {
      event.preventDefault();
      event.stopPropagation();
      this.handleCloseManager();
      return;
    }

    // Ctrl+Shift+Z or Cmd+Shift+Z to toggle zoom popup
    if (isCtrlOrCmd && event.shiftKey && event.key === 'Z') {
      event.preventDefault();
      event.stopPropagation();
      this._handleZoomShortcut();
      return;
    }

    // Ctrl+Shift+? or Ctrl+Shift+/ to show keyboard shortcuts
    if (isCtrlOrCmd && event.shiftKey && (event.key === '?' || event.key === '/')) {
      event.preventDefault();
      event.stopPropagation();
      this._handleKeyboardShortcutsShortcut();
      return;
    }

    // Ctrl+Shift+. (period) or Cmd+Shift+. to open settings (profile manager)
    // Also check keyCode 190 for Period key
    if (isCtrlOrCmd && event.shiftKey && (event.key === '.' || event.keyCode === 190)) {
      event.preventDefault();
      event.stopPropagation();
      this._handleSettingsShortcut();
      return;
    }

    // Ctrl+Shift+F - already handled above for input focus
  },

  /**
   * Handle close manager action - click the close button
   */
  handleCloseManager() {
    const closeBtn = document.querySelector('.manager-slot.slot-visible .manager-ui-close-btn');
    if (closeBtn) {
      closeBtn.click();
    }
  },

  /**
   * Handle zoom shortcut - toggle zoom popup
   */
  _handleZoomShortcut() {
    const zoomBtn = document.querySelector('.manager-ui-zoom-btn');
    if (zoomBtn) {
      import('../../core/zoom-control.js').then(({ toggleZoomPopup }) => {
        toggleZoomPopup(zoomBtn);
      });
    }
  },

  /**
   * Handle keyboard shortcuts popup shortcut
   */
  _handleKeyboardShortcutsShortcut() {
    const kbBtn = document.querySelector('.manager-ui-shortcuts-btn');
    if (kbBtn) {
      kbBtn.click();
    }
  },

  /**
   * Handle settings shortcut - click the manager menu button (gear icon)
   */
  _handleSettingsShortcut() {
    const menuBtn = document.querySelector('.manager-slot.slot-visible .manager-ui-manager-menu-btn');
    if (menuBtn) {
      menuBtn.click();
    }
  },

  /**
   * Handle search shortcut - focus search input
   */
  _handleSearchShortcut() {
    const activeSlot = document.querySelector('.manager-slot.slot-visible');
    if (activeSlot) {
      const searchInput = activeSlot.querySelector('.slot-header-search-input');
      if (searchInput) {
        searchInput.focus();
        searchInput.select();
      }
    }
  },

  /**
   * Update offline indicator on all visible slots
   */
  _setOfflineIndicators(isOffline) {
    const area = this.elements.managerArea;
    if (!area) return;
    area.querySelectorAll('.offline-indicator').forEach(el => {
      el.classList.toggle('visible', isOffline);
    });
  },

  /**
   * Open mobile sidebar
   */
  openMobileSidebar() {
    this.elements.sidebar?.classList.add('open');
    this.elements.sidebarOverlay?.classList.add('visible');
  },

  /**
   * Close mobile sidebar
   */
  closeMobileSidebar() {
    this.elements.sidebar?.classList.remove('open');
    this.elements.sidebarOverlay?.classList.remove('visible');
  },

  /**
   * Handle keyboard shortcuts for logout panel
   * ESC - cancel, Arrow Up/Left - previous, Arrow Down/Right - next, Enter - select, 1-4 - select option
   */
  handleKeyDown(event) {
    if (!this.isLogoutPanelVisible) return;

    const optionsCount = this.elements.logoutOptions?.length || 0;

    switch (event.key) {
      case 'Escape':
        event.preventDefault();
        this.hideLogoutPanel();
        break;

      case 'ArrowUp':
      case 'ArrowLeft':
        event.preventDefault();
        this.selectedLogoutIndex = (this.selectedLogoutIndex - 1 + optionsCount) % optionsCount;
        this._updateLogoutSelection();
        break;

      case 'ArrowDown':
      case 'ArrowRight':
        event.preventDefault();
        this.selectedLogoutIndex = (this.selectedLogoutIndex + 1) % optionsCount;
        this._updateLogoutSelection();
        break;

      case 'Enter':
        event.preventDefault();
        this._executeSelectedLogout();
        break;

      case '1':
        event.preventDefault();
        this.selectedLogoutIndex = 0;
        this._updateLogoutSelection();
        this._executeSelectedLogout();
        break;

      case '2':
        event.preventDefault();
        this.selectedLogoutIndex = 1;
        this._updateLogoutSelection();
        this._executeSelectedLogout();
        break;

      case '3':
        event.preventDefault();
        this.selectedLogoutIndex = 2;
        this._updateLogoutSelection();
        this._executeSelectedLogout();
        break;

      case '4':
        event.preventDefault();
        this.selectedLogoutIndex = 3;
        this._updateLogoutSelection();
        this._executeSelectedLogout();
        break;

      default:
        break;
    }
  },

  /**
   * Update visual selection state for logout options
   */
  _updateLogoutSelection() {
    const options = this.elements.logoutOptions;
    if (!options) return;
    options.forEach((option, index) => {
      if (index === this.selectedLogoutIndex) {
        option.classList.add('selected');
        option.focus();
      } else {
        option.classList.remove('selected');
      }
    });
  },

  /**
   * Execute logout based on currently selected option
   */
  _executeSelectedLogout() {
    const options = this.elements.logoutOptions;
    if (!options) return;
    const selectedOption = options[this.selectedLogoutIndex];
    if (selectedOption) {
      const logoutType = selectedOption.getAttribute('data-logout-type');
      this.executeLogout(logoutType);
    }
  },

  /**
   * Execute logout with the specified type
   * @param {string} logoutType - 'quick', 'normal', 'public', or 'global'
   */
  executeLogout(logoutType) {
    // First, fade out and hide the logout panel
    this.hideLogoutPanel();

    // Then, after the panel is fully hidden, proceed with logout
    // Use 300ms to match the CSS transition duration
    setTimeout(() => {
      // Emit logout:initiate event with the logout type
      // The app.js will handle the actual logout process
      eventBus.emit('logout:initiate', { logoutType });
    }, 300);
  },
};
