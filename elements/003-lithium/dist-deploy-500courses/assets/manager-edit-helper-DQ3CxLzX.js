import { l as log, S as Status, a as Subsystems } from './index-CEuz7QhE.js';

/**
 * LithiumSplitter - Reusable Panel Splitter Component
 *
 * A draggable splitter for resizable panels. Supports:
 * - Horizontal resizing (left/right)
 * - Touch events for mobile
 * - Hover and active visual states
 * - Collapse/expand functionality
 * - Configurable min/max widths
 * - Callbacks for resize events
 * - Automatic width-mode clearing when user manually resizes
 *
 * Usage:
 *   import { LithiumSplitter } from './core/lithium-splitter.js';
 *
 *   const splitter = new LithiumSplitter({
 *     element: document.getElementById('splitter'),
 *     leftPanel: document.getElementById('left-panel'),
 *     rightPanel: document.getElementById('right-panel'),
 *     minWidth: 150,
 *     maxWidth: 500,
 *     onResize: (width) => { ... },
 *   });
 *
 * @module core/lithium-splitter
 */

class LithiumSplitter {
  /**
   * Create a LithiumSplitter instance
   * @param {Object} options - Configuration options
   */
  constructor(options) {
    this.element = options.element;
    this.leftPanel = options.leftPanel;
    this.rightPanel = options.rightPanel || null;

    // Size constraints
    this.minWidth = options.minWidth || 150;
    this.maxWidth = options.maxWidth || 500;

    // LithiumTable instance(s) whose tableWidthMode should be cleared on manual resize.
    // Accepts a single LithiumTable or an array. When the user drags the splitter,
    // tableWidthMode is set to null so the Width popup shows no checkmark.
    this.tables = options.tables
      ? (Array.isArray(options.tables) ? options.tables : [options.tables])
      : [];

    // State
    this.isResizing = false;
    this.startX = 0;
    this.startWidth = 0;

    // Callbacks
    this.onResize = options.onResize || null;
    this.onResizeEnd = options.onResizeEnd || null;

    // Bind methods
    this._onMouseDown = this._onMouseDown.bind(this);
    this._onMouseMove = this._onMouseMove.bind(this);
    this._onMouseUp = this._onMouseUp.bind(this);
    this._onTouchStart = this._onTouchStart.bind(this);
    this._onTouchMove = this._onTouchMove.bind(this);
    this._onTouchEnd = this._onTouchEnd.bind(this);

    // Setup
    this._attachListeners();
  }

  /**
   * Clear tableWidthMode on associated LithiumTable instances.
   * Called when the user manually drags the splitter so that the
   * Width popup checkmark is cleared (the width no longer matches any preset).
   * Also clears the localStorage entry so that on reload, the manually
   * adjusted width (from PanelStateManager) is used instead of the old preset.
   * @private
   */
  _clearWidthModes() {
    for (const t of this.tables) {
      if (t) {
        t.tableWidthMode = null;
        // Also clear the persisted width mode from localStorage
        // so that setupPersistence() doesn't restore the old preset on reload
        if (t.storageKey) {
          try {
            localStorage.removeItem(`${t.storageKey}_width_mode`);
          } catch {
            // Ignore storage errors
          }
        }
      }
    }
  }

  /**
   * Attach event listeners to the splitter element
   * @private
   */
  _attachListeners() {
    if (!this.element) return;

    // Mouse events
    this.element.addEventListener('mousedown', this._onMouseDown);

    // Touch events
    this.element.addEventListener('touchstart', this._onTouchStart, { passive: true });
  }

  /**
   * Remove event listeners
   * @private
   */
  _detachListeners() {
    if (!this.element) return;

    this.element.removeEventListener('mousedown', this._onMouseDown);
    this.element.removeEventListener('touchstart', this._onTouchStart);
  }

  /**
   * Handle mouse down on splitter
   * @private
   */
  _onMouseDown(e) {
    if (this.leftPanel?.classList.contains('collapsed')) return;

    this.isResizing = true;
    this.element.classList.add('lithium-splitter-resizing');
    document.body.style.cursor = 'col-resize';
    document.body.style.userSelect = 'none';

    // Remove transition for immediate response
    if (this.leftPanel) {
      this.leftPanel.style.transition = 'none';
    }

    this.startX = e.clientX;
    this.startWidth = this.leftPanel?.offsetWidth || 0;

    document.addEventListener('mousemove', this._onMouseMove);
    document.addEventListener('mouseup', this._onMouseUp);
  }

  /**
   * Handle mouse move during resize
   * @private
   */
  _onMouseMove(e) {
    if (!this.isResizing) return;

    const delta = e.clientX - this.startX;
    const newWidth = Math.max(this.minWidth, Math.min(this.maxWidth, this.startWidth + delta));

    if (this.leftPanel) {
      this.leftPanel.style.width = `${newWidth}px`;
    }

    // Clear any LithiumTable width mode — the width no longer matches a preset
    this._clearWidthModes();

    if (this.onResize) {
      this.onResize(newWidth);
    }
  }

