/**
 * Slot management for MainManager
 * Handles creation and management of manager slots in the workspace
 */

import { processIcons, setIcon } from '../../core/icons.js';
import { setupHeaderButtons, setupFooterButtons, unregisterManagerShortcuts } from '../../core/manager-ui.js';
import { initZoom } from '../../core/zoom-control.js';
import { initTooltips } from '../../core/tooltip-api.js';
import { log, Subsystems, Status } from '../../core/log.js';

// Icon group sizing (matches CSS --icon-w, --icon-h, --icon-gap)
const ICON_W = 40;
const ICON_H = 35;
const ICON_GAP = 2;

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
 * Methods to be mixed into MainManager prototype
 */
export const slotMethods = {
  /**
   * Build the slot id for a menu manager
   */
  _slotId(managerId) {
    return `manager-slot-${managerId}`;
  },

  /**
   * Build the slot id for a utility manager
   */
  _utilitySlotId(managerKey) {
    return `utility-slot-${managerKey}`;
  },

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
    setupHeaderButtons(slotId, managerId, {
      showKeyboard: true,
      showZoom: true,
      showFullscreen: true,
      showClose: true,
      onClose: () => this.handleCloseSlot(slotId),
      group: headerGroup,
      api: this.app.api,
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
      afterPlaceholder: placeholder,
    });

    // Initialize zoom on first slot creation
    initZoom();

    // Initialize Crimson shortcut (dynamic import for code-splitting)
    import('../../managers/crimson/crimson.js').then(({ initCrimsonShortcut }) => {
      initCrimsonShortcut();
    }).catch(err => {
      log(Subsystems.MANAGER, Status.WARN, `Failed to load Crimson: ${err.message}`);
    });

    return {
      slotEl,
      workspaceEl: slotEl.querySelector(`#${slotId}-workspace`),
    };
  },

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

    // Remove the slot from DOM and clean up via app
    this.app.closeManager(managerId);

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
  },

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
  },

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
  },

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
  },
};

export { buildSlotHTML };
