const __vite__mapDeps=(i,m=__vite__mapDeps,d=(m.f||(m.f=["./crimson-CyL7fHCQ.js","./index-CEuz7QhE.js","./index-ZlKPF4Cw.css","./manager-ui-BnjjCmoo.js","./conduit-B8Ld7ghl.js","./languages-Coru4QdC.js","./codemirror-setup-lxLGSWU-.js","./codemirror-Bg1rM8n2.js","./manager-ui-BfAXUPa9.css","./lithium-table-main-DwQsS17R.js","./tabulator_esm-DsDetXL7.js","./scrollbar-manager-DHRoVvTV.js","./lithium-table-main-BKkowb5Q.css","./zoom-control-C94pA38V.js","./crimson-4jrngLJX.css","./popup-BXcIgOoO.css","./terminal-Dww3-3gJ.js","./terminal-BJU_cDIN.css"])))=>i.map(i=>d[i]);
/* empty css               */
import { t as toggleFullscreen, b as showShortcutsPopup, d as showManagerMenuPopup, f as showTrainingPopup, g as showToursPopup, u as updateFullscreenIcon, i as unregisterManagerShortcuts, j as getMenu, k as buildManagerIconsRegistry, l as clearMenuCache, r as registerShortcut } from './manager-ui-BnjjCmoo.js';
import { l as log, S as Status, a as Subsystems, _ as __vitePreload, p as processIcons, e as eventBus, i as initTooltips, k as canAccessManager, d as getTip, E as Events, f as getConfigValue, b as getClaims } from './index-CEuz7QhE.js';
import { initZoom } from './zoom-control-C94pA38V.js';
import { r as removeTarget, b as addTarget, s as setActiveRadar, o as onHeartbeat, w as wsDisconnected, c as wsFlaky, d as wsConnected, e as setRadarTooltip, g as getRadarTooltip, i as initRadar, f as destroyRadar } from './conduit-B8Ld7ghl.js';

// ===== Shortcuts Button =====

function createShortcutsButton() {
  const button = document.createElement('button');
  button.type = 'button';
  button.className = 'subpanel-header-btn subpanel-header-close manager-ui-btn manager-ui-shortcuts-btn';
  button.title = 'Keyboard Shortcuts';
  button.innerHTML = '<fa fa-command></fa>';

  button.addEventListener('click', (e) => {
    e.stopPropagation();
    showShortcutsPopup(button);
  });

  return button;
}

// ===== Fullscreen Button =====

function createFullscreenButton() {
  const button = document.createElement('button');
  button.type = 'button';
  button.className = 'subpanel-header-btn subpanel-header-close manager-ui-btn manager-ui-fullscreen-btn';
  button.dataset.tooltip = 'Toggle Fullscreen';
  button.dataset.shortcutId = 'fullscreen';
  button.innerHTML = '<fa fa-maximize></fa>';

  button.addEventListener('click', (e) => {
    e.stopPropagation();
    toggleFullscreen();
  });

  const updateIcon = () => updateFullscreenIcon(button);
  document.addEventListener('fullscreenchange', updateIcon);
  document.addEventListener('webkitfullscreenchange', updateIcon);
  document.addEventListener('mozfullscreenchange', updateIcon);
  document.addEventListener('MSFullscreenChange', updateIcon);

  button._cleanupFullscreen = () => {
    document.removeEventListener('fullscreenchange', updateIcon);
    document.removeEventListener('webkitfullscreenchange', updateIcon);
    document.removeEventListener('mozfullscreenchange', updateIcon);
    document.removeEventListener('MSFullscreenChange', updateIcon);
  };

  return button;
}

// ===== Close Button =====

function createCloseButton(onClose) {
  const button = document.createElement('button');
  button.type = 'button';
  button.className = 'subpanel-header-btn subpanel-header-close manager-ui-btn manager-ui-close-btn';
  button.title = 'Close';
  button.dataset.tooltip = 'Close';
  button.dataset.shortcutId = 'close';
  button.innerHTML = '<fa fa-xmark></fa>';

  button.addEventListener('click', (e) => {
    e.stopPropagation();
    if (onClose) onClose();
  });

  return button;
}

// ===== Crimson Button =====

function createCrimsonButton() {
  const button = document.createElement('button');
  button.type = 'button';
  button.className = 'subpanel-header-btn subpanel-header-close manager-ui-btn manager-ui-crimson-btn slot-footer-anchor';
  button.dataset.tooltip = 'Chat with Crimson';
  button.dataset.shortcutId = 'crimson';
  button.innerHTML = '<i class="fa-kit-duotone fa-crimson"></i>';

  button.addEventListener('click', (e) => {
    e.stopPropagation();
    __vitePreload(async () => { const {toggleCrimson} = await import('./crimson-CyL7fHCQ.js');return { toggleCrimson }},true              ?__vite__mapDeps([0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15]):void 0,import.meta.url).then(({ toggleCrimson }) => {
      toggleCrimson();
    });
  });

  return button;
}

// ===== Notifications Button =====