  /**
   * Handle mouse up to end resize
   * @private
   */
  _onMouseUp() {
    this._endResize();
  }

  /**
   * Handle touch start for mobile support
   * @private
   */
  _onTouchStart(e) {
    if (this.leftPanel?.classList.contains('collapsed')) return;

    this.isResizing = true;
    this.element.classList.add('lithium-splitter-resizing');

    // Remove transition for immediate response
    if (this.leftPanel) {
      this.leftPanel.style.transition = 'none';
    }

    const touch = e.touches[0];
    this.startX = touch.clientX;
    this.startWidth = this.leftPanel?.offsetWidth || 0;

    document.addEventListener('touchmove', this._onTouchMove, { passive: true });
    document.addEventListener('touchend', this._onTouchEnd);
  }

  /**
   * Handle touch move during resize
   * @private
   */
  _onTouchMove(e) {
    if (!this.isResizing) return;

    const touch = e.touches[0];
    const delta = touch.clientX - this.startX;
    const newWidth = Math.max(this.minWidth, Math.min(this.maxWidth, this.startWidth + delta));

    if (this.leftPanel) {
      this.leftPanel.style.width = `${newWidth}px`;
    }

    // Clear any LithiumTable width mode — the width no longer matches a preset
    this._clearWidthModes();

    if (this.onResize) {
      this.onResize(newWidth);
    }
  }

  /**
   * Handle touch end to end resize
   * @private
   */
  _onTouchEnd() {
    this._endResize();
  }

  /**
   * End the resize operation
   * @private
   */
  _endResize() {
    if (!this.isResizing) return;

    this.isResizing = false;
    this.element.classList.remove('lithium-splitter-resizing');
    document.body.style.cursor = '';
    document.body.style.userSelect = '';

    // Restore transition
    if (this.leftPanel) {
      this.leftPanel.style.transition = '';
    }

    // Remove document listeners
    document.removeEventListener('mousemove', this._onMouseMove);
    document.removeEventListener('mouseup', this._onMouseUp);
    document.removeEventListener('touchmove', this._onTouchMove);
    document.removeEventListener('touchend', this._onTouchEnd);

    // Get final width for callback
    const finalWidth = this.leftPanel?.offsetWidth || 0;

    if (this.onResizeEnd) {
      this.onResizeEnd(finalWidth);
    }
  }

  /**
   * Toggle collapse state of the left panel.
   * Updates startWidth to ensure subsequent drag operations use
   * the correct baseline width.
   *
   * @param {boolean} collapsed - Whether to collapse or expand
   */
  setCollapsed(collapsed) {
    if (!this.leftPanel || !this.element) {
      console.warn('[Splitter] Missing required elements', {
        leftPanel: !!this.leftPanel,
        element: !!this.element,
      });
      return;
    }

    if (collapsed) {
      this.leftPanel.classList.add('collapsed');
      this.element.classList.add('collapsed');
    } else {
      this.leftPanel.classList.remove('collapsed');
      this.element.classList.remove('collapsed');
      // Update startWidth when expanding so that subsequent drag
      // operations use the correct baseline width, not the 0 width
      // that the panel had while collapsed.
      this.startWidth = this.leftPanel.offsetWidth;
    }
  }

  /**
   * Check if left panel is collapsed
   * @returns {boolean}
   */
  isCollapsed() {
    return this.leftPanel?.classList.contains('collapsed') || false;
  }

  /**
   * Toggle collapse state
   */
  toggleCollapse() {
    this.setCollapsed(!this.isCollapsed());
  }

  /**
   * Destroy the splitter and clean up listeners
   */
  destroy() {
    this._detachListeners();

    // Clean up any active resize state
    if (this.isResizing) {
      this._endResize();
    }
  }
}

/**
 * PanelStateManager - Persistent state for collapsible panels
 *
 * Saves and restores:
 * - Panel collapsed state
 * - Panel width
 *
 * @module core/panel-state-manager
 */

/**
 * PanelStateManager - Handles localStorage persistence for panel state
 *
 * Storage keys follow the pattern:
 *   - `{storageKey}_collapsed` — Boolean (true/false)
 *   - `{storageKey}_width` — Integer (pixel width)
 */
class PanelStateManager {
  /**
   * Create a PanelStateManager instance
   * @param {string} storageKey - Base key for localStorage (e.g., 'lithium_style_left')
   */
  constructor(storageKey) {
    this.storageKey = storageKey;
    this.collapsedKey = `${storageKey}_collapsed`;
    this.widthKey = `${storageKey}_width`;
  }

