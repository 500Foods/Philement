/**
 * Main Menu Manager
 * 
 * Handles the main application layout: resizable sidebar, header bar, workspace.
 * Loads permitted managers into the workspace on demand.
 * Sidebar features:
 *   - Gradient header matching login panel
 *   - Scrollable menu
 *   - Icon-only footer with collapse/expand button
 *   - Resizable width (200px-400px) with splitter
 *   - Width persisted to localStorage
 */

import { eventBus, Events } from '../../core/event-bus.js';
import { getClaims } from '../../core/jwt.js';
import { getPermittedManagers, canAccessManager } from '../../core/permissions.js';
import { setIcon } from '../../core/icons.js';
import './main.css';

// localStorage keys for sidebar state
const SIDEBAR_WIDTH_KEY = 'lithium_sidebar_width';
const SIDEBAR_COLLAPSED_KEY = 'lithium_sidebar_collapsed';

// Sidebar constraints
const MIN_SIDEBAR_WIDTH = 200;
const MAX_SIDEBAR_WIDTH = 400;
const DEFAULT_SIDEBAR_WIDTH = 260;
const COLLAPSED_WIDTH = 56;

/**
 * Main Menu Manager Class
 */
export default class MainManager {
  constructor(app, container, permittedManagers) {
    this.app = app;
    this.container = container;
    this.permittedManagers = permittedManagers || [];
    this.elements = {};
    this.currentManagerId = null;
    this.user = null;
    this.isLogoutPanelVisible = false;
    this.isSidebarCollapsed = false;
    this.isResizing = false;
    
    // Bind event handlers
    this.handleKeyDown = this.handleKeyDown.bind(this);
    this.handleGlobalKeyDown = this.handleGlobalKeyDown.bind(this);
    this.handleSplitterMouseDown = this.handleSplitterMouseDown.bind(this);
    this.handleSplitterMouseMove = this.handleSplitterMouseMove.bind(this);
    this.handleSplitterMouseUp = this.handleSplitterMouseUp.bind(this);
    this.handleSplitterTouchStart = this.handleSplitterTouchStart.bind(this);
    this.handleSplitterTouchMove = this.handleSplitterTouchMove.bind(this);
    this.handleSplitterTouchEnd = this.handleSplitterTouchEnd.bind(this);

    // Manager registry with icons
    this.managerIcons = {
      1: { icon: 'fa-palette', name: 'Style Manager' },
      2: { icon: 'fa-user-cog', name: 'Profile Manager' },
      3: { icon: 'fa-chart-line', name: 'Dashboard' },
      4: { icon: 'fa-list', name: 'Lookups' },
      5: { icon: 'fa-search', name: 'Queries' },
    };
  }

  /**
   * Initialize the main manager
   * @param {Object} options - Initialization options
   * @param {boolean} options.skipShowAnimation - If true, skip the show animation (used during crossfade)
   */
  async init(options = {}) {
    console.log('[MainManager] Initializing...');
    
    try {
      await this.render();
      this.setupEventListeners();
      this.setupGlobalKeyboardShortcuts();
      this.setupSplitter();
      this.loadUserInfo();
      this.buildSidebar();
      this.loadSidebarState();
      this.updateVersionBox();
      
      // Only run show animation if not skipping (e.g., during crossfade transition)
      if (!options.skipShowAnimation) {
        this.show();
      } else {
        // During crossfade, make layout visible immediately (wrapper handles the animation)
        if (this.elements.layout) {
          this.elements.layout.style.transition = 'none';
          this.elements.layout.classList.add('visible');
          void this.elements.layout.offsetHeight;
          this.elements.layout.style.transition = '';
        }
      }

      // Load first permitted manager by default
      if (this.permittedManagers.length > 0) {
        await this.loadManager(this.permittedManagers[0]);
      }
    } catch (error) {
      console.error('[MainManager] Initialization error:', error);
    }
  }