function createNotificationsButton() {
  let count = 0;

  const button = document.createElement('button');
  button.type = 'button';
  button.className = 'subpanel-header-btn subpanel-header-close manager-ui-btn manager-ui-notifications-btn';
  button.title = 'Notifications';
  button.innerHTML = `
    <fa fa-bell></fa>
    <span class="btn-counter" data-counter="notifications" style="display: none;">0</span>
  `;

  const counterEl = button.querySelector('.btn-counter');

  const updateCount = (newCount) => {
    count = newCount;
    if (counterEl) {
      counterEl.textContent = String(count).padStart(3, '0');
      counterEl.style.display = count > 0 ? '' : 'none';
    }
  };

  button.addEventListener('click', (e) => {
    e.stopPropagation();
    eventBus.emit('notifications:open');
  });

  return { button, updateCount };
}

// ===== Concierge Button =====

function createConciergeButton() {
  let count = 0;

  const button = document.createElement('button');
  button.type = 'button';
  button.className = 'subpanel-header-btn subpanel-header-close manager-ui-btn manager-ui-concierge-btn';
  button.title = 'Concierge';
  button.innerHTML = `
    <fa fa-bell-concierge></fa>
    <span class="btn-counter" data-counter="concierge" style="display: none;">0</span>
  `;

  const counterEl = button.querySelector('.btn-counter');

  const updateCount = (newCount) => {
    count = newCount;
    if (counterEl) {
      counterEl.textContent = String(count).padStart(3, '0');
      counterEl.style.display = count > 0 ? '' : 'none';
    }
  };

  button.addEventListener('click', (e) => {
    e.stopPropagation();
    eventBus.emit('concierge:open');
  });

  return { button, updateCount };
}

// ===== Annotations Button =====

function createAnnotationsButton() {
  let count = 0;

  const button = document.createElement('button');
  button.type = 'button';
  button.className = 'subpanel-header-btn subpanel-header-close manager-ui-btn manager-ui-annotations-btn';
  button.title = 'Annotations';
  button.innerHTML = `
    <fa fa-note-sticky></fa>
    <span class="btn-counter" data-counter="annotations" style="display: none;">0</span>
  `;

  const counterEl = button.querySelector('.btn-counter');

  const updateCount = (newCount) => {
    count = newCount;
    if (counterEl) {
      counterEl.textContent = String(count).padStart(3, '0');
      counterEl.style.display = count > 0 ? '' : 'none';
    }
  };

  button.addEventListener('click', (e) => {
    e.stopPropagation();
    eventBus.emit('annotations:open');
  });

  return { button, updateCount };
}

// ===== Tours Button =====

function createToursButton(managerId) {
  const button = document.createElement('button');
  button.type = 'button';
  button.className = 'subpanel-header-btn subpanel-header-close manager-ui-btn manager-ui-tours-btn';
  button.title = 'Tour Suggestions';
  button.innerHTML = '<fa fa-signs-post></fa>';

  button.addEventListener('click', (e) => {
    e.stopPropagation();
    showToursPopup(button, managerId);
  });

  return button;
}

// ===== Training Button =====

function createTrainingButton(managerId) {
  const button = document.createElement('button');
  button.type = 'button';
  button.className = 'subpanel-header-btn subpanel-header-close manager-ui-btn manager-ui-training-btn';
  button.title = 'Training Courses';
  button.innerHTML = '<fa fa-graduation-cap fa-flip-horizontal></fa>';

  button.addEventListener('click', (e) => {
    e.stopPropagation();
    showTrainingPopup(button);
  });

  return button;
}

// ===== Manager Menu Button =====

function createManagerMenuButton(managerId, api) {
  const button = document.createElement('button');
  button.type = 'button';
  button.className = 'subpanel-header-btn subpanel-header-close manager-ui-btn manager-ui-manager-menu-btn';
  button.title = 'Manager Menu';
  button.innerHTML = '<fa fa-gear-complex></fa>';

  button.addEventListener('click', (e) => {
    e.stopPropagation();
    showManagerMenuPopup(button, managerId, api);
  });

  return button;
}

// ===== Terminal Button =====

function createTerminalButton() {
  const button = document.createElement('button');
  button.type = 'button';
  button.className = 'subpanel-header-btn subpanel-header-close manager-ui-btn manager-ui-terminal-btn';
  button.title = 'Terminal';
  button.innerHTML = '<fa fa-square-terminal></fa>';

  button.addEventListener('click', (e) => {
    e.stopPropagation();
    __vitePreload(async () => { const {toggleTerminal} = await import('./terminal-Dww3-3gJ.js');return { toggleTerminal }},true              ?__vite__mapDeps([16,1,2,17]):void 0,import.meta.url).then(({ toggleTerminal }) => {
      toggleTerminal();
    });
  });

  return button;
}

// ===== Header Setup =====