  /**
   * Save collapsed state to localStorage
   * @param {boolean} collapsed - Whether the panel is collapsed
   */
  saveCollapsed(collapsed) {
    try {
      localStorage.setItem(this.collapsedKey, JSON.stringify(collapsed));
      // console.log(`[PanelState] SAVE collapsed "${collapsed}" key="${this.collapsedKey}"`);
    } catch (e) {
      console.error(`[PanelState] FAILED to save collapsed: ${e?.message}`, { key: this.collapsedKey, collapsed });
    }
  }

  /**
   * Load collapsed state from localStorage
   * @param {boolean} defaultValue - Default value if not stored
   * @returns {boolean} The stored collapsed state or default
   */
  loadCollapsed(defaultValue = false) {
    try {
      const stored = localStorage.getItem(this.collapsedKey);
      // console.log(`[PanelState] LOAD collapsed key="${this.collapsedKey}" stored="${stored}" default=${defaultValue}`);
      if (stored != null) {
        return JSON.parse(stored);
      }
    } catch {
      // Invalid JSON or localStorage not available
    }
    return defaultValue;
  }

  /**
   * Save panel width to localStorage
   * @param {number} widthPx - Panel width in pixels
   */
  saveWidth(widthPx) {
    try {
      const val = String(Math.round(widthPx));
      localStorage.setItem(this.widthKey, val);
      // console.log(`[PanelState] SAVE width "${val}" key="${this.widthKey}"`);
    } catch (e) {
      console.error(`[PanelState] FAILED to save width: ${e?.message}`, { key: this.widthKey, widthPx });
    }
  }

  /**
   * Load panel width from localStorage
   * @param {number} defaultValue - Default width if not stored
   * @returns {number} The stored width or default
   */
  loadWidth(defaultValue = 280) {
    try {
      const stored = localStorage.getItem(this.widthKey);
      // console.log(`[PanelState] LOAD width key="${this.widthKey}" stored="${stored}" default=${defaultValue}`);
      if (stored != null) {
        const px = parseInt(stored, 10);
        if (!isNaN(px) && px >= 100) {
          return px;
        }
      }
    } catch {
      // localStorage not available
    }
    return defaultValue;
  }

  /**
   * Clear all stored state for this panel
   */
  clear() {
    try {
      localStorage.removeItem(this.collapsedKey);
      localStorage.removeItem(this.widthKey);
    } catch {
      // localStorage may not be available
    }
  }
}

/**
 * Panel Collapse/Expand Utility
 *
 * Consolidated panel collapse/expand logic for all managers using LithiumTable.
 * Provides smooth animated transitions for panel width and collapse button icon rotation.
 *
 * Usage:
 *   import { togglePanelCollapse, restorePanelState } from '../../core/panel-collapse.js';
 *
 *   // Toggle collapse
 *   togglePanelCollapse({
 *     panel: this.elements.leftPanel,
 *     splitter: this.leftSplitter,
 *     collapseBtn: this.elements.collapseLeftBtn,
 *     panelWidth: this.leftPanelWidth,
 *     isCollapsed: this.isLeftPanelCollapsed,
 *     onAfterToggle: () => this.tables.forEach(t => t?.table?.redraw?.()),
 *   });
 *
 *   // Restore state on init (call AFTER LithiumTable.init() and setupSplitters())
 *   restorePanelState({
 *     panel: this.elements.leftPanel,
 *     splitter: this.leftSplitter,
 *     collapseBtn: this.elements.collapseLeftBtn,
 *     isCollapsed: this.isLeftPanelCollapsed,
 *   });
 *
 * @module core/panel-collapse
 */

/**
 * Get the transition duration in ms from CSS variable.
 * Falls back to 350ms if not set.
 * @returns {number} Transition duration in milliseconds
 */
function getTransitionDuration() {
  const root = getComputedStyle(document.documentElement);
  const duration = root.getPropertyValue('--transition-duration').trim();
  if (duration) {
    const ms = parseFloat(duration);
    if (!isNaN(ms)) return ms;
  }
  return 350;
}

/**
 * Set panel to collapsed state via inline styles (not CSS class).
 * This ensures collapse works regardless of CSS layer/specificity issues.
 * @param {HTMLElement} panel - The panel element
 */
function applyCollapsedStyles(panel) {
  panel.style.width = '0px';
  panel.style.minWidth = '0px';
  panel.style.maxWidth = '0px';
  panel.style.overflow = 'hidden';
  panel.style.flexShrink = '0';
}

/**
 * Clear collapsed inline styles so the panel can use its normal CSS rules.
 * @param {HTMLElement} panel - The panel element
 */
function clearCollapsedStyles(panel) {
  panel.style.minWidth = '';
  panel.style.maxWidth = '';
  panel.style.overflow = '';
  panel.style.flexShrink = '';
}

