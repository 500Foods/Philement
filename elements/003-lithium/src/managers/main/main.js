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

import { eventBus, Events } from '../../core/event-bus.js';
import { getClaims } from '../../core/jwt.js';
import { getPermittedManagers, canAccessManager } from '../../core/permissions.js';
import { setIcon, processIcons } from '../../core/icons.js';
import { getMenu, buildManagerIconsRegistry, clearMenuCache } from '../../shared/menu.js';
import { getTip, initTooltips } from '../../core/tooltip-api.js';
import { log, Subsystems, Status } from '../../core/log.js';
import {
  initZoom,
  hideZoomPopup,
  toggleZoomPopup,
} from '../../core/zoom-control.js';
import { initCrimsonShortcut, toggleCrimson } from '../../managers/crimson/crimson.js';
import {
  setupHeaderButtons,
  setupFooterButtons,
  registerShortcut,
  unregisterManagerShortcuts,
} from '../../core/manager-ui.js';
import { getAppWS, ConnectionState } from '../../shared/app-ws.js';
import { initRadar, wsConnected, wsFlaky, wsDisconnected, onHeartbeat, destroyRadar } from '../../shared/radar-controller.js';
import '../../core/manager-ui.css';
import './main.css';

// localStorage keys for sidebar state
const SIDEBAR_WIDTH_KEY = 'lithium_sidebar_width';
const SIDEBAR_COLLAPSED_KEY = 'lithium_sidebar_collapsed';
const LAST_MANAGER_KEY = 'lithium_last_manager';
const COLLAPSED_GROUPS_KEY = 'lithium_collapsed_groups';

// Sidebar constraints
const MIN_SIDEBAR_WIDTH = 220;
const MAX_SIDEBAR_WIDTH = 400;
const DEFAULT_SIDEBAR_WIDTH = 260;

// Icon group sizing (matches CSS --icon-w, --icon-h, --icon-gap)
const ICON_W = 40;
const ICON_H = 35;
const ICON_GAP = 2;
const STEP_X = ICON_W + ICON_GAP;   // 42px — horizontal step
const STEP_Y = ICON_H + ICON_GAP;   // 37px — vertical step

/**
 * Build the HTML for a manager slot.
 *
 * Header and footer each use a single unified subpanel-header-group so that
 * any buttons added by the manager via addHeaderButtons() / addFooterButtons()
 * merge seamlessly into the same visually connected button strip — no breaks.
 *
 * Buttons are inserted directly into the group via insertBefore() — the
 * slot-header-extras / slot-footer-*-tras divs are kept as HTML markers
 * only and are NOT used as the injection target (display:contents divs can
 * hide injected content in some browsers).
 *
 * Header (right-side): keyboard shortcuts, zoom (aperture), fullscreen, close
 * Footer (right-side): crimson, notifications, concierge, annotations, tours, training
 *
 * @param {string} slotId - The slot's DOM id
 * @param {string} icon - FA icon class e.g. 'fa-palette'
 * @param {string} name - Manager display name
 * @returns {string} HTML string
 */