function setupHeaderButtons(slotId, managerId, options = {}) {
  const {
    showKeyboard = true,
    showZoom = true,
    showFullscreen = true,
    showClose = true,
    showManagerMenu = true,
    onClose,
    group,
    api,
  } = options;

  const headerGroup = group || document.querySelector(`#${slotId} .manager-slot-header .subpanel-header-group`);
  if (!headerGroup) {
    log(Subsystems.MANAGER, Status.ERROR, `[ManagerUI] Header group not found for slot: ${slotId}`);
    return;
  }

  const placeholder = headerGroup.querySelectorAll('.slot-header-placeholder');
  const lastPlaceholder = placeholder[placeholder.length - 1];

  const insertAfterPlaceholder = (btn) => {
    if (lastPlaceholder && headerGroup.contains(lastPlaceholder)) {
      headerGroup.insertBefore(btn, lastPlaceholder.nextSibling);
    } else {
      headerGroup.appendChild(btn);
    }
  };

  if (showClose && onClose) {
    const closeBtn = createCloseButton(onClose);
    insertAfterPlaceholder(closeBtn);
  }

  if (showFullscreen) {
    const fsBtn = createFullscreenButton();
    insertAfterPlaceholder(fsBtn);
  }

  if (showZoom) {
    const zoomBtn = document.createElement('button');
    zoomBtn.type = 'button';
    zoomBtn.className = 'subpanel-header-btn subpanel-header-close manager-ui-btn manager-ui-zoom-btn';
    zoomBtn.title = 'Zoom';
    zoomBtn.dataset.tooltip = 'Zoom';
    zoomBtn.dataset.shortcutId = 'zoom';
    zoomBtn.innerHTML = '<fa fa-aperture></fa>';
    zoomBtn.addEventListener('click', (e) => {
      e.stopPropagation();
      __vitePreload(async () => { const {toggleZoomPopup} = await import('./zoom-control-C94pA38V.js');return { toggleZoomPopup }},true              ?__vite__mapDeps([13,3,1,2,4,5,6,7,8]):void 0,import.meta.url).then(({ toggleZoomPopup }) => {
        toggleZoomPopup(zoomBtn);
      });
    });
    insertAfterPlaceholder(zoomBtn);
  }

  if (showKeyboard) {
    const kbBtn = createShortcutsButton();
    kbBtn.dataset.shortcutId = 'shortcuts';
    insertAfterPlaceholder(kbBtn);
  }

  const terminalBtn = createTerminalButton();
  insertAfterPlaceholder(terminalBtn);

  if (showManagerMenu && managerId) {
    const menuBtn = createManagerMenuButton(managerId, api);
    insertAfterPlaceholder(menuBtn);
  }

  processIcons(headerGroup);
}

// ===== Footer Setup =====