/**
 * Toggle panel collapse/expand with smooth animation.
 *
 * @param {Object} options - Configuration options
 * @param {HTMLElement} options.panel - The panel element to collapse/expand
 * @param {Object} options.splitter - LithiumSplitter instance
 * @param {HTMLElement} options.collapseBtn - The collapse button element
 * @param {number} options.panelWidth - The target width when expanding (pixels)
 * @param {boolean} options.isCollapsed - Current collapsed state (will be toggled)
 * @param {Function} [options.onAfterToggle] - Optional callback after animation completes
 * @returns {boolean} The new collapsed state
 */
function togglePanelCollapse(options) {
  const { panel, splitter, collapseBtn, panelWidth, isCollapsed, onAfterToggle } = options;

  if (!panel || !splitter || !collapseBtn) {
    console.warn('[PanelCollapse] Missing required elements', { panel: !!panel, splitter: !!splitter, collapseBtn: !!collapseBtn });
    return isCollapsed;
  }

  const newCollapsedState = !isCollapsed;
  const transitionMs = getTransitionDuration();

  // Toggle the collapsed class on the button (triggers icon rotation via CSS)
  collapseBtn.classList.toggle('collapsed', newCollapsedState);

  if (newCollapsedState) {
    // COLLAPSING: Set explicit collapsed styles
    applyCollapsedStyles(panel);
  } else {
    // EXPANDING: Clear collapsed styles, then set target width
    clearCollapsedStyles(panel);
    panel.style.width = `${panelWidth}px`;
  }

  // Update splitter state (adds/removes .collapsed class on panel)
  splitter.setCollapsed(newCollapsedState);

  // Schedule callback after animation completes
  if (onAfterToggle) {
    setTimeout(onAfterToggle, transitionMs);
  }

  return newCollapsedState;
}

/**
 * Restore panel collapsed state on manager init.
 * Sets the panel and button to the persisted collapsed state without animation.
 * Panel width is handled by LithiumTable's setupPersistence(), which applies
 * the saved width mode or PanelStateManager fallback directly to the panel.
 *
 * @param {Object} options - Configuration options
 * @param {HTMLElement} options.panel - The panel element
 * @param {Object} options.splitter - LithiumSplitter instance
 * @param {HTMLElement} options.collapseBtn - The collapse button element
 * @param {boolean} options.isCollapsed - The persisted collapsed state
 */
function restorePanelState(options) {
  const { panel, splitter, collapseBtn, isCollapsed } = options;

  // console.log('[PanelCollapse] restorePanelState called', {
  //   hasPanel: !!panel,
  //   panelId: panel?.id,
  //   hasSplitter: !!splitter,
  //   hasCollapseBtn: !!collapseBtn,
  //   collapseBtnId: collapseBtn?.id,
  //   isCollapsed,
  //   panelWidthBefore: panel?.offsetWidth,
  //   panelStyleWidth: panel?.style?.width,
  // });

  if (!panel || !splitter || !collapseBtn) {
    console.warn('[PanelCollapse] restorePanelState ABORTED - missing elements', {
      panel: !!panel, splitter: !!splitter, collapseBtn: !!collapseBtn
    });
    return;
  }

  // Set button collapsed state (for icon rotation)
  collapseBtn.classList.toggle('collapsed', isCollapsed);

  // Set panel collapsed state
  splitter.setCollapsed(isCollapsed);

  if (isCollapsed) {
    // Explicitly collapse via inline styles — don't rely on CSS class alone.
    applyCollapsedStyles(panel);
    // console.log('[PanelCollapse] Panel COLLAPSED', {
    //   panelId: panel.id,
    //   panelStyleWidth: panel.style.width,
    //   panelOffsetWidth: panel.offsetWidth,
    //   panelComputedWidth: getComputedStyle(panel).width,
    // });
  }
}

/**
 * ManagerEditHelper
 *
 * Consolidates all edit mode / dirty tracking / save-cancel button logic
 * that was previously duplicated across every Manager class.
 *
 * Usage:
 *   // In your Manager constructor:
 *   this.editHelper = new ManagerEditHelper({ name: 'Style' });
 *
 *   // After creating LithiumTable instances, register them:
 *   this.editHelper.registerTable(this.lookupTable);
 *   this.editHelper.registerTable(this.sectionsTable);
 *
 *   // Register external editors (CodeMirror, SunEditor, JSON tree, etc.):
 *   this.editHelper.registerEditor('css', {
 *     getContent:   () => this.cssEditor?.state.doc.toString() || '',
 *     setEditable:  (editable) => { ... },
 *     boundTable:   this.lookupTable,  // which table this editor relates to
 *   });
 *
 *   // After setupManagerFooterIcons, wire the buttons:
 *   this.editHelper.wireFooterButtons(saveBtn, cancelBtn, dummyBtn);
 *
 *   // From editor change listeners (CodeMirror updateListener, SunEditor onChange):
 *   this.editHelper.checkDirtyState();
 *
 * The helper handles:
 *   - onEditModeChange / onDirtyChange callbacks (auto-wired on registerTable)
 *   - activeEditingTable tracking
 *   - Snapshot-based dirty comparison (table rows + registered editors)
 *   - Footer save/cancel button state (disabled → green/red on dirty)
 *   - Editor enable/disable on edit mode transitions
 *
 * @module core/manager-edit-helper
 */