  /**
   * Render the main layout
   */
  async render() {
    try {
      const response = await fetch('/src/managers/main/main.html');
      const html = await response.text();
      this.container.innerHTML = html;
    } catch (error) {
      console.error('[MainManager] Failed to load template:', error);
      this.renderFallback();
    }

    // Cache DOM elements
    this.elements = {
      layout: this.container.querySelector('#main-layout'),
      sidebar: this.container.querySelector('#sidebar'),
      sidebarMenu: this.container.querySelector('#sidebar-menu'),
      sidebarSplitter: this.container.querySelector('#sidebar-splitter'),
      sidebarCollapseBtn: this.container.querySelector('#sidebar-collapse-btn'),
      collapseIcon: this.container.querySelector('#collapse-icon'),
      sidebarThemeBtn: this.container.querySelector('#sidebar-theme-btn'),
      sidebarProfileBtn: this.container.querySelector('#sidebar-profile-btn'),
      sidebarHelpBtn: this.container.querySelector('#sidebar-help-btn'),
      sidebarLogoutBtn: this.container.querySelector('#sidebar-logout-btn'),
      sidebarOverlay: this.container.querySelector('#sidebar-overlay'),
      mobileMenuBtn: this.container.querySelector('#mobile-menu-btn'),
      headerTitle: this.container.querySelector('#header-title'),
      versionBox: this.container.querySelector('#version-box'),
      versionBuild: this.container.querySelector('#version-build'),
      versionYear: this.container.querySelector('#version-year'),
      versionDate: this.container.querySelector('#version-date'),
      versionTime: this.container.querySelector('#version-time'),
      workspace: this.container.querySelector('#workspace'),
      logoutBtn: this.container.querySelector('#logout-btn'),
      profileBtn: this.container.querySelector('#profile-btn'),
      profileAvatar: this.container.querySelector('#profile-avatar'),
      profileName: this.container.querySelector('#profile-name'),
      profileRole: this.container.querySelector('#profile-role'),
      themeBtn: this.container.querySelector('#theme-btn'),
      offlineIndicator: this.container.querySelector('#offline-indicator'),
      logoutPanel: this.container.querySelector('#logout-panel'),
      logoutOverlay: this.container.querySelector('#logout-overlay'),
      logoutCloseBtn: this.container.querySelector('#logout-close-btn'),
      logoutOptions: this.container.querySelectorAll('.logout-option'),
    };

    // Move logout overlay and panel to document.body for proper fixed positioning.
    // This ensures they're independent of any parent transforms/opacity that would
    // break position:fixed (e.g., during login→main crossfade transition).
    if (this.elements.logoutOverlay) {
      document.body.appendChild(this.elements.logoutOverlay);
    }
    if (this.elements.logoutPanel) {
      document.body.appendChild(this.elements.logoutPanel);
    }
  }

  /**
   * Render a fallback layout if template fails to load
   */
  renderFallback() {
    this.container.innerHTML = `
      <div id="main-layout">
        <aside class="main-sidebar" id="sidebar">
          <div class="sidebar-menu" id="sidebar-menu"></div>
          <div class="sidebar-footer">
            <button class="logout-btn" id="logout-btn">Logout</button>
          </div>
        </aside>
        <div class="main-content">
          <header class="main-header">
            <h2 class="header-title" id="header-title">Dashboard</h2>
          </header>
          <main class="workspace" id="workspace"></main>
        </div>
      </div>
    `;
  }