function setupFooterButtons(slotId, managerId, options = {}) {
  const {
    showCrimson = true,
    showNotifications = true,
    showConcierge = true,
    showAnnotations = true,
    showTours = true,
    showTraining = true,
    group,
    afterPlaceholder,
  } = options;

  const footerGroup = group || document.querySelector(`#${slotId} .manager-slot-footer .subpanel-header-group`);
  if (!footerGroup) {
    log(Subsystems.MANAGER, Status.ERROR, `[ManagerUI] Footer group not found for slot: ${slotId}`);
    return {};
  }

  const counterButtons = {};

  const insertButton = (btn) => {
    if (afterPlaceholder && footerGroup.contains(afterPlaceholder)) {
      footerGroup.insertBefore(btn, afterPlaceholder.nextSibling);
    } else {
      footerGroup.appendChild(btn);
    }
  };

  if (showTraining) {
    const btn = createTrainingButton();
    insertButton(btn);
  }

  if (showTours) {
    const btn = createToursButton(managerId);
    insertButton(btn);
  }

  if (showAnnotations) {
    const { button, updateCount } = createAnnotationsButton();
    counterButtons.annotations = { button, updateCount };
    insertButton(button);
  }

  if (showConcierge) {
    const { button, updateCount } = createConciergeButton();
    counterButtons.concierge = { button, updateCount };
    insertButton(button);
  }

  if (showNotifications) {
    const { button, updateCount } = createNotificationsButton();
    counterButtons.notifications = { button, updateCount };
    insertButton(button);
  }

  if (showCrimson) {
    const btn = createCrimsonButton();
    insertButton(btn);
  }

  processIcons(footerGroup);

  return counterButtons;
}

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
const slotMethods = {
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
    __vitePreload(async () => { const {initCrimsonShortcut} = await import('./crimson-CyL7fHCQ.js');return { initCrimsonShortcut }},true              ?__vite__mapDeps([0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15]):void 0,import.meta.url).then(({ initCrimsonShortcut }) => {
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

/**
 * Sidebar management for MainManager
 * Handles building the sidebar menu, menu items, and group management
 */


/**
 * Methods to be mixed into MainManager prototype
 */
const sidebarMethods = {
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
  },

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
  },

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
  },

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
  },

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
  },

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
  },

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
        <span class="group-name">${group.name} (${permittedItems.length})</span>
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
        button.dataset.tipWhen = 'collapsed';
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
  },

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
      button.dataset.tipWhen = 'collapsed';
      button.innerHTML = `${managerInfo.iconHtml}<span class="menu-item-name">${managerInfo.name}</span>`;

      button.addEventListener('click', () => {
        this.loadManager(managerId);
        this.closeMobileSidebar();
      });

      menu.appendChild(button);
    });

    processIcons(menu);
    initTooltips(menu);
  },

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
  },

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
  },

  /**
   * Update which menu items show as "open" (loaded but not active).
   * Applies 'manager-open' class to items whose manager is in the openManagers set.
   */
  updateOpenMenuItems() {
    const items = this.elements.sidebarMenu?.querySelectorAll('.menu-item');
    const openManagers = this.app?.openManagers;

    items?.forEach((item) => {
      const managerId = parseInt(item.dataset.managerId, 10);
      // Check if this manager is in the open set (and is a numeric ID, not utility)
      const isOpen = !isNaN(managerId) && openManagers?.has(managerId);
      item.classList.toggle('manager-open', isOpen);
    });
  },

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
  },

  /**
   * Clear active utility manager buttons
   */
  clearActiveUtilityButtons() {
    this.elements.sidebarProfileBtn?.classList.remove('active');
    this.elements.sidebarLogsBtn?.classList.remove('active');
    this.currentUtilityKey = null;
  },

  /**
   * Clear active menu item
   */
  clearActiveMenuItem() {
    const items = this.elements.sidebarMenu?.querySelectorAll('.menu-item');
    items?.forEach((item) => {
      item.classList.remove('active');
    });
  },

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
  },

  /**
   * Save collapsed group states to localStorage
   */
  _saveCollapsedGroups() {
    try {
      localStorage.setItem(COLLAPSED_GROUPS_KEY, JSON.stringify([...this.collapsedGroups]));
    } catch (e) {
      // Non-fatal
    }
  },

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
  },

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
  },

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
  },

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
  },

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
  },

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
  },

  /**
   * Expand groups that contain open managers.
   * This is called when expanding the sidebar to ensure open managers are visible.
   * @returns {Set<number>} The group IDs that were expanded
   */
  _expandGroupsWithOpenManagers() {
    if (!this.elements.sidebarMenu || !this.app?.openManagers) return new Set();
    
    // Find all groups that contain open managers
    const groupsToExpand = new Set();
    const openManagers = this.app.openManagers;
    
    this.elements.sidebarMenu.querySelectorAll('.menu-group').forEach(groupEl => {
      const groupId = parseInt(groupEl.dataset.groupId, 10);
      if (isNaN(groupId)) return;
      
      // Check if this group contains any open managers
      const menuItems = groupEl.querySelectorAll('.menu-item');
      for (const item of menuItems) {
        const managerId = parseInt(item.dataset.managerId, 10);
        if (!isNaN(managerId) && openManagers.has(managerId)) {
          groupsToExpand.add(groupId);
          break;
        }
      }
    });
    
    // Expand the identified groups (remove from collapsed set)
    groupsToExpand.forEach(groupId => {
      this.collapsedGroups.delete(groupId);
    });
    
    // Update DOM
    this.elements.sidebarMenu.querySelectorAll('.menu-group').forEach(groupEl => {
      const groupId = parseInt(groupEl.dataset.groupId, 10);
      if (isNaN(groupId)) return;
      
      const isCollapsed = this.collapsedGroups.has(groupId);
      const itemsEl = groupEl.querySelector('.menu-group-items');
      const toggleIcon = groupEl.querySelector('.group-toggle-icon');
      
      if (itemsEl) {
        itemsEl.classList.toggle('collapsed', isCollapsed);
      }
      if (toggleIcon) {
        toggleIcon.style.transform = isCollapsed ? 'rotate(-90deg)' : 'rotate(0deg)';
      }
      groupEl.classList.toggle('is-collapsed', isCollapsed);
    });
    
    // Save the updated state
    this._saveCollapsedGroups();
    
    return groupsToExpand;
  },

  /**
   * Scroll the active menu item into view within the sidebar.
   */
  _scrollActiveManagerIntoView() {
    if (!this.elements.sidebarMenu) return;
    
    const activeItem = this.elements.sidebarMenu.querySelector('.menu-item.active');
    if (activeItem) {
      activeItem.scrollIntoView({ behavior: 'smooth', block: 'nearest' });
    }
  },
};
const LAST_MANAGER_KEY = 'lithium_last_manager';
const COLLAPSED_GROUPS_KEY = 'lithium_collapsed_groups';

/**
 * Sidebar collapse/expand animations for MainManager
 * Handles the two-stage animation for collapsing and expanding the sidebar
 */


// Icon group sizing (matches CSS --icon-w, --icon-h, --icon-gap)
const ICON_W = 40;
const ICON_H = 35;
const ICON_GAP = 2;
const STEP_X = ICON_W + ICON_GAP;   // 42px — horizontal step
const STEP_Y = ICON_H + ICON_GAP;   // 37px — vertical step

/**
 * Methods to be mixed into MainManager prototype
 */