class ManagerEditHelper {
  /**
   * @param {Object} options
   * @param {string} options.name - Manager name for logging (e.g. 'Style', 'Queries')
   * @param {Function} [options.onAfterEditModeChange] - Called after edit mode changes: (isEditing, rowData) => void
   */
  constructor(options = {}) {
    this.name = options.name || 'Manager';
    this.onAfterEditModeChange = options.onAfterEditModeChange || null;

    // Registered LithiumTable instances
    this.tables = new Map();

    // Registered external editors { name → { getContent, setEditable, boundTable } }
    this.editors = new Map();

    // Which LithiumTable is currently in edit mode (at most one)
    this.activeEditingTable = null;

    // Footer button references
    this.footerSaveBtn = null;
    this.footerCancelBtn = null;
    this.footerDummyBtn = null;

    // Snapshot for dirty detection
    this.editModeSnapshot = null;
    this._isFormDirty = false;
    this._dirtyCheckScheduled = false;
  }

  // ── Table Registration ──────────────────────────────────────────────────

  /**
   * Register a LithiumTable and auto-wire its onEditModeChange / onDirtyChange.
   *
   * @param {LithiumTable} lithiumTable - The table instance
   */
  registerTable(lithiumTable) {
    if (!lithiumTable) return;

    const key = lithiumTable.storageKey || lithiumTable.cssPrefix || String(this.tables.size);
    this.tables.set(key, lithiumTable);

    // Back-reference so handleCancel() can call restoreEditorSnapshots()
    lithiumTable._editHelper = this;

    // Auto-wire callbacks — these replace any previously assigned callbacks
    lithiumTable.onEditModeChange = (isEditing, rowData) =>
      this._handleEditModeChange(lithiumTable, isEditing, rowData);

    lithiumTable.onDirtyChange = (isDirty, rowData) =>
      this._handleDirtyChange(lithiumTable, isDirty, rowData);

    log(Subsystems.MANAGER, Status.DEBUG,
      `[${this.name}] EditHelper: registered table "${key}"`);
  }

  // ── Editor Registration ─────────────────────────────────────────────────

  /**
   * Register an external editor (CodeMirror, SunEditor, JSON tree, etc.)
   *
   * @param {string} name - Unique name for this editor (e.g. 'sql', 'css', 'json')
   * @param {Object} descriptor
   * @param {Function} descriptor.getContent    - Returns current content as a string
   * @param {Function} descriptor.setEditable   - Called with (boolean) on edit mode change
   * @param {Function} [descriptor.setContent]  - Called with (string) to restore content on cancel
   * @param {LithiumTable} descriptor.boundTable - The table this editor is associated with
   */
  registerEditor(name, descriptor) {
    this.editors.set(name, descriptor);
    log(Subsystems.MANAGER, Status.DEBUG,
      `[${this.name}] EditHelper: registered editor "${name}"`);
  }

  /**
   * Unregister an editor (e.g. when it's destroyed and recreated).
   * @param {string} name
   */
  unregisterEditor(name) {
    this.editors.delete(name);
  }

  // ── CodeMirror Keymap Helpers ───────────────────────────────────────────

  /**
   * Get keymap options for CodeMirror to handle Escape and Ctrl+Enter.
   * These allow keyboard control of edit mode from within CodeMirror editors.
   *
   * Usage:
   *   const extensions = buildEditorExtensions({
   *     language: 'sql',
   *     ...this.editHelper.getCodeMirrorKeymapOptions(),
   *   });
   *
   * @returns {{ onEscape: Function, onSave: Function }}
   */
  getCodeMirrorKeymapOptions() {
    return {
      onEscape: () => {
        if (this.activeEditingTable?.handleCancel) {
          this.activeEditingTable.handleCancel();
        }
      },
      onSave: () => {
        if (this.activeEditingTable?.handleSave) {
          this.activeEditingTable.handleSave();
        }
      },
    };
  }

  // ── Footer Button Wiring ────────────────────────────────────────────────

