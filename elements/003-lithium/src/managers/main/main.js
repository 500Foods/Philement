nsh/**
 * Main Menu Manager
 * 
 * Handles the main application layout: resizable sidebar + manager area.
 * The manager area uses "slots" — each loaded manager gets its own slot
 * (header + workspace + footer) that crossfades as a unit when switching.
 *
 * Sidebar features:
 *   - Gradient header matching login panel
 *   - Scrollable menu
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
import './main.css';

// localStorage keys for sidebar state
const SIDEBAR_WIDTH_KEY = 'lithium_sidebar_width';
const SIDEBAR_COLLAPSED_KEY = 'lithium_sidebar_collapsed';

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
 * slot-header-extras / slot-footer-*-extras divs are kept as HTML markers
 * only and are NOT used as the injection target (display:contents divs can
 * hide injected content in some browsers).
 *
 * @param {string} slotId - The slot's DOM id
 * @param {string} icon - FA icon class e.g. 'fa-palette'
 * @param {string} name - Manager display name
 * @returns {string} HTML string
 */
function buildSlotHTML(slotId, icon, name) {
  return `
<div class="manager-slot" id="${slotId}">

  <!-- Slot Header: one unified button group spanning full width.
       Title btn (left, flex:1) → header-extras insertion point → close btn (right).
       Managers inject buttons by calling mainManager.addHeaderButtons(slotId, [...]).  -->
  <div class="manager-slot-header">
    <div class="subpanel-header-group" style="flex:1;min-width:0;">
      <button type="button" class="subpanel-header-btn subpanel-header-primary slot-title-btn" style="flex:1;min-width:0;overflow:hidden;">
        <fa ${icon} class="slot-icon"></fa>
        <span class="slot-name" style="white-space:nowrap;text-overflow:ellipsis;">${name}</span>
      </button>
      <!-- manager-injected header buttons are inserted here by addHeaderButtons() -->
      <button type="button" class="subpanel-header-btn subpanel-header-close slot-close-btn" title="Close">
        <fa fa-xmark></fa>
      </button>
    </div>
  </div>

  <!-- Slot Workspace: manager content renders here -->
  <div class="manager-slot-workspace" id="${slotId}-workspace">
  </div>

  <!-- Slot Footer: single unified button group spanning full width.
       Reports btn (left, flex:1) → footer-left-extras → footer-right-extras → fixed action icons.
       Managers inject buttons by calling mainManager.addFooterButtons(slotId, 'left'|'right', [...]).  -->
  <div class="manager-slot-footer">
    <div class="subpanel-header-group" style="flex:1;min-width:0;">
      <button type="button" class="subpanel-header-btn subpanel-header-primary slot-reports-btn" style="flex:1;min-width:0;overflow:hidden;">
        <fa fa-chart-bar></fa>
        <span style="white-space:nowrap;text-overflow:ellipsis;">Reports Placeholder</span>
      </button>
      <!-- manager-injected footer buttons are inserted here by addFooterButtons() -->
      <button type="button" class="subpanel-header-btn subpanel-header-close slot-notifications-btn" title="Notifications">
        <fa fa-bell></fa>
        <span class="btn-counter" data-counter="notifications">000</span>
      </button>
      <button type="button" class="subpanel-header-btn subpanel-header-close slot-tickets-btn" title="Support Tickets">
        <fa fa-bell-concierge></fa>
        <span class="btn-counter" data-counter="tickets">000</span>
      </button>
      <button type="button" class="subpanel-header-btn subpanel-header-close slot-annotations-btn" title="Annotations">
        <fa fa-tags></fa>
        <span class="btn-counter" data-counter="annotations">000</span>
      </button>
      <button type="button" class="subpanel-header-btn subpanel-header-close slot-tours-btn" title="Tours">
        <fa fa-signs-post></fa>
      </button>
      <button type="button" class="subpanel-header-btn subpanel-header-close slot-lms-btn" title="LMS">
        <fa fa-graduation-cap></fa>
      </button>
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
    
    // Bind event handlers
    this.handleKeyDown = this.handleKeyDown.bind(this);
    this.handleGlobalKeyDown = this.handleGlobalKeyDown.bind(this);
    this.handleSplitterMouseDown = this.handleSplitterMouseDown.bind(this);
    this.handleSplitterMouseMove = this.handleSplitterMouseMove.bind(this);
    this.handleSplitterMouseUp = this.handleSplitterMouseUp.bind(this);
    this.handleSplitterTouchStart = this.handleSplitterTouchStart.bind(this);
    this.handleSplitterTouchMove = this.handleSplitterTouchMove.bind(this);
    this.handleSplitterTouchEnd = this.handleSplitterTouchEnd.bind(this);

    // Manager registry with icons (for sidebar menu + slot headers)
    this.managerIcons = {
      1: { icon: 'fa-palette', name: 'Style Manager' },
      2: { icon: 'fa-user-cog', name: 'Profile Manager' },
      3: { icon: 'fa-chart-line', name: 'Dashboard' },
      4: { icon: 'fa-list', name: 'Lookups' },
      5: { icon: 'fa-search', name: 'Queries' },
    };

    // Utility manager registry with icons
    this.utilityManagerIcons = {
      'session-log': { icon: 'fa-scroll', name: 'Session Log' },
      'user-profile': { icon: 'fa-user-cog', name: 'User Profile' },
    };
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
   * @returns {{ slotEl: HTMLElement, workspaceEl: HTMLElement }}
   */
  createSlot(slotId, icon, name) {
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

    // Wire close button (stub)
    const closeBtn = slotEl.querySelector('.slot-close-btn');
    closeBtn?.addEventListener('click', () => {
      // TODO: Implement slot close functionality
    });

    // Footer button handlers are set up by individual managers

    return {
      slotEl,
      workspaceEl: slotEl.querySelector(`#${slotId}-workspace`),
    };
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
        if (def.title)   btn.title = def.title;
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
        if (def.title)   btn.title = def.title;
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
      this.buildSidebar();
      this.loadSidebarState();
      
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
        }, halfDur);
      }, halfDur);

    } else {
      /* ---- EXPANDING ---- */

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