const collapseMethods = {
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

    // Update collapse button tooltip text
    const collapseBtn = this.elements.sidebarCollapseBtn;
    if (collapseBtn) {
      const tipInstance = getTip(collapseBtn);
      if (tipInstance) {
        tipInstance.updateContent(this.isSidebarCollapsed ? 'Expand Sidebar' : 'Collapse Sidebar');
      }
    }

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
      
      // Expand groups containing open managers when sidebar expands
      this._expandGroupsWithOpenManagers();

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
          // Scroll the active manager into view after expansion completes
          this._scrollActiveManagerIntoView();
        }, halfDur);
      }, halfDur);
    }

    this.saveSidebarState();
  },

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
  },

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
  },

  /**
   * Set the transition property on all icon wrappers.
   * @param {NodeList} wrappers - The wrapper elements
   * @param {string} value - CSS transition value ('' to restore, 'none' to disable)
   */
  _setWrapperTransitions(wrappers, value) {
    wrappers.forEach(w => { w.style.transition = value; });
  },

  /**
   * Get the collapse arrow icon element via a fresh DOM query.
   * Font Awesome replaces <fa> → <i> → <svg>, so cached references go stale.
   * @returns {Element|null}
   */
  _getArrowEl() {
    return document.querySelector('#collapse-icon') ||
           document.querySelector('#sidebar-collapse-btn')?.firstElementChild;
  },

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
  },
};

/**
 * Splitter functionality for MainManager
 * Handles resizable sidebar with mouse and touch events
 */


// Sidebar constraints
const MIN_SIDEBAR_WIDTH = 220;
const MAX_SIDEBAR_WIDTH = 400;

/**
 * Methods to be mixed into MainManager prototype
 */
const splitterMethods = {
  /**
   * Set up the resizable splitter
   */
  setupSplitter() {
    if (!this.elements.sidebarSplitter) return;

    // Mouse events
    this.elements.sidebarSplitter.addEventListener('mousedown', this.handleSplitterMouseDown);
    
    // Touch events for mobile
    this.elements.sidebarSplitter.addEventListener('touchstart', this.handleSplitterTouchStart, { passive: false });
  },

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
  },

  /**
   * Handle splitter mouse move
   */
  handleSplitterMouseMove(event) {
    if (!this.isResizing) return;
    
    const newWidth = event.clientX;
    this.setSidebarWidth(newWidth);
  },

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
  },

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
  },

  /**
   * Handle splitter touch move
   */
  handleSplitterTouchMove(event) {
    if (!this.isResizing) return;
    
    event.preventDefault();
    const touch = event.touches[0];
    const newWidth = touch.clientX;
    this.setSidebarWidth(newWidth);
  },

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
  },

  /**
   * Set sidebar width with constraints
   */
  setSidebarWidth(width) {
    // Constrain width to valid range
    const constrainedWidth = Math.max(MIN_SIDEBAR_WIDTH, Math.min(MAX_SIDEBAR_WIDTH, width));
    
    if (this.elements.sidebar) {
      this.elements.sidebar.style.width = `${constrainedWidth}px`;
    }
  },
};

/**
 * Methods to be mixed into MainManager prototype
 */