  /**
   * Wire footer save/cancel/dummy buttons produced by setupManagerFooterIcons().
   *
   * @param {HTMLElement} saveBtn
   * @param {HTMLElement} cancelBtn
   * @param {HTMLElement} dummyBtn
   */
  wireFooterButtons(saveBtn, cancelBtn, dummyBtn) {
    this.footerSaveBtn = saveBtn;
    this.footerCancelBtn = cancelBtn;
    this.footerDummyBtn = dummyBtn;

    if (this.footerSaveBtn) {
      this.footerSaveBtn.addEventListener('click', () => {
        if (this.activeEditingTable?.handleSave) {
          this.activeEditingTable.handleSave();
        }
      });
    }

    if (this.footerCancelBtn) {
      this.footerCancelBtn.addEventListener('click', () => {
        if (this.activeEditingTable?.handleCancel) {
          this.activeEditingTable.handleCancel();
        }
      });
    }

    // Start with buttons visible but disabled
    this.updateFooterSaveCancelState(true, false);
  }

  // ── Edit Mode Change (auto-wired via registerTable) ─────────────────────

  /**
   * Called when any registered LithiumTable fires onEditModeChange.
   * Handles the "only one table in edit mode at a time" rule, enables/disables
   * external editors, captures/clears snapshot, and updates button state.
   *
   * @param {LithiumTable} lithiumTable
   * @param {boolean} isEditing
   * @param {Object|null} rowData
   */
  _handleEditModeChange(lithiumTable, isEditing, rowData) {
    if (isEditing) {
      // Only one table in edit mode — cancel the other if active
      if (this.activeEditingTable && this.activeEditingTable !== lithiumTable) {
        this.activeEditingTable.exitEditMode('cancel');
      }
      this.activeEditingTable = lithiumTable;

      // Enable external editors bound to this table
      for (const [, editor] of this.editors) {
        if (editor.boundTable === lithiumTable && editor.setEditable) {
          editor.setEditable(true);
        }
      }

      // Snapshot AFTER editors are enabled (so we capture their current state)
      this._takeSnapshot();

      // Buttons start disabled — only enable once dirty
      this.updateFooterSaveCancelState(true, false);

      log(Subsystems.MANAGER, Status.INFO,
        `[${this.name}] Edit mode entered`);

      if (this.onAfterEditModeChange) {
        this.onAfterEditModeChange(true, rowData);
      }
    } else {
      // Exiting edit mode
      if (this.activeEditingTable === lithiumTable) {
        this.activeEditingTable = null;
      }

      // Disable external editors bound to this table
      for (const [, editor] of this.editors) {
        if (editor.boundTable === lithiumTable && editor.setEditable) {
          editor.setEditable(false);
        }
      }

      // Clear snapshot + dirty state
      this._clearSnapshot();

      this.updateFooterSaveCancelState(true, false);

      log(Subsystems.MANAGER, Status.INFO,
        `[${this.name}] Edit mode exited`);

      if (this.onAfterEditModeChange) {
        this.onAfterEditModeChange(false, null);
      }
    }
  }

  // ── Dirty Change (auto-wired via registerTable) ─────────────────────────

  /**
   * Called when any registered LithiumTable fires onDirtyChange (table cell edits).
   * Triggers a full dirty comparison against the snapshot.
   *
   * @param {LithiumTable} lithiumTable
   * @param {boolean} isDirty
   * @param {Object|null} rowData
   */
  _handleDirtyChange(lithiumTable, _isDirty, _rowData) {
    if (this.activeEditingTable === lithiumTable) {
      this.checkDirtyState();
    }
  }

  // ── Snapshot-Based Dirty Tracking ───────────────────────────────────────

  /**
   * Take a snapshot of all editable content.
   * Captures data in "database format" — exactly what we'd send to the server.
   * Change events are merely TRIGGERS to regenerate and compare.
   */
  _takeSnapshot() {
    // Use getEditingRow() to ensure we capture the row being edited,
    // not just the currently selected row (which might differ during transitions)
    const editingRow = this.activeEditingTable?.getEditingRow?.() || null;
    const rowData = editingRow?.getData?.() || null;

    // Log what row we're snapshotting
    const pkField = this.activeEditingTable?.primaryKeyField;
    const pkValue = pkField && rowData ? (Array.isArray(pkField) ? pkField.map(f => rowData[f]).join(',') : rowData[pkField]) : 'none';

    const snapshot = {
      tableRowData: rowData ? JSON.parse(JSON.stringify(rowData)) : null,
      editors: {},
      timestamp: Date.now(),
    };

    // Capture content from every registered editor bound to the active table
    for (const [name, editor] of this.editors) {
      if (editor.boundTable === this.activeEditingTable && editor.getContent) {
        snapshot.editors[name] = editor.getContent();
      }
    }

    this.editModeSnapshot = snapshot;
    this._isFormDirty = false;

    // DEBUG: Log the snapshot query_id
    const snapshotQueryId = rowData?.query_id ?? rowData?.id ?? 'none';
    log(Subsystems.MANAGER, Status.DEBUG,
      `[${this.name}] Snapshot taken for row ${pkValue} (editors: ${Object.keys(snapshot.editors).join(', ') || 'none'})`);
    log(Subsystems.MANAGER, Status.DEBUG,
      `[${this.name}] Snapshot query_id=${snapshotQueryId} (type: ${typeof snapshotQueryId})`);
  }

