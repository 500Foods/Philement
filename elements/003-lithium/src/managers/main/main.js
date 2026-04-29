/**
 * Main Menu Manager
 * 
 * Handles the main application layout: resizable sidebar + manager area.
 * The manager area uses "slots" — each loaded manager gets its own slot
 * (header + workspace + footer) that crossfades as a unit when switching.
 *
 * Sidebar features:
 *   - Gradient header matching login panel
 *   - Scrollable menu with collapsible groups
 *   - Icon-only footer with collapse/expand button
 *   - Resizable width (220px-400px) with splitter
 *   - Width persisted to localStorage
 *
 * Manager slot:
 *   - Header: unified subpanel-header-group (icon+name on left, xmark close on right)
 *   - Workspace: manager content (padding, scrollable)
 *   - Footer: reports placeholder (left) + action icons: Annotations, Tours, LMS (right)
 */

// CSS imports
import '../../core/manager-ui.css';
import '../../core/popup.css';
import './main.css';
import '../../managers/tour/tour.js';  // Tour event listeners (imports tour.css)

// Import method modules
import { slotMethods, buildSlotHTML } from './main-slots.js';
import { sidebarMethods } from './main-sidebar.js';
import { collapseMethods } from './main-collapse.js';
import { splitterMethods } from './main-splitter.js';
import { keyboardMethods } from './main-keyboard.js';
import { logoutMethods } from './main-logout.js';
import { websocketMethods } from './main-websocket.js';
import { stateMethods } from './main-state.js';

import { initRadar, destroyRadar } from '../../shared/radar-controller.js';
import { initTooltips } from '../../core/tooltip-api.js';
import { log, Subsystems, Status } from '../../core/log.js';

// Sidebar constraints
const MIN_SIDEBAR_WIDTH = 220;
const MAX_SIDEBAR_WIDTH = 400;
const DEFAULT_SIDEBAR_WIDTH = 250;

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
    this.currentUtilityKey = null;
    this.user = null;
    this.isLogoutPanelVisible = false;
    this.selectedLogoutIndex = 0;
    this.isSidebarCollapsed = false;
    this.isResizing = false;
    this._stageTimer = null;
    this._arrowRotation = 0;
    this._isAnimating = false;
    this._isRadarInSidebar = false;
    
    // Menu data
    this.menuData = null;
    this.collapsedGroups = new Set();
    
    // Bind event handlers
    this.handleKeyDown = this.handleKeyDown.bind(this);
    this.handleGlobalKeyDown = this.handleGlobalKeyDown.bind(this);
    this.handleSplitterMouseDown = this.handleSplitterMouseDown.bind(this);
    this.handleSplitterMouseMove = this.handleSplitterMouseMove.bind(this);
    this.handleSplitterMouseUp = this.handleSplitterMouseUp.bind(this);
    this.handleSplitterTouchStart = this.handleSplitterTouchStart.bind(this);
    this.handleSplitterTouchMove = this.handleSplitterTouchMove.bind(this);
    this.handleSplitterTouchEnd = this.handleSplitterTouchEnd.bind(this);

    // Manager registry with icons (populated dynamically from menu data)
    this.managerIcons = {};

    // Flat list of accessible manager names for Crimson context.
    // Built once during menu load from the final filtered menu data,
    // so it reflects exactly what the user sees in the sidebar.
    this.accessibleManagerNames = [];

    // Utility manager registry with icons
    this.utilityManagerIcons = {
      'session-log': { iconHtml: '<fa fa-receipt></fa>', name: 'Session Log' },
      'user-profile': { iconHtml: '<fa fa-user-cog></fa>', name: 'User Profile' },
    };
    
    // Load collapsed groups from localStorage
    this._loadCollapsedGroups();
  }

  /**
   * Initialize the main manager
   * @param {Object} options - Initialization options
   * @param {boolean} options.skipShowAnimation - If true, skip the show animation (used during crossfade)
   */
  async init(options = {}) {
    try {
      await this.render();
      this.setupEventListeners();
      this.setupGlobalKeyboardShortcuts();
      this.setupSplitter();
      this.loadUserInfo();
      
      // Fetch menu data before building sidebar
      await this.loadMenuData();
      
      this.buildSidebar();
      this.loadSidebarState();
      
      // Initialize the radar status icon
      initRadar();

      // Only run show animation if not skipping (e.g., during crossfade transition)
      if (!options.skipShowAnimation) {
        this.show();
      }
      // When skipping animation, do nothing - layout stays at CSS default (opacity: 0)
      // _crossfadeToMainUI() will handle the fade-in

      // Load last-used manager (from localStorage), or default to user profile
      // This will also collapse all groups except the one containing the initial manager
      await this._loadInitialManager();

      // Initialize the app-wide WebSocket connection
      await this.initWebSocket();
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
      sidebarIconGroup: this.container.querySelector('#sidebar-icon-group'),
      sidebarCollapseBtn: this.container.querySelector('#sidebar-collapse-btn'),
      collapseIcon: this.container.querySelector('#collapse-icon'),
      sidebarThemeBtn: this.container.querySelector('#sidebar-theme-btn'),
      sidebarProfileBtn: this.container.querySelector('#sidebar-profile-btn'),
      sidebarLogsBtn: this.container.querySelector('#sidebar-logs-btn'),
      sidebarLogoutBtn: this.container.querySelector('#sidebar-logout-btn'),
      sidebarOverlay: this.container.querySelector('#sidebar-overlay'),
      mobileMenuBtn: this.container.querySelector('#mobile-menu-btn'),
      managerArea: this.container.querySelector('#manager-area'),
      logoutPanel: this.container.querySelector('#logout-panel'),
      logoutOverlay: this.container.querySelector('#logout-overlay'),
      logoutCloseBtn: this.container.querySelector('#logout-close-btn'),
      logoutOptions: this.container.querySelectorAll('.logout-option'),
      radarIcon: this.container.querySelector('#radar-icon'),
      radarIconSidebar: this.container.querySelector('#radar-icon-sidebar'),
      sidebarRadarContainer: this.container.querySelector('#sidebar-radar-container'),
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

    // Initialize tooltips on radar icons
    if (this.elements.radarIcon) {
      initTooltips(this.elements.radarIcon);
    }
    if (this.elements.radarIconSidebar) {
      initTooltips(this.elements.radarIconSidebar);
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
        <div class="manager-area" id="manager-area">
          <!-- Manager slots will be injected here -->
        </div>
      </div>
    `;
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
   * Teardown the main manager
   */
  teardown() {
    // Clear animation timers
    clearTimeout(this._stageTimer);
    this._isAnimating = false;

    // Clean up radar controller
    destroyRadar();

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

// Mix in methods from separate modules
Object.assign(MainManager.prototype, slotMethods);
Object.assign(MainManager.prototype, sidebarMethods);
Object.assign(MainManager.prototype, collapseMethods);
Object.assign(MainManager.prototype, splitterMethods);
Object.assign(MainManager.prototype, keyboardMethods);
Object.assign(MainManager.prototype, logoutMethods);
Object.assign(MainManager.prototype, websocketMethods);
Object.assign(MainManager.prototype, stateMethods);