function buildSlotHTML(slotId, icon, name) {
  return `
<div class="manager-slot" id="${slotId}">

  <!-- Slot Header: title button + placeholder + search + placeholder + fixed buttons (toolbox, keyboard, zoom, fullscreen, close) -->
  <div class="manager-slot-header">
    <div class="subpanel-header-group" style="flex:1;min-width:0;">
      <!-- Title button with icon and name -->
      <button type="button" class="subpanel-header-btn subpanel-header-primary slot-title-btn">
        <span class="slot-icon">${icon}</span>
        <span class="slot-name">${name}</span>
      </button>
      <!-- Placeholder: fills space between title and search -->
      <button type="button" class="subpanel-header-btn slot-header-placeholder slot-header-flex"></button>
      <!-- Search section -->
      <div class="slot-header-search slot-header-flex">
        <button type="button" class="slot-header-search-btn" title="Search">
          <fa fa-magnifying-glass></fa>
        </button>
        <input type="text" class="slot-header-search-input" placeholder="Search Lithium...">
        <button type="button" class="slot-header-search-clear" title="Clear">
          <fa fa-xmark></fa>
        </button>
      </div>
      <!-- Placeholder: fills space between search and header buttons -->
      <button type="button" class="subpanel-header-btn slot-header-placeholder slot-header-flex"></button>
      <!-- Fixed header buttons are added after placeholder by setupHeaderButtons() -->
    </div>
  </div>

  <!-- Slot Workspace: manager content renders here -->
  <div class="manager-slot-workspace" id="${slotId}-workspace">
  </div>

  <!-- Slot Footer: placeholder + fixed buttons (Crimson, Notifications, etc.)
       Manager buttons are added before placeholder by each manager -->
  <div class="manager-slot-footer">
    <div class="subpanel-header-group" style="flex:1;min-width:0;">
      <!-- Placeholder: empty button that fills space between left and right buttons -->
      <button type="button" class="subpanel-header-btn slot-footer-placeholder" style="flex:1;min-width:0;"></button>
      <!-- Fixed buttons are added after placeholder by setupFooterButtons() -->
    </div>
  </div>

</div>
`.trim();
}

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
    this.isSidebarCollapsed = false;
    this.isResizing = false;
    this._stageTimer = null;
    this._arrowRotation = 0;
    this._isAnimating = false;
    
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

    // Flat list of accessible manager names for Crimson context packet.
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
   * Load collapsed group states from localStorage
   */
  _loadCollapsedGroups() {
    try {
      const stored = localStorage.getItem(COLLAPSED_GROUPS_KEY);
      if (stored) {
        const groupIds = JSON.parse(stored);
        this.collapsedGroups = new Set(groupIds);
      }
    } catch (e) {
      this.collapsedGroups = new Set();
    }
  }

  /**
   * Save collapsed group states to localStorage
   */
  _saveCollapsedGroups() {
    try {
      localStorage.setItem(COLLAPSED_GROUPS_KEY, JSON.stringify([...this.collapsedGroups]));
    } catch (e) {
      // Non-fatal
    }
  }

  /**
   * Toggle a group's collapsed state
   * @param {number} groupId - The group ID
   */
  _toggleGroup(groupId) {
    if (this.collapsedGroups.has(groupId)) {
      this.collapsedGroups.delete(groupId);
    } else {
      this.collapsedGroups.add(groupId);
    }
    this._saveCollapsedGroups();
    this._updateGroupVisibility(groupId);
  }

  /**
   * Update a group's visibility in the DOM
   * @param {number} groupId - The group ID
   */
  _updateGroupVisibility(groupId) {
    const groupEl = this.elements.sidebarMenu?.querySelector(`[data-group-id="${groupId}"]`);
    if (!groupEl) return;

    const itemsContainer = groupEl.querySelector('.menu-group-items');
    const toggleIcon = groupEl.querySelector('.group-toggle-icon');
    const isCollapsed = this.collapsedGroups.has(groupId);

    if (itemsContainer) {
      itemsContainer.classList.toggle('collapsed', isCollapsed);
    }
    if (toggleIcon) {
      toggleIcon.style.transform = isCollapsed ? 'rotate(-90deg)' : 'rotate(0deg)';
    }
    groupEl.classList.toggle('is-collapsed', isCollapsed);
  }

  /**
   * Build the slot id for a menu manager
   */
  _slotId(managerId) {
    return `manager-slot-${managerId}`;
  }

  /**
   * Build the slot id for a utility manager
   */
  _utilitySlotId(managerKey) {
    return `utility-slot-${managerKey}`;
  }

  /**
   * Create a manager slot in the manager-area and return its workspace element.
   * The slot starts hidden.
   * @param {string} slotId
   * @param {string} icon
   * @param {string} name
   * @param {string|number} managerId - The manager ID for tours/training context
   * @returns {{ slotEl: HTMLElement, workspaceEl: HTMLElement }}
   */
  createSlot(slotId, icon, name, managerId) {
    const area = this.elements.managerArea;
    if (!area) return null;

    // Only create if not already existing
    if (area.querySelector(`#${slotId}`)) {
      return {
        slotEl: area.querySelector(`#${slotId}`),
        workspaceEl: area.querySelector(`#${slotId}-workspace`),
      };
    }

    const wrapper = document.createElement('div');
    wrapper.innerHTML = buildSlotHTML(slotId, icon, name);
    const slotEl = wrapper.firstElementChild;
    area.appendChild(slotEl);

    // Process FA icons inside the new slot
    processIcons(slotEl);

    // Setup search clear button to clear the search input
    const searchInput = slotEl.querySelector('.slot-header-search-input');
    const searchClearBtn = slotEl.querySelector('.slot-header-search-clear');
    if (searchInput && searchClearBtn) {
      searchClearBtn.addEventListener('click', () => {
        searchInput.value = '';
        searchInput.focus();
      });
    }

    // Setup header buttons using manager-ui
    const headerGroup = slotEl.querySelector('.manager-slot-header .subpanel-header-group');
    setupHeaderButtons(slotId, {
      showKeyboard: true,
      showZoom: true,
      showFullscreen: true,
      showClose: true,
      onClose: () => this.handleCloseSlot(slotId),
      group: headerGroup,
    });

    // Setup footer buttons using manager-ui
    const footerGroup = slotEl.querySelector('.manager-slot-footer .subpanel-header-group');
    const placeholder = slotEl.querySelector('.manager-slot-footer .slot-footer-placeholder');
    setupFooterButtons(slotId, managerId, {
      showCrimson: true,
      showNotifications: true,
      showConcierge: true,
      showAnnotations: true,
      showTours: true,
      showTraining: true,
      group: footerGroup,
      afterPlaceholder: placeholder, // Insert fixed buttons after placeholder
    });

    // Initialize zoom on first slot creation
    initZoom();

    // Initialize Crimson shortcut
    initCrimsonShortcut();

    return {
      slotEl,
      workspaceEl: slotEl.querySelector(`#${slotId}-workspace`),
    };
  }

  /**
   * Handle closing a manager slot.
   * Switches to the previously open manager, or profile manager if none.
   * @param {string} slotId - The slot ID to close
   */
  handleCloseSlot(slotId) {
    // Parse manager ID from slot ID
    const isUtility = slotId.startsWith('utility-slot-');
    const managerId = isUtility
      ? slotId.replace('utility-slot-', '')
      : parseInt(slotId.replace('manager-slot-', ''), 10);

    log(Subsystems.MANAGER, Status.INFO, `[MainManager] Closing slot: ${slotId}`);

    // Unregister any shortcuts for this manager
    if (!isUtility) {
      unregisterManagerShortcuts(String(managerId));
    }

    // Hide the slot
    const slotEl = document.getElementById(slotId);
    if (slotEl) {
      slotEl.classList.remove('visible');
    }

    // Try to find a previously open manager to switch to
    const previousManager = this._findPreviousManager(slotId);
    if (previousManager) {
      if (previousManager.type === 'utility') {
        this.app.loadUtilityManager(previousManager.id);
      } else {
        this.loadManager(previousManager.id);
      }
    } else {
      // Default to profile manager
      this.app.loadUtilityManager('user-profile');
    }
  }

  /**
   * Find a previously open manager to switch to after closing.
   * @param {string} excludeSlotId - The slot ID being closed (to exclude)
   * @returns {{type: string, id: string|number}|null}
   */
  _findPreviousManager(excludeSlotId) {
    const area = this.elements.managerArea;
    if (!area) return null;

    // Find all visible slots except the one being closed
    const visibleSlots = Array.from(area.querySelectorAll('.manager-slot.visible'))
      .filter(slot => slot.id !== excludeSlotId);

    if (visibleSlots.length === 0) return null;

    // Get the most recently visible slot
    const lastSlot = visibleSlots[visibleSlots.length - 1];
    const slotId = lastSlot.id;

    if (slotId.startsWith('utility-slot-')) {
      return { type: 'utility', id: slotId.replace('utility-slot-', '') };
    } else {
      const managerId = parseInt(slotId.replace('manager-slot-', ''), 10);
      return { type: 'manager', id: managerId };
    }
  }

  /**
   * Inject buttons into the header of a manager slot.
   *
   * Buttons are inserted directly into the slot's `subpanel-header-group`,
   * immediately before the close button, so they appear between the title
   * and the close in the unified button strip.  All injected buttons inherit
   * the same `.subpanel-header-close` styling — no visual break.
   *
   * Each button object must have:
   *   { el: HTMLButtonElement }
   *
   * OR the shorthand form:
   *   { icon: 'fa-rotate', title: 'Refresh', onClick: fn, id: 'optional-id' }
   *
   * @param {string} slotId        - The slot id (from _slotId() or _utilitySlotId())
   * @param {Array}  buttonDefs    - Array of button descriptors (see above)
   */
  addHeaderButtons(slotId, buttonDefs) {
    const area = this.elements.managerArea;
    if (!area) return;
    const slot = area.querySelector(`#${slotId}`);
    if (!slot) return;

    // Inject buttons directly into the subpanel-header-group, before the close
    // button.  We do NOT use the display:contents wrapper — inserting into it
    // causes the buttons to be invisible in some browsers even though they are
    // present in the DOM.  Direct insertion into the flex parent is reliable.
    const group = slot.querySelector('.manager-slot-header .subpanel-header-group');
    const closeBtn = slot.querySelector('.slot-close-btn');
    if (!group) return;

    for (const def of buttonDefs) {
      const el = def.el ?? (() => {
        const btn = document.createElement('button');
        btn.type = 'button';
        btn.className = 'subpanel-header-btn subpanel-header-close';
        if (def.id)      btn.id = def.id;
        if (def.title)   btn.dataset.tooltip = def.title;
        if (def.tooltip) btn.dataset.tooltip = def.tooltip;
        btn.innerHTML = `<fa ${def.icon}></fa>`;
        if (def.onClick) btn.addEventListener('click', def.onClick);
        return btn;
      })();

      // Insert before the close button so the close stays on the far right.
      // If there is no close button fall back to appending.
      if (closeBtn) {
        group.insertBefore(el, closeBtn);
      } else {
        group.appendChild(el);
      }
    }

    // Convert any newly injected <fa> tags to Font Awesome <i> elements.
    processIcons(group);
    initTooltips(group);
  }

  /**
   * Inject buttons into the footer of a manager slot.
   *
   * The footer uses a single unified subpanel-header-group.  Two named
   * insertion points are provided so managers can add buttons on either side
   * of the fixed action icons:
   *
   *   'left'  → .slot-footer-left-extras  (between Reports btn and right extras)
   *   'right' → .slot-footer-right-extras (between left extras and fixed icons)
   *
   * Button descriptor format is identical to addHeaderButtons().
   *
   * @param {string} slotId     - The slot id
   * @param {string} side       - 'left' or 'right'
   * @param {Array}  buttonDefs - Array of button descriptors
   */
  addFooterButtons(slotId, side, buttonDefs) {
    const area = this.elements.managerArea;
    if (!area) return;
    const slot = area.querySelector(`#${slotId}`);
    if (!slot) return;

    // Inject buttons directly into the footer subpanel-header-group.
    // For 'left': insert before the right-extras anchor (or notifications btn).
    // For 'right': insert before the notifications button.
    // Using display:contents wrapper divs caused buttons to be invisible —
    // direct insertion into the flex parent is reliable.
    const group = slot.querySelector('.manager-slot-footer .subpanel-header-group');
    if (!group) return;

    // Find the insertion anchor for this side.
    // 'left'  → insert before the first right-side fixed button (notifications)
    // 'right' → same anchor (between any left extras and fixed buttons)
    const anchor = group.querySelector('.slot-notifications-btn');

    for (const def of buttonDefs) {
      const el = def.el ?? (() => {
        const btn = document.createElement('button');
        btn.type = 'button';
        btn.className = 'subpanel-header-btn subpanel-header-close';
        if (def.id)      btn.id = def.id;
        if (def.title)   btn.dataset.tooltip = def.title;
        if (def.tooltip) btn.dataset.tooltip = def.tooltip;
        btn.innerHTML = `<fa ${def.icon}></fa>`;
        if (def.onClick) btn.addEventListener('click', def.onClick);
        return btn;
      })();

      if (anchor) {
        group.insertBefore(el, anchor);
      } else {
        group.appendChild(el);
      }
    }

    // Convert any newly injected <fa> tags to Font Awesome <i> elements.
    processIcons(group);
    initTooltips(group);
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
      } else {
        // During crossfade, make layout visible immediately (wrapper handles the animation)
        if (this.elements.layout) {
          this.elements.layout.style.transition = 'none';
          this.elements.layout.classList.add('visible');
          void this.elements.layout.offsetHeight;
          this.elements.layout.style.transition = '';
        }
      }

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
   * Load menu data from server
   */
  async loadMenuData() {
    try {
      this.menuData = await getMenu(this.app.api);

      // Build manager icons registry from menu data
      this.managerIcons = buildManagerIconsRegistry(this.menuData);

      // Build flat list of accessible manager names for Crimson context.
      // This is the final filtered list — only managers that pass config,
      // permission, and visibility filters appear in the menu.
      this.accessibleManagerNames = Object.values(this.managerIcons)
        .map(info => info?.name)
        .filter(Boolean);

      // Update permitted managers from menu data
      const menuManagerIds = Object.keys(this.managerIcons).map(id => parseInt(id, 10));

      // Filter to only include managers that are in the menu and permitted
      this.permittedManagers = this.permittedManagers.filter(id => menuManagerIds.includes(id));

      // If no permitted managers from permission system, use all menu managers
      if (this.permittedManagers.length === 0) {
        this.permittedManagers = menuManagerIds;
      }
    } catch (error) {
      console.error('[MainManager] Failed to load menu data:', error);
      // Clear cache to force fresh fetch on next load
      clearMenuCache();
      // Fall back to static manager icons
      this.managerIcons = {
        1: { iconHtml: '<fa fa-palette></fa>', name: 'Style Manager' },
        2: { iconHtml: '<fa fa-user-cog></fa>', name: 'Profile Manager' },
        3: { iconHtml: '<fa fa-chart-line></fa>', name: 'Dashboard' },
        4: { iconHtml: '<fa fa-search></fa>', name: 'Queries' },
        5: { iconHtml: '<fa fa-list-check></fa>', name: 'Lookups' },
      };
      this.accessibleManagerNames = Object.values(this.managerIcons)
        .map(info => info?.name)
        .filter(Boolean);
    }
  }

  /**
   * Get the last-used manager key from localStorage.
   * @returns {string|null} Key in format 'manager:ID' or 'utility:KEY', or null
   */
  _getLastManagerKey() {
    try {
      return localStorage.getItem(LAST_MANAGER_KEY) || null;
    } catch (e) {
      return null;
    }
  }

  /**
   * Parse a last manager key into its components.
   * @param {string} key - The last manager key (e.g., 'manager:1' or 'utility:user-profile')
   * @returns {Object|null} Parsed type and id, or null if invalid
   * @private
   */
  _parseLastManagerKey(key) {
    const colonIdx = key.indexOf(':');
    if (colonIdx <= 0) return null;
    
    const type = key.substring(0, colonIdx);
    const id = key.substring(colonIdx + 1);
    return { type, id };
  }

  /**
   * Try to load a manager by ID.
   * @param {string} id - Manager ID string
   * @returns {boolean} True if manager was loaded successfully
   * @private
   */
  _tryLoadManager(id) {
    const managerId = parseInt(id, 10);
    if (isNaN(managerId)) return false;
    if (!this.permittedManagers.includes(managerId)) return false;
    
    this.loadManager(managerId);
    return true;
  }

  /**
   * Try to load a utility manager by key.
   * @param {string} key - Utility manager key
   * @returns {boolean} True if utility manager was loaded successfully
   * @private
   */
  _tryLoadUtility(key) {
    if (!this.app.utilityManagerRegistry[key]) return false;
    
    this.app.loadUtilityManager(key);
    return true;
  }

  /**
   * Load the initial manager after login.
   * Restores the last-used manager from localStorage if available and permitted.
   * Falls back to the user profile utility manager if no stored preference
   * or the stored manager is not in the permitted list.
   * 
   * On startup, all groups are collapsed except the one containing the active manager.
   */
  async _loadInitialManager() {
    const lastManagerKey = this._getLastManagerKey();
    let loadedManagerId = null;
    let loadedUtilityKey = null;

    if (lastManagerKey) {
      const parsed = this._parseLastManagerKey(lastManagerKey);
      
      if (parsed) {
        if (parsed.type === 'manager' && this._tryLoadManager(parsed.id)) {
          loadedManagerId = parsed.id;
        } else if (parsed.type === 'utility' && this._tryLoadUtility(parsed.id)) {
          loadedUtilityKey = parsed.id;
        }
      }
    }

    // If no manager was loaded, default to user profile
    if (!loadedManagerId && !loadedUtilityKey) {
      await this.app.loadUtilityManager('user-profile');
      loadedUtilityKey = 'user-profile';
    }

    // Collapse all groups, then expand only the group containing the active menu manager
    if (loadedManagerId) {
      // Menu manager loaded - collapse all, then expand its group
      this._collapseAllGroups();
      this._expandGroupForManager(loadedManagerId);
    } else {
      // Utility manager loaded (or default) - collapse all groups
      this._collapseAllGroups();
    }
  }

  /**
   * Find which group contains a specific manager ID
   * @param {number} managerId - The manager ID to find
   * @returns {number|null} The group ID, or null if not found
   */
  _findGroupForManager(managerId) {
    if (!this.menuData || !Array.isArray(this.menuData)) {
      return null;
    }
    const targetId = Number(managerId);
    if (isNaN(targetId)) return null;

    for (const group of this.menuData) {
      const hasManager = group.items.some(item => item.managerId === targetId);
      if (hasManager) {
        return group.id;
      }
    }

    return null;
  }

  /**
   * Expand a specific group (remove from collapsed set and update UI).
   * Assumes all groups are already collapsed - this just expands one.
   * @param {number} managerId - The manager ID whose group should be expanded
   */
  _expandGroupForManager(managerId) {
    const targetGroupId = this._findGroupForManager(managerId);
    
    if (targetGroupId === null) {
      return;
    }

    // Remove the target group from collapsed set (expanding it)
    this.collapsedGroups.delete(targetGroupId);

    // Save the state
    this._saveCollapsedGroups();

    // Update the UI for the target group
    this._updateGroupVisibility(targetGroupId);
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
        <div class="manager-area" id="manager-area">
          <!-- Manager slots will be injected here -->
        </div>
      </div>
    `;
  }

  /**
   * Update the radar status icon based on WebSocket connection state
   * @param {string} state - Connection state from ConnectionState
   */
  updateWSStatus(state) {
    const radarIcon = this.elements.radarIcon;
    if (!radarIcon) return;

    switch (state) {
      case ConnectionState.CONNECTED:
        wsConnected();
        radarIcon.dataset.tooltip = 'Status: Connected';
        break;
      case ConnectionState.CONNECTING:
        wsFlaky();
        radarIcon.dataset.tooltip = 'Status: Connecting...';
        break;
      case ConnectionState.ERROR:
        wsDisconnected();
        radarIcon.dataset.tooltip = 'Status: Error';
        break;
      case ConnectionState.DISCONNECTED:
      default:
        wsDisconnected();
        radarIcon.dataset.tooltip = 'Status: Disconnected';
        break;
    }
    const tipInstance = getTip(radarIcon);
    if (tipInstance) tipInstance.updateContent(radarIcon.getAttribute('data-tooltip'));
  }

  /**
   * Flash the radar sweep line (send activity / heartbeat)
   */
  flashWSStatus() {
    onHeartbeat();
  }

  /**
   * Initialize the app-wide WebSocket connection
   * Called after successful login
   */
  async initWebSocket() {
    const ws = getAppWS({
      onStateChange: (newState, oldState) => {
        this.updateWSStatus(newState);
      },
      onSendActivity: () => {
        this.flashWSStatus();
      },
    });

    // Set initial status
    this.updateWSStatus(ws.getState());

    // Connect to the WebSocket server
    try {
      await ws.connect();
    } catch (error) {
      log(Subsystems.MANAGER, Status.WARN, `[MainManager] WebSocket connection failed: ${error.message}`);
      // Not fatal - connection will be retried when needed
    }

    return ws;
  }

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

    // Network status events
    eventBus.on(Events.NETWORK_ONLINE, () => {
      this._setOfflineIndicators(false);
    });

    eventBus.on(Events.NETWORK_OFFLINE, () => {
      this._setOfflineIndicators(true);
    });

    // Logout event from keyboard shortcut
    eventBus.on('logout:show', () => {
      this.showLogoutPanel();
    });

    // Handle window resize to reset sidebar on mobile/desktop transition
    window.addEventListener('resize', () => {
      this.handleWindowResize();
    });
  }

  /**
   * Update offline indicator on all visible slots
   */
  _setOfflineIndicators(isOffline) {
    const area = this.elements.managerArea;
    if (!area) return;
    area.querySelectorAll('.offline-indicator').forEach(el => {
      el.classList.toggle('visible', isOffline);
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
    
    // Remove width transition so resizing follows mouse immediately
    if (this.elements.sidebar) {
      this.elements.sidebar.style.transition = 'none';
    }
    
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
    
    // Restore width transition for expand/collapse button
    if (this.elements.sidebar) {
      this.elements.sidebar.style.transition = '';
    }
    
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
    
    // Remove width transition so resizing follows touch immediately
    if (this.elements.sidebar) {
      this.elements.sidebar.style.transition = 'none';
    }
    
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
    
    // Restore width transition for expand/collapse button
    if (this.elements.sidebar) {
      this.elements.sidebar.style.transition = '';
    }
    
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
   * Toggle sidebar collapse/expand with two-stage animation.
   *
   * Collapsing
   *   Stage 1 — Icons stack horizontally behind arrow, sidebar narrows,
   *             arrow rotates left → up.
   *   Stage 2 — flex-direction snaps to column, icons fan out vertically,
   *             arrow rotates up → right.
   *
   * Expanding (reverse)
   *   Stage 1 — Icons collapse vertically back to a stack,
   *             arrow rotates right → down.
   *   Stage 2 — flex-direction snaps to row, sidebar widens,
   *             icons fan out horizontally, arrow rotates down → left.
   */
  toggleSidebarCollapse() {
    if (this._isAnimating) return;

    this.isSidebarCollapsed = !this.isSidebarCollapsed;

    const iconGroup = this.elements.sidebarIconGroup;
    const sidebar   = this.elements.sidebar;
    const arrow     = this._getArrowEl();
    if (!iconGroup || !sidebar) return;

    const wrappers    = iconGroup.querySelectorAll('.sidebar-icon-wrapper');
    const halfDur     = this.getTransitionDuration() / 2;
    this._isAnimating = true;

    clearTimeout(this._stageTimer);

    if (this.isSidebarCollapsed) {
      /* ---- COLLAPSING ---- */
      this.expandedWidth = sidebar.offsetWidth;

      // Stage 1: sidebar narrows, icons stack horizontally
      iconGroup.style.transition = 'none';
      this._setWrapperTransitions(wrappers, 'none');
      wrappers.forEach(w => { w.style.flex = `0 0 ${ICON_W}px`; });
      iconGroup.style.width =
        (wrappers.length * ICON_W + (wrappers.length - 1) * ICON_GAP) + 'px';
      void iconGroup.offsetHeight;                       // flush snap

      // Re-enable transitions and animate to collapsed targets
      iconGroup.style.transition = '';
      this._setWrapperTransitions(wrappers, '');

      sidebar.classList.add('collapsed');
      iconGroup.classList.add('collapsed');

      // Clear inline overrides — CSS .collapsed provides identical values
      wrappers.forEach(w => { w.style.flex = ''; });
      iconGroup.style.width = '';

      wrappers.forEach((w, i) => {
        if (i > 0) w.style.transform = `translateX(${-i * STEP_X}px)`;
      });

      // Arrow: +90° (left → up)
      this._arrowRotation += 90;
      if (arrow) arrow.style.transform = `rotate(${this._arrowRotation}deg)`;

      // Stage 2: after stage 1 completes, fan out vertically
      this._stageTimer = setTimeout(() => {
        // Snap: disable transitions, switch to column, set stacked-Y transforms
        this._setWrapperTransitions(wrappers, 'none');
        iconGroup.classList.add('vertical');
        iconGroup.style.height = `${ICON_H}px`;            // lock height
        wrappers.forEach((w, i) => {
          w.style.transform = i > 0 ? `translateY(${-i * STEP_Y}px)` : '';
        });
        void iconGroup.offsetHeight;                         // flush

        // Animate: re-enable transitions, release height, fan out
        this._setWrapperTransitions(wrappers, '');
        iconGroup.style.height = '';                         // CSS → full vertical height
        wrappers.forEach(w => { w.style.transform = ''; }); // natural column positions

        // Arrow: +90° (up → right)
        this._arrowRotation += 90;
        const arrow2 = this._getArrowEl();
        if (arrow2) arrow2.style.transform = `rotate(${this._arrowRotation}deg)`;

        // Mark animation complete after stage 2 finishes
        this._stageTimer = setTimeout(() => {
          this._isAnimating = false;
          // Collapse all groups when sidebar is collapsed
          this._collapseAllGroups();
        }, halfDur);
      }, halfDur);

    } else {
      /* ---- EXPANDING ---- */
      
      // Expand all groups when sidebar expands
      this._expandAllGroups();

      // Stage 1: collapse vertically back to stack, full rounding
      iconGroup.classList.add('stacking');
      iconGroup.style.height = `${ICON_H}px`;              // shrink container
      wrappers.forEach((w, i) => {
        if (i > 0) w.style.transform = `translateY(${-i * STEP_Y}px)`;
      });

      // Arrow: +90° (right → down)
      this._arrowRotation += 90;
      if (arrow) arrow.style.transform = `rotate(${this._arrowRotation}deg)`;

      // Stage 2: after vertical collapse, fan out horizontally
      this._stageTimer = setTimeout(() => {
        // Snap: disable transitions, switch to row, set stacked-X transforms
        this._setWrapperTransitions(wrappers, 'none');
        iconGroup.classList.remove('vertical');
        iconGroup.classList.remove('stacking');
        iconGroup.style.height = '';                         // back to CSS row height
        wrappers.forEach((w, i) => {
          w.style.transform = i > 0 ? `translateX(${-i * STEP_X}px)` : '';
        });
        void iconGroup.offsetHeight;                         // flush

        // Animate: re-enable transitions, expand sidebar + icons
        this._setWrapperTransitions(wrappers, '');
        sidebar.classList.remove('collapsed');
        iconGroup.classList.remove('collapsed');
        wrappers.forEach(w => { w.style.transform = ''; }); // natural row positions

        if (this.expandedWidth) {
          this.setSidebarWidth(this.expandedWidth);
        }

        // Arrow: +90° (down → left)
        this._arrowRotation += 90;
        const arrow2 = this._getArrowEl();
        if (arrow2) arrow2.style.transform = `rotate(${this._arrowRotation}deg)`;

        // Mark animation complete after stage 2 finishes
        this._stageTimer = setTimeout(() => {
          this._isAnimating = false;
        }, halfDur);
      }, halfDur);
    }

    this.saveSidebarState();
  }

  /**
   * Collapse all groups and update the collapsedGroups Set
   */
  _collapseAllGroups() {
    if (!this.elements.sidebarMenu) return;
    
    // Update the Set for all groups
    this.elements.sidebarMenu.querySelectorAll('.menu-group').forEach(el => {
      const groupId = parseInt(el.dataset.groupId, 10);
      if (!isNaN(groupId)) {
        this.collapsedGroups.add(groupId);
      }
    });
    
    // Update DOM for all groups
    this.elements.sidebarMenu.querySelectorAll('.menu-group-items').forEach(el => {
      el.classList.add('collapsed');
    });
    this.elements.sidebarMenu.querySelectorAll('.group-toggle-icon').forEach(el => {
      el.style.transform = 'rotate(-90deg)';
    });
    this.elements.sidebarMenu.querySelectorAll('.menu-group').forEach(el => {
      el.classList.add('is-collapsed');
    });
  }

  /**
   * Expand all groups (for sidebar expanded mode)
   */
  _expandAllGroups() {
    if (!this.elements.sidebarMenu) return;
    
    this.elements.sidebarMenu.querySelectorAll('.menu-group-items').forEach(el => {
      const groupId = parseInt(el.closest('.menu-group')?.dataset.groupId, 10);
      el.classList.toggle('collapsed', this.collapsedGroups.has(groupId));
    });
    this.elements.sidebarMenu.querySelectorAll('.group-toggle-icon').forEach(el => {
      const groupId = parseInt(el.closest('.menu-group')?.dataset.groupId, 10);
      el.style.transform = this.collapsedGroups.has(groupId) ? 'rotate(-90deg)' : 'rotate(0deg)';
    });
    this.elements.sidebarMenu.querySelectorAll('.menu-group').forEach(el => {
      const groupId = parseInt(el.dataset.groupId, 10);
      el.classList.toggle('is-collapsed', this.collapsedGroups.has(groupId));
    });
  }

  /**
   * Get transition duration from CSS variable
   */
  getTransitionDuration() {
    const duration = getComputedStyle(document.documentElement)
      .getPropertyValue('--transition-duration')
      .trim();
    // Parse the value - could be in ms or s
    if (duration.includes('ms')) {
      return parseInt(duration, 10);
    } else if (duration.includes('s')) {
      return parseFloat(duration) * 1000;
    }
    // Default fallback
    return 2500;
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
   * Load sidebar state from localStorage.
   * Applies the final collapsed state (including .vertical) without animation.
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

      // Apply state (skip animation on initial load)
      if (this.elements.sidebar) {
        if (this.isSidebarCollapsed) {
          this.applyWithoutTransition(() => {
            this.elements.sidebar.classList.add('collapsed');
            this.elements.sidebarIconGroup?.classList.add('collapsed');
            this.elements.sidebarIconGroup?.classList.add('vertical');
            // Arrow at final collapsed position (pointing right = 180°)
            this._arrowRotation = 180;
            const arrowEl = this._getArrowEl();
            if (arrowEl) {
              arrowEl.style.transform = 'rotate(180deg)';
            }
          });
          // Collapse all groups in the menu
          this._collapseAllGroups();
        } else if (this.expandedWidth) {
          this.setSidebarWidth(this.expandedWidth);
        }
      }
    } catch (error) {
      console.warn('[MainManager] Failed to load sidebar state:', error);
    }
  }

  /**
   * Handle window resize — reset icon group state on mobile/desktop transitions
   */
  handleWindowResize() {
    if (window.innerWidth <= 768) {
      // On mobile, reset to expanded state for the slide-out overlay
      if (this.elements.sidebar) {
        this.elements.sidebar.classList.remove('collapsed');
      }
      if (this.elements.sidebarIconGroup) {
        this.elements.sidebarIconGroup.classList.remove('collapsed');
        this.elements.sidebarIconGroup.classList.remove('vertical');
        this.elements.sidebarIconGroup.style.height = '';
        this.elements.sidebarIconGroup.querySelectorAll('.sidebar-icon-wrapper').forEach(w => {
          w.style.transform = '';
        });
      }
      // Reset arrow rotation
      const arrowMobile = this._getArrowEl();
      if (arrowMobile) {
        arrowMobile.style.transform = '';
      }
    } else {
      // On desktop, restore collapsed state if needed
      if (this.isSidebarCollapsed) {
        if (this.elements.sidebar) {
          this.elements.sidebar.classList.add('collapsed');
        }
        if (this.elements.sidebarIconGroup) {
          this.elements.sidebarIconGroup.classList.add('collapsed');
          this.elements.sidebarIconGroup.classList.add('vertical');
        }
        this._arrowRotation = 180;
        const arrowDesktop = this._getArrowEl();
        if (arrowDesktop) {
          arrowDesktop.style.transform = 'rotate(180deg)';
        }
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
    }
  }

  /**
   * Build sidebar menu from menu data with collapsible groups
   */
  buildSidebar() {
    const menu = this.elements.sidebarMenu;
    if (!menu) return;

    menu.innerHTML = '';

    if (!this.menuData || this.menuData.length === 0) {
      // Fallback to static menu if no menu data
      this._buildStaticSidebar();
      return;
    }

    // Build from menu data with groups
    this.menuData.forEach((group) => {
      // Skip internal System/Internal groups (should not appear in menu)
      if (group.id === 0 || group.name === 'System' || group.name === 'Internal') return;

      // Filter items to only include permitted managers
      const permittedItems = group.items.filter(item => 
        this.permittedManagers.includes(item.managerId)
      );
      
      if (permittedItems.length === 0) return;

      const groupEl = document.createElement('div');
      groupEl.className = 'menu-group';
      groupEl.dataset.groupId = group.id;
      
      const isCollapsed = this.collapsedGroups.has(group.id);
      if (isCollapsed) {
        groupEl.classList.add('is-collapsed');
      }

      // Group header with toggle
      const headerEl = document.createElement('div');
      headerEl.className = 'menu-group-header';
      headerEl.innerHTML = `
        <span class="group-toggle-icon" style="transform: ${isCollapsed ? 'rotate(-90deg)' : 'rotate(0deg)'}">
          <fa fa-chevron-down></fa>
        </span>
        <span class="group-name">${group.name}</span>
      `;
      headerEl.addEventListener('click', () => this._toggleGroup(group.id));

      // Group items container
      const itemsEl = document.createElement('div');
      itemsEl.className = 'menu-group-items';
      if (isCollapsed || this.isSidebarCollapsed) {
        itemsEl.classList.add('collapsed');
      }

      // Build menu items
      permittedItems.forEach((item) => {
        const button = document.createElement('div');
        button.className = 'menu-item';
        button.dataset.managerId = item.managerId;
        const itemName = item.name || `Manager ${item.managerId}`;
        button.dataset.tooltip = itemName;
        // Build HTML without extra whitespace to avoid parsing issues
        let htmlContent = item.iconHtml || '<fa fa-cube></fa>';
        htmlContent += `<span class="menu-item-name">${itemName}</span>`;
        if (item.count > 0) {
          htmlContent += `<span class="menu-item-count">${item.count}</span>`;
        }
        button.innerHTML = htmlContent;

        button.addEventListener('click', () => {
          this.loadManager(item.managerId);
          this.closeMobileSidebar();
        });

        itemsEl.appendChild(button);
      });

      groupEl.appendChild(headerEl);
      groupEl.appendChild(itemsEl);
      menu.appendChild(groupEl);
    });

    // Process icons
    processIcons(menu);
    initTooltips(menu);
  }

  /**
   * Build static sidebar as fallback when menu data is unavailable
   */
  _buildStaticSidebar() {
    const menu = this.elements.sidebarMenu;
    if (!menu) return;

    menu.innerHTML = '';

    this.permittedManagers.forEach((managerId) => {
      if (!canAccessManager(managerId)) return;

      const managerInfo = this.managerIcons[managerId] || { iconHtml: '<fa fa-cube></fa>', name: `Manager ${managerId}` };

      const button = document.createElement('div');
      button.className = 'menu-item';
      button.dataset.managerId = managerId;
      button.dataset.tooltip = managerInfo.name;
      button.innerHTML = `${managerInfo.iconHtml}<span class="menu-item-name">${managerInfo.name}</span>`;

      button.addEventListener('click', () => {
        this.loadManager(managerId);
        this.closeMobileSidebar();
      });

      menu.appendChild(button);
    });

    processIcons(menu);
    initTooltips(menu);
  }

  /**
   * Load a manager into the workspace (creates a slot if needed)
   */
  async loadManager(managerId) {
    if (this.currentManagerId === managerId) return;

    // Clear any active utility button (menu managers take precedence)
    this.clearActiveUtilityButtons();
    // Reset utility key so clicking a utility button will work
    this.currentUtilityKey = null;

    // Update active state in sidebar
    this.updateActiveMenuItem(managerId);

    // Delegate to app to load the manager (app will create/show the slot)
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
   * Set active utility manager button and clear menu selection
   * @param {string} utilityKey - The utility manager key (e.g., 'user-profile', 'session-log')
   */
  setActiveUtilityButton(utilityKey) {
    // Clear any active menu item
    this.clearActiveMenuItem();

    // Clear any other active utility buttons
    this.clearActiveUtilityButtons();

    // Set the active button
    const buttonMap = {
      'user-profile': this.elements.sidebarProfileBtn,
      'session-log': this.elements.sidebarLogsBtn,
    };

    const button = buttonMap[utilityKey];
    if (button) {
      button.classList.add('active');
    }

    this.currentUtilityKey = utilityKey;
    // Reset currentManagerId so that clicking a menu item will always show it
    this.currentManagerId = null;
  }

  /**
   * Clear active utility manager buttons
   */
  clearActiveUtilityButtons() {
    this.elements.sidebarProfileBtn?.classList.remove('active');
    this.elements.sidebarLogsBtn?.classList.remove('active');
    this.currentUtilityKey = null;
  }

  /**
   * Clear active menu item
   */
  clearActiveMenuItem() {
    const items = this.elements.sidebarMenu?.querySelectorAll('.menu-item');
    items?.forEach((item) => {
      item.classList.remove('active');
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
   * Apply changes without CSS transitions (for instant state setup on load).
   * @param {Function} fn - Function to execute with transitions disabled
   */
  applyWithoutTransition(fn) {
    const sidebar   = this.elements.sidebar;
    const iconGroup = this.elements.sidebarIconGroup;

    const els = [
      sidebar,
      iconGroup,
      ...(iconGroup?.querySelectorAll('.sidebar-icon-wrapper, .sidebar-icon-btn') || [])
    ].filter(Boolean);

    // Also include the collapse icon for arrow rotation
    const arrowIcon = this._getArrowEl();
    if (arrowIcon) els.push(arrowIcon);

    els.forEach(el => { el.style.transition = 'none'; });
    fn();
    void sidebar?.offsetHeight;
    requestAnimationFrame(() => {
      els.forEach(el => { el.style.transition = ''; });
    });
  }

  /**
   * Set the transition property on all icon wrappers.
   * @param {NodeList} wrappers - The wrapper elements
   * @param {string} value - CSS transition value ('' to restore, 'none' to disable)
   */
  _setWrapperTransitions(wrappers, value) {
    wrappers.forEach(w => { w.style.transition = value; });
  }

  /**
   * Get the collapse arrow icon element via a fresh DOM query.
   * Font Awesome replaces <fa> → <i> → <svg>, so cached references go stale.
   * @returns {Element|null}
   */
  _getArrowEl() {
    return document.querySelector('#collapse-icon') ||
           document.querySelector('#sidebar-collapse-btn')?.firstElementChild;
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