  /**
   * Clear the snapshot when exiting edit mode.
   */
  _clearSnapshot() {
    this.editModeSnapshot = null;
    this._isFormDirty = false;
  }

  /**
   * Restore all registered editors to their snapshot content.
   * Called during cancel to revert CodeMirror, SunEditor, etc. changes.
   * Table row data is reverted by LithiumTable's handleCancel() — this only
   * handles external editors.
   */
  restoreEditorSnapshots() {
    if (!this.editModeSnapshot) return;

    for (const [name, editor] of this.editors) {
      if (editor.boundTable !== this.activeEditingTable) continue;
      if (!editor.setContent) continue;

      const snapshotContent = this.editModeSnapshot.editors[name];
      if (snapshotContent == null) continue;

      try {
        editor.setContent(snapshotContent);
      } catch (err) {
        log(Subsystems.MANAGER, Status.WARN,
          `[${this.name}] Failed to restore editor "${name}": ${err.message}`);
      }
    }

    log(Subsystems.MANAGER, Status.DEBUG,
      `[${this.name}] Editor snapshots restored`);
  }

  /**
   * Compare current state to the snapshot.
   * Returns true if ANY editable content differs from the snapshot.
   */
  isAnythingDirty() {
    if (!this.editModeSnapshot) return false;

    // ── 0. Verify we're comparing the same row ───────────────────────────
    // IMPORTANT: Use getEditingRow() not getSelectedDataRow() because when
    // switching rows, the new row is already selected before dirty check runs.
    if (this.activeEditingTable && this.editModeSnapshot.tableRowData) {
      const pkFields = this.activeEditingTable.primaryKeyField;
      const snapshotData = this.editModeSnapshot.tableRowData;
      const editingRow = this.activeEditingTable.getEditingRow?.();
      const currentData = editingRow?.getData?.();

      if (pkFields && snapshotData && currentData) {
        const pkFieldArray = Array.isArray(pkFields) ? pkFields : [pkFields];
        const snapshotPk = pkFieldArray.map(f => snapshotData[f]).join(',');
        const currentPk = pkFieldArray.map(f => currentData[f]).join(',');

        if (snapshotPk !== currentPk) {
          log(Subsystems.MANAGER, Status.WARN,
            `[${this.name}] ROW MISMATCH! Snapshot for row ${snapshotPk}, but editing row is ${currentPk}`);
          // Still continue with comparison as the data might still be relevant
        }
      }
    }

    // ── 1. Compare table row data ────────────────────────────────────────
    // IMPORTANT: Use getEditingRow() not getSelectedDataRow() because when
    // switching rows, the new row is already selected before dirty check runs.
    if (this.activeEditingTable && this.editModeSnapshot.tableRowData) {
      const editingRow = this.activeEditingTable.getEditingRow?.();
      const currentData = editingRow?.getData?.();
      const snapshotData = this.editModeSnapshot.tableRowData;

      // DEBUG: Log current query_id
      const currentQueryId = currentData?.query_id ?? currentData?.id ?? 'none';
      log(Subsystems.MANAGER, Status.DEBUG,
        `[${this.name}] isAnythingDirty check: editing query_id=${currentQueryId} (type: ${typeof currentQueryId})`);

      if (currentData) {
        // Note: Primary key (which may be compound) is included in dirty check
        // since PK changes are valid in our system
        const pkFields = this.activeEditingTable.primaryKeyField;
        const pkFieldArray = Array.isArray(pkFields) ? pkFields : (pkFields ? [pkFields] : []);

        for (const key in snapshotData) {
          if (key.startsWith('_') || key === 'tabulator') continue;
          if (!(key in currentData)) continue;

          const snapshotVal = snapshotData[key];
          const currentVal = currentData[key];

          // DEBUG: Log comparison for primary key fields specifically
          if (pkFieldArray.includes(key)) {
            log(Subsystems.MANAGER, Status.DEBUG,
              `[${this.name}] Comparing PK field ${key}: snapshot="${snapshotVal}" (${typeof snapshotVal}) vs current="${currentVal}" (${typeof currentVal}), equal=${snapshotVal == currentVal}`);
          }

          // Special handling for null/undefined/empty - treat them as equivalent
          const isEmptySnapshot = snapshotVal == null || snapshotVal === '';
          const isEmptyCurrent = currentVal == null || currentVal === '';
          if (isEmptySnapshot && isEmptyCurrent) continue;

          // Compare with type coercion for primitives, strict for objects
          const isChanged = (typeof snapshotVal === 'object' || typeof currentVal === 'object')
            ? JSON.stringify(snapshotVal) !== JSON.stringify(currentVal)
            : snapshotVal != currentVal; // Use != for loose comparison (handles "431" vs 431)

          if (isChanged) {
            log(Subsystems.MANAGER, Status.DEBUG,
              `[${this.name}] Dirty detected: table row field "${key}" changed from "${snapshotVal}" (${typeof snapshotVal}) to "${currentVal}" (${typeof currentVal})`);
            return true;
          }
        }
      }
    }

    // ── 2. Compare registered editors bound to the active table ──────────
    for (const [name, editor] of this.editors) {
      if (editor.boundTable !== this.activeEditingTable) continue;
      if (!editor.getContent) continue;

      const snapshotContent = this.editModeSnapshot.editors[name];
      if (snapshotContent == null) continue;  // Editor wasn't captured

      const currentContent = editor.getContent();
      if (currentContent !== snapshotContent) {
        // Log the difference for debugging (truncated for readability)
        const snapPreview = snapshotContent.substring(0, 50);
        const currPreview = currentContent.substring(0, 50);
        log(Subsystems.MANAGER, Status.DEBUG,
          `[${this.name}] Dirty detected: editor "${name}" changed`);
        log(Subsystems.MANAGER, Status.DEBUG,
          `  Snapshot: "${snapPreview}"...`);
        log(Subsystems.MANAGER, Status.DEBUG,
          `  Current:  "${currPreview}"...`);
        return true;
      }
    }

    return false;
  }