  /**
   * Set up event listeners
   */
  setupEventListeners() {
    // Sidebar collapse/expand button
    this.elements.sidebarCollapseBtn?.addEventListener('click', () => {
      this.toggleSidebarCollapse();
    });

    // Footer icon buttons
    this.elements.sidebarThemeBtn?.addEventListener('click', () => {
      console.log('[MainManager] Theme button clicked (stub)');
    });

    this.elements.sidebarProfileBtn?.addEventListener('click', () => {
      console.log('[MainManager] Profile button clicked (stub)');
      // Load profile manager if accessible
      if (canAccessManager(2)) {
        this.loadManager(2);
      }
    });

    this.elements.sidebarHelpBtn?.addEventListener('click', () => {
      console.log('[MainManager] Help button clicked (stub)');
    });

    // Sidebar logout button (icon-only in footer)
    this.elements.sidebarLogoutBtn?.addEventListener('click', () => {
      this.showLogoutPanel();
    });

    // Legacy logout button (if still in template)
    this.elements.logoutBtn?.addEventListener('click', () => {
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
        const logoutType = option.dataset.logoutType;
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

    // Theme button in header (stub)
    this.elements.themeBtn?.addEventListener('click', () => {
      console.log('[MainManager] Theme toggle clicked (stub)');
    });

    // Profile button in header (stub)
    this.elements.profileBtn?.addEventListener('click', () => {
      console.log('[MainManager] Profile button clicked (stub)');
      // In the future, this could open a dropdown or navigate to profile
    });

    // Network status events
    eventBus.on(Events.NETWORK_ONLINE, () => {
      this.elements.offlineIndicator?.classList.remove('visible');
    });

    eventBus.on(Events.NETWORK_OFFLINE, () => {
      this.elements.offlineIndicator?.classList.add('visible');
    });

    // Handle window resize to reset sidebar on mobile/desktop transition
    window.addEventListener('resize', () => {
      this.handleWindowResize();
    });
  }

  /**
   * Set up the resizable splitter
   */
  setupSplitter() {
    if (!this.elements.sidebarSplitter) return;

    // Mouse events
    this.elements.sidebarSplitter.addEventListener('mousedown', this.handleSplitterMouseDown);
    
    // Touch events for mobile
    this.elements.sidebarSplitter.addEventListener('touchstart', this.handleSplitterTouchStart, { passive: false });
  }

  /**
   * Handle splitter mouse down
   */
  handleSplitterMouseDown(event) {
    if (this.isSidebarCollapsed) return;
    
    event.preventDefault();
    this.isResizing = true;
    this.elements.sidebarSplitter.classList.add('resizing');
    document.body.classList.add('resizing');
    
    document.addEventListener('mousemove', this.handleSplitterMouseMove);
    document.addEventListener('mouseup', this.handleSplitterMouseUp);
  }

  /**
   * Handle splitter mouse move
   */
  handleSplitterMouseMove(event) {
    if (!this.isResizing) return;
    
    const newWidth = event.clientX;
    this.setSidebarWidth(newWidth);
  }

  /**
   * Handle splitter mouse up
   */
  handleSplitterMouseUp() {
    this.isResizing = false;
    this.elements.sidebarSplitter?.classList.remove('resizing');
    document.body.classList.remove('resizing');
    
    document.removeEventListener('mousemove', this.handleSplitterMouseMove);
    document.removeEventListener('mouseup', this.handleSplitterMouseUp);
    
    // Save the current width
    this.saveSidebarState();
  }

  /**
   * Handle splitter touch start
   */
  handleSplitterTouchStart(event) {
    if (this.isSidebarCollapsed) return;
    
    event.preventDefault();
    this.isResizing = true;
    this.elements.sidebarSplitter.classList.add('resizing');
    document.body.classList.add('resizing');
    
    document.addEventListener('touchmove', this.handleSplitterTouchMove, { passive: false });
    document.addEventListener('touchend', this.handleSplitterTouchEnd);
    document.addEventListener('touchcancel', this.handleSplitterTouchEnd);
  }

  /**
   * Handle splitter touch move
   */
  handleSplitterTouchMove(event) {
    if (!this.isResizing) return;
    
    event.preventDefault();
    const touch = event.touches[0];
    const newWidth = touch.clientX;
    this.setSidebarWidth(newWidth);
  }

  /**
   * Handle splitter touch end
   */
  handleSplitterTouchEnd() {
    this.isResizing = false;
    this.elements.sidebarSplitter?.classList.remove('resizing');
    document.body.classList.remove('resizing');
    
    document.removeEventListener('touchmove', this.handleSplitterTouchMove);
    document.removeEventListener('touchend', this.handleSplitterTouchEnd);
    document.removeEventListener('touchcancel', this.handleSplitterTouchEnd);
    
    // Save the current width
    this.saveSidebarState();
  }

  /**
   * Set sidebar width with constraints
   */
  setSidebarWidth(width) {
    // Constrain width to valid range
    const constrainedWidth = Math.max(MIN_SIDEBAR_WIDTH, Math.min(MAX_SIDEBAR_WIDTH, width));
    
    if (this.elements.sidebar) {
      this.elements.sidebar.style.width = `${constrainedWidth}px`;
    }
  }

  /**
   * Toggle sidebar collapse/expand
   */
  toggleSidebarCollapse() {
    this.isSidebarCollapsed = !this.isSidebarCollapsed;
    
    if (this.elements.sidebar) {
      if (this.isSidebarCollapsed) {
        // Store current width before collapsing
        this.expandedWidth = this.elements.sidebar.offsetWidth;
        this.elements.sidebar.classList.add('collapsed');
      } else {
        this.elements.sidebar.classList.remove('collapsed');
        // Restore width after expanding
        if (this.expandedWidth) {
          this.setSidebarWidth(this.expandedWidth);
        }
      }
    }
    
    this.saveSidebarState();
  }

  /**
   * Save sidebar state to localStorage
   */
  saveSidebarState() {
    try {
      localStorage.setItem(SIDEBAR_COLLAPSED_KEY, JSON.stringify(this.isSidebarCollapsed));
      
      if (!this.isSidebarCollapsed && this.elements.sidebar) {
        const currentWidth = this.elements.sidebar.offsetWidth;
        localStorage.setItem(SIDEBAR_WIDTH_KEY, String(currentWidth));
      }
    } catch (error) {
      console.warn('[MainManager] Failed to save sidebar state:', error);
    }
  }

  /**
   * Load sidebar state from localStorage
   */
  loadSidebarState() {
    try {
      // Load collapsed state
      const collapsedStored = localStorage.getItem(SIDEBAR_COLLAPSED_KEY);
      if (collapsedStored) {
        this.isSidebarCollapsed = JSON.parse(collapsedStored);
      }
      
      // Load width
      const widthStored = localStorage.getItem(SIDEBAR_WIDTH_KEY);
      if (widthStored) {
        this.expandedWidth = parseInt(widthStored, 10);
      }
      
      // Apply state
      if (this.elements.sidebar) {
        if (this.isSidebarCollapsed) {
          this.elements.sidebar.classList.add('collapsed');
        } else if (this.expandedWidth) {
          this.setSidebarWidth(this.expandedWidth);
        }
      }
    } catch (error) {
      console.warn('[MainManager] Failed to load sidebar state:', error);
    }
  }

  /**
   * Update the version info box with build and timestamp data
   * Displays: build, YYYY, MMDD, HHMM
   */
  updateVersionBox() {
    if (!this.elements.versionBuild || !this.app?.build) return;

    const build = this.app.build;
    const timestamp = this.app.timestamp || new Date().toISOString();

    // Parse the timestamp
    const date = new Date(timestamp);
    const year = date.getFullYear();
    // Month is 0-indexed, so add 1 and pad with leading zero
    const month = String(date.getMonth() + 1).padStart(2, '0');
    const day = String(date.getDate()).padStart(2, '0');
    const hours = String(date.getHours()).padStart(2, '0');
    const minutes = String(date.getMinutes()).padStart(2, '0');

    // Update the version box elements
    this.elements.versionBuild.textContent = build;
    this.elements.versionYear.textContent = year;
    this.elements.versionDate.textContent = month + day;
    this.elements.versionTime.textContent = hours + minutes;
  }

  /**
   * Handle window resize
   */
  handleWindowResize() {
    // On mobile, always reset to expanded state for the slide-out menu
    if (window.innerWidth <= 768) {
      if (this.elements.sidebar) {
        this.elements.sidebar.classList.remove('collapsed');
      }
    } else {
      // On desktop, restore collapsed state if needed
      if (this.isSidebarCollapsed && this.elements.sidebar) {
        this.elements.sidebar.classList.add('collapsed');
      }
    }
  }

  /**
   * Set up global keyboard shortcuts
   * Ctrl+Shift+L - Open logout panel
   */
  setupGlobalKeyboardShortcuts() {
    document.addEventListener('keydown', this.handleGlobalKeyDown);
  }

  /**
   * Handle global keyboard shortcuts
   */
  handleGlobalKeyDown(event) {
    // Ctrl+Shift+L or Cmd+Shift+L to open logout panel
    if ((event.ctrlKey || event.metaKey) && event.shiftKey && event.key === 'L') {
      event.preventDefault();
      if (!this.isLogoutPanelVisible) {
        this.showLogoutPanel();
      }
    }
  }

  /**
   * Load user info from JWT claims
   */
  loadUserInfo() {
    const claims = getClaims();
    if (claims) {
      this.user = {
        id: claims.user_id,
        username: claims.username || 'User',
        email: claims.email,
        roles: claims.roles || [],
      };

      // Update profile display
      if (this.elements.profileName) {
        this.elements.profileName.textContent = this.user.username;
      }
      if (this.elements.profileAvatar) {
        this.elements.profileAvatar.textContent = this.user.username.charAt(0).toUpperCase();
      }
      if (this.elements.profileRole) {
        const role = Array.isArray(this.user.roles) && this.user.roles.length > 0
          ? this.user.roles[0]
          : 'User';
        this.elements.profileRole.textContent = role;
      }
    }
  }

  /**
   * Build sidebar menu from permitted managers
   */
  buildSidebar() {
    const menu = this.elements.sidebarMenu;
    if (!menu) return;

    menu.innerHTML = '';

    this.permittedManagers.forEach((managerId) => {
      if (!canAccessManager(managerId)) return;

      const managerInfo = this.managerIcons[managerId] || { icon: 'fa-cube', name: `Manager ${managerId}` };

      const button = document.createElement('div');
      button.className = 'menu-item';
      button.dataset.managerId = managerId;
      button.innerHTML = `
        <fa ${managerInfo.icon}></fa>
        <span>${managerInfo.name}</span>
      `;

      button.addEventListener('click', () => {
        this.loadManager(managerId);
        this.closeMobileSidebar();
      });

      menu.appendChild(button);
    });
  }

  /**
   * Load a manager into the workspace
   */
  async loadManager(managerId) {
    if (this.currentManagerId === managerId) return;

    // Update active state in sidebar
    this.updateActiveMenuItem(managerId);

    // Update header title
    const managerInfo = this.managerIcons[managerId];
    if (managerInfo && this.elements.headerTitle) {
      this.elements.headerTitle.textContent = managerInfo.name;
    }

    // Delegate to app to load the manager
    await this.app.loadManager(managerId);
    this.currentManagerId = managerId;
  }

  /**
   * Update active state in sidebar menu
   */
  updateActiveMenuItem(managerId) {
    const items = this.elements.sidebarMenu?.querySelectorAll('.menu-item');
    items?.forEach((item) => {
      if (parseInt(item.dataset.managerId, 10) === managerId) {
        item.classList.add('active');
      } else {
        item.classList.remove('active');
      }
    });
  }

  /**
   * Open mobile sidebar
   */
  openMobileSidebar() {
    this.elements.sidebar?.classList.add('open');
    this.elements.sidebarOverlay?.classList.add('visible');
  }

  /**
   * Close mobile sidebar
   */
  closeMobileSidebar() {
    this.elements.sidebar?.classList.remove('open');
    this.elements.sidebarOverlay?.classList.remove('visible');
  }

  /**
   * Show the main layout
   */
  show() {
    requestAnimationFrame(() => {
      this.elements.layout?.classList.add('visible');
    });
  }

  /**
   * Show the logout panel with overlay
   */
  showLogoutPanel() {
    if (this.isLogoutPanelVisible) return;

    this.isLogoutPanelVisible = true;

    // Show overlay and panel by adding visible class
    // CSS handles the opacity/visibility transition
    this.elements.logoutOverlay?.classList.add('visible');
    this.elements.logoutPanel?.classList.add('visible');

    // Add keyboard listener
    document.addEventListener('keydown', this.handleKeyDown);

    // Focus the first logout option for accessibility after transition
    setTimeout(() => {
      const firstOption = this.elements.logoutOptions?.[0];
      firstOption?.focus();
    }, 300);
  }

  /**
   * Hide the logout panel
   */
  hideLogoutPanel() {
    if (!this.isLogoutPanelVisible) return;

    this.isLogoutPanelVisible = false;

    // Hide overlay and panel by removing visible class
    // CSS handles the opacity/visibility transition
    this.elements.logoutOverlay?.classList.remove('visible');
    this.elements.logoutPanel?.classList.remove('visible');

    // Remove keyboard listener
    document.removeEventListener('keydown', this.handleKeyDown);

    // Return focus to logout button after transition completes
    setTimeout(() => {
      this.elements.sidebarLogoutBtn?.focus();
    }, 300);
  }

  /**
   * Handle keyboard shortcuts for logout panel
   * ESC - cancel, Enter - quick logout, 1-4 - select option
   */
  handleKeyDown(event) {
    if (!this.isLogoutPanelVisible) return;

    switch (event.key) {
      case 'Escape':
        event.preventDefault();
        this.hideLogoutPanel();
        break;

      case 'Enter':
        event.preventDefault();
        this.executeLogout('quick');
        break;

      case '1':
        event.preventDefault();
        this.executeLogout('quick');
        break;

      case '2':
        event.preventDefault();
        this.executeLogout('normal');
        break;

      case '3':
        event.preventDefault();
        this.executeLogout('public');
        break;

      case '4':
        event.preventDefault();
        this.executeLogout('global');
        break;

      default:
        break;
    }
  }

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
  }

  /**
   * Teardown the main manager
   */
  teardown() {
    // Remove keyboard listeners if still active
    document.removeEventListener('keydown', this.handleKeyDown);
    document.removeEventListener('keydown', this.handleGlobalKeyDown);
    
    // Remove splitter event listeners
    if (this.elements.sidebarSplitter) {
      this.elements.sidebarSplitter.removeEventListener('mousedown', this.handleSplitterMouseDown);
      this.elements.sidebarSplitter.removeEventListener('touchstart', this.handleSplitterTouchStart);
    }
    
    // Remove document-level event listeners
    document.removeEventListener('mousemove', this.handleSplitterMouseMove);
    document.removeEventListener('mouseup', this.handleSplitterMouseUp);
    document.removeEventListener('touchmove', this.handleSplitterTouchMove);
    document.removeEventListener('touchend', this.handleSplitterTouchEnd);
    document.removeEventListener('touchcancel', this.handleSplitterTouchEnd);

    // Remove body-level logout elements (moved to body during render)
    this.elements.logoutOverlay?.remove();
    this.elements.logoutPanel?.remove();

    // Clean up event listeners if needed
    this.elements = {};
  }
}