const keyboardMethods = {
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
      __vitePreload(async () => { const {toggleZoomPopup} = await import('./zoom-control-C94pA38V.js');return { toggleZoomPopup }},true              ?__vite__mapDeps([13,3,1,2,4,5,6,7,8]):void 0,import.meta.url).then(({ toggleZoomPopup }) => {
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

/**
 * Logout panel for MainManager
 * Handles showing/hiding the logout panel with keyboard navigation
 */


/**
 * Methods to be mixed into MainManager prototype
 */
const logoutMethods = {
  /**
   * Show the logout panel with overlay
   */
  showLogoutPanel() {
    if (this.isLogoutPanelVisible) return;

    this.isLogoutPanelVisible = true;
    this.selectedLogoutIndex = 0;

    // Show overlay and panel by adding visible class
    // CSS handles the opacity/visibility transition
    this.elements.logoutOverlay?.classList.add('visible');
    this.elements.logoutPanel?.classList.add('visible');

    // Add keyboard listener
    document.addEventListener('keydown', this.handleKeyDown);

    // Focus and select the first logout option for accessibility after transition
    setTimeout(() => {
      this._updateLogoutSelection();
    }, 300);
  },

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
  },
};

/**
 * Application WebSocket Client
 *
 * Simple persistent WebSocket connection to the Hydrogen server.
 * - Created on app start (after login)
 * - Maintained until logout
 * - Auto-reconnects if connection drops for >10s
 * - Sends periodic keepalive messages to prevent load balancer timeouts
 */


const ConnectionState = {
  DISCONNECTED: 'disconnected',
  CONNECTING: 'connecting',
  CONNECTED: 'connected',
  ERROR: 'error',
};

const RECONNECT_DELAY = 10000; // 10 seconds before attempting reconnect
const KEEPALIVE_INTERVAL = 25000; // Send keepalive every 25 seconds (before typical 30-60s LB timeout)

class AppWebSocket {
  constructor(options = {}) {
    this.ws = null;
    this.state = ConnectionState.DISCONNECTED;
    this.userInitiatedDisconnect = false;
    this.reconnectTimeout = null;
    this.keepaliveInterval = null;
    
    // Callbacks
    this.onStateChange = options.onStateChange || null;
    this.onMessage = options.onMessage || null;
    
    // Message handlers registered by modules
    this.messageHandlers = new Map();
  }

  getWebSocketUrl() {
    const baseUrl = getConfigValue('server.websocket_url', 'wss://lithium.philement.com/wss');
    const wsKey = getConfigValue('server.websocket_key', 'ABCDEFGHIJKLMNOP');
    if (!wsKey) return baseUrl;
    const separator = baseUrl.includes('?') ? '&' : '?';
    return `${baseUrl}${separator}key=${encodeURIComponent(wsKey)}`;
  }

  getWebSocketProtocol() {
    return getConfigValue('server.websocket_protocol', 'hydrogen');
  }

  setState(newState) {
    if (this.state !== newState) {
      const oldState = this.state;
      this.state = newState;
      log(Subsystems.WEBSOCKET, Status.DEBUG, `[WS] State: ${oldState} -> ${newState}`);
      if (this.onStateChange) {
        this.onStateChange(newState, oldState);
      }
    }
  }

  registerHandler(type, handler) {
    log(Subsystems.WEBSOCKET, Status.DEBUG, `[WS] Registering handler for type: "${type}"`);
    this.messageHandlers.set(type, handler);
  }

  unregisterHandler(type) {
    log(Subsystems.WEBSOCKET, Status.DEBUG, `[WS] Unregistering handler for type: "${type}"`);
    this.messageHandlers.delete(type);
  }

  /**
   * Debug: List all registered handlers
   */
  debugHandlers() {
    const types = Array.from(this.messageHandlers.keys());
    log(Subsystems.WEBSOCKET, Status.DEBUG, `[WS] Registered handlers: ${types.join(', ') || 'NONE'}`);
    return types;
  }

  connect() {
    return new Promise((resolve, reject) => {
      if (this.ws && this.ws.readyState === WebSocket.OPEN) {
        resolve();
        return;
      }

      // Clean up any existing connection
      if (this.ws) {
        this.ws.onclose = null;
        this.ws.onerror = null;
        this.ws.onmessage = null;
        try { this.ws.close(1000, 'Reconnecting'); } catch { /* ignore close errors */ }
        this.ws = null;
      }

      this.setState(ConnectionState.CONNECTING);

      const url = this.getWebSocketUrl();
      const protocol = this.getWebSocketProtocol();
      log(Subsystems.WEBSOCKET, Status.DEBUG, `[WS] Connecting to ${url}`);

      try {
        this.ws = new WebSocket(url, protocol);
      } catch (error) {
        this.setState(ConnectionState.ERROR);
        reject(new Error(`Failed to create WebSocket: ${error.message}`));
        return;
      }

      let resolved = false;

      this.ws.onopen = () => {
        if (resolved) return;
        resolved = true;
        this.connectedAt = Date.now();
        log(Subsystems.WEBSOCKET, Status.DEBUG, '[WS] Connection ESTABLISHED');
        this.userInitiatedDisconnect = false;
        this.setState(ConnectionState.CONNECTED);
        this.startKeepalive();
        resolve();
      };

      this.ws.onclose = (event) => {
        const wasClean = event.wasClean ? 'clean' : 'unclean';
        log(Subsystems.WEBSOCKET, Status.DEBUG, `[WS] Connection CLOSED: code=${event.code} reason="${event.reason}" wasClean=${wasClean}`);
        log(Subsystems.WEBSOCKET, Status.DEBUG, `[WS] Connection duration was approximately ${Date.now() - (this.connectedAt || Date.now())}ms`);
        this.stopKeepalive();
        this.ws = null;
        this.setState(ConnectionState.DISCONNECTED);
        
        // Auto-reconnect after delay if not user-initiated
        if (!this.userInitiatedDisconnect && event.code !== 1000) {
          this.scheduleReconnect();
        }
      };

      this.ws.onerror = (_error) => {
        log(Subsystems.WEBSOCKET, Status.DEBUG, `[WS] Error event fired, readyState=${this.ws?.readyState}`);
        if (!resolved) {
          resolved = true;
          this.ws = null;
          this.setState(ConnectionState.ERROR);
          reject(new Error('WebSocket connection failed'));
        }
      };

      this.ws.onmessage = (event) => {
        this.handleMessage(event.data);
      };

      // Connection timeout
      setTimeout(() => {
        if (!resolved && this.ws && this.ws.readyState !== WebSocket.OPEN) {
          resolved = true;
          this.ws.close();
          this.setState(ConnectionState.ERROR);
          reject(new Error('WebSocket connection timeout'));
        }
      }, 10000);
    });
  }

  handleMessage(data) {
    try {
      const message = JSON.parse(data);
      const { type } = message;

      // Remove a target when we get a response back
      if (type === 'keepalive_ok') {
        removeTarget();
        return;
      }

      if (type === 'chat_done' || type === 'chat_error') {
        removeTarget();
      }

      // Skip verbose logging for chat chunks - handled by CrimsonWS
      if (type === 'chat_chunk') {
        const handler = this.messageHandlers.get(type);
        if (handler) {
          handler(message);
        }
        return;
      }

      // Log only important message types
      if (type !== 'chat_done' && type !== 'chat_error') {
        log(Subsystems.WEBSOCKET, Status.DEBUG, `[WS] ${type} received`);
      }

      // Dispatch to registered handler
      const handler = this.messageHandlers.get(type);
      if (handler) {
        handler(message);
      } else if (this.onMessage) {
        this.onMessage(message);
      }
    } catch (error) {
      log(Subsystems.WEBSOCKET, Status.DEBUG, `[WS] Parse error: ${error.message}`);
    }
  }

  send(message) {
    if (!this.ws || this.ws.readyState !== WebSocket.OPEN) {
      log(Subsystems.WEBSOCKET, Status.DEBUG, '[WS] Cannot send - not connected');
      return false;
    }

    try {
      this.ws.send(JSON.stringify(message));
      // Track on radar — triangle for chat/command, square for keepalive, category 'ws'
      const msgType = message?.type;
      const isKeepalive = msgType === 'keepalive';
      addTarget(isKeepalive ? 'square' : 'triangle', 'ws');
      return true;
    } catch (error) {
      log(Subsystems.WEBSOCKET, Status.DEBUG, `[WS] Failed to send: ${error.message}`);
      return false;
    }
  }

  scheduleReconnect() {
    if (this.reconnectTimeout) {
      clearTimeout(this.reconnectTimeout);
    }
    log(Subsystems.WEBSOCKET, Status.DEBUG, `[WS] Scheduling reconnect in ${RECONNECT_DELAY/1000}s`);
    this.reconnectTimeout = setTimeout(() => {
      this.reconnectTimeout = null;
      if (this.state !== ConnectionState.CONNECTED && this.state !== ConnectionState.CONNECTING) {
        log(Subsystems.WEBSOCKET, Status.DEBUG, '[WS] Attempting reconnect...');
        this.connect().catch(err => {
          log(Subsystems.WEBSOCKET, Status.DEBUG, `[WS] Reconnect failed: ${err.message}`);
        });
      }
    }, RECONNECT_DELAY);
  }

  /**
   * Start sending periodic keepalive messages to prevent load balancer timeout
   * Uses lightweight keepalive message type (returns ~50 bytes vs ~6KB for status)
   */
  startKeepalive() {
    this.stopKeepalive();
    
    this.keepaliveCount = 0;
    this.keepaliveInterval = setInterval(() => {
      if (this.ws && this.ws.readyState === WebSocket.OPEN) {
        this.keepaliveCount++;
        const keepaliveMsg = JSON.stringify({ type: 'keepalive' });
        try {
          this.ws.send(keepaliveMsg);
          log(Subsystems.WEBSOCKET, Status.DEBUG, `[WS] Keepalive #${this.keepaliveCount} sent`);
        } catch (error) {
          log(Subsystems.WEBSOCKET, Status.DEBUG, `[WS] Keepalive #${this.keepaliveCount} failed: ${error.message}`);
        }
      } else {
        log(Subsystems.WEBSOCKET, Status.DEBUG, `[WS] Keepalive skipped - ws state: ${this.ws?.readyState}`);
      }
    }, KEEPALIVE_INTERVAL);
    
    log(Subsystems.WEBSOCKET, Status.DEBUG, `[WS] Keepalive started (every ${KEEPALIVE_INTERVAL/1000}s)`);
  }

  /**
   * Stop the keepalive interval
   */
  stopKeepalive() {
    if (this.keepaliveInterval) {
      clearInterval(this.keepaliveInterval);
      this.keepaliveInterval = null;
      log(Subsystems.WEBSOCKET, Status.DEBUG, '[WS] Keepalive stopped');
    }
  }

  disconnect(graceful = true, userInitiated = false) {
    if (this.reconnectTimeout) {
      clearTimeout(this.reconnectTimeout);
      this.reconnectTimeout = null;
    }
    this.stopKeepalive();
    this.userInitiatedDisconnect = userInitiated;
    
    if (this.ws) {
      if (graceful) {
        this.ws.close(1000, 'Client disconnect');
      } else {
        this.ws.close();
      }
      this.ws = null;
    }
    this.setState(ConnectionState.DISCONNECTED);
  }

  isConnected() {
    return this.ws && this.ws.readyState === WebSocket.OPEN;
  }

  getState() {
    return this.state;
  }
}

// Singleton
let instance = null;

function getAppWS(options = {}) {
  if (!instance) {
    instance = new AppWebSocket(options);
  }
  return instance;
}

function isAppWSConnected() {
  return instance && instance.isConnected();
}

function registerWSHandler(type, handler) {
  getAppWS().registerHandler(type, handler);
}

function unregisterWSHandler(type) {
  if (instance) instance.unregisterHandler(type);
}

/**
 * WebSocket and Radar for MainManager
 * Handles WebSocket connection and radar icon toggle between header/sidebar
 */


/**
 * Methods to be mixed into MainManager prototype
 */
const websocketMethods = {
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
  },

  /**
   * Update the radar status icon based on WebSocket connection state
   * @param {string} state - Connection state from ConnectionState
   */
  updateWSStatus(state) {
    const radarIcon = this.elements.radarIcon;
    if (!radarIcon) return;

    let tooltipText = 'Status: Disconnected';
    switch (state) {
      case ConnectionState.CONNECTED:
        wsConnected();
        tooltipText = 'Status: Connected';
        break;
      case ConnectionState.CONNECTING:
        wsFlaky();
        tooltipText = 'Status: Connecting...';
        break;
      case ConnectionState.ERROR:
        wsDisconnected();
        tooltipText = 'Status: Error';
        break;
      case ConnectionState.DISCONNECTED:
      default:
        wsDisconnected();
        tooltipText = 'Status: Disconnected';
        break;
    }

    // Update tooltip on active radar element and track for sync
    radarIcon.dataset.tooltip = tooltipText;
    const tipInstance = getTip(radarIcon);
    if (tipInstance) tipInstance.updateContent(tooltipText);
    setRadarTooltip(tooltipText);
  },

  /**
   * Flash the radar sweep line (send activity / heartbeat)
   */
  flashWSStatus() {
    onHeartbeat();
  },

  /**
   * Toggle radar position between header and sidebar
   * Uses --transition-duration to animate opacity
   */
  _toggleRadarPosition() {
    const headerRadar = this.elements.radarIcon;
    const sidebarRadar = this.elements.radarIconSidebar;
    if (!headerRadar || !sidebarRadar) return;
  
    this._isRadarInSidebar = !this._isRadarInSidebar;
  
    const duration = this.getTransitionDuration() || 300;
    const transitionStr = `opacity ${duration}ms ease`;
  
    // Ensure transition is set (safe to call every toggle)
    headerRadar.style.transition = transitionStr;
    sidebarRadar.style.transition = transitionStr;
  
    // Disable pointer events during the whole animation
    headerRadar.style.pointerEvents = 'none';
    sidebarRadar.style.pointerEvents = 'none';
  
    if (this._isRadarInSidebar) {
      // === MOVING TO SIDEBAR ===
      setActiveRadar('sidebar');

      // Sync tooltip text to both radars
      const tooltipText = getRadarTooltip();
      headerRadar.dataset.tooltip = tooltipText;
      sidebarRadar.dataset.tooltip = tooltipText;
      const headerTip = getTip(headerRadar);
      const sidebarTip = getTip(sidebarRadar);
      if (headerTip && headerTip.updateContent) headerTip.updateContent(tooltipText);
      if (sidebarTip && sidebarTip.updateContent) sidebarTip.updateContent(tooltipText);
   
      // 1. Bring sidebar into the render tree at opacity 0
      sidebarRadar.style.display = 'block';
      sidebarRadar.style.opacity = '0';
   
      // Start fade-out on header immediately (it was already visible)
      headerRadar.style.opacity = '0';
   
      // Double RAF = the trick that makes SVGs fade IN reliably
      requestAnimationFrame(() => {
        requestAnimationFrame(() => {
          sidebarRadar.style.opacity = '1';
        });
      });
   
      // Cleanup: hide header once its fade-out finishes
      headerRadar.addEventListener('transitionend', function handler(e) {
        if (e.propertyName === 'opacity') {
          headerRadar.style.display = 'none';
          headerRadar.removeEventListener('transitionend', handler);
        }
      }, { once: true });
   
    } else {
      // === MOVING BACK TO HEADER ===
      setActiveRadar('header');

      // Sync tooltip text to both radars
      const tooltipText = getRadarTooltip();
      headerRadar.dataset.tooltip = tooltipText;
      sidebarRadar.dataset.tooltip = tooltipText;
      const headerTip = getTip(headerRadar);
      const sidebarTip = getTip(sidebarRadar);
      if (headerTip && headerTip.updateContent) headerTip.updateContent(tooltipText);
      if (sidebarTip && sidebarTip.updateContent) sidebarTip.updateContent(tooltipText);
   
      // 1. Bring header into the render tree at opacity 0
      headerRadar.style.display = 'block';
      headerRadar.style.opacity = '0';
   
      // Start fade-out on sidebar immediately
      sidebarRadar.style.opacity = '0';
   
      // Double RAF for the incoming header SVG
      requestAnimationFrame(() => {
        requestAnimationFrame(() => {
          headerRadar.style.opacity = '1';
        });
      });
  
      // Cleanup: hide sidebar once its fade-out finishes
      sidebarRadar.addEventListener('transitionend', function handler(e) {
        if (e.propertyName === 'opacity') {
          sidebarRadar.style.display = 'none';
          sidebarRadar.removeEventListener('transitionend', handler);
        }
      }, { once: true });
    }
  
    // Re-enable pointer events once everything is definitely done
    setTimeout(() => {
      headerRadar.style.pointerEvents = '';
      sidebarRadar.style.pointerEvents = '';
    }, duration + 50);

    log(Subsystems.MANAGER, Status.INFO, `[MainManager] Radar moved to ${this._isRadarInSidebar ? 'sidebar' : 'header'}`);
  },
};

/**
 * State management for MainManager
 * Handles saving/loading sidebar state and user info
 */


// localStorage keys for sidebar state
const SIDEBAR_WIDTH_KEY = 'lithium_sidebar_width';
const SIDEBAR_COLLAPSED_KEY = 'lithium_sidebar_collapsed';

/**
 * Methods to be mixed into MainManager prototype
 */
const stateMethods = {
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
  },

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
  },

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
  },
};

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


/**
 * Main Menu Manager Class
 */
class MainManager {
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

const main = /*#__PURE__*/Object.freeze(/*#__PURE__*/Object.defineProperty({
  __proto__: null,
  default: MainManager
}, Symbol.toStringTag, { value: 'Module' }));

export { getAppWS as g, isAppWSConnected as i, main as m, registerWSHandler as r, unregisterWSHandler as u };