  /**
   * Recompute dirty state and update button state accordingly.
   * Call this from every editor change listener.
   *
   * When called from CodeMirror's updateListener the call is synchronous
   * inside CM's update cycle.  DOM mutations here (e.g. toggling `disabled`
   * on footer buttons) can interfere with CM's focus management and cause
   * the editor to lose focus mid-keystroke.
   *
   * To avoid this, the actual comparison + DOM update is deferred to the
   * next animation frame via `requestAnimationFrame`.  Multiple calls
   * within the same frame are coalesced (only one comparison runs).
   */
  checkDirtyState() {
    if (this._dirtyCheckScheduled) return;
    this._dirtyCheckScheduled = true;

    requestAnimationFrame(() => {
      this._dirtyCheckScheduled = false;
      this._executeCheckDirtyState();
    });
  }

  /**
   * Internal: perform the dirty comparison and update buttons.
   * Separated from checkDirtyState() so the logic can be tested directly.
   */
  _executeCheckDirtyState() {
    const isDirty = this.isAnythingDirty();

    if (this.activeEditingTable) {
      // Sync the table's isDirty flag so its handleSave() will proceed
      this.activeEditingTable.isDirty = isDirty;
      this.updateFooterSaveCancelState(true, isDirty);

      if (this._isFormDirty !== isDirty) {
        log(Subsystems.MANAGER, Status.DEBUG,
          `[${this.name}] Dirty state: ${isDirty}`);
        this._isFormDirty = isDirty;
      }
    }
  }

  // ── Footer Button State ─────────────────────────────────────────────────

  /**
   * Show/hide and enable/disable the footer save/cancel buttons.
   *
   * @param {boolean} visible  - Whether the buttons should be visible
   * @param {boolean} enabled  - Whether the buttons should be clickable (requires visible)
   */
  updateFooterSaveCancelState(visible, enabled) {
    if (this.footerSaveBtn) {
      this.footerSaveBtn.style.display = visible ? '' : 'none';
      this.footerSaveBtn.disabled = !visible || !enabled;
    }
    if (this.footerCancelBtn) {
      this.footerCancelBtn.style.display = visible ? '' : 'none';
      this.footerCancelBtn.disabled = !visible || !enabled;
    }
    if (this.footerDummyBtn) {
      this.footerDummyBtn.style.display = visible ? '' : 'none';
    }
  }

  // ── Convenience Getters ─────────────────────────────────────────────────

  /** @returns {boolean} Whether any table is currently in edit mode */
  isEditing() {
    return this.activeEditingTable != null;
  }

  /** @returns {LithiumTable|null} The table currently in edit mode */
  getActiveTable() {
    return this.activeEditingTable;
  }

  // ── Cleanup ─────────────────────────────────────────────────────────────

  destroy() {
    this.tables.clear();
    this.editors.clear();
    this.activeEditingTable = null;
    this.editModeSnapshot = null;
    this._dirtyCheckScheduled = false;
    this.footerSaveBtn = null;
    this.footerCancelBtn = null;
    this.footerDummyBtn = null;
  }
}

export { LithiumSplitter as L, ManagerEditHelper as M, PanelStateManager as P, restorePanelState as r, togglePanelCollapse as t };
