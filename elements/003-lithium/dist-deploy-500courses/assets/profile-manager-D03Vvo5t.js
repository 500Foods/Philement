import { l as log, a as Subsystems, S as Status, b as getClaims, e as eventBus, E as Events, T as Tooltip, p as processIcons, y as normalizeIconHtml, z as createRequest, f as getConfigValue, s as storeJWT, t as toast } from './index-DLPCDk6c.js';
import { P as PanelStateManager, M as ManagerEditHelper, L as LithiumSplitter, t as togglePanelCollapse, r as restorePanelState } from './manager-edit-helper-DrP4BAzu.js';
import { c as closeAllPopups, j as getMenu, k as buildManagerIconsRegistry, s as setupManagerFooterIcons } from './manager-ui-UMxicChL.js';
import { createReadOnlyCompartment, createWordWrapCompartment, createBracketMatchCompartment, createSelectionHighlightCompartment, createCommentContinuationCompartment, createWhitespaceCompartment, createVirtualColumnsCompartment, createIndentUnitCompartment, formatSortedJson, buildEditorExtensions, setEditorEditable, compareJsonIgnoringKeyOrder, setEditorContentNoHistory, foldAllInEditor, unfoldAllInEditor, updateUndoRedoButtons } from './codemirror-setup-lxLGSWU-.js';
import { E as EditorState, a as EditorView, u as undo, r as redo } from './codemirror-Bg1rM8n2.js';
import { L as LithiumEditorFooter, c as createFontPopup } from './font-popup-BNqpywzD.js';
import { D as DateTime } from './luxon-DbyXf6dF.js';
import { s as scrollbarManager } from './scrollbar-manager-DTR0NQaT.js';
import { L as LithiumTable } from './lithium-table-main-BZjHbi6_.js';
import { a as authQuery } from './conduit-D2QlMBxM.js';
/* empty css                          */
/* empty css               */
import { i as initToolbars } from './toolbar-DtAJm8yC.js';
import './languages-Coru4QdC.js';
import './tabulator_esm-DsDetXL7.js';

/**
 * Profile Manager - Collection Tab Handler
 *
 * Handles the JSON editor for viewing and editing all profile preferences
 * in a single CodeMirror instance.
 *
 * Pattern: Matches Lookups Manager JSON tab
 * - Editor starts read-only
 * - Double-click to enter edit mode
 * - Escape / Ctrl+Enter keyboard shortcuts
 * - Undo/Redo/Fold/Unfold/Prettify toolbar buttons
 * - Dirty state drives footer Save/Cancel buttons
 */


/**
 * CollectionTabHandler class
 * Manages the CodeMirror JSON editor for the Collection tab
 */
class CollectionTabHandler {
  constructor(options = {}) {
    this.container = options.container;
    this.parent = options.parent || options.container?.parentNode; // Manager main container (sibling of toolbar)
    this.onDirtyChange = options.onDirtyChange || (() => {});
    this.onSave = options.onSave || (() => {});
    this.onCancel = options.onCancel || (() => {});

    this.editor = null;
    this.readOnlyCompartment = null;
    this.wordWrapCompartment = null;
    this.bracketMatchCompartment = null;
    this.footer = null;
    this._initialData = null;
    this._isEditing = false;

    // Font settings
    this.fontSize = 13;
    this.fontFamily = '"Vanadium Mono", var(--font-mono, monospace)';
  }

  /**
   * Initialize the Collection JSON editor
   * @param {Object} initialData - Initial preferences data
   */
  async init(initialData = {}) {
    if (!this.container) return;

    // If editor already exists, just update content
    if (this.editor) {
      await this.setData(initialData);
      return;
    }

    this._initialData = JSON.parse(JSON.stringify(initialData));

    try {
       this.readOnlyCompartment = createReadOnlyCompartment();
       this.wordWrapCompartment = createWordWrapCompartment();
       this.bracketMatchCompartment = createBracketMatchCompartment();
       this.selectionHighlightCompartment = createSelectionHighlightCompartment();
       this.commentContinuationCompartment = createCommentContinuationCompartment();
       this.whitespaceCompartment = createWhitespaceCompartment();
       this.virtualColumnsCompartment = createVirtualColumnsCompartment();
       this.indentUnitCompartment = createIndentUnitCompartment();

      const jsonStr = formatSortedJson(initialData, 2);

const extensions = buildEditorExtensions({
         language: 'json',
         readOnlyCompartment: this.readOnlyCompartment,
         readOnly: true, // Start read-only, double-click to edit
         fontSize: this.fontSize,
         fontFamily: this.fontFamily,
          wordWrapCompartment: this.wordWrapCompartment,
          bracketMatchCompartment: this.bracketMatchCompartment,
          selectionHighlightCompartment: this.selectionHighlightCompartment,
          commentContinuationCompartment: this.commentContinuationCompartment,
           whitespaceCompartment: this.whitespaceCompartment,
           virtualColumnsCompartment: this.virtualColumnsCompartment,
           indentUnitCompartment: this.indentUnitCompartment,
           wordWrap: false,
          bracketMatch: true,
          selectionHighlight: true,
          commentContinuation: true,
          onUpdate: (update) => {
            if (update.selectionSet) {
              // Defer footer update to prevent DOM interference during CodeMirror updates
              requestAnimationFrame(() => this.footer?.updateCursorPosition());
            }
            if (update.docChanged) {
              this.onDirtyChange(this.isDirty());
            }
            if (update.transactions.length > 0) {
              this._updateUndoRedoButtons();
            }
          },
         onEscape: () => this.onCancel(),
         onSave: () => this.onSave(),
       });

      const startState = EditorState.create({ doc: jsonStr, extensions });

      this.container.innerHTML = '';
      this.editor = new EditorView({
        state: startState,
        parent: this.container,
      });

      // Initialize editor footer (already in HTML)
      const footerEl = document.querySelector('#profile-collection-editor-footer');
       this.footer = new LithiumEditorFooter({
         container: footerEl,
         editorView: this.editor,
         wordWrapCompartment: this.wordWrapCompartment,
         bracketMatchCompartment: this.bracketMatchCompartment,
         selectionHighlightCompartment: this.selectionHighlightCompartment,
         commentContinuationCompartment: this.commentContinuationCompartment,
         whitespaceCompartment: this.whitespaceCompartment,
         indentUnitCompartment: this.indentUnitCompartment,
         virtualColumnsCompartment: this.virtualColumnsCompartment,
         initialWordWrap: false,
         initialBracketMatch: true,
         initialSelectionHighlight: true,
         initialCommentContinuation: true,
         initialWhitespace: false,
         initialVirtualColumns: true,
         storageKey: 'editors.profile.collection',
       });
      this.footer.init();



      // Store reference for external access
      this.container._cmView = this.editor;
      this.container._cmReadOnlyCompartment = this.readOnlyCompartment;

      // Set initial visual state (read-only)
      setEditorEditable(this.editor, this.readOnlyCompartment, false, this.container);

      // Double-click to enter edit mode
      this.container.addEventListener('dblclick', () => {
        if (!this._isEditing) {
          this.setEditable(true);
        }
      });

      log(Subsystems.MANAGER, Status.INFO, '[CollectionTab] Editor initialized');
    } catch (error) {
      log(Subsystems.MANAGER, Status.ERROR, '[CollectionTab] Failed to initialize editor:', error);
    }
  }

  /**
   * Toggle editor between read-only and editable
   * @param {boolean} editable - true = editable, false = read-only
   */
  setEditable(editable) {
    if (!this.editor || !this.readOnlyCompartment) return;
    this._isEditing = editable;
    setEditorEditable(this.editor, this.readOnlyCompartment, editable, this.container);
    this._updateUndoRedoButtons();
    log(Subsystems.MANAGER, Status.DEBUG, `[CollectionTab] Edit mode: ${editable}`);
  }

  /**
   * Check if the editor has unsaved changes
   * @returns {boolean}
   */
  isDirty() {
    if (!this.editor || !this._initialData) return false;
    try {
      const currentContent = this.editor.state.doc.toString();
      const initialContent = formatSortedJson(this._initialData, 2);
      return !compareJsonIgnoringKeyOrder(currentContent, initialContent);
    } catch {
      return true; // Invalid JSON is considered dirty
    }
  }

  /**
   * Get current editor content as parsed JSON
   * @returns {Object|null}
   */
  getData() {
    if (!this.editor) return null;
    try {
      const content = this.editor.state.doc.toString();
      return JSON.parse(content);
    } catch (error) {
      log(Subsystems.MANAGER, Status.ERROR, '[CollectionTab] Invalid JSON:', error);
      return null;
    }
  }

  /**
   * Get current editor content as a string (for editHelper integration)
   * @returns {string}
   */
  getContent() {
    return this.editor?.state.doc.toString() || '{}';
  }

  /**
   * Update editor content without marking as dirty and without adding to undo history.
   * Also resets edit mode to read-only.
   * @param {Object} data - New data to display
   */
  async setData(data) {
    if (!this.editor) {
      await this.init(data);
      return;
    }

    this._initialData = JSON.parse(JSON.stringify(data));

    const jsonStr = formatSortedJson(data, 2);

    setEditorContentNoHistory(this.editor, jsonStr);

    // Reset to read-only on data load
    this.setEditable(false);

    this.onDirtyChange(false);
  }

  /**
   * Update the editor's data reference without changing content.
   * Used after a successful save to reset the dirty baseline.
   * @param {Object} data - The newly saved data
   */
  setInitialData(data) {
    this._initialData = JSON.parse(JSON.stringify(data));
    this.onDirtyChange(false);
  }

  /**
   * Handle undo action
   */
  undo() {
    if (!this.editor) return;
    undo(this.editor);
    log(Subsystems.MANAGER, Status.DEBUG, '[CollectionTab] Undo');
  }

  /**
   * Handle redo action
   */
  redo() {
    if (!this.editor) return;
    redo(this.editor);
    log(Subsystems.MANAGER, Status.DEBUG, '[CollectionTab] Redo');
  }

  /**
   * Fold all JSON blocks
   */
  foldAll() {
    if (!this.editor) return;
    foldAllInEditor(this.editor);
    log(Subsystems.MANAGER, Status.DEBUG, '[CollectionTab] Fold all');
  }

  /**
   * Unfold all JSON blocks
   */
  unfoldAll() {
    if (!this.editor) return;
    unfoldAllInEditor(this.editor);
    log(Subsystems.MANAGER, Status.DEBUG, '[CollectionTab] Unfold all');
  }

  /**
   * Prettify/format the JSON content
   */
  prettify() {
    if (!this.editor) return;
    try {
      const currentContent = this.editor.state.doc.toString();
      const parsed = JSON.parse(currentContent);
      const formatted = formatSortedJson(parsed, 2);

      this.editor.dispatch({
        changes: {
          from: 0,
          to: this.editor.state.doc.length,
          insert: formatted,
        },
      });
      log(Subsystems.MANAGER, Status.DEBUG, '[CollectionTab] Prettify');
    } catch (error) {
      log(Subsystems.MANAGER, Status.ERROR, '[CollectionTab] Prettify failed:', error);
    }
  }

  /**
   * Set font size for the editor
   * @param {number} size - Font size in pixels
   */
  setFontSize(size) {
    if (!this.editor?.dom) return;
    this.fontSize = size;
    this.editor.dom.style.fontSize = `${size}px`;
    this.editor.requestMeasure();
  }

  /**
   * Set font family for the editor
   * @param {string} family - CSS font-family string
   */
  setFontFamily(family) {
    if (!this.editor?.dom) return;
    this.fontFamily = family;
    this.editor.dom.style.fontFamily = family;
    this.editor.requestMeasure();
  }

  /**
   * Update undo/redo button state.
   * Delegates to the shared updateUndoRedoButtons from codemirror-setup.
   * @param {HTMLElement} undoBtn
   * @param {HTMLElement} redoBtn
   */
  updateUndoRedoButtons(undoBtn, redoBtn) {
    updateUndoRedoButtons({
      undoBtn,
      redoBtn,
      view: this.editor,
      isEditing: this._isEditing,
    });
  }

  /**
   * Internal: update undo/redo buttons if they are cached on the container
   */
  _updateUndoRedoButtons() {
    const undoBtn = this.container?.closest('.manager-page')?.querySelector('#btn-undo');
    const redoBtn = this.container?.closest('.manager-page')?.querySelector('#btn-redo');
    if (undoBtn && redoBtn) {
      this.updateUndoRedoButtons(undoBtn, redoBtn);
    }
  }

  /**
   * Destroy the editor instance
   */
  destroy() {
    // Destroy OverlayScrollbars first
    if (this.editor) {
      destroyCodeMirrorScrollbars(this.editor);
    }

    if (this.footer) {
      this.footer.destroy();
      this.footer = null;
    }
    this.editor?.destroy();
    this.editor = null;
    this.readOnlyCompartment = null;
    this.wordWrapCompartment = null;
    this.bracketMatchCompartment = null;
    this._initialData = null;
    this._isEditing = false;
  }
}

/**
 * Profile Manager - Settings Page Base Class
 *
 * Base class for manager-specific settings pages. These pages are loaded
 * dynamically when a manager entry is selected in the user options table.
 *
 * Each page controls a unique "section" of the profile JSON, identified by
 * its sectionKey (negative index for internal pages, manager ID for managers).
 * Pages should use this.settings (a ProfileSettingsService instance) to read
 * and write their configuration rather than touching localStorage directly.
 */


/**
 * BaseSettingsPage class
 * Extend this class to create manager-specific settings pages
 */
class BaseSettingsPage {
  constructor(options = {}) {
    this.index = options.index || 0;
    this.managerId = options.managerId || 0;
    this.sectionKey = String(options.sectionKey ?? this.index);
    this.container = null;
    this._isDirty = false;
    this._originalData = null;
    this._onDirtyChange = options.onDirtyChange || (() => {});
    this.settings = options.settings || null;
  }

  /**
   * Initialize the page
   * @param {HTMLElement} container - The page container element
   * @param {Object} data - Page data from the user options table
   */
  async init(container, data) {
    this.container = container;
    this._originalData = JSON.parse(JSON.stringify(data));

    // Override in subclasses to perform setup
    await this.onInit();

    log(Subsystems.MANAGER, Status.DEBUG, `[SettingsPage] Initialized page ${this.index}`);
  }

  /**
   * Called when the page is initialized
   * Override in subclasses
   */
  async onInit() {
    // Override in subclass
  }

  /**
   * Called when the page becomes visible
   * Override in subclasses
   */
  onShow() {
    // Override in subclass
  }

  /**
   * Called when the page is hidden
   * Override in subclasses
   */
  onHide() {
    // Override in subclass
  }

  /**
   * Check if the page has unsaved changes
   * @returns {boolean}
   */
  isDirty() {
    return this._isDirty;
  }

  /**
   * Mark the page as dirty/clean
   * @param {boolean} dirty
   */
  setDirty(dirty) {
    this._isDirty = dirty;
    this._onDirtyChange(dirty);
  }

  /**
   * Save the page's data
   * Override in subclasses
   * @returns {Promise<Object>}
   */
  async save() {
    // Override in subclass
    this.setDirty(false);
    return { success: true };
  }

  /**
   * Cancel/reset the page's changes
   * Override in subclasses
   */
  cancel() {
    // Override in subclass
    this.setDirty(false);
  }

  /**
   * Refresh the page with current data
   * Override in subclasses
   */
  refresh() {
    // Override in subclass
  }

  // ── Settings Service Helpers ─────────────────────────────────────────────

  /**
   * Read a setting value from the profile JSON via the Settings Service.
    * Returns defaultValue if the path does not exist or the service is unavailable.
    *
    * @param {string} path - Dotted path within this page's section (e.g. "dates.short")
    * @param {*} defaultValue - Fallback value
    * @returns {*}
    */
   getSetting(path, defaultValue = undefined) {
     if (!this.settings) return defaultValue;
     return this.settings.getSection(this.sectionKey, path, defaultValue);
   }

   /**
    * Write a setting value into the profile JSON via the Settings Service.
    * The value is written under this page's sectionKey.
    *
    * @param {string} path - Dotted path within this page's section (e.g. "dates.short")
    * @param {*} value - Value to store
    */
   setSetting(path, value) {
     if (!this.settings) return;
     this.settings.setSection(this.sectionKey, path, value);
   }

  /**
   * Batch-write multiple settings at once. Only one persist is triggered.
    *
    * @param {Object} changes - Map of dotted paths to values
    */
   setSettingsBatch(changes) {
     if (!this.settings) return;
     this.settings.batchSetSection(this.sectionKey, changes);
   }

  /**
   * Get this page's entire section object from the profile JSON.
   * Returns an empty object if unavailable.
   * @returns {Object}
   */
  getSectionData() {
    if (!this.settings) return {};
    return this.settings.getSection(this.sectionKey);
  }

  /**
   * Replace this page's entire section in the profile JSON.
   * This is the preferred way to bulk-write page data.
   *
   * @param {Object} data - Section data
   * @param {string} [sectionName] - Human-readable name (stored as _name)
   */
  setSectionData(data, sectionName = null) {
    if (!this.settings) return;
    this.settings.setSection(this.sectionKey, data, sectionName);
  }

  // ── Form Helpers ─────────────────────────────────────────────────────────

  /**
   * Get form data from the page
   * @param {string} formSelector - CSS selector for the form
   * @returns {Object}
   */
  getFormData(formSelector = 'form') {
    if (!this.container) return {};

    const form = this.container.querySelector(formSelector);
    if (!form) return {};

    const data = {};
    const elements = form.querySelectorAll('input, select, textarea');

    elements.forEach((el) => {
      const name = el.name;
      if (!name) return;

      if (el.type === 'checkbox') {
        data[name] = el.checked;
      } else if (el.type === 'number') {
        data[name] = parseFloat(el.value) || 0;
      } else {
        data[name] = el.value;
      }
    });

    return data;
  }

  /**
   * Set form data on the page
   * @param {Object} data - Data to set
   * @param {string} formSelector - CSS selector for the form
   */
  setFormData(data, formSelector = 'form') {
    if (!this.container) return;

    const form = this.container.querySelector(formSelector);
    if (!form) return;

    Object.entries(data).forEach(([name, value]) => {
      const el = form.querySelector(`[name="${name}"]`);
      if (!el) return;

      if (el.type === 'checkbox') {
        el.checked = !!value;
      } else {
        el.value = value ?? '';
      }
    });
  }

  /**
   * Setup change listeners for form elements
   * @param {string} formSelector - CSS selector for the form
   */
  setupChangeListeners(formSelector = 'form') {
    if (!this.container) return;

    const form = this.container.querySelector(formSelector);
    if (!form) return;

    const elements = form.querySelectorAll('input, select, textarea');
    elements.forEach((el) => {
      el.addEventListener('change', () => this.setDirty(true));
    });
  }

  /**
   * Destroy the page and clean up
   * Override in subclasses
   */
  destroy() {
    this.container = null;
    this._originalData = null;
  }
}

/**
 * SimpleSettingsPage class
 * A simple implementation that handles basic form binding
 */
class SimpleSettingsPage extends BaseSettingsPage {
  constructor(options = {}) {
    super(options);
    this.formSelector = options.formSelector || 'form';
  }

  /**
   * Called when the page is initialized
   */
  async onInit() {
    this.setupChangeListeners(this.formSelector);
    await this.loadData();
  }

  /**
   * Load data into the form
   * Override in subclasses to fetch data from API or settings service
   */
  async loadData() {
    // Override in subclass to load from settings service
    // Then call setFormData with the loaded data
  }

  /**
   * Save the form data
   */
  async save() {
    const data = this.getFormData(this.formSelector);
    // Override in subclass to save to settings service
    this.setDirty(false);
    return { success: true, data };
  }

  /**
   * Cancel changes and reload data
   */
  cancel() {
    this.loadData();
    this.setDirty(false);
  }
}

/**
 * Profile Manager - Account Page Handler
 *
 * Handles the Account settings page (index: -1)
 * Displays user information from JWT claims
 */


/**
 * Account Page Handler
 */
class AccountPage extends BaseSettingsPage {
  constructor(options = {}) {
    super({
      ...options,
      index: -1,
    });
  }

  /**
   * Called when the page is initialized
   */
  async onInit() {
    // Load and display user info
    this.loadUserInfo();
    log(Subsystems.MANAGER, Status.DEBUG, '[AccountPage] Initialized');
  }

  /**
   * Load user information from JWT claims
   */
  loadUserInfo() {
    const claims = getClaims();
    if (!claims || !this.container) return;

    const fields = {
      '#profile-username': claims.username || '-',
      '#profile-email': claims.email || '-',
      '#profile-roles': Array.isArray(claims.roles) ? claims.roles.join(', ') : claims.roles || '-',
      '#profile-database': claims.database || '-',
    };

    Object.entries(fields).forEach(([selector, value]) => {
      const el = this.container.querySelector(selector);
      if (el) el.textContent = value;
    });
  }

  /**
   * Called when the page becomes visible
   */
  onShow() {
    // Refresh user info when page is shown (in case claims changed)
    this.loadUserInfo();
  }

  /**
   * Refresh the page data
   */
  refresh() {
    this.loadUserInfo();
  }
}

/**
 * Profile Manager - Names Page Handler
 *
 * Handles the Names settings page (index: -2)
 */


/**
 * Names Page Handler
 */
class NamesPage extends SimpleSettingsPage {
  constructor(options = {}) {
    super({
      ...options,
      index: -2,
      formSelector: 'form',
    });
  }

  /**
   * Called when the page is initialized
   */
  async onInit() {
    await super.onInit();
    log(Subsystems.MANAGER, Status.DEBUG, '[NamesPage] Initialized (stub)');
  }

  /**
   * Load data - to be implemented when API is available
   */
  async loadData() {
    // TODO: Load user profile data from API
    log(Subsystems.MANAGER, Status.DEBUG, '[NamesPage] loadData (stub)');
  }

  /**
   * Save data - to be implemented when API is available
   */
  async save() {
    // TODO: Save to API
    log(Subsystems.MANAGER, Status.DEBUG, '[NamesPage] save (stub)');
    this.setDirty(false);
    return { success: true };
  }
}

/**
 * Profile Manager - Addresses Page Handler
 *
 * Handles the Addresses settings page (index: -3)
 */


/**
 * Addresses Page Handler
 */
class AddressesPage extends SimpleSettingsPage {
  constructor(options = {}) {
    super({
      ...options,
      index: -3,
      formSelector: 'form',
    });
  }

  async onInit() {
    await super.onInit();
    log(Subsystems.MANAGER, Status.DEBUG, '[AddressesPage] Initialized (stub)');
  }

  async loadData() {
    // TODO: Load from API
  }

  async save() {
    // TODO: Save to API
    this.setDirty(false);
    return { success: true };
  }
}

/**
 * Profile Manager - E-Mail Page Handler
 *
 * Handles the E-Mail settings page (index: -4)
 */


/**
 * Email Page Handler
 */
class EmailPage extends SimpleSettingsPage {
  constructor(options = {}) {
    super({
      ...options,
      index: -4,
      formSelector: 'form',
    });
  }

  async onInit() {
    await super.onInit();
    log(Subsystems.MANAGER, Status.DEBUG, '[EmailPage] Initialized (stub)');
  }

  async loadData() {
    // TODO: Load from API
  }

  async save() {
    // TODO: Save to API
    this.setDirty(false);
    return { success: true };
  }
}

/**
 * Profile Manager - Phone Page Handler
 *
 * Handles the Phone settings page (index: -5)
 */


class PhonePage extends SimpleSettingsPage {
  constructor(options = {}) {
    super({ ...options, index: -5, formSelector: 'form' });
  }
  async onInit() { await super.onInit(); log(Subsystems.MANAGER, Status.DEBUG, '[PhonePage] Initialized (stub)'); }
  async loadData() { /* TODO */ }
  async save() { this.setDirty(false); return { success: true }; }
}

/**
 * Profile Manager - Authentication Page Handler
 *
 * Handles the Authentication settings page (index: -6)
 */


class AuthenticationPage extends BaseSettingsPage {
  constructor(options = {}) {
    super({ ...options, index: -6 });
  }
  async onInit() {
    log(Subsystems.MANAGER, Status.DEBUG, '[AuthenticationPage] Initialized (stub)');
  }
}

/**
 * Profile Manager - Tokens Page Handler
 *
 * Handles the Tokens settings page (index: -7)
 */


class TokensPage extends SimpleSettingsPage {
  constructor(options = {}) {
    super({ ...options, index: -7, formSelector: 'form' });
  }
  async onInit() { await super.onInit(); log(Subsystems.MANAGER, Status.DEBUG, '[TokensPage] Initialized (stub)'); }
}

/**
 * Profile Manager - Language Page Handler
 *
 * Handles the Language settings page (index: -8)
 * Manages language selection with change detection
 */


/**
 * Language Page Handler
 */
class LanguagePage extends SimpleSettingsPage {
  constructor(options = {}) {
    super({
      ...options,
      index: -8,
      formSelector: '#preferences-form',
    });

    this._originalLanguage = null;
  }

  /**
   * Called when the page is initialized
   */
  async onInit() {
    await super.onInit();

    // Store original language for change detection
    const select = this.container?.querySelector('#pref-language');
    if (select) {
      this._originalLanguage = select.value;
    }

    log(Subsystems.MANAGER, Status.DEBUG, '[LanguagePage] Initialized');
  }

  /**
   * Load data into the form
   */
  async loadData() {
    // Load from localStorage or use defaults
    const stored = localStorage.getItem('lithium_preferences');
    if (stored) {
      try {
        const prefs = JSON.parse(stored);
        this.setFormData({ language: prefs.language || 'en-US' });
        this._originalLanguage = prefs.language || 'en-US';
      } catch (error) {
        log(Subsystems.MANAGER, Status.WARN, '[LanguagePage] Failed to load preferences:', error);
      }
    }
  }

  /**
   * Save the form data
   */
  async save() {
    const data = this.getFormData(this.formSelector);

    // Get current preferences
    let prefs = {};
    try {
      const stored = localStorage.getItem('lithium_preferences');
      if (stored) prefs = JSON.parse(stored);
    } catch (_e) {
      // Ignore
    }

    // Update language
    const previousLanguage = prefs.language;
    prefs.language = data.language;

    // Save to localStorage
    localStorage.setItem('lithium_preferences', JSON.stringify(prefs));

    // Emit locale changed event if language changed
    if (previousLanguage !== data.language) {
      eventBus.emit(Events.LOCALE_CHANGED, {
        lang: data.language,
      });
    }

    this._originalLanguage = data.language;
    this.setDirty(false);

    log(Subsystems.MANAGER, Status.INFO, '[LanguagePage] Saved language:', data.language);
    return { success: true, language: data.language };
  }

  /**
   * Cancel changes and reload
   */
  cancel() {
    this.loadData();
    this.setDirty(false);
    log(Subsystems.MANAGER, Status.DEBUG, '[LanguagePage] Cancelled changes');
  }

  /**
   * Check if the page is dirty
   */
  isDirty() {
    const select = this.container?.querySelector('#pref-language');
    if (!select) return false;
    return select.value !== this._originalLanguage;
  }
}

/**
 * TimezonePicker Component
 *
 * A resizable, searchable timezone dropdown picker with grouped display.
 * Used by the Date Formats page for timezone selection.
 */


/**
 * Common timezone abbreviations mapped to IANA identifiers and full names
 */
const TIMEZONE_ABBREVIATIONS = {
  // North America
  'PST': { iana: 'America/Los_Angeles', fullName: 'Pacific Standard Time' },
  'PDT': { iana: 'America/Los_Angeles', fullName: 'Pacific Daylight Time' },
  'MST': { iana: 'America/Denver', fullName: 'Mountain Standard Time' },
  'MDT': { iana: 'America/Denver', fullName: 'Mountain Daylight Time' },
  'CST': { iana: 'America/Chicago', fullName: 'Central Standard Time' },
  'CDT': { iana: 'America/Chicago', fullName: 'Central Daylight Time' },
  'EST': { iana: 'America/New_York', fullName: 'Eastern Standard Time' },
  'EDT': { iana: 'America/New_York', fullName: 'Eastern Daylight Time' },
  'AKST': { iana: 'America/Anchorage', fullName: 'Alaska Standard Time' },
  'AKDT': { iana: 'America/Anchorage', fullName: 'Alaska Daylight Time' },
  'HST': { iana: 'Pacific/Honolulu', fullName: 'Hawaii Standard Time' },

  // Europe
  'GMT': { iana: 'Europe/London', fullName: 'Greenwich Mean Time' },
  'BST': { iana: 'Europe/London', fullName: 'British Summer Time' },
  'CET': { iana: 'Europe/Paris', fullName: 'Central European Time' },
  'CEST': { iana: 'Europe/Paris', fullName: 'Central European Summer Time' },

  // Asia
  'IST': { iana: 'Asia/Kolkata', fullName: 'India Standard Time' },
  'JST': { iana: 'Asia/Tokyo', fullName: 'Japan Standard Time' },
  'KST': { iana: 'Asia/Seoul', fullName: 'Korea Standard Time' },
  'CST_ASIA': { iana: 'Asia/Shanghai', fullName: 'China Standard Time' }, // Note: CST conflicts with US Central

  // Oceania
  'AEST': { iana: 'Australia/Sydney', fullName: 'Australian Eastern Standard Time' },
  'AEDT': { iana: 'Australia/Sydney', fullName: 'Australian Eastern Daylight Time' },
  'ACST': { iana: 'Australia/Adelaide', fullName: 'Australian Central Standard Time' },
  'ACDT': { iana: 'Australia/Adelaide', fullName: 'Australian Central Daylight Time' },
  'AWST': { iana: 'Australia/Perth', fullName: 'Australian Western Standard Time' },

  // UTC
  'UTC': { iana: 'UTC', fullName: 'Coordinated Universal Time' },
};

/**
 * Map IANA identifiers to user-friendly display names for abbreviated timezones
 */
const DISPLAY_NAME_MAP = {
  // UTC and GMT
  'UTC': 'Coordinated Universal Time',
  'GMT': 'Greenwich Mean Time',

  // US Timezones
  'America/Los_Angeles': 'Pacific Time',
  'America/Denver': 'Mountain Time',
  'America/Chicago': 'Central Time',
  'America/New_York': 'Eastern Time',
  'America/Anchorage': 'Alaska Time',
  'Pacific/Honolulu': 'Hawaii Time',

  // European Timezones
  'Europe/Paris': 'Central European Time',
  'Europe/London': 'British Time',
  'Europe/Berlin': 'Central European Time',
  'Europe/Rome': 'Central European Time',
  'Europe/Madrid': 'Central European Time',

  // Asian Timezones
  'Asia/Kolkata': 'India Time',
  'Asia/Tokyo': 'Japan Time',
  'Asia/Shanghai': 'China Time',
  'Asia/Seoul': 'Korea Time',
  'Asia/Bangkok': 'Indochina Time',
  'Asia/Manila': 'Philippine Time',
  'Asia/Singapore': 'Singapore Time',

  // Australian Timezones
  'Australia/Sydney': 'Australian Eastern Time',
  'Australia/Adelaide': 'Australian Central Time',
  'Australia/Perth': 'Australian Western Time',

  // Other major cities
  'Pacific/Auckland': 'New Zealand Time',
  'Africa/Johannesburg': 'South Africa Time',
  'Europe/Moscow': 'Moscow Time',
  'Asia/Yekaterinburg': 'Yekaterinburg Time'
};

class TimezonePicker {
  constructor(container, timezones, onSelect, onPositionChange = null) {
    this.container = container;
    this.timezones = timezones;
    this.onSelect = onSelect;
    this.onPositionChange = onPositionChange;
    this.selectedTimezone = 'local';
    this.isOpen = false;
    this.popupWidth = null;
    this.popupHeight = null;
    this.isResizing = false;
    this._closeTimeout = null;
    this._outsideClickHandler = null;
    this._escKeyHandler = null;
    this._transitioning = false;

    // Create dropdown element
    this.dropdown = document.createElement('div');
    this.dropdown.className = 'df-timezone-dropdown manager-ui-popup';
    this.dropdown.innerHTML = `
      <div class="df-timezone-filter">
        <input type="text" class="df-timezone-filter-input" placeholder="Search timezones...">
        <button type="button" class="df-timezone-filter-clear" title="Clear">&times;</button>
      </div>
      <div class="df-timezone-list"></div>
      <div class="df-timezone-resize-handle"></div>
    `;

    // Cache DOM references
    this.filterInput = this.dropdown.querySelector('.df-timezone-filter-input');
    this.listContainer = this.dropdown.querySelector('.df-timezone-list');
    this.clearButton = this.dropdown.querySelector('.df-timezone-filter-clear');

    // Setup event listeners
    this.container.addEventListener('click', () => this.toggle());
    this.filterInput.addEventListener('input', () => this.filterTimezones());
    this.clearButton.addEventListener('click', () => {
      this.filterInput.value = '';
      this.filterTimezones();
      this.filterInput.focus();
    });

    // Setup resize functionality
    this.setupResize();

    // OverlayScrollbars will be initialized in open() when element is visible

    // Hide dropdown initially
    this.dropdown.style.display = 'none';
    document.body.appendChild(this.dropdown);

    // Set initial display
    this._updateDisplay();

    // Listen for close-all-popups
    document.addEventListener('close-all-popups', () => this.close());
  }

  /**
   * Get the browser timezone IANA name
   */
  _getBrowserTimezone() {
    try {
      return Intl.DateTimeFormat().resolvedOptions().timeZone || 'UTC';
    } catch (_e) {
      return 'UTC';
    }
  }

  /**
   * Setup resize functionality
   */
  setupResize() {
    const resizeHandle = this.dropdown.querySelector('.df-timezone-resize-handle');
    if (!resizeHandle) return;

    let startX, startY, startWidth, startHeight;

    const startResize = (e) => {
      this.isResizing = true;
      startX = e.clientX;
      startY = e.clientY;
      startWidth = this.listContainerWrapper.offsetWidth;
      startHeight = this.listContainerWrapper.offsetHeight;

      document.body.style.cursor = 'nw-resize';
      document.body.style.userSelect = 'none';

      document.addEventListener('mousemove', resize);
      document.addEventListener('mouseup', stopResize);
      e.preventDefault();
    };

    const resize = (e) => {
      if (!this.isResizing) return;

      const newWidth = Math.max(250, startWidth + (e.clientX - startX));
      const newHeight = Math.max(200, startHeight + (e.clientY - startY));

      this.dropdown.style.width = `${newWidth}px`;
      this.dropdown.style.height = `${newHeight}px`;

      // Store the new size
      this.popupWidth = newWidth;
      this.popupHeight = newHeight;

      // Notify position change
      if (this.onPositionChange) {
        this.onPositionChange({ width: newWidth, height: newHeight });
      }
    };

    const stopResize = () => {
      this.isResizing = false;
      document.body.style.cursor = '';
      document.body.style.userSelect = '';

      document.removeEventListener('mousemove', resize);
      document.removeEventListener('mouseup', stopResize);
    };

    resizeHandle.addEventListener('mousedown', startResize);
  }

  toggle() {
    // Prevent toggle if we're in a transition state
    if (this._transitioning) return;

    if (this.isOpen) {
      this.close();
    } else {
      this.open();
    }
  }

  open() {
    if (this.isOpen || this._transitioning) return;

    this._transitioning = true;
    log(Subsystems.UI, Status.DEBUG, '[TimezonePicker.open] Opening popup');

    // Cancel any pending close timeout (popup is being reopened)
    if (this._closeTimeout) {
      clearTimeout(this._closeTimeout);
      this._closeTimeout = null;
      log(Subsystems.UI, Status.DEBUG, '[TimezonePicker.open] Cancelled pending close timeout');
    }

    // Recreate dropdown element if it was removed from DOM
    if (!this.dropdown || !this.dropdown.parentNode) {
      this.dropdown = document.createElement('div');
      this.dropdown.className = 'df-timezone-dropdown manager-ui-popup';
      this.dropdown.innerHTML = `
        <div class="df-timezone-filter">
          <input type="text" class="df-timezone-filter-input" placeholder="Search timezones...">
          <button type="button" class="df-timezone-filter-clear" title="Clear">&times;</button>
        </div>
        <div class="df-timezone-list"></div>
        <div class="df-timezone-resize-handle"></div>
      `;

      // Cache DOM references
      this.filterInput = this.dropdown.querySelector('.df-timezone-filter-input');
      this.listContainer = this.dropdown.querySelector('.df-timezone-list');
      this.clearButton = this.dropdown.querySelector('.df-timezone-filter-clear');

      // Setup event listeners
      this.container.addEventListener('click', () => this.toggle());
      this.filterInput.addEventListener('input', () => this.filterTimezones());
      this.clearButton.addEventListener('click', () => {
        this.filterInput.value = '';
        this.filterTimezones();
        this.filterInput.focus();
      });

      // Setup resize functionality
      this.setupResize();

      document.body.appendChild(this.dropdown);
      log(Subsystems.UI, Status.DEBUG, '[TimezonePicker.open] Recreated dropdown element');
    }

    // Close other popups first
    document.dispatchEvent(new CustomEvent('close-all-popups'));

    this.isOpen = true;
    this.container.classList.add('open');

    // Position dropdown first
    const rect = this.container.getBoundingClientRect();
    this.dropdown.style.top = `${rect.bottom + 2}px`;
    this.dropdown.style.left = `${rect.left}px`;
    this.dropdown.style.width = this.popupWidth ? `${this.popupWidth}px` : `${rect.width}px`;
    this.dropdown.style.height = this.popupHeight ? `${this.popupHeight}px` : '';

    // Force show dropdown
    this.dropdown.style.display = 'flex';

    // Populate the timezone list initially
    this.filterTimezones();

    // Initialize OSB after content is populated
    requestAnimationFrame(() => {
      requestAnimationFrame(() => {
        if (this.listContainer) {
          this.overlayScrollbar = scrollbarManager.initPopup(this.listContainer);
          log(Subsystems.UI, Status.DEBUG, '[TimezonePicker.open] OSB initialized on list container');
        }
      });
    });

    // Setup outside click handler and ESC key support (similar to Flatpickr pattern)
    this._outsideClickHandler = (e) => {
      // Prevent handling if we're transitioning or if click is inside dropdown/container
      if (this._transitioning || !this.dropdown || !this.container) return;
      if (this.dropdown.contains(e.target) || this.container.contains(e.target)) return;

      log(Subsystems.UI, Status.DEBUG, '[TimezonePicker] Outside click detected, closing');
      this.close();
    };

    this._escKeyHandler = (e) => {
      if (e.key === 'Escape' && this.isOpen && !this._transitioning) {
        log(Subsystems.UI, Status.DEBUG, '[TimezonePicker] ESC key detected, closing');
        this.close();
      }
    };

    // Use setTimeout to attach listeners after current click event finishes bubbling
    setTimeout(() => {
      if (!this._outsideClickHandler || !this._escKeyHandler) return;
      document.addEventListener('click', this._outsideClickHandler);
      document.addEventListener('keydown', this._escKeyHandler);
      this._transitioning = false; // Clear transition flag after handlers are set up
    }, 0);

    // Show with animation
    setTimeout(() => this.dropdown.classList.add('visible'), 50);
  }

  close() {
    if (!this.isOpen || this._transitioning) return;

    this._transitioning = true;
    this.isOpen = false;
    this.container.classList.remove('open');
    this.dropdown.classList.remove('visible');

    // Cancel any pending close timeout
    if (this._closeTimeout) {
      clearTimeout(this._closeTimeout);
      this._closeTimeout = null;
    }

    // Remove outside click handler and ESC key handler immediately
    if (this._outsideClickHandler) {
      document.removeEventListener('click', this._outsideClickHandler);
      this._outsideClickHandler = null;
    }
    if (this._escKeyHandler) {
      document.removeEventListener('keydown', this._escKeyHandler);
      this._escKeyHandler = null;
    }

    // Wait for animation to complete before removing from DOM
    this._closeTimeout = setTimeout(() => {
      // Clean up OSB instance
      if (this.overlayScrollbar) {
        log(Subsystems.UI, Status.DEBUG, '[TimezonePicker.close] Destroying OSB instance');
        scrollbarManager.destroy(this.overlayScrollbar);
        this.overlayScrollbar = null;
      }
      // Remove dropdown from DOM completely
      if (this.dropdown && this.dropdown.parentNode) {
        this.dropdown.parentNode.removeChild(this.dropdown);
        log(Subsystems.UI, Status.DEBUG, '[TimezonePicker.close] Removed dropdown from DOM');
      }
      this._transitioning = false; // Clear transition flag after cleanup
      this._closeTimeout = null;
    }, 350); // Match transition duration
  }

  setTimezone(timezone) {
    this.selectedTimezone = timezone;
    this._updateDisplay();
  }

  /**
   * Update the display text based on selected timezone
   */
  _updateDisplay() {
    const displayEl = this.container.querySelector('#df-selected-timezone');
    if (!displayEl) return;

    if (this.selectedTimezone === 'local') {
      const browserTz = this._getBrowserTimezone();
      displayEl.textContent = `Browser Timezone (${browserTz})`;
    } else {
      try {
        const now = DateTime.now().setZone(this.selectedTimezone);
        const abbr = now.toFormat('ZZZZ');
        displayEl.textContent = `${this.selectedTimezone} (${abbr})`;
      } catch (_e) {
        displayEl.textContent = this.selectedTimezone;
      }
    }
  }

  /**
   * Filter and display timezones based on current filter input
   */
  filterTimezones() {
    const filter = this.filterInput.value.toLowerCase();

    log(Subsystems.UI, Status.DEBUG, `[filterTimezones] Called, filter: "${filter}"`);

    // Get the correct target container
    // After OSB initialization on dropdown, content is inside .os-content
    let targetContainer = this.listContainer;
    if (this.overlayScrollbar && !this.overlayScrollbar.destroyed) {
      const elements = this.overlayScrollbar.elements();
      if (elements && elements.content) {
        targetContainer = elements.content;
        log(Subsystems.UI, Status.DEBUG, `[filterTimezones] Using OSB content element`);
      } else {
        log(Subsystems.UI, Status.WARN, `[filterTimezones] OSB instance exists but no content element found`);
      }
    } else {
      log(Subsystems.UI, Status.DEBUG, `[filterTimezones] Using listContainer directly, hasOSB: ${!!this.overlayScrollbar}`);
    }

    log(Subsystems.UI, Status.DEBUG, `[filterTimezones] targetContainer classes: ${targetContainer.className}`);

    // Clear existing items
    targetContainer.innerHTML = '';

    // Group timezones by region
    const grouped = {};
    const abbreviations = {};

    this.timezones.forEach(tz => {
      const parts = tz.split('/');
      const region = parts[0];

      // Handle abbreviations (like UTC, GMT)
      if (parts.length === 1) {
        abbreviations[tz] = tz;
        return;
      }

      if (!grouped[region]) {
        grouped[region] = [];
      }
      grouped[region].push(tz);
    });

    // Sort regions
    const sortedRegions = Object.keys(grouped).sort();

    // Create browser timezone section first
    const browserSection = document.createElement('div');
    browserSection.className = 'df-timezone-group';
    browserSection.innerHTML = '<div class="df-timezone-group-header">Current</div>';

    if ('Browser Timezone'.toLowerCase().includes(filter)) {
      const item = this._createTimezoneItem('Browser Timezone', 'local');
      browserSection.appendChild(item);
    }

    if (browserSection.children.length > 1) { // Has title + at least one item
      targetContainer.appendChild(browserSection);
    }

    // Create abbreviated timezones section
    const abbreviatedSection = document.createElement('div');
    abbreviatedSection.className = 'df-timezone-group';
    abbreviatedSection.innerHTML = '<div class="df-timezone-group-header">Abbreviated Timezones</div>';

    Object.keys(TIMEZONE_ABBREVIATIONS).sort().forEach(abbrev => {
      if (abbrev.toLowerCase().includes(filter) ||
          TIMEZONE_ABBREVIATIONS[abbrev].fullName.toLowerCase().includes(filter)) {
        const mapping = TIMEZONE_ABBREVIATIONS[abbrev];
        const displayName = `${abbrev} (${mapping.fullName})`;
        const item = this._createTimezoneItem(displayName, mapping.iana);
        abbreviatedSection.appendChild(item);
      }
    });

    if (abbreviatedSection.children.length > 1) { // Has title + at least one item
      targetContainer.appendChild(abbreviatedSection);
    }

    // Create major timezones section
    if (Object.keys(abbreviations).length > 0) {
      const majorSection = document.createElement('div');
      majorSection.className = 'df-timezone-group';
      majorSection.innerHTML = '<div class="df-timezone-group-header">Major Timezones</div>';

      Object.keys(abbreviations).sort().forEach(tz => {
        if (tz.toLowerCase().includes(filter)) {
          const friendlyName = DISPLAY_NAME_MAP[tz] || tz.split('/').pop().replace(/_/g, ' ');
          const displayName = `${friendlyName}`;
          const item = this._createTimezoneItem(displayName, tz);
          majorSection.appendChild(item);
        }
      });

      if (majorSection.children.length > 1) { // Has title + at least one item
        targetContainer.appendChild(majorSection);
      }
    }

    // Create region sections
    sortedRegions.forEach(region => {
      const timezones = grouped[region];
      const section = document.createElement('div');
      section.className = 'df-timezone-group';
      section.innerHTML = `<div class="df-timezone-group-header">${region}</div>`;

      timezones.forEach(tz => {
        if (tz.toLowerCase().includes(filter)) {
          const item = this._createTimezoneItem(tz, tz);
          section.appendChild(item);
        }
      });

      if (section.children.length > 1) { // Has title + at least one item
        targetContainer.appendChild(section);
      }
    });
  }

  /**
   * Create a timezone list item
   */
  _createTimezoneItem(displayName, value) {
    const item = document.createElement('div');
    item.className = 'df-timezone-item';
    item.dataset.value = value;

    const tzName = typeof this.selectedTimezone === 'object' ? this.selectedTimezone.name : this.selectedTimezone;
    if (value === tzName) {
      item.classList.add('selected');
    }

    // Get current time in this timezone
    const timeString = (() => {
      try {
        const now = DateTime.now().setZone(value);
        return now.toFormat('HH:mm');
      } catch (_e) {
        return '--:--';
      }
    })();

    item.innerHTML = `
      <span class="df-timezone-name">${displayName}</span>
      <span class="df-timezone-time">${timeString}</span>
    `;

    item.addEventListener('click', () => {
      this.selectedTimezone = value;
      this.onSelect(value);
      this.close();
    });

    return item;
  }

  destroy() {
    // Restore page scrolling in case popup was open during destroy
    document.body.style.overflow = '';
    if (this.overlay) {
      this.overlay.remove();
    }
    if (this.dropdown) {
      this.dropdown.remove();
    }
    if (this.overlayScrollbar) {
      this.overlayScrollbar.destroy();
    }
  }
}

var HOOKS = [
    "onChange",
    "onClose",
    "onDayCreate",
    "onDestroy",
    "onKeyDown",
    "onMonthChange",
    "onOpen",
    "onParseConfig",
    "onReady",
    "onValueUpdate",
    "onYearChange",
    "onPreCalendarPosition",
];
var defaults = {
    _disable: [],
    allowInput: false,
    allowInvalidPreload: false,
    altFormat: "F j, Y",
    altInput: false,
    altInputClass: "form-control input",
    animate: typeof window === "object" &&
        window.navigator.userAgent.indexOf("MSIE") === -1,
    ariaDateFormat: "F j, Y",
    autoFillDefaultTime: true,
    clickOpens: true,
    closeOnSelect: true,
    conjunction: ", ",
    dateFormat: "Y-m-d",
    defaultHour: 12,
    defaultMinute: 0,
    defaultSeconds: 0,
    disable: [],
    disableMobile: false,
    enableSeconds: false,
    enableTime: false,
    errorHandler: function (err) {
        return typeof console !== "undefined" && console.warn(err);
    },
    getWeek: function (givenDate) {
        var date = new Date(givenDate.getTime());
        date.setHours(0, 0, 0, 0);
        date.setDate(date.getDate() + 3 - ((date.getDay() + 6) % 7));
        var week1 = new Date(date.getFullYear(), 0, 4);
        return (1 +
            Math.round(((date.getTime() - week1.getTime()) / 86400000 -
                3 +
                ((week1.getDay() + 6) % 7)) /
                7));
    },
    hourIncrement: 1,
    ignoredFocusElements: [],
    inline: false,
    locale: "default",
    minuteIncrement: 5,
    mode: "single",
    monthSelectorType: "dropdown",
    nextArrow: "<svg version='1.1' xmlns='http://www.w3.org/2000/svg' xmlns:xlink='http://www.w3.org/1999/xlink' viewBox='0 0 17 17'><g></g><path d='M13.207 8.472l-7.854 7.854-0.707-0.707 7.146-7.146-7.146-7.148 0.707-0.707 7.854 7.854z' /></svg>",
    noCalendar: false,
    now: new Date(),
    onChange: [],
    onClose: [],
    onDayCreate: [],
    onDestroy: [],
    onKeyDown: [],
    onMonthChange: [],
    onOpen: [],
    onParseConfig: [],
    onReady: [],
    onValueUpdate: [],
    onYearChange: [],
    onPreCalendarPosition: [],
    plugins: [],
    position: "auto",
    positionElement: undefined,
    prevArrow: "<svg version='1.1' xmlns='http://www.w3.org/2000/svg' xmlns:xlink='http://www.w3.org/1999/xlink' viewBox='0 0 17 17'><g></g><path d='M5.207 8.471l7.146 7.147-0.707 0.707-7.853-7.854 7.854-7.853 0.707 0.707-7.147 7.146z' /></svg>",
    shorthandCurrentMonth: false,
    showMonths: 1,
    static: false,
    time_24hr: false,
    weekNumbers: false,
    wrap: false,
};

var english = {
    weekdays: {
        shorthand: ["Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"],
        longhand: [
            "Sunday",
            "Monday",
            "Tuesday",
            "Wednesday",
            "Thursday",
            "Friday",
            "Saturday",
        ],
    },
    months: {
        shorthand: [
            "Jan",
            "Feb",
            "Mar",
            "Apr",
            "May",
            "Jun",
            "Jul",
            "Aug",
            "Sep",
            "Oct",
            "Nov",
            "Dec",
        ],
        longhand: [
            "January",
            "February",
            "March",
            "April",
            "May",
            "June",
            "July",
            "August",
            "September",
            "October",
            "November",
            "December",
        ],
    },
    daysInMonth: [31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31],
    firstDayOfWeek: 0,
    ordinal: function (nth) {
        var s = nth % 100;
        if (s > 3 && s < 21)
            return "th";
        switch (s % 10) {
            case 1:
                return "st";
            case 2:
                return "nd";
            case 3:
                return "rd";
            default:
                return "th";
        }
    },
    rangeSeparator: " to ",
    weekAbbreviation: "Wk",
    scrollTitle: "Scroll to increment",
    toggleTitle: "Click to toggle",
    amPM: ["AM", "PM"],
    yearAriaLabel: "Year",
    monthAriaLabel: "Month",
    hourAriaLabel: "Hour",
    minuteAriaLabel: "Minute",
    time_24hr: false,
};

var pad = function (number, length) {
    if (length === void 0) { length = 2; }
    return ("000" + number).slice(length * -1);
};
var int = function (bool) { return (bool === true ? 1 : 0); };
function debounce(fn, wait) {
    var t;
    return function () {
        var _this = this;
        var args = arguments;
        clearTimeout(t);
        t = setTimeout(function () { return fn.apply(_this, args); }, wait);
    };
}
var arrayify = function (obj) {
    return obj instanceof Array ? obj : [obj];
};

function toggleClass(elem, className, bool) {
    if (bool === true)
        return elem.classList.add(className);
    elem.classList.remove(className);
}
function createElement(tag, className, content) {
    var e = window.document.createElement(tag);
    className = className || "";
    content = content || "";
    e.className = className;
    if (content !== undefined)
        e.textContent = content;
    return e;
}
function clearNode(node) {
    while (node.firstChild)
        node.removeChild(node.firstChild);
}
function findParent(node, condition) {
    if (condition(node))
        return node;
    else if (node.parentNode)
        return findParent(node.parentNode, condition);
    return undefined;
}
function createNumberInput(inputClassName, opts) {
    var wrapper = createElement("div", "numInputWrapper"), numInput = createElement("input", "numInput " + inputClassName), arrowUp = createElement("span", "arrowUp"), arrowDown = createElement("span", "arrowDown");
    if (navigator.userAgent.indexOf("MSIE 9.0") === -1) {
        numInput.type = "number";
    }
    else {
        numInput.type = "text";
        numInput.pattern = "\\d*";
    }
    if (opts !== undefined)
        for (var key in opts)
            numInput.setAttribute(key, opts[key]);
    wrapper.appendChild(numInput);
    wrapper.appendChild(arrowUp);
    wrapper.appendChild(arrowDown);
    return wrapper;
}
function getEventTarget(event) {
    try {
        if (typeof event.composedPath === "function") {
            var path = event.composedPath();
            return path[0];
        }
        return event.target;
    }
    catch (error) {
        return event.target;
    }
}

var doNothing = function () { return undefined; };
var monthToStr = function (monthNumber, shorthand, locale) { return locale.months[shorthand ? "shorthand" : "longhand"][monthNumber]; };
var revFormat = {
    D: doNothing,
    F: function (dateObj, monthName, locale) {
        dateObj.setMonth(locale.months.longhand.indexOf(monthName));
    },
    G: function (dateObj, hour) {
        dateObj.setHours((dateObj.getHours() >= 12 ? 12 : 0) + parseFloat(hour));
    },
    H: function (dateObj, hour) {
        dateObj.setHours(parseFloat(hour));
    },
    J: function (dateObj, day) {
        dateObj.setDate(parseFloat(day));
    },
    K: function (dateObj, amPM, locale) {
        dateObj.setHours((dateObj.getHours() % 12) +
            12 * int(new RegExp(locale.amPM[1], "i").test(amPM)));
    },
    M: function (dateObj, shortMonth, locale) {
        dateObj.setMonth(locale.months.shorthand.indexOf(shortMonth));
    },
    S: function (dateObj, seconds) {
        dateObj.setSeconds(parseFloat(seconds));
    },
    U: function (_, unixSeconds) { return new Date(parseFloat(unixSeconds) * 1000); },
    W: function (dateObj, weekNum, locale) {
        var weekNumber = parseInt(weekNum);
        var date = new Date(dateObj.getFullYear(), 0, 2 + (weekNumber - 1) * 7, 0, 0, 0, 0);
        date.setDate(date.getDate() - date.getDay() + locale.firstDayOfWeek);
        return date;
    },
    Y: function (dateObj, year) {
        dateObj.setFullYear(parseFloat(year));
    },
    Z: function (_, ISODate) { return new Date(ISODate); },
    d: function (dateObj, day) {
        dateObj.setDate(parseFloat(day));
    },
    h: function (dateObj, hour) {
        dateObj.setHours((dateObj.getHours() >= 12 ? 12 : 0) + parseFloat(hour));
    },
    i: function (dateObj, minutes) {
        dateObj.setMinutes(parseFloat(minutes));
    },
    j: function (dateObj, day) {
        dateObj.setDate(parseFloat(day));
    },
    l: doNothing,
    m: function (dateObj, month) {
        dateObj.setMonth(parseFloat(month) - 1);
    },
    n: function (dateObj, month) {
        dateObj.setMonth(parseFloat(month) - 1);
    },
    s: function (dateObj, seconds) {
        dateObj.setSeconds(parseFloat(seconds));
    },
    u: function (_, unixMillSeconds) {
        return new Date(parseFloat(unixMillSeconds));
    },
    w: doNothing,
    y: function (dateObj, year) {
        dateObj.setFullYear(2000 + parseFloat(year));
    },
};
var tokenRegex = {
    D: "",
    F: "",
    G: "(\\d\\d|\\d)",
    H: "(\\d\\d|\\d)",
    J: "(\\d\\d|\\d)\\w+",
    K: "",
    M: "",
    S: "(\\d\\d|\\d)",
    U: "(.+)",
    W: "(\\d\\d|\\d)",
    Y: "(\\d{4})",
    Z: "(.+)",
    d: "(\\d\\d|\\d)",
    h: "(\\d\\d|\\d)",
    i: "(\\d\\d|\\d)",
    j: "(\\d\\d|\\d)",
    l: "",
    m: "(\\d\\d|\\d)",
    n: "(\\d\\d|\\d)",
    s: "(\\d\\d|\\d)",
    u: "(.+)",
    w: "(\\d\\d|\\d)",
    y: "(\\d{2})",
};
var formats = {
    Z: function (date) { return date.toISOString(); },
    D: function (date, locale, options) {
        return locale.weekdays.shorthand[formats.w(date, locale, options)];
    },
    F: function (date, locale, options) {
        return monthToStr(formats.n(date, locale, options) - 1, false, locale);
    },
    G: function (date, locale, options) {
        return pad(formats.h(date, locale, options));
    },
    H: function (date) { return pad(date.getHours()); },
    J: function (date, locale) {
        return locale.ordinal !== undefined
            ? date.getDate() + locale.ordinal(date.getDate())
            : date.getDate();
    },
    K: function (date, locale) { return locale.amPM[int(date.getHours() > 11)]; },
    M: function (date, locale) {
        return monthToStr(date.getMonth(), true, locale);
    },
    S: function (date) { return pad(date.getSeconds()); },
    U: function (date) { return date.getTime() / 1000; },
    W: function (date, _, options) {
        return options.getWeek(date);
    },
    Y: function (date) { return pad(date.getFullYear(), 4); },
    d: function (date) { return pad(date.getDate()); },
    h: function (date) { return (date.getHours() % 12 ? date.getHours() % 12 : 12); },
    i: function (date) { return pad(date.getMinutes()); },
    j: function (date) { return date.getDate(); },
    l: function (date, locale) {
        return locale.weekdays.longhand[date.getDay()];
    },
    m: function (date) { return pad(date.getMonth() + 1); },
    n: function (date) { return date.getMonth() + 1; },
    s: function (date) { return date.getSeconds(); },
    u: function (date) { return date.getTime(); },
    w: function (date) { return date.getDay(); },
    y: function (date) { return String(date.getFullYear()).substring(2); },
};

var createDateFormatter = function (_a) {
    var _b = _a.config, config = _b === void 0 ? defaults : _b, _c = _a.l10n, l10n = _c === void 0 ? english : _c, _d = _a.isMobile, isMobile = _d === void 0 ? false : _d;
    return function (dateObj, frmt, overrideLocale) {
        var locale = overrideLocale || l10n;
        if (config.formatDate !== undefined && !isMobile) {
            return config.formatDate(dateObj, frmt, locale);
        }
        return frmt
            .split("")
            .map(function (c, i, arr) {
            return formats[c] && arr[i - 1] !== "\\"
                ? formats[c](dateObj, locale, config)
                : c !== "\\"
                    ? c
                    : "";
        })
            .join("");
    };
};
var createDateParser = function (_a) {
    var _b = _a.config, config = _b === void 0 ? defaults : _b, _c = _a.l10n, l10n = _c === void 0 ? english : _c;
    return function (date, givenFormat, timeless, customLocale) {
        if (date !== 0 && !date)
            return undefined;
        var locale = customLocale || l10n;
        var parsedDate;
        var dateOrig = date;
        if (date instanceof Date)
            parsedDate = new Date(date.getTime());
        else if (typeof date !== "string" &&
            date.toFixed !== undefined)
            parsedDate = new Date(date);
        else if (typeof date === "string") {
            var format = givenFormat || (config || defaults).dateFormat;
            var datestr = String(date).trim();
            if (datestr === "today") {
                parsedDate = new Date();
                timeless = true;
            }
            else if (config && config.parseDate) {
                parsedDate = config.parseDate(date, format);
            }
            else if (/Z$/.test(datestr) ||
                /GMT$/.test(datestr)) {
                parsedDate = new Date(date);
            }
            else {
                var matched = void 0, ops = [];
                for (var i = 0, matchIndex = 0, regexStr = ""; i < format.length; i++) {
                    var token = format[i];
                    var isBackSlash = token === "\\";
                    var escaped = format[i - 1] === "\\" || isBackSlash;
                    if (tokenRegex[token] && !escaped) {
                        regexStr += tokenRegex[token];
                        var match = new RegExp(regexStr).exec(date);
                        if (match && (matched = true)) {
                            ops[token !== "Y" ? "push" : "unshift"]({
                                fn: revFormat[token],
                                val: match[++matchIndex],
                            });
                        }
                    }
                    else if (!isBackSlash)
                        regexStr += ".";
                }
                parsedDate =
                    !config || !config.noCalendar
                        ? new Date(new Date().getFullYear(), 0, 1, 0, 0, 0, 0)
                        : new Date(new Date().setHours(0, 0, 0, 0));
                ops.forEach(function (_a) {
                    var fn = _a.fn, val = _a.val;
                    return (parsedDate = fn(parsedDate, val, locale) || parsedDate);
                });
                parsedDate = matched ? parsedDate : undefined;
            }
        }
        if (!(parsedDate instanceof Date && !isNaN(parsedDate.getTime()))) {
            config.errorHandler(new Error("Invalid date provided: " + dateOrig));
            return undefined;
        }
        if (timeless === true)
            parsedDate.setHours(0, 0, 0, 0);
        return parsedDate;
    };
};
function compareDates(date1, date2, timeless) {
    if (timeless === void 0) { timeless = true; }
    if (timeless !== false) {
        return (new Date(date1.getTime()).setHours(0, 0, 0, 0) -
            new Date(date2.getTime()).setHours(0, 0, 0, 0));
    }
    return date1.getTime() - date2.getTime();
}
var isBetween = function (ts, ts1, ts2) {
    return ts > Math.min(ts1, ts2) && ts < Math.max(ts1, ts2);
};
var calculateSecondsSinceMidnight = function (hours, minutes, seconds) {
    return hours * 3600 + minutes * 60 + seconds;
};
var parseSeconds = function (secondsSinceMidnight) {
    var hours = Math.floor(secondsSinceMidnight / 3600), minutes = (secondsSinceMidnight - hours * 3600) / 60;
    return [hours, minutes, secondsSinceMidnight - hours * 3600 - minutes * 60];
};
var duration = {
    DAY: 86400000,
};
function getDefaultHours(config) {
    var hours = config.defaultHour;
    var minutes = config.defaultMinute;
    var seconds = config.defaultSeconds;
    if (config.minDate !== undefined) {
        var minHour = config.minDate.getHours();
        var minMinutes = config.minDate.getMinutes();
        var minSeconds = config.minDate.getSeconds();
        if (hours < minHour) {
            hours = minHour;
        }
        if (hours === minHour && minutes < minMinutes) {
            minutes = minMinutes;
        }
        if (hours === minHour && minutes === minMinutes && seconds < minSeconds)
            seconds = config.minDate.getSeconds();
    }
    if (config.maxDate !== undefined) {
        var maxHr = config.maxDate.getHours();
        var maxMinutes = config.maxDate.getMinutes();
        hours = Math.min(hours, maxHr);
        if (hours === maxHr)
            minutes = Math.min(maxMinutes, minutes);
        if (hours === maxHr && minutes === maxMinutes)
            seconds = config.maxDate.getSeconds();
    }
    return { hours: hours, minutes: minutes, seconds: seconds };
}

if (typeof Object.assign !== "function") {
    Object.assign = function (target) {
        var args = [];
        for (var _i = 1; _i < arguments.length; _i++) {
            args[_i - 1] = arguments[_i];
        }
        if (!target) {
            throw TypeError("Cannot convert undefined or null to object");
        }
        var _loop_1 = function (source) {
            if (source) {
                Object.keys(source).forEach(function (key) { return (target[key] = source[key]); });
            }
        };
        for (var _a = 0, args_1 = args; _a < args_1.length; _a++) {
            var source = args_1[_a];
            _loop_1(source);
        }
        return target;
    };
}

var __assign = (undefined && undefined.__assign) || function () {
    __assign = Object.assign || function(t) {
        for (var s, i = 1, n = arguments.length; i < n; i++) {
            s = arguments[i];
            for (var p in s) if (Object.prototype.hasOwnProperty.call(s, p))
                t[p] = s[p];
        }
        return t;
    };
    return __assign.apply(this, arguments);
};
var __spreadArrays = (undefined && undefined.__spreadArrays) || function () {
    for (var s = 0, i = 0, il = arguments.length; i < il; i++) s += arguments[i].length;
    for (var r = Array(s), k = 0, i = 0; i < il; i++)
        for (var a = arguments[i], j = 0, jl = a.length; j < jl; j++, k++)
            r[k] = a[j];
    return r;
};
var DEBOUNCED_CHANGE_MS = 300;
function FlatpickrInstance(element, instanceConfig) {
    var self = {
        config: __assign(__assign({}, defaults), flatpickr.defaultConfig),
        l10n: english,
    };
    self.parseDate = createDateParser({ config: self.config, l10n: self.l10n });
    self._handlers = [];
    self.pluginElements = [];
    self.loadedPlugins = [];
    self._bind = bind;
    self._setHoursFromDate = setHoursFromDate;
    self._positionCalendar = positionCalendar;
    self.changeMonth = changeMonth;
    self.changeYear = changeYear;
    self.clear = clear;
    self.close = close;
    self.onMouseOver = onMouseOver;
    self._createElement = createElement;
    self.createDay = createDay;
    self.destroy = destroy;
    self.isEnabled = isEnabled;
    self.jumpToDate = jumpToDate;
    self.updateValue = updateValue;
    self.open = open;
    self.redraw = redraw;
    self.set = set;
    self.setDate = setDate;
    self.toggle = toggle;
    function setupHelperFunctions() {
        self.utils = {
            getDaysInMonth: function (month, yr) {
                if (month === void 0) { month = self.currentMonth; }
                if (yr === void 0) { yr = self.currentYear; }
                if (month === 1 && ((yr % 4 === 0 && yr % 100 !== 0) || yr % 400 === 0))
                    return 29;
                return self.l10n.daysInMonth[month];
            },
        };
    }
    function init() {
        self.element = self.input = element;
        self.isOpen = false;
        parseConfig();
        setupLocale();
        setupInputs();
        setupDates();
        setupHelperFunctions();
        if (!self.isMobile)
            build();
        bindEvents();
        if (self.selectedDates.length || self.config.noCalendar) {
            if (self.config.enableTime) {
                setHoursFromDate(self.config.noCalendar ? self.latestSelectedDateObj : undefined);
            }
            updateValue(false);
        }
        setCalendarWidth();
        var isSafari = /^((?!chrome|android).)*safari/i.test(navigator.userAgent);
        if (!self.isMobile && isSafari) {
            positionCalendar();
        }
        triggerEvent("onReady");
    }
    function getClosestActiveElement() {
        var _a;
        return (((_a = self.calendarContainer) === null || _a === void 0 ? void 0 : _a.getRootNode())
            .activeElement || document.activeElement);
    }
    function bindToInstance(fn) {
        return fn.bind(self);
    }
    function setCalendarWidth() {
        var config = self.config;
        if (config.weekNumbers === false && config.showMonths === 1) {
            return;
        }
        else if (config.noCalendar !== true) {
            window.requestAnimationFrame(function () {
                if (self.calendarContainer !== undefined) {
                    self.calendarContainer.style.visibility = "hidden";
                    self.calendarContainer.style.display = "block";
                }
                if (self.daysContainer !== undefined) {
                    var daysWidth = (self.days.offsetWidth + 1) * config.showMonths;
                    self.daysContainer.style.width = daysWidth + "px";
                    self.calendarContainer.style.width =
                        daysWidth +
                            (self.weekWrapper !== undefined
                                ? self.weekWrapper.offsetWidth
                                : 0) +
                            "px";
                    self.calendarContainer.style.removeProperty("visibility");
                    self.calendarContainer.style.removeProperty("display");
                }
            });
        }
    }
    function updateTime(e) {
        if (self.selectedDates.length === 0) {
            var defaultDate = self.config.minDate === undefined ||
                compareDates(new Date(), self.config.minDate) >= 0
                ? new Date()
                : new Date(self.config.minDate.getTime());
            var defaults = getDefaultHours(self.config);
            defaultDate.setHours(defaults.hours, defaults.minutes, defaults.seconds, defaultDate.getMilliseconds());
            self.selectedDates = [defaultDate];
            self.latestSelectedDateObj = defaultDate;
        }
        if (e !== undefined && e.type !== "blur") {
            timeWrapper(e);
        }
        var prevValue = self._input.value;
        setHoursFromInputs();
        updateValue();
        if (self._input.value !== prevValue) {
            self._debouncedChange();
        }
    }
    function ampm2military(hour, amPM) {
        return (hour % 12) + 12 * int(amPM === self.l10n.amPM[1]);
    }
    function military2ampm(hour) {
        switch (hour % 24) {
            case 0:
            case 12:
                return 12;
            default:
                return hour % 12;
        }
    }
    function setHoursFromInputs() {
        if (self.hourElement === undefined || self.minuteElement === undefined)
            return;
        var hours = (parseInt(self.hourElement.value.slice(-2), 10) || 0) % 24, minutes = (parseInt(self.minuteElement.value, 10) || 0) % 60, seconds = self.secondElement !== undefined
            ? (parseInt(self.secondElement.value, 10) || 0) % 60
            : 0;
        if (self.amPM !== undefined) {
            hours = ampm2military(hours, self.amPM.textContent);
        }
        var limitMinHours = self.config.minTime !== undefined ||
            (self.config.minDate &&
                self.minDateHasTime &&
                self.latestSelectedDateObj &&
                compareDates(self.latestSelectedDateObj, self.config.minDate, true) ===
                    0);
        var limitMaxHours = self.config.maxTime !== undefined ||
            (self.config.maxDate &&
                self.maxDateHasTime &&
                self.latestSelectedDateObj &&
                compareDates(self.latestSelectedDateObj, self.config.maxDate, true) ===
                    0);
        if (self.config.maxTime !== undefined &&
            self.config.minTime !== undefined &&
            self.config.minTime > self.config.maxTime) {
            var minBound = calculateSecondsSinceMidnight(self.config.minTime.getHours(), self.config.minTime.getMinutes(), self.config.minTime.getSeconds());
            var maxBound = calculateSecondsSinceMidnight(self.config.maxTime.getHours(), self.config.maxTime.getMinutes(), self.config.maxTime.getSeconds());
            var currentTime = calculateSecondsSinceMidnight(hours, minutes, seconds);
            if (currentTime > maxBound && currentTime < minBound) {
                var result = parseSeconds(minBound);
                hours = result[0];
                minutes = result[1];
                seconds = result[2];
            }
        }
        else {
            if (limitMaxHours) {
                var maxTime = self.config.maxTime !== undefined
                    ? self.config.maxTime
                    : self.config.maxDate;
                hours = Math.min(hours, maxTime.getHours());
                if (hours === maxTime.getHours())
                    minutes = Math.min(minutes, maxTime.getMinutes());
                if (minutes === maxTime.getMinutes())
                    seconds = Math.min(seconds, maxTime.getSeconds());
            }
            if (limitMinHours) {
                var minTime = self.config.minTime !== undefined
                    ? self.config.minTime
                    : self.config.minDate;
                hours = Math.max(hours, minTime.getHours());
                if (hours === minTime.getHours() && minutes < minTime.getMinutes())
                    minutes = minTime.getMinutes();
                if (minutes === minTime.getMinutes())
                    seconds = Math.max(seconds, minTime.getSeconds());
            }
        }
        setHours(hours, minutes, seconds);
    }
    function setHoursFromDate(dateObj) {
        var date = dateObj || self.latestSelectedDateObj;
        if (date && date instanceof Date) {
            setHours(date.getHours(), date.getMinutes(), date.getSeconds());
        }
    }
    function setHours(hours, minutes, seconds) {
        if (self.latestSelectedDateObj !== undefined) {
            self.latestSelectedDateObj.setHours(hours % 24, minutes, seconds || 0, 0);
        }
        if (!self.hourElement || !self.minuteElement || self.isMobile)
            return;
        self.hourElement.value = pad(!self.config.time_24hr
            ? ((12 + hours) % 12) + 12 * int(hours % 12 === 0)
            : hours);
        self.minuteElement.value = pad(minutes);
        if (self.amPM !== undefined)
            self.amPM.textContent = self.l10n.amPM[int(hours >= 12)];
        if (self.secondElement !== undefined)
            self.secondElement.value = pad(seconds);
    }
    function onYearInput(event) {
        var eventTarget = getEventTarget(event);
        var year = parseInt(eventTarget.value) + (event.delta || 0);
        if (year / 1000 > 1 ||
            (event.key === "Enter" && !/[^\d]/.test(year.toString()))) {
            changeYear(year);
        }
    }
    function bind(element, event, handler, options) {
        if (event instanceof Array)
            return event.forEach(function (ev) { return bind(element, ev, handler, options); });
        if (element instanceof Array)
            return element.forEach(function (el) { return bind(el, event, handler, options); });
        element.addEventListener(event, handler, options);
        self._handlers.push({
            remove: function () { return element.removeEventListener(event, handler, options); },
        });
    }
    function triggerChange() {
        triggerEvent("onChange");
    }
    function bindEvents() {
        if (self.config.wrap) {
            ["open", "close", "toggle", "clear"].forEach(function (evt) {
                Array.prototype.forEach.call(self.element.querySelectorAll("[data-" + evt + "]"), function (el) {
                    return bind(el, "click", self[evt]);
                });
            });
        }
        if (self.isMobile) {
            setupMobile();
            return;
        }
        var debouncedResize = debounce(onResize, 50);
        self._debouncedChange = debounce(triggerChange, DEBOUNCED_CHANGE_MS);
        if (self.daysContainer && !/iPhone|iPad|iPod/i.test(navigator.userAgent))
            bind(self.daysContainer, "mouseover", function (e) {
                if (self.config.mode === "range")
                    onMouseOver(getEventTarget(e));
            });
        bind(self._input, "keydown", onKeyDown);
        if (self.calendarContainer !== undefined) {
            bind(self.calendarContainer, "keydown", onKeyDown);
        }
        if (!self.config.inline && !self.config.static)
            bind(window, "resize", debouncedResize);
        if (window.ontouchstart !== undefined)
            bind(window.document, "touchstart", documentClick);
        else
            bind(window.document, "mousedown", documentClick);
        bind(window.document, "focus", documentClick, { capture: true });
        if (self.config.clickOpens === true) {
            bind(self._input, "focus", self.open);
            bind(self._input, "click", self.open);
        }
        if (self.daysContainer !== undefined) {
            bind(self.monthNav, "click", onMonthNavClick);
            bind(self.monthNav, ["keyup", "increment"], onYearInput);
            bind(self.daysContainer, "click", selectDate);
        }
        if (self.timeContainer !== undefined &&
            self.minuteElement !== undefined &&
            self.hourElement !== undefined) {
            var selText = function (e) {
                return getEventTarget(e).select();
            };
            bind(self.timeContainer, ["increment"], updateTime);
            bind(self.timeContainer, "blur", updateTime, { capture: true });
            bind(self.timeContainer, "click", timeIncrement);
            bind([self.hourElement, self.minuteElement], ["focus", "click"], selText);
            if (self.secondElement !== undefined)
                bind(self.secondElement, "focus", function () { return self.secondElement && self.secondElement.select(); });
            if (self.amPM !== undefined) {
                bind(self.amPM, "click", function (e) {
                    updateTime(e);
                });
            }
        }
        if (self.config.allowInput) {
            bind(self._input, "blur", onBlur);
        }
    }
    function jumpToDate(jumpDate, triggerChange) {
        var jumpTo = jumpDate !== undefined
            ? self.parseDate(jumpDate)
            : self.latestSelectedDateObj ||
                (self.config.minDate && self.config.minDate > self.now
                    ? self.config.minDate
                    : self.config.maxDate && self.config.maxDate < self.now
                        ? self.config.maxDate
                        : self.now);
        var oldYear = self.currentYear;
        var oldMonth = self.currentMonth;
        try {
            if (jumpTo !== undefined) {
                self.currentYear = jumpTo.getFullYear();
                self.currentMonth = jumpTo.getMonth();
            }
        }
        catch (e) {
            e.message = "Invalid date supplied: " + jumpTo;
            self.config.errorHandler(e);
        }
        if (triggerChange && self.currentYear !== oldYear) {
            triggerEvent("onYearChange");
            buildMonthSwitch();
        }
        if (triggerChange &&
            (self.currentYear !== oldYear || self.currentMonth !== oldMonth)) {
            triggerEvent("onMonthChange");
        }
        self.redraw();
    }
    function timeIncrement(e) {
        var eventTarget = getEventTarget(e);
        if (~eventTarget.className.indexOf("arrow"))
            incrementNumInput(e, eventTarget.classList.contains("arrowUp") ? 1 : -1);
    }
    function incrementNumInput(e, delta, inputElem) {
        var target = e && getEventTarget(e);
        var input = inputElem ||
            (target && target.parentNode && target.parentNode.firstChild);
        var event = createEvent("increment");
        event.delta = delta;
        input && input.dispatchEvent(event);
    }
    function build() {
        var fragment = window.document.createDocumentFragment();
        self.calendarContainer = createElement("div", "flatpickr-calendar");
        self.calendarContainer.tabIndex = -1;
        if (!self.config.noCalendar) {
            fragment.appendChild(buildMonthNav());
            self.innerContainer = createElement("div", "flatpickr-innerContainer");
            if (self.config.weekNumbers) {
                var _a = buildWeeks(), weekWrapper = _a.weekWrapper, weekNumbers = _a.weekNumbers;
                self.innerContainer.appendChild(weekWrapper);
                self.weekNumbers = weekNumbers;
                self.weekWrapper = weekWrapper;
            }
            self.rContainer = createElement("div", "flatpickr-rContainer");
            self.rContainer.appendChild(buildWeekdays());
            if (!self.daysContainer) {
                self.daysContainer = createElement("div", "flatpickr-days");
                self.daysContainer.tabIndex = -1;
            }
            buildDays();
            self.rContainer.appendChild(self.daysContainer);
            self.innerContainer.appendChild(self.rContainer);
            fragment.appendChild(self.innerContainer);
        }
        if (self.config.enableTime) {
            fragment.appendChild(buildTime());
        }
        toggleClass(self.calendarContainer, "rangeMode", self.config.mode === "range");
        toggleClass(self.calendarContainer, "animate", self.config.animate === true);
        toggleClass(self.calendarContainer, "multiMonth", self.config.showMonths > 1);
        self.calendarContainer.appendChild(fragment);
        var customAppend = self.config.appendTo !== undefined &&
            self.config.appendTo.nodeType !== undefined;
        if (self.config.inline || self.config.static) {
            self.calendarContainer.classList.add(self.config.inline ? "inline" : "static");
            if (self.config.inline) {
                if (!customAppend && self.element.parentNode)
                    self.element.parentNode.insertBefore(self.calendarContainer, self._input.nextSibling);
                else if (self.config.appendTo !== undefined)
                    self.config.appendTo.appendChild(self.calendarContainer);
            }
            if (self.config.static) {
                var wrapper = createElement("div", "flatpickr-wrapper");
                if (self.element.parentNode)
                    self.element.parentNode.insertBefore(wrapper, self.element);
                wrapper.appendChild(self.element);
                if (self.altInput)
                    wrapper.appendChild(self.altInput);
                wrapper.appendChild(self.calendarContainer);
            }
        }
        if (!self.config.static && !self.config.inline)
            (self.config.appendTo !== undefined
                ? self.config.appendTo
                : window.document.body).appendChild(self.calendarContainer);
    }
    function createDay(className, date, _dayNumber, i) {
        var dateIsEnabled = isEnabled(date, true), dayElement = createElement("span", className, date.getDate().toString());
        dayElement.dateObj = date;
        dayElement.$i = i;
        dayElement.setAttribute("aria-label", self.formatDate(date, self.config.ariaDateFormat));
        if (className.indexOf("hidden") === -1 &&
            compareDates(date, self.now) === 0) {
            self.todayDateElem = dayElement;
            dayElement.classList.add("today");
            dayElement.setAttribute("aria-current", "date");
        }
        if (dateIsEnabled) {
            dayElement.tabIndex = -1;
            if (isDateSelected(date)) {
                dayElement.classList.add("selected");
                self.selectedDateElem = dayElement;
                if (self.config.mode === "range") {
                    toggleClass(dayElement, "startRange", self.selectedDates[0] &&
                        compareDates(date, self.selectedDates[0], true) === 0);
                    toggleClass(dayElement, "endRange", self.selectedDates[1] &&
                        compareDates(date, self.selectedDates[1], true) === 0);
                    if (className === "nextMonthDay")
                        dayElement.classList.add("inRange");
                }
            }
        }
        else {
            dayElement.classList.add("flatpickr-disabled");
        }
        if (self.config.mode === "range") {
            if (isDateInRange(date) && !isDateSelected(date))
                dayElement.classList.add("inRange");
        }
        if (self.weekNumbers &&
            self.config.showMonths === 1 &&
            className !== "prevMonthDay" &&
            i % 7 === 6) {
            self.weekNumbers.insertAdjacentHTML("beforeend", "<span class='flatpickr-day'>" + self.config.getWeek(date) + "</span>");
        }
        triggerEvent("onDayCreate", dayElement);
        return dayElement;
    }
    function focusOnDayElem(targetNode) {
        targetNode.focus();
        if (self.config.mode === "range")
            onMouseOver(targetNode);
    }
    function getFirstAvailableDay(delta) {
        var startMonth = delta > 0 ? 0 : self.config.showMonths - 1;
        var endMonth = delta > 0 ? self.config.showMonths : -1;
        for (var m = startMonth; m != endMonth; m += delta) {
            var month = self.daysContainer.children[m];
            var startIndex = delta > 0 ? 0 : month.children.length - 1;
            var endIndex = delta > 0 ? month.children.length : -1;
            for (var i = startIndex; i != endIndex; i += delta) {
                var c = month.children[i];
                if (c.className.indexOf("hidden") === -1 && isEnabled(c.dateObj))
                    return c;
            }
        }
        return undefined;
    }
    function getNextAvailableDay(current, delta) {
        var givenMonth = current.className.indexOf("Month") === -1
            ? current.dateObj.getMonth()
            : self.currentMonth;
        var endMonth = delta > 0 ? self.config.showMonths : -1;
        var loopDelta = delta > 0 ? 1 : -1;
        for (var m = givenMonth - self.currentMonth; m != endMonth; m += loopDelta) {
            var month = self.daysContainer.children[m];
            var startIndex = givenMonth - self.currentMonth === m
                ? current.$i + delta
                : delta < 0
                    ? month.children.length - 1
                    : 0;
            var numMonthDays = month.children.length;
            for (var i = startIndex; i >= 0 && i < numMonthDays && i != (delta > 0 ? numMonthDays : -1); i += loopDelta) {
                var c = month.children[i];
                if (c.className.indexOf("hidden") === -1 &&
                    isEnabled(c.dateObj) &&
                    Math.abs(current.$i - i) >= Math.abs(delta))
                    return focusOnDayElem(c);
            }
        }
        self.changeMonth(loopDelta);
        focusOnDay(getFirstAvailableDay(loopDelta), 0);
        return undefined;
    }
    function focusOnDay(current, offset) {
        var activeElement = getClosestActiveElement();
        var dayFocused = isInView(activeElement || document.body);
        var startElem = current !== undefined
            ? current
            : dayFocused
                ? activeElement
                : self.selectedDateElem !== undefined && isInView(self.selectedDateElem)
                    ? self.selectedDateElem
                    : self.todayDateElem !== undefined && isInView(self.todayDateElem)
                        ? self.todayDateElem
                        : getFirstAvailableDay(offset > 0 ? 1 : -1);
        if (startElem === undefined) {
            self._input.focus();
        }
        else if (!dayFocused) {
            focusOnDayElem(startElem);
        }
        else {
            getNextAvailableDay(startElem, offset);
        }
    }
    function buildMonthDays(year, month) {
        var firstOfMonth = (new Date(year, month, 1).getDay() - self.l10n.firstDayOfWeek + 7) % 7;
        var prevMonthDays = self.utils.getDaysInMonth((month - 1 + 12) % 12, year);
        var daysInMonth = self.utils.getDaysInMonth(month, year), days = window.document.createDocumentFragment(), isMultiMonth = self.config.showMonths > 1, prevMonthDayClass = isMultiMonth ? "prevMonthDay hidden" : "prevMonthDay", nextMonthDayClass = isMultiMonth ? "nextMonthDay hidden" : "nextMonthDay";
        var dayNumber = prevMonthDays + 1 - firstOfMonth, dayIndex = 0;
        for (; dayNumber <= prevMonthDays; dayNumber++, dayIndex++) {
            days.appendChild(createDay("flatpickr-day " + prevMonthDayClass, new Date(year, month - 1, dayNumber), dayNumber, dayIndex));
        }
        for (dayNumber = 1; dayNumber <= daysInMonth; dayNumber++, dayIndex++) {
            days.appendChild(createDay("flatpickr-day", new Date(year, month, dayNumber), dayNumber, dayIndex));
        }
        for (var dayNum = daysInMonth + 1; dayNum <= 42 - firstOfMonth &&
            (self.config.showMonths === 1 || dayIndex % 7 !== 0); dayNum++, dayIndex++) {
            days.appendChild(createDay("flatpickr-day " + nextMonthDayClass, new Date(year, month + 1, dayNum % daysInMonth), dayNum, dayIndex));
        }
        var dayContainer = createElement("div", "dayContainer");
        dayContainer.appendChild(days);
        return dayContainer;
    }
    function buildDays() {
        if (self.daysContainer === undefined) {
            return;
        }
        clearNode(self.daysContainer);
        if (self.weekNumbers)
            clearNode(self.weekNumbers);
        var frag = document.createDocumentFragment();
        for (var i = 0; i < self.config.showMonths; i++) {
            var d = new Date(self.currentYear, self.currentMonth, 1);
            d.setMonth(self.currentMonth + i);
            frag.appendChild(buildMonthDays(d.getFullYear(), d.getMonth()));
        }
        self.daysContainer.appendChild(frag);
        self.days = self.daysContainer.firstChild;
        if (self.config.mode === "range" && self.selectedDates.length === 1) {
            onMouseOver();
        }
    }
    function buildMonthSwitch() {
        if (self.config.showMonths > 1 ||
            self.config.monthSelectorType !== "dropdown")
            return;
        var shouldBuildMonth = function (month) {
            if (self.config.minDate !== undefined &&
                self.currentYear === self.config.minDate.getFullYear() &&
                month < self.config.minDate.getMonth()) {
                return false;
            }
            return !(self.config.maxDate !== undefined &&
                self.currentYear === self.config.maxDate.getFullYear() &&
                month > self.config.maxDate.getMonth());
        };
        self.monthsDropdownContainer.tabIndex = -1;
        self.monthsDropdownContainer.innerHTML = "";
        for (var i = 0; i < 12; i++) {
            if (!shouldBuildMonth(i))
                continue;
            var month = createElement("option", "flatpickr-monthDropdown-month");
            month.value = new Date(self.currentYear, i).getMonth().toString();
            month.textContent = monthToStr(i, self.config.shorthandCurrentMonth, self.l10n);
            month.tabIndex = -1;
            if (self.currentMonth === i) {
                month.selected = true;
            }
            self.monthsDropdownContainer.appendChild(month);
        }
    }
    function buildMonth() {
        var container = createElement("div", "flatpickr-month");
        var monthNavFragment = window.document.createDocumentFragment();
        var monthElement;
        if (self.config.showMonths > 1 ||
            self.config.monthSelectorType === "static") {
            monthElement = createElement("span", "cur-month");
        }
        else {
            self.monthsDropdownContainer = createElement("select", "flatpickr-monthDropdown-months");
            self.monthsDropdownContainer.setAttribute("aria-label", self.l10n.monthAriaLabel);
            bind(self.monthsDropdownContainer, "change", function (e) {
                var target = getEventTarget(e);
                var selectedMonth = parseInt(target.value, 10);
                self.changeMonth(selectedMonth - self.currentMonth);
                triggerEvent("onMonthChange");
            });
            buildMonthSwitch();
            monthElement = self.monthsDropdownContainer;
        }
        var yearInput = createNumberInput("cur-year", { tabindex: "-1" });
        var yearElement = yearInput.getElementsByTagName("input")[0];
        yearElement.setAttribute("aria-label", self.l10n.yearAriaLabel);
        if (self.config.minDate) {
            yearElement.setAttribute("min", self.config.minDate.getFullYear().toString());
        }
        if (self.config.maxDate) {
            yearElement.setAttribute("max", self.config.maxDate.getFullYear().toString());
            yearElement.disabled =
                !!self.config.minDate &&
                    self.config.minDate.getFullYear() === self.config.maxDate.getFullYear();
        }
        var currentMonth = createElement("div", "flatpickr-current-month");
        currentMonth.appendChild(monthElement);
        currentMonth.appendChild(yearInput);
        monthNavFragment.appendChild(currentMonth);
        container.appendChild(monthNavFragment);
        return {
            container: container,
            yearElement: yearElement,
            monthElement: monthElement,
        };
    }
    function buildMonths() {
        clearNode(self.monthNav);
        self.monthNav.appendChild(self.prevMonthNav);
        if (self.config.showMonths) {
            self.yearElements = [];
            self.monthElements = [];
        }
        for (var m = self.config.showMonths; m--;) {
            var month = buildMonth();
            self.yearElements.push(month.yearElement);
            self.monthElements.push(month.monthElement);
            self.monthNav.appendChild(month.container);
        }
        self.monthNav.appendChild(self.nextMonthNav);
    }
    function buildMonthNav() {
        self.monthNav = createElement("div", "flatpickr-months");
        self.yearElements = [];
        self.monthElements = [];
        self.prevMonthNav = createElement("span", "flatpickr-prev-month");
        self.prevMonthNav.innerHTML = self.config.prevArrow;
        self.nextMonthNav = createElement("span", "flatpickr-next-month");
        self.nextMonthNav.innerHTML = self.config.nextArrow;
        buildMonths();
        Object.defineProperty(self, "_hidePrevMonthArrow", {
            get: function () { return self.__hidePrevMonthArrow; },
            set: function (bool) {
                if (self.__hidePrevMonthArrow !== bool) {
                    toggleClass(self.prevMonthNav, "flatpickr-disabled", bool);
                    self.__hidePrevMonthArrow = bool;
                }
            },
        });
        Object.defineProperty(self, "_hideNextMonthArrow", {
            get: function () { return self.__hideNextMonthArrow; },
            set: function (bool) {
                if (self.__hideNextMonthArrow !== bool) {
                    toggleClass(self.nextMonthNav, "flatpickr-disabled", bool);
                    self.__hideNextMonthArrow = bool;
                }
            },
        });
        self.currentYearElement = self.yearElements[0];
        updateNavigationCurrentMonth();
        return self.monthNav;
    }
    function buildTime() {
        self.calendarContainer.classList.add("hasTime");
        if (self.config.noCalendar)
            self.calendarContainer.classList.add("noCalendar");
        var defaults = getDefaultHours(self.config);
        self.timeContainer = createElement("div", "flatpickr-time");
        self.timeContainer.tabIndex = -1;
        var separator = createElement("span", "flatpickr-time-separator", ":");
        var hourInput = createNumberInput("flatpickr-hour", {
            "aria-label": self.l10n.hourAriaLabel,
        });
        self.hourElement = hourInput.getElementsByTagName("input")[0];
        var minuteInput = createNumberInput("flatpickr-minute", {
            "aria-label": self.l10n.minuteAriaLabel,
        });
        self.minuteElement = minuteInput.getElementsByTagName("input")[0];
        self.hourElement.tabIndex = self.minuteElement.tabIndex = -1;
        self.hourElement.value = pad(self.latestSelectedDateObj
            ? self.latestSelectedDateObj.getHours()
            : self.config.time_24hr
                ? defaults.hours
                : military2ampm(defaults.hours));
        self.minuteElement.value = pad(self.latestSelectedDateObj
            ? self.latestSelectedDateObj.getMinutes()
            : defaults.minutes);
        self.hourElement.setAttribute("step", self.config.hourIncrement.toString());
        self.minuteElement.setAttribute("step", self.config.minuteIncrement.toString());
        self.hourElement.setAttribute("min", self.config.time_24hr ? "0" : "1");
        self.hourElement.setAttribute("max", self.config.time_24hr ? "23" : "12");
        self.hourElement.setAttribute("maxlength", "2");
        self.minuteElement.setAttribute("min", "0");
        self.minuteElement.setAttribute("max", "59");
        self.minuteElement.setAttribute("maxlength", "2");
        self.timeContainer.appendChild(hourInput);
        self.timeContainer.appendChild(separator);
        self.timeContainer.appendChild(minuteInput);
        if (self.config.time_24hr)
            self.timeContainer.classList.add("time24hr");
        if (self.config.enableSeconds) {
            self.timeContainer.classList.add("hasSeconds");
            var secondInput = createNumberInput("flatpickr-second");
            self.secondElement = secondInput.getElementsByTagName("input")[0];
            self.secondElement.value = pad(self.latestSelectedDateObj
                ? self.latestSelectedDateObj.getSeconds()
                : defaults.seconds);
            self.secondElement.setAttribute("step", self.minuteElement.getAttribute("step"));
            self.secondElement.setAttribute("min", "0");
            self.secondElement.setAttribute("max", "59");
            self.secondElement.setAttribute("maxlength", "2");
            self.timeContainer.appendChild(createElement("span", "flatpickr-time-separator", ":"));
            self.timeContainer.appendChild(secondInput);
        }
        if (!self.config.time_24hr) {
            self.amPM = createElement("span", "flatpickr-am-pm", self.l10n.amPM[int((self.latestSelectedDateObj
                ? self.hourElement.value
                : self.config.defaultHour) > 11)]);
            self.amPM.title = self.l10n.toggleTitle;
            self.amPM.tabIndex = -1;
            self.timeContainer.appendChild(self.amPM);
        }
        return self.timeContainer;
    }
    function buildWeekdays() {
        if (!self.weekdayContainer)
            self.weekdayContainer = createElement("div", "flatpickr-weekdays");
        else
            clearNode(self.weekdayContainer);
        for (var i = self.config.showMonths; i--;) {
            var container = createElement("div", "flatpickr-weekdaycontainer");
            self.weekdayContainer.appendChild(container);
        }
        updateWeekdays();
        return self.weekdayContainer;
    }
    function updateWeekdays() {
        if (!self.weekdayContainer) {
            return;
        }
        var firstDayOfWeek = self.l10n.firstDayOfWeek;
        var weekdays = __spreadArrays(self.l10n.weekdays.shorthand);
        if (firstDayOfWeek > 0 && firstDayOfWeek < weekdays.length) {
            weekdays = __spreadArrays(weekdays.splice(firstDayOfWeek, weekdays.length), weekdays.splice(0, firstDayOfWeek));
        }
        for (var i = self.config.showMonths; i--;) {
            self.weekdayContainer.children[i].innerHTML = "\n      <span class='flatpickr-weekday'>\n        " + weekdays.join("</span><span class='flatpickr-weekday'>") + "\n      </span>\n      ";
        }
    }
    function buildWeeks() {
        self.calendarContainer.classList.add("hasWeeks");
        var weekWrapper = createElement("div", "flatpickr-weekwrapper");
        weekWrapper.appendChild(createElement("span", "flatpickr-weekday", self.l10n.weekAbbreviation));
        var weekNumbers = createElement("div", "flatpickr-weeks");
        weekWrapper.appendChild(weekNumbers);
        return {
            weekWrapper: weekWrapper,
            weekNumbers: weekNumbers,
        };
    }
    function changeMonth(value, isOffset) {
        if (isOffset === void 0) { isOffset = true; }
        var delta = isOffset ? value : value - self.currentMonth;
        if ((delta < 0 && self._hidePrevMonthArrow === true) ||
            (delta > 0 && self._hideNextMonthArrow === true))
            return;
        self.currentMonth += delta;
        if (self.currentMonth < 0 || self.currentMonth > 11) {
            self.currentYear += self.currentMonth > 11 ? 1 : -1;
            self.currentMonth = (self.currentMonth + 12) % 12;
            triggerEvent("onYearChange");
            buildMonthSwitch();
        }
        buildDays();
        triggerEvent("onMonthChange");
        updateNavigationCurrentMonth();
    }
    function clear(triggerChangeEvent, toInitial) {
        if (triggerChangeEvent === void 0) { triggerChangeEvent = true; }
        if (toInitial === void 0) { toInitial = true; }
        self.input.value = "";
        if (self.altInput !== undefined)
            self.altInput.value = "";
        if (self.mobileInput !== undefined)
            self.mobileInput.value = "";
        self.selectedDates = [];
        self.latestSelectedDateObj = undefined;
        if (toInitial === true) {
            self.currentYear = self._initialDate.getFullYear();
            self.currentMonth = self._initialDate.getMonth();
        }
        if (self.config.enableTime === true) {
            var _a = getDefaultHours(self.config), hours = _a.hours, minutes = _a.minutes, seconds = _a.seconds;
            setHours(hours, minutes, seconds);
        }
        self.redraw();
        if (triggerChangeEvent)
            triggerEvent("onChange");
    }
    function close() {
        self.isOpen = false;
        if (!self.isMobile) {
            if (self.calendarContainer !== undefined) {
                self.calendarContainer.classList.remove("open");
            }
            if (self._input !== undefined) {
                self._input.classList.remove("active");
            }
        }
        triggerEvent("onClose");
    }
    function destroy() {
        if (self.config !== undefined)
            triggerEvent("onDestroy");
        for (var i = self._handlers.length; i--;) {
            self._handlers[i].remove();
        }
        self._handlers = [];
        if (self.mobileInput) {
            if (self.mobileInput.parentNode)
                self.mobileInput.parentNode.removeChild(self.mobileInput);
            self.mobileInput = undefined;
        }
        else if (self.calendarContainer && self.calendarContainer.parentNode) {
            if (self.config.static && self.calendarContainer.parentNode) {
                var wrapper = self.calendarContainer.parentNode;
                wrapper.lastChild && wrapper.removeChild(wrapper.lastChild);
                if (wrapper.parentNode) {
                    while (wrapper.firstChild)
                        wrapper.parentNode.insertBefore(wrapper.firstChild, wrapper);
                    wrapper.parentNode.removeChild(wrapper);
                }
            }
            else
                self.calendarContainer.parentNode.removeChild(self.calendarContainer);
        }
        if (self.altInput) {
            self.input.type = "text";
            if (self.altInput.parentNode)
                self.altInput.parentNode.removeChild(self.altInput);
            delete self.altInput;
        }
        if (self.input) {
            self.input.type = self.input._type;
            self.input.classList.remove("flatpickr-input");
            self.input.removeAttribute("readonly");
        }
        [
            "_showTimeInput",
            "latestSelectedDateObj",
            "_hideNextMonthArrow",
            "_hidePrevMonthArrow",
            "__hideNextMonthArrow",
            "__hidePrevMonthArrow",
            "isMobile",
            "isOpen",
            "selectedDateElem",
            "minDateHasTime",
            "maxDateHasTime",
            "days",
            "daysContainer",
            "_input",
            "_positionElement",
            "innerContainer",
            "rContainer",
            "monthNav",
            "todayDateElem",
            "calendarContainer",
            "weekdayContainer",
            "prevMonthNav",
            "nextMonthNav",
            "monthsDropdownContainer",
            "currentMonthElement",
            "currentYearElement",
            "navigationCurrentMonth",
            "selectedDateElem",
            "config",
        ].forEach(function (k) {
            try {
                delete self[k];
            }
            catch (_) { }
        });
    }
    function isCalendarElem(elem) {
        return self.calendarContainer.contains(elem);
    }
    function documentClick(e) {
        if (self.isOpen && !self.config.inline) {
            var eventTarget_1 = getEventTarget(e);
            var isCalendarElement = isCalendarElem(eventTarget_1);
            var isInput = eventTarget_1 === self.input ||
                eventTarget_1 === self.altInput ||
                self.element.contains(eventTarget_1) ||
                (e.path &&
                    e.path.indexOf &&
                    (~e.path.indexOf(self.input) ||
                        ~e.path.indexOf(self.altInput)));
            var lostFocus = !isInput &&
                !isCalendarElement &&
                !isCalendarElem(e.relatedTarget);
            var isIgnored = !self.config.ignoredFocusElements.some(function (elem) {
                return elem.contains(eventTarget_1);
            });
            if (lostFocus && isIgnored) {
                if (self.config.allowInput) {
                    self.setDate(self._input.value, false, self.config.altInput
                        ? self.config.altFormat
                        : self.config.dateFormat);
                }
                if (self.timeContainer !== undefined &&
                    self.minuteElement !== undefined &&
                    self.hourElement !== undefined &&
                    self.input.value !== "" &&
                    self.input.value !== undefined) {
                    updateTime();
                }
                self.close();
                if (self.config &&
                    self.config.mode === "range" &&
                    self.selectedDates.length === 1)
                    self.clear(false);
            }
        }
    }
    function changeYear(newYear) {
        if (!newYear ||
            (self.config.minDate && newYear < self.config.minDate.getFullYear()) ||
            (self.config.maxDate && newYear > self.config.maxDate.getFullYear()))
            return;
        var newYearNum = newYear, isNewYear = self.currentYear !== newYearNum;
        self.currentYear = newYearNum || self.currentYear;
        if (self.config.maxDate &&
            self.currentYear === self.config.maxDate.getFullYear()) {
            self.currentMonth = Math.min(self.config.maxDate.getMonth(), self.currentMonth);
        }
        else if (self.config.minDate &&
            self.currentYear === self.config.minDate.getFullYear()) {
            self.currentMonth = Math.max(self.config.minDate.getMonth(), self.currentMonth);
        }
        if (isNewYear) {
            self.redraw();
            triggerEvent("onYearChange");
            buildMonthSwitch();
        }
    }
    function isEnabled(date, timeless) {
        var _a;
        if (timeless === void 0) { timeless = true; }
        var dateToCheck = self.parseDate(date, undefined, timeless);
        if ((self.config.minDate &&
            dateToCheck &&
            compareDates(dateToCheck, self.config.minDate, timeless !== undefined ? timeless : !self.minDateHasTime) < 0) ||
            (self.config.maxDate &&
                dateToCheck &&
                compareDates(dateToCheck, self.config.maxDate, timeless !== undefined ? timeless : !self.maxDateHasTime) > 0))
            return false;
        if (!self.config.enable && self.config.disable.length === 0)
            return true;
        if (dateToCheck === undefined)
            return false;
        var bool = !!self.config.enable, array = (_a = self.config.enable) !== null && _a !== void 0 ? _a : self.config.disable;
        for (var i = 0, d = void 0; i < array.length; i++) {
            d = array[i];
            if (typeof d === "function" &&
                d(dateToCheck))
                return bool;
            else if (d instanceof Date &&
                dateToCheck !== undefined &&
                d.getTime() === dateToCheck.getTime())
                return bool;
            else if (typeof d === "string") {
                var parsed = self.parseDate(d, undefined, true);
                return parsed && parsed.getTime() === dateToCheck.getTime()
                    ? bool
                    : !bool;
            }
            else if (typeof d === "object" &&
                dateToCheck !== undefined &&
                d.from &&
                d.to &&
                dateToCheck.getTime() >= d.from.getTime() &&
                dateToCheck.getTime() <= d.to.getTime())
                return bool;
        }
        return !bool;
    }
    function isInView(elem) {
        if (self.daysContainer !== undefined)
            return (elem.className.indexOf("hidden") === -1 &&
                elem.className.indexOf("flatpickr-disabled") === -1 &&
                self.daysContainer.contains(elem));
        return false;
    }
    function onBlur(e) {
        var isInput = e.target === self._input;
        var valueChanged = self._input.value.trimEnd() !== getDateStr();
        if (isInput &&
            valueChanged &&
            !(e.relatedTarget && isCalendarElem(e.relatedTarget))) {
            self.setDate(self._input.value, true, e.target === self.altInput
                ? self.config.altFormat
                : self.config.dateFormat);
        }
    }
    function onKeyDown(e) {
        var eventTarget = getEventTarget(e);
        var isInput = self.config.wrap
            ? element.contains(eventTarget)
            : eventTarget === self._input;
        var allowInput = self.config.allowInput;
        var allowKeydown = self.isOpen && (!allowInput || !isInput);
        var allowInlineKeydown = self.config.inline && isInput && !allowInput;
        if (e.keyCode === 13 && isInput) {
            if (allowInput) {
                self.setDate(self._input.value, true, eventTarget === self.altInput
                    ? self.config.altFormat
                    : self.config.dateFormat);
                self.close();
                return eventTarget.blur();
            }
            else {
                self.open();
            }
        }
        else if (isCalendarElem(eventTarget) ||
            allowKeydown ||
            allowInlineKeydown) {
            var isTimeObj = !!self.timeContainer &&
                self.timeContainer.contains(eventTarget);
            switch (e.keyCode) {
                case 13:
                    if (isTimeObj) {
                        e.preventDefault();
                        updateTime();
                        focusAndClose();
                    }
                    else
                        selectDate(e);
                    break;
                case 27:
                    e.preventDefault();
                    focusAndClose();
                    break;
                case 8:
                case 46:
                    if (isInput && !self.config.allowInput) {
                        e.preventDefault();
                        self.clear();
                    }
                    break;
                case 37:
                case 39:
                    if (!isTimeObj && !isInput) {
                        e.preventDefault();
                        var activeElement = getClosestActiveElement();
                        if (self.daysContainer !== undefined &&
                            (allowInput === false ||
                                (activeElement && isInView(activeElement)))) {
                            var delta_1 = e.keyCode === 39 ? 1 : -1;
                            if (!e.ctrlKey)
                                focusOnDay(undefined, delta_1);
                            else {
                                e.stopPropagation();
                                changeMonth(delta_1);
                                focusOnDay(getFirstAvailableDay(1), 0);
                            }
                        }
                    }
                    else if (self.hourElement)
                        self.hourElement.focus();
                    break;
                case 38:
                case 40:
                    e.preventDefault();
                    var delta = e.keyCode === 40 ? 1 : -1;
                    if ((self.daysContainer &&
                        eventTarget.$i !== undefined) ||
                        eventTarget === self.input ||
                        eventTarget === self.altInput) {
                        if (e.ctrlKey) {
                            e.stopPropagation();
                            changeYear(self.currentYear - delta);
                            focusOnDay(getFirstAvailableDay(1), 0);
                        }
                        else if (!isTimeObj)
                            focusOnDay(undefined, delta * 7);
                    }
                    else if (eventTarget === self.currentYearElement) {
                        changeYear(self.currentYear - delta);
                    }
                    else if (self.config.enableTime) {
                        if (!isTimeObj && self.hourElement)
                            self.hourElement.focus();
                        updateTime(e);
                        self._debouncedChange();
                    }
                    break;
                case 9:
                    if (isTimeObj) {
                        var elems = [
                            self.hourElement,
                            self.minuteElement,
                            self.secondElement,
                            self.amPM,
                        ]
                            .concat(self.pluginElements)
                            .filter(function (x) { return x; });
                        var i = elems.indexOf(eventTarget);
                        if (i !== -1) {
                            var target = elems[i + (e.shiftKey ? -1 : 1)];
                            e.preventDefault();
                            (target || self._input).focus();
                        }
                    }
                    else if (!self.config.noCalendar &&
                        self.daysContainer &&
                        self.daysContainer.contains(eventTarget) &&
                        e.shiftKey) {
                        e.preventDefault();
                        self._input.focus();
                    }
                    break;
            }
        }
        if (self.amPM !== undefined && eventTarget === self.amPM) {
            switch (e.key) {
                case self.l10n.amPM[0].charAt(0):
                case self.l10n.amPM[0].charAt(0).toLowerCase():
                    self.amPM.textContent = self.l10n.amPM[0];
                    setHoursFromInputs();
                    updateValue();
                    break;
                case self.l10n.amPM[1].charAt(0):
                case self.l10n.amPM[1].charAt(0).toLowerCase():
                    self.amPM.textContent = self.l10n.amPM[1];
                    setHoursFromInputs();
                    updateValue();
                    break;
            }
        }
        if (isInput || isCalendarElem(eventTarget)) {
            triggerEvent("onKeyDown", e);
        }
    }
    function onMouseOver(elem, cellClass) {
        if (cellClass === void 0) { cellClass = "flatpickr-day"; }
        if (self.selectedDates.length !== 1 ||
            (elem &&
                (!elem.classList.contains(cellClass) ||
                    elem.classList.contains("flatpickr-disabled"))))
            return;
        var hoverDate = elem
            ? elem.dateObj.getTime()
            : self.days.firstElementChild.dateObj.getTime(), initialDate = self.parseDate(self.selectedDates[0], undefined, true).getTime(), rangeStartDate = Math.min(hoverDate, self.selectedDates[0].getTime()), rangeEndDate = Math.max(hoverDate, self.selectedDates[0].getTime());
        var containsDisabled = false;
        var minRange = 0, maxRange = 0;
        for (var t = rangeStartDate; t < rangeEndDate; t += duration.DAY) {
            if (!isEnabled(new Date(t), true)) {
                containsDisabled =
                    containsDisabled || (t > rangeStartDate && t < rangeEndDate);
                if (t < initialDate && (!minRange || t > minRange))
                    minRange = t;
                else if (t > initialDate && (!maxRange || t < maxRange))
                    maxRange = t;
            }
        }
        var hoverableCells = Array.from(self.rContainer.querySelectorAll("*:nth-child(-n+" + self.config.showMonths + ") > ." + cellClass));
        hoverableCells.forEach(function (dayElem) {
            var date = dayElem.dateObj;
            var timestamp = date.getTime();
            var outOfRange = (minRange > 0 && timestamp < minRange) ||
                (maxRange > 0 && timestamp > maxRange);
            if (outOfRange) {
                dayElem.classList.add("notAllowed");
                ["inRange", "startRange", "endRange"].forEach(function (c) {
                    dayElem.classList.remove(c);
                });
                return;
            }
            else if (containsDisabled && !outOfRange)
                return;
            ["startRange", "inRange", "endRange", "notAllowed"].forEach(function (c) {
                dayElem.classList.remove(c);
            });
            if (elem !== undefined) {
                elem.classList.add(hoverDate <= self.selectedDates[0].getTime()
                    ? "startRange"
                    : "endRange");
                if (initialDate < hoverDate && timestamp === initialDate)
                    dayElem.classList.add("startRange");
                else if (initialDate > hoverDate && timestamp === initialDate)
                    dayElem.classList.add("endRange");
                if (timestamp >= minRange &&
                    (maxRange === 0 || timestamp <= maxRange) &&
                    isBetween(timestamp, initialDate, hoverDate))
                    dayElem.classList.add("inRange");
            }
        });
    }
    function onResize() {
        if (self.isOpen && !self.config.static && !self.config.inline)
            positionCalendar();
    }
    function open(e, positionElement) {
        if (positionElement === void 0) { positionElement = self._positionElement; }
        if (self.isMobile === true) {
            if (e) {
                e.preventDefault();
                var eventTarget = getEventTarget(e);
                if (eventTarget) {
                    eventTarget.blur();
                }
            }
            if (self.mobileInput !== undefined) {
                self.mobileInput.focus();
                self.mobileInput.click();
            }
            triggerEvent("onOpen");
            return;
        }
        else if (self._input.disabled || self.config.inline) {
            return;
        }
        var wasOpen = self.isOpen;
        self.isOpen = true;
        if (!wasOpen) {
            self.calendarContainer.classList.add("open");
            self._input.classList.add("active");
            triggerEvent("onOpen");
            positionCalendar(positionElement);
        }
        if (self.config.enableTime === true && self.config.noCalendar === true) {
            if (self.config.allowInput === false &&
                (e === undefined ||
                    !self.timeContainer.contains(e.relatedTarget))) {
                setTimeout(function () { return self.hourElement.select(); }, 50);
            }
        }
    }
    function minMaxDateSetter(type) {
        return function (date) {
            var dateObj = (self.config["_" + type + "Date"] = self.parseDate(date, self.config.dateFormat));
            var inverseDateObj = self.config["_" + (type === "min" ? "max" : "min") + "Date"];
            if (dateObj !== undefined) {
                self[type === "min" ? "minDateHasTime" : "maxDateHasTime"] =
                    dateObj.getHours() > 0 ||
                        dateObj.getMinutes() > 0 ||
                        dateObj.getSeconds() > 0;
            }
            if (self.selectedDates) {
                self.selectedDates = self.selectedDates.filter(function (d) { return isEnabled(d); });
                if (!self.selectedDates.length && type === "min")
                    setHoursFromDate(dateObj);
                updateValue();
            }
            if (self.daysContainer) {
                redraw();
                if (dateObj !== undefined)
                    self.currentYearElement[type] = dateObj.getFullYear().toString();
                else
                    self.currentYearElement.removeAttribute(type);
                self.currentYearElement.disabled =
                    !!inverseDateObj &&
                        dateObj !== undefined &&
                        inverseDateObj.getFullYear() === dateObj.getFullYear();
            }
        };
    }
    function parseConfig() {
        var boolOpts = [
            "wrap",
            "weekNumbers",
            "allowInput",
            "allowInvalidPreload",
            "clickOpens",
            "time_24hr",
            "enableTime",
            "noCalendar",
            "altInput",
            "shorthandCurrentMonth",
            "inline",
            "static",
            "enableSeconds",
            "disableMobile",
        ];
        var userConfig = __assign(__assign({}, JSON.parse(JSON.stringify(element.dataset || {}))), instanceConfig);
        var formats = {};
        self.config.parseDate = userConfig.parseDate;
        self.config.formatDate = userConfig.formatDate;
        Object.defineProperty(self.config, "enable", {
            get: function () { return self.config._enable; },
            set: function (dates) {
                self.config._enable = parseDateRules(dates);
            },
        });
        Object.defineProperty(self.config, "disable", {
            get: function () { return self.config._disable; },
            set: function (dates) {
                self.config._disable = parseDateRules(dates);
            },
        });
        var timeMode = userConfig.mode === "time";
        if (!userConfig.dateFormat && (userConfig.enableTime || timeMode)) {
            var defaultDateFormat = flatpickr.defaultConfig.dateFormat || defaults.dateFormat;
            formats.dateFormat =
                userConfig.noCalendar || timeMode
                    ? "H:i" + (userConfig.enableSeconds ? ":S" : "")
                    : defaultDateFormat + " H:i" + (userConfig.enableSeconds ? ":S" : "");
        }
        if (userConfig.altInput &&
            (userConfig.enableTime || timeMode) &&
            !userConfig.altFormat) {
            var defaultAltFormat = flatpickr.defaultConfig.altFormat || defaults.altFormat;
            formats.altFormat =
                userConfig.noCalendar || timeMode
                    ? "h:i" + (userConfig.enableSeconds ? ":S K" : " K")
                    : defaultAltFormat + (" h:i" + (userConfig.enableSeconds ? ":S" : "") + " K");
        }
        Object.defineProperty(self.config, "minDate", {
            get: function () { return self.config._minDate; },
            set: minMaxDateSetter("min"),
        });
        Object.defineProperty(self.config, "maxDate", {
            get: function () { return self.config._maxDate; },
            set: minMaxDateSetter("max"),
        });
        var minMaxTimeSetter = function (type) { return function (val) {
            self.config[type === "min" ? "_minTime" : "_maxTime"] = self.parseDate(val, "H:i:S");
        }; };
        Object.defineProperty(self.config, "minTime", {
            get: function () { return self.config._minTime; },
            set: minMaxTimeSetter("min"),
        });
        Object.defineProperty(self.config, "maxTime", {
            get: function () { return self.config._maxTime; },
            set: minMaxTimeSetter("max"),
        });
        if (userConfig.mode === "time") {
            self.config.noCalendar = true;
            self.config.enableTime = true;
        }
        Object.assign(self.config, formats, userConfig);
        for (var i = 0; i < boolOpts.length; i++)
            self.config[boolOpts[i]] =
                self.config[boolOpts[i]] === true ||
                    self.config[boolOpts[i]] === "true";
        HOOKS.filter(function (hook) { return self.config[hook] !== undefined; }).forEach(function (hook) {
            self.config[hook] = arrayify(self.config[hook] || []).map(bindToInstance);
        });
        self.isMobile =
            !self.config.disableMobile &&
                !self.config.inline &&
                self.config.mode === "single" &&
                !self.config.disable.length &&
                !self.config.enable &&
                !self.config.weekNumbers &&
                /Android|webOS|iPhone|iPad|iPod|BlackBerry|IEMobile|Opera Mini/i.test(navigator.userAgent);
        for (var i = 0; i < self.config.plugins.length; i++) {
            var pluginConf = self.config.plugins[i](self) || {};
            for (var key in pluginConf) {
                if (HOOKS.indexOf(key) > -1) {
                    self.config[key] = arrayify(pluginConf[key])
                        .map(bindToInstance)
                        .concat(self.config[key]);
                }
                else if (typeof userConfig[key] === "undefined")
                    self.config[key] = pluginConf[key];
            }
        }
        if (!userConfig.altInputClass) {
            self.config.altInputClass =
                getInputElem().className + " " + self.config.altInputClass;
        }
        triggerEvent("onParseConfig");
    }
    function getInputElem() {
        return self.config.wrap
            ? element.querySelector("[data-input]")
            : element;
    }
    function setupLocale() {
        if (typeof self.config.locale !== "object" &&
            typeof flatpickr.l10ns[self.config.locale] === "undefined")
            self.config.errorHandler(new Error("flatpickr: invalid locale " + self.config.locale));
        self.l10n = __assign(__assign({}, flatpickr.l10ns.default), (typeof self.config.locale === "object"
            ? self.config.locale
            : self.config.locale !== "default"
                ? flatpickr.l10ns[self.config.locale]
                : undefined));
        tokenRegex.D = "(" + self.l10n.weekdays.shorthand.join("|") + ")";
        tokenRegex.l = "(" + self.l10n.weekdays.longhand.join("|") + ")";
        tokenRegex.M = "(" + self.l10n.months.shorthand.join("|") + ")";
        tokenRegex.F = "(" + self.l10n.months.longhand.join("|") + ")";
        tokenRegex.K = "(" + self.l10n.amPM[0] + "|" + self.l10n.amPM[1] + "|" + self.l10n.amPM[0].toLowerCase() + "|" + self.l10n.amPM[1].toLowerCase() + ")";
        var userConfig = __assign(__assign({}, instanceConfig), JSON.parse(JSON.stringify(element.dataset || {})));
        if (userConfig.time_24hr === undefined &&
            flatpickr.defaultConfig.time_24hr === undefined) {
            self.config.time_24hr = self.l10n.time_24hr;
        }
        self.formatDate = createDateFormatter(self);
        self.parseDate = createDateParser({ config: self.config, l10n: self.l10n });
    }
    function positionCalendar(customPositionElement) {
        if (typeof self.config.position === "function") {
            return void self.config.position(self, customPositionElement);
        }
        if (self.calendarContainer === undefined)
            return;
        triggerEvent("onPreCalendarPosition");
        var positionElement = customPositionElement || self._positionElement;
        var calendarHeight = Array.prototype.reduce.call(self.calendarContainer.children, (function (acc, child) { return acc + child.offsetHeight; }), 0), calendarWidth = self.calendarContainer.offsetWidth, configPos = self.config.position.split(" "), configPosVertical = configPos[0], configPosHorizontal = configPos.length > 1 ? configPos[1] : null, inputBounds = positionElement.getBoundingClientRect(), distanceFromBottom = window.innerHeight - inputBounds.bottom, showOnTop = configPosVertical === "above" ||
            (configPosVertical !== "below" &&
                distanceFromBottom < calendarHeight &&
                inputBounds.top > calendarHeight);
        var top = window.pageYOffset +
            inputBounds.top +
            (!showOnTop ? positionElement.offsetHeight + 2 : -calendarHeight - 2);
        toggleClass(self.calendarContainer, "arrowTop", !showOnTop);
        toggleClass(self.calendarContainer, "arrowBottom", showOnTop);
        if (self.config.inline)
            return;
        var left = window.pageXOffset + inputBounds.left;
        var isCenter = false;
        var isRight = false;
        if (configPosHorizontal === "center") {
            left -= (calendarWidth - inputBounds.width) / 2;
            isCenter = true;
        }
        else if (configPosHorizontal === "right") {
            left -= calendarWidth - inputBounds.width;
            isRight = true;
        }
        toggleClass(self.calendarContainer, "arrowLeft", !isCenter && !isRight);
        toggleClass(self.calendarContainer, "arrowCenter", isCenter);
        toggleClass(self.calendarContainer, "arrowRight", isRight);
        var right = window.document.body.offsetWidth -
            (window.pageXOffset + inputBounds.right);
        var rightMost = left + calendarWidth > window.document.body.offsetWidth;
        var centerMost = right + calendarWidth > window.document.body.offsetWidth;
        toggleClass(self.calendarContainer, "rightMost", rightMost);
        if (self.config.static)
            return;
        self.calendarContainer.style.top = top + "px";
        if (!rightMost) {
            self.calendarContainer.style.left = left + "px";
            self.calendarContainer.style.right = "auto";
        }
        else if (!centerMost) {
            self.calendarContainer.style.left = "auto";
            self.calendarContainer.style.right = right + "px";
        }
        else {
            var doc = getDocumentStyleSheet();
            if (doc === undefined)
                return;
            var bodyWidth = window.document.body.offsetWidth;
            var centerLeft = Math.max(0, bodyWidth / 2 - calendarWidth / 2);
            var centerBefore = ".flatpickr-calendar.centerMost:before";
            var centerAfter = ".flatpickr-calendar.centerMost:after";
            var centerIndex = doc.cssRules.length;
            var centerStyle = "{left:" + inputBounds.left + "px;right:auto;}";
            toggleClass(self.calendarContainer, "rightMost", false);
            toggleClass(self.calendarContainer, "centerMost", true);
            doc.insertRule(centerBefore + "," + centerAfter + centerStyle, centerIndex);
            self.calendarContainer.style.left = centerLeft + "px";
            self.calendarContainer.style.right = "auto";
        }
    }
    function getDocumentStyleSheet() {
        var editableSheet = null;
        for (var i = 0; i < document.styleSheets.length; i++) {
            var sheet = document.styleSheets[i];
            if (!sheet.cssRules)
                continue;
            try {
                sheet.cssRules;
            }
            catch (err) {
                continue;
            }
            editableSheet = sheet;
            break;
        }
        return editableSheet != null ? editableSheet : createStyleSheet();
    }
    function createStyleSheet() {
        var style = document.createElement("style");
        document.head.appendChild(style);
        return style.sheet;
    }
    function redraw() {
        if (self.config.noCalendar || self.isMobile)
            return;
        buildMonthSwitch();
        updateNavigationCurrentMonth();
        buildDays();
    }
    function focusAndClose() {
        self._input.focus();
        if (window.navigator.userAgent.indexOf("MSIE") !== -1 ||
            navigator.msMaxTouchPoints !== undefined) {
            setTimeout(self.close, 0);
        }
        else {
            self.close();
        }
    }
    function selectDate(e) {
        e.preventDefault();
        e.stopPropagation();
        var isSelectable = function (day) {
            return day.classList &&
                day.classList.contains("flatpickr-day") &&
                !day.classList.contains("flatpickr-disabled") &&
                !day.classList.contains("notAllowed");
        };
        var t = findParent(getEventTarget(e), isSelectable);
        if (t === undefined)
            return;
        var target = t;
        var selectedDate = (self.latestSelectedDateObj = new Date(target.dateObj.getTime()));
        var shouldChangeMonth = (selectedDate.getMonth() < self.currentMonth ||
            selectedDate.getMonth() >
                self.currentMonth + self.config.showMonths - 1) &&
            self.config.mode !== "range";
        self.selectedDateElem = target;
        if (self.config.mode === "single")
            self.selectedDates = [selectedDate];
        else if (self.config.mode === "multiple") {
            var selectedIndex = isDateSelected(selectedDate);
            if (selectedIndex)
                self.selectedDates.splice(parseInt(selectedIndex), 1);
            else
                self.selectedDates.push(selectedDate);
        }
        else if (self.config.mode === "range") {
            if (self.selectedDates.length === 2) {
                self.clear(false, false);
            }
            self.latestSelectedDateObj = selectedDate;
            self.selectedDates.push(selectedDate);
            if (compareDates(selectedDate, self.selectedDates[0], true) !== 0)
                self.selectedDates.sort(function (a, b) { return a.getTime() - b.getTime(); });
        }
        setHoursFromInputs();
        if (shouldChangeMonth) {
            var isNewYear = self.currentYear !== selectedDate.getFullYear();
            self.currentYear = selectedDate.getFullYear();
            self.currentMonth = selectedDate.getMonth();
            if (isNewYear) {
                triggerEvent("onYearChange");
                buildMonthSwitch();
            }
            triggerEvent("onMonthChange");
        }
        updateNavigationCurrentMonth();
        buildDays();
        updateValue();
        if (!shouldChangeMonth &&
            self.config.mode !== "range" &&
            self.config.showMonths === 1)
            focusOnDayElem(target);
        else if (self.selectedDateElem !== undefined &&
            self.hourElement === undefined) {
            self.selectedDateElem && self.selectedDateElem.focus();
        }
        if (self.hourElement !== undefined)
            self.hourElement !== undefined && self.hourElement.focus();
        if (self.config.closeOnSelect) {
            var single = self.config.mode === "single" && !self.config.enableTime;
            var range = self.config.mode === "range" &&
                self.selectedDates.length === 2 &&
                !self.config.enableTime;
            if (single || range) {
                focusAndClose();
            }
        }
        triggerChange();
    }
    var CALLBACKS = {
        locale: [setupLocale, updateWeekdays],
        showMonths: [buildMonths, setCalendarWidth, buildWeekdays],
        minDate: [jumpToDate],
        maxDate: [jumpToDate],
        positionElement: [updatePositionElement],
        clickOpens: [
            function () {
                if (self.config.clickOpens === true) {
                    bind(self._input, "focus", self.open);
                    bind(self._input, "click", self.open);
                }
                else {
                    self._input.removeEventListener("focus", self.open);
                    self._input.removeEventListener("click", self.open);
                }
            },
        ],
    };
    function set(option, value) {
        if (option !== null && typeof option === "object") {
            Object.assign(self.config, option);
            for (var key in option) {
                if (CALLBACKS[key] !== undefined)
                    CALLBACKS[key].forEach(function (x) { return x(); });
            }
        }
        else {
            self.config[option] = value;
            if (CALLBACKS[option] !== undefined)
                CALLBACKS[option].forEach(function (x) { return x(); });
            else if (HOOKS.indexOf(option) > -1)
                self.config[option] = arrayify(value);
        }
        self.redraw();
        updateValue(true);
    }
    function setSelectedDate(inputDate, format) {
        var dates = [];
        if (inputDate instanceof Array)
            dates = inputDate.map(function (d) { return self.parseDate(d, format); });
        else if (inputDate instanceof Date || typeof inputDate === "number")
            dates = [self.parseDate(inputDate, format)];
        else if (typeof inputDate === "string") {
            switch (self.config.mode) {
                case "single":
                case "time":
                    dates = [self.parseDate(inputDate, format)];
                    break;
                case "multiple":
                    dates = inputDate
                        .split(self.config.conjunction)
                        .map(function (date) { return self.parseDate(date, format); });
                    break;
                case "range":
                    dates = inputDate
                        .split(self.l10n.rangeSeparator)
                        .map(function (date) { return self.parseDate(date, format); });
                    break;
            }
        }
        else
            self.config.errorHandler(new Error("Invalid date supplied: " + JSON.stringify(inputDate)));
        self.selectedDates = (self.config.allowInvalidPreload
            ? dates
            : dates.filter(function (d) { return d instanceof Date && isEnabled(d, false); }));
        if (self.config.mode === "range")
            self.selectedDates.sort(function (a, b) { return a.getTime() - b.getTime(); });
    }
    function setDate(date, triggerChange, format) {
        if (triggerChange === void 0) { triggerChange = false; }
        if (format === void 0) { format = self.config.dateFormat; }
        if ((date !== 0 && !date) || (date instanceof Array && date.length === 0))
            return self.clear(triggerChange);
        setSelectedDate(date, format);
        self.latestSelectedDateObj =
            self.selectedDates[self.selectedDates.length - 1];
        self.redraw();
        jumpToDate(undefined, triggerChange);
        setHoursFromDate();
        if (self.selectedDates.length === 0) {
            self.clear(false);
        }
        updateValue(triggerChange);
        if (triggerChange)
            triggerEvent("onChange");
    }
    function parseDateRules(arr) {
        return arr
            .slice()
            .map(function (rule) {
            if (typeof rule === "string" ||
                typeof rule === "number" ||
                rule instanceof Date) {
                return self.parseDate(rule, undefined, true);
            }
            else if (rule &&
                typeof rule === "object" &&
                rule.from &&
                rule.to)
                return {
                    from: self.parseDate(rule.from, undefined),
                    to: self.parseDate(rule.to, undefined),
                };
            return rule;
        })
            .filter(function (x) { return x; });
    }
    function setupDates() {
        self.selectedDates = [];
        self.now = self.parseDate(self.config.now) || new Date();
        var preloadedDate = self.config.defaultDate ||
            ((self.input.nodeName === "INPUT" ||
                self.input.nodeName === "TEXTAREA") &&
                self.input.placeholder &&
                self.input.value === self.input.placeholder
                ? null
                : self.input.value);
        if (preloadedDate)
            setSelectedDate(preloadedDate, self.config.dateFormat);
        self._initialDate =
            self.selectedDates.length > 0
                ? self.selectedDates[0]
                : self.config.minDate &&
                    self.config.minDate.getTime() > self.now.getTime()
                    ? self.config.minDate
                    : self.config.maxDate &&
                        self.config.maxDate.getTime() < self.now.getTime()
                        ? self.config.maxDate
                        : self.now;
        self.currentYear = self._initialDate.getFullYear();
        self.currentMonth = self._initialDate.getMonth();
        if (self.selectedDates.length > 0)
            self.latestSelectedDateObj = self.selectedDates[0];
        if (self.config.minTime !== undefined)
            self.config.minTime = self.parseDate(self.config.minTime, "H:i");
        if (self.config.maxTime !== undefined)
            self.config.maxTime = self.parseDate(self.config.maxTime, "H:i");
        self.minDateHasTime =
            !!self.config.minDate &&
                (self.config.minDate.getHours() > 0 ||
                    self.config.minDate.getMinutes() > 0 ||
                    self.config.minDate.getSeconds() > 0);
        self.maxDateHasTime =
            !!self.config.maxDate &&
                (self.config.maxDate.getHours() > 0 ||
                    self.config.maxDate.getMinutes() > 0 ||
                    self.config.maxDate.getSeconds() > 0);
    }
    function setupInputs() {
        self.input = getInputElem();
        if (!self.input) {
            self.config.errorHandler(new Error("Invalid input element specified"));
            return;
        }
        self.input._type = self.input.type;
        self.input.type = "text";
        self.input.classList.add("flatpickr-input");
        self._input = self.input;
        if (self.config.altInput) {
            self.altInput = createElement(self.input.nodeName, self.config.altInputClass);
            self._input = self.altInput;
            self.altInput.placeholder = self.input.placeholder;
            self.altInput.disabled = self.input.disabled;
            self.altInput.required = self.input.required;
            self.altInput.tabIndex = self.input.tabIndex;
            self.altInput.type = "text";
            self.input.setAttribute("type", "hidden");
            if (!self.config.static && self.input.parentNode)
                self.input.parentNode.insertBefore(self.altInput, self.input.nextSibling);
        }
        if (!self.config.allowInput)
            self._input.setAttribute("readonly", "readonly");
        updatePositionElement();
    }
    function updatePositionElement() {
        self._positionElement = self.config.positionElement || self._input;
    }
    function setupMobile() {
        var inputType = self.config.enableTime
            ? self.config.noCalendar
                ? "time"
                : "datetime-local"
            : "date";
        self.mobileInput = createElement("input", self.input.className + " flatpickr-mobile");
        self.mobileInput.tabIndex = 1;
        self.mobileInput.type = inputType;
        self.mobileInput.disabled = self.input.disabled;
        self.mobileInput.required = self.input.required;
        self.mobileInput.placeholder = self.input.placeholder;
        self.mobileFormatStr =
            inputType === "datetime-local"
                ? "Y-m-d\\TH:i:S"
                : inputType === "date"
                    ? "Y-m-d"
                    : "H:i:S";
        if (self.selectedDates.length > 0) {
            self.mobileInput.defaultValue = self.mobileInput.value = self.formatDate(self.selectedDates[0], self.mobileFormatStr);
        }
        if (self.config.minDate)
            self.mobileInput.min = self.formatDate(self.config.minDate, "Y-m-d");
        if (self.config.maxDate)
            self.mobileInput.max = self.formatDate(self.config.maxDate, "Y-m-d");
        if (self.input.getAttribute("step"))
            self.mobileInput.step = String(self.input.getAttribute("step"));
        self.input.type = "hidden";
        if (self.altInput !== undefined)
            self.altInput.type = "hidden";
        try {
            if (self.input.parentNode)
                self.input.parentNode.insertBefore(self.mobileInput, self.input.nextSibling);
        }
        catch (_a) { }
        bind(self.mobileInput, "change", function (e) {
            self.setDate(getEventTarget(e).value, false, self.mobileFormatStr);
            triggerEvent("onChange");
            triggerEvent("onClose");
        });
    }
    function toggle(e) {
        if (self.isOpen === true)
            return self.close();
        self.open(e);
    }
    function triggerEvent(event, data) {
        if (self.config === undefined)
            return;
        var hooks = self.config[event];
        if (hooks !== undefined && hooks.length > 0) {
            for (var i = 0; hooks[i] && i < hooks.length; i++)
                hooks[i](self.selectedDates, self.input.value, self, data);
        }
        if (event === "onChange") {
            self.input.dispatchEvent(createEvent("change"));
            self.input.dispatchEvent(createEvent("input"));
        }
    }
    function createEvent(name) {
        var e = document.createEvent("Event");
        e.initEvent(name, true, true);
        return e;
    }
    function isDateSelected(date) {
        for (var i = 0; i < self.selectedDates.length; i++) {
            var selectedDate = self.selectedDates[i];
            if (selectedDate instanceof Date &&
                compareDates(selectedDate, date) === 0)
                return "" + i;
        }
        return false;
    }
    function isDateInRange(date) {
        if (self.config.mode !== "range" || self.selectedDates.length < 2)
            return false;
        return (compareDates(date, self.selectedDates[0]) >= 0 &&
            compareDates(date, self.selectedDates[1]) <= 0);
    }
    function updateNavigationCurrentMonth() {
        if (self.config.noCalendar || self.isMobile || !self.monthNav)
            return;
        self.yearElements.forEach(function (yearElement, i) {
            var d = new Date(self.currentYear, self.currentMonth, 1);
            d.setMonth(self.currentMonth + i);
            if (self.config.showMonths > 1 ||
                self.config.monthSelectorType === "static") {
                self.monthElements[i].textContent =
                    monthToStr(d.getMonth(), self.config.shorthandCurrentMonth, self.l10n) + " ";
            }
            else {
                self.monthsDropdownContainer.value = d.getMonth().toString();
            }
            yearElement.value = d.getFullYear().toString();
        });
        self._hidePrevMonthArrow =
            self.config.minDate !== undefined &&
                (self.currentYear === self.config.minDate.getFullYear()
                    ? self.currentMonth <= self.config.minDate.getMonth()
                    : self.currentYear < self.config.minDate.getFullYear());
        self._hideNextMonthArrow =
            self.config.maxDate !== undefined &&
                (self.currentYear === self.config.maxDate.getFullYear()
                    ? self.currentMonth + 1 > self.config.maxDate.getMonth()
                    : self.currentYear > self.config.maxDate.getFullYear());
    }
    function getDateStr(specificFormat) {
        var format = specificFormat ||
            (self.config.altInput ? self.config.altFormat : self.config.dateFormat);
        return self.selectedDates
            .map(function (dObj) { return self.formatDate(dObj, format); })
            .filter(function (d, i, arr) {
            return self.config.mode !== "range" ||
                self.config.enableTime ||
                arr.indexOf(d) === i;
        })
            .join(self.config.mode !== "range"
            ? self.config.conjunction
            : self.l10n.rangeSeparator);
    }
    function updateValue(triggerChange) {
        if (triggerChange === void 0) { triggerChange = true; }
        if (self.mobileInput !== undefined && self.mobileFormatStr) {
            self.mobileInput.value =
                self.latestSelectedDateObj !== undefined
                    ? self.formatDate(self.latestSelectedDateObj, self.mobileFormatStr)
                    : "";
        }
        self.input.value = getDateStr(self.config.dateFormat);
        if (self.altInput !== undefined) {
            self.altInput.value = getDateStr(self.config.altFormat);
        }
        if (triggerChange !== false)
            triggerEvent("onValueUpdate");
    }
    function onMonthNavClick(e) {
        var eventTarget = getEventTarget(e);
        var isPrevMonth = self.prevMonthNav.contains(eventTarget);
        var isNextMonth = self.nextMonthNav.contains(eventTarget);
        if (isPrevMonth || isNextMonth) {
            changeMonth(isPrevMonth ? -1 : 1);
        }
        else if (self.yearElements.indexOf(eventTarget) >= 0) {
            eventTarget.select();
        }
        else if (eventTarget.classList.contains("arrowUp")) {
            self.changeYear(self.currentYear + 1);
        }
        else if (eventTarget.classList.contains("arrowDown")) {
            self.changeYear(self.currentYear - 1);
        }
    }
    function timeWrapper(e) {
        e.preventDefault();
        var isKeyDown = e.type === "keydown", eventTarget = getEventTarget(e), input = eventTarget;
        if (self.amPM !== undefined && eventTarget === self.amPM) {
            self.amPM.textContent =
                self.l10n.amPM[int(self.amPM.textContent === self.l10n.amPM[0])];
        }
        var min = parseFloat(input.getAttribute("min")), max = parseFloat(input.getAttribute("max")), step = parseFloat(input.getAttribute("step")), curValue = parseInt(input.value, 10), delta = e.delta ||
            (isKeyDown ? (e.which === 38 ? 1 : -1) : 0);
        var newValue = curValue + step * delta;
        if (typeof input.value !== "undefined" && input.value.length === 2) {
            var isHourElem = input === self.hourElement, isMinuteElem = input === self.minuteElement;
            if (newValue < min) {
                newValue =
                    max +
                        newValue +
                        int(!isHourElem) +
                        (int(isHourElem) && int(!self.amPM));
                if (isMinuteElem)
                    incrementNumInput(undefined, -1, self.hourElement);
            }
            else if (newValue > max) {
                newValue =
                    input === self.hourElement ? newValue - max - int(!self.amPM) : min;
                if (isMinuteElem)
                    incrementNumInput(undefined, 1, self.hourElement);
            }
            if (self.amPM &&
                isHourElem &&
                (step === 1
                    ? newValue + curValue === 23
                    : Math.abs(newValue - curValue) > step)) {
                self.amPM.textContent =
                    self.l10n.amPM[int(self.amPM.textContent === self.l10n.amPM[0])];
            }
            input.value = pad(newValue);
        }
    }
    init();
    return self;
}
function _flatpickr(nodeList, config) {
    var nodes = Array.prototype.slice
        .call(nodeList)
        .filter(function (x) { return x instanceof HTMLElement; });
    var instances = [];
    for (var i = 0; i < nodes.length; i++) {
        var node = nodes[i];
        try {
            if (node.getAttribute("data-fp-omit") !== null)
                continue;
            if (node._flatpickr !== undefined) {
                node._flatpickr.destroy();
                node._flatpickr = undefined;
            }
            node._flatpickr = FlatpickrInstance(node, config || {});
            instances.push(node._flatpickr);
        }
        catch (e) {
            console.error(e);
        }
    }
    return instances.length === 1 ? instances[0] : instances;
}
if (typeof HTMLElement !== "undefined" &&
    typeof HTMLCollection !== "undefined" &&
    typeof NodeList !== "undefined") {
    HTMLCollection.prototype.flatpickr = NodeList.prototype.flatpickr = function (config) {
        return _flatpickr(this, config);
    };
    HTMLElement.prototype.flatpickr = function (config) {
        return _flatpickr([this], config);
    };
}
var flatpickr = function (selector, config) {
    if (typeof selector === "string") {
        return _flatpickr(window.document.querySelectorAll(selector), config);
    }
    else if (selector instanceof Node) {
        return _flatpickr([selector], config);
    }
    else {
        return _flatpickr(selector, config);
    }
};
flatpickr.defaultConfig = {};
flatpickr.l10ns = {
    en: __assign({}, english),
    default: __assign({}, english),
};
flatpickr.localize = function (l10n) {
    flatpickr.l10ns.default = __assign(__assign({}, flatpickr.l10ns.default), l10n);
};
flatpickr.setDefaults = function (config) {
    flatpickr.defaultConfig = __assign(__assign({}, flatpickr.defaultConfig), config);
};
flatpickr.parseDate = createDateParser({});
flatpickr.formatDate = createDateFormatter({});
flatpickr.compareDates = compareDates;
if (typeof jQuery !== "undefined" && typeof jQuery.fn !== "undefined") {
    jQuery.fn.flatpickr = function (config) {
        return _flatpickr(this, config);
    };
}
Date.prototype.fp_incr = function (days) {
    return new Date(this.getFullYear(), this.getMonth(), this.getDate() + (typeof days === "string" ? parseInt(days, 10) : days));
};
if (typeof window !== "undefined") {
    window.flatpickr = flatpickr;
}

/**
 * FlatpickrPicker Component
 *
 * A reusable Flatpickr date/time picker component with popup positioning,
 * animations, and proper event handling for Lithium applications.
 *
 * Features:
 * - Popup positioning like header-dropdowns
 * - Scale animation matching Lithium popup system
 * - Outside click and ESC key handling
 * - Integration with close-all-popups event
 * - Proper cleanup and state management
 */


class FlatpickrPicker {
  constructor(options = {}) {
    this.options = {
      enableTime: true,
      enableSeconds: true,
      dateFormat: 'Y-m-d\\TH:i:S',
      time_24hr: true,
      allowInput: true,
      clickOpens: false,
      disableMobile: true,
      ...options,
    };

    this.triggerButton = options.triggerButton || null;
    this.inputElement = options.inputElement || null;
    this.onChange = options.onChange || (() => {});
    this.onOpen = options.onOpen || (() => {});
    this.onClose = options.onClose || (() => {});

    // Internal state
    this.flatpickrInstance = null;
    this.isOpen = false;
    this.isTransitioning = false;
    this.popupWrapper = null;
    this.calendarElement = null;
    this.closeTimeout = null;

    // Event handlers
    this.outsideClickHandler = null;
    this.escKeyHandler = null;
    this.closeAllPopupsHandler = null;
  }

  /**
   * Initialize the picker with a trigger button and input element
   */
  init(triggerButton, inputElement) {
    this.triggerButton = triggerButton;
    this.inputElement = inputElement;

    if (!this.triggerButton || !this.inputElement) {
      log(Subsystems.UI, Status.ERROR, '[FlatpickrPicker] Missing trigger button or input element');
      return false;
    }

    // Set up click handler on trigger button
    this.triggerButton.addEventListener('click', () => this.toggle());

    return true;
  }

  /**
   * Toggle the picker open/closed
   */
  toggle() {
    if (this.isTransitioning) return;

    if (this.isOpen && this.popupWrapper) {
      this.close();
    } else {
      this.open();
    }
  }

  /**
   * Open the picker
   */
  open() {
    if (this.isOpen || this.isTransitioning || !this.triggerButton || !this.inputElement) return;

    this.isTransitioning = true;
    log(Subsystems.UI, Status.DEBUG, '[FlatpickrPicker] Opening picker');

    // Cancel any pending close timeout
    if (this.closeTimeout) {
      clearTimeout(this.closeTimeout);
      this.closeTimeout = null;
    }

    // Close other popups first
    document.dispatchEvent(new CustomEvent('close-all-popups'));

    // Create popup wrapper
    this.popupWrapper = document.createElement('div');
    this.popupWrapper.className = 'manager-ui-popup manager-header-popup flatpickr-popup-wrapper';
    this.popupWrapper.style.minWidth = '313px';
    this.popupWrapper.style.minHeight = '341px';

    this.isOpen = true;

    // Create Flatpickr instance
    this.flatpickrInstance = this._createFlatpickrInstance();

    // Position wrapper like a header-dropdown popup
    const btnRect = this.triggerButton.getBoundingClientRect();
    this.popupWrapper.style.top = `${btnRect.bottom + 1}px`;
    this.popupWrapper.style.right = `${window.innerWidth - btnRect.right}px`;
    this.popupWrapper.style.left = 'auto';
    this.popupWrapper.style.bottom = 'auto';

    document.body.appendChild(this.popupWrapper);

    // Trigger scale animation and open Flatpickr
    requestAnimationFrame(() => {
      requestAnimationFrame(() => {
        this.popupWrapper.classList.add('visible');
        // Open Flatpickr after animation starts
        setTimeout(() => {
          this.flatpickrInstance.open();
        }, 50);
      });
    });

    // Clear transitioning after animation completes
    setTimeout(() => {
      this.isTransitioning = false;
    }, 350);

    // Set up event handlers
    this._setupEventHandlers();

    // Call onOpen callback
    this.onOpen();
  }

  /**
   * Close the picker
   */
  close() {
    if (!this.isOpen || this.isTransitioning) return;

    this.isTransitioning = true;

    log(Subsystems.UI, Status.DEBUG, '[FlatpickrPicker] Closing picker');

    // Remove visible class for animation
    if (this.popupWrapper && this.popupWrapper.parentNode) {
      this.popupWrapper.classList.remove('visible');
    }

    // Remove event listeners
    this._removeEventHandlers();

    // Wait for animation to complete before cleanup
    this.closeTimeout = setTimeout(() => {
      this._cleanup();
      this.isTransitioning = false;
      this.isOpen = false;

      // Call onClose callback
      this.onClose();
    }, 350);
  }

  /**
   * Set the date programmatically without triggering onChange
   */
  setDate(date, triggerChange = false) {
    if (this.flatpickrInstance && date) {
      this.flatpickrInstance.setDate(date, triggerChange);
    }
  }

  /**
   * Get the current selected date
   */
  getDate() {
    return this.flatpickrInstance ? this.flatpickrInstance.selectedDates[0] : null;
  }

  /**
   * Check if picker is currently open
   */
  getIsOpen() {
    return this.isOpen;
  }

  /**
   * Destroy the picker and clean up all resources
   */
  destroy() {
    this.close();

    // Additional cleanup for complete destruction
    if (this.flatpickrInstance) {
      this.flatpickrInstance.destroy();
      this.flatpickrInstance = null;
    }

    // Remove popup wrapper if it still exists
    const existing = document.querySelector('.flatpickr-popup-wrapper');
    if (existing) {
      existing.remove();
    }

    // Clear references
    this.popupWrapper = null;
    this.calendarElement = null;
    this.triggerButton = null;
    this.inputElement = null;
  }

  /**
   * Create the Flatpickr instance with custom configuration
   * @private
   */
  _createFlatpickrInstance() {
    // Parse current input value for defaultDate
    let defaultDate = null;
    try {
      const parsed = DateTime.fromISO(this.inputElement.value);
      if (parsed.isValid) defaultDate = parsed.toJSDate();
    } catch (_e) {
      // Use default from options or null
    }

    const config = {
      ...this.options,
      defaultDate,
      onChange: (selectedDates) => {
        if (selectedDates.length > 0) {
          const dt = DateTime.fromJSDate(selectedDates[0]);
          this.inputElement.value = dt.toFormat("yyyy-MM-dd'T'HH:mm:ss");
          this.onChange(selectedDates[0], dt.toFormat("yyyy-MM-dd'T'HH:mm:ss"));
        }
      },
      onOpen: () => {
        // Move the calendar to our wrapper when it opens
        this._moveCalendarToWrapper();
      },
    };

    return flatpickr(this.inputElement, config);
  }

  /**
   * Move the Flatpickr calendar to our popup wrapper
   * @private
   */
  _moveCalendarToWrapper() {
    const moveCalendar = () => {
      // Find all calendars and take the last one (most recently created)
      const calendars = document.querySelectorAll('.flatpickr-calendar');
      const calendar = calendars[calendars.length - 1];

      if (calendar && !this.popupWrapper.contains(calendar)) {
        calendar.style.position = 'absolute';
        calendar.style.top = '0';
        calendar.style.left = '0';
        calendar.style.display = 'block';
        calendar.style.visibility = 'visible';
        calendar.style.opacity = '1';
        // Add CSS override to prevent Flatpickr from hiding it
        calendar.style.setProperty('display', 'block', 'important');
        calendar.style.setProperty('visibility', 'visible', 'important');
        this.popupWrapper.appendChild(calendar);
        // Store reference to the calendar for cleanup
        this.calendarElement = calendar;
      } else if (!calendar) {
        // If calendar not found yet, try again
        setTimeout(moveCalendar, 10);
      }
    };
    moveCalendar();
  }

  /**
   * Set up event handlers for outside clicks, ESC key, etc.
   * @private
   */
  _setupEventHandlers() {
    // Remove any existing handlers
    this._removeEventHandlers();

    // Outside click handler
    this.outsideClickHandler = (e) => {
      if (!this.popupWrapper.contains(e.target) && !this.triggerButton.contains(e.target)) {
        if (this.isTransitioning) return;
        this.close();
      }
    };

    // ESC key handler
    this.escKeyHandler = (e) => {
      if (e.key === 'Escape') {
        if (this.isTransitioning) return;
        this.close();
      }
    };

    // close-all-popups handler
    this.closeAllPopupsHandler = () => this.close();

    // Use setTimeout to attach listeners after current click event finishes bubbling
    setTimeout(() => {
      document.addEventListener('click', this.outsideClickHandler);
      document.addEventListener('keydown', this.escKeyHandler);
      document.addEventListener('close-all-popups', this.closeAllPopupsHandler);
    }, 0);
  }

  /**
   * Remove event handlers
   * @private
   */
  _removeEventHandlers() {
    if (this.outsideClickHandler) {
      document.removeEventListener('click', this.outsideClickHandler);
      this.outsideClickHandler = null;
    }
    if (this.escKeyHandler) {
      document.removeEventListener('keydown', this.escKeyHandler);
      this.escKeyHandler = null;
    }
    if (this.closeAllPopupsHandler) {
      document.removeEventListener('close-all-popups', this.closeAllPopupsHandler);
      this.closeAllPopupsHandler = null;
    }
  }

  /**
   * Clean up resources after closing
   * @private
   */
  _cleanup() {
    if (this.flatpickrInstance) {
      this.flatpickrInstance.destroy();
      this.flatpickrInstance = null;
    }

    // Remove the calendar if we have a reference to it
    if (this.calendarElement && this.calendarElement.parentNode) {
      this.calendarElement.remove();
      this.calendarElement = null;
    }

    // Remove popup wrapper
    if (this.popupWrapper && this.popupWrapper.parentNode) {
      this.popupWrapper.remove();
    }

    this.popupWrapper = null;
  }
}

/**
 * LuxonTokenTable Component
 *
 * A reusable component for displaying Luxon date/time format tokens in a table.
 * Provides a comprehensive reference of all available format tokens with examples.
 *
 * Features:
 * - Complete Luxon format token reference
 * - Live examples using sample and current datetime
 * - Grouped by category (Timezones, Years, Months, etc.)
 * - Integrated with LithiumTable for navigation and search
 * - Auto-updates current time examples
 */


/**
 * Comprehensive list of Luxon format tokens organized by category
 */
const LUXON_FORMAT_TOKENS = [
  // Timezones
  { token: 'Z', desc: 'Narrow offset (+5)', group: 'Timezones' },
  { token: 'ZZ', desc: 'Short offset (+05:00)', group: 'Timezones' },
  { token: 'ZZZ', desc: 'Techie offset (+0500)', group: 'Timezones' },
  { token: 'ZZZZ', desc: 'Abbreviated named offset', group: 'Timezones' },
  { token: 'ZZZZZ', desc: 'Full named offset', group: 'Timezones' },
  { token: 'z', desc: 'IANA zone name', group: 'Timezones' },
  // Eras
  { token: 'G', desc: 'Abbreviated era', group: 'Eras' },
  { token: 'GG', desc: 'Full era', group: 'Eras' },
  { token: 'GGGGG', desc: 'One-letter era', group: 'Eras' },
  // Years
  { token: 'y', desc: 'Year, unpadded', group: 'Years' },
  { token: 'yy', desc: 'Two-digit year', group: 'Years' },
  { token: 'yyyy', desc: 'Four-digit year', group: 'Years' },
  { token: 'kk', desc: 'ISO week year, unpadded', group: 'Years' },
  { token: 'kkkk', desc: 'ISO week year, padded', group: 'Years' },
  { token: 'ii', desc: 'Local week year, unpadded', group: 'Years' },
  { token: 'iiii', desc: 'Local week year, padded', group: 'Years' },
  // Months
  { token: 'M', desc: 'Month, unpadded', group: 'Months' },
  { token: 'MM', desc: 'Month, padded to 2', group: 'Months' },
  { token: 'MMM', desc: 'Abbreviated month', group: 'Months' },
  { token: 'MMMM', desc: 'Full month name', group: 'Months' },
  { token: 'MMMMM', desc: 'Month as single letter', group: 'Months' },
  // Weeks
  { token: 'W', desc: 'ISO week number', group: 'Weeks' },
  { token: 'WW', desc: 'Padded ISO week', group: 'Weeks' },
  { token: 'n', desc: 'Local week number', group: 'Weeks' },
  { token: 'nn', desc: 'Padded local week', group: 'Weeks' },
  // Days
  { token: 'd', desc: 'Day of month, unpadded', group: 'Days' },
  { token: 'dd', desc: 'Day of month, padded', group: 'Days' },
  { token: 'E', desc: 'Weekday as number (1-7)', group: 'Days' },
  { token: 'EEE', desc: 'Abbreviated weekday', group: 'Days' },
  { token: 'EEEE', desc: 'Full weekday name', group: 'Days' },
  { token: 'EEEEE', desc: 'Weekday as single letter', group: 'Days' },
  // Ordinal / Quarter
  { token: 'o', desc: 'Ordinal day of year', group: 'Ordinal / Quarter' },
  { token: 'ooo', desc: 'Padded ordinal day', group: 'Ordinal / Quarter' },
  { token: 'q', desc: 'Quarter, unpadded', group: 'Ordinal / Quarter' },
  { token: 'qq', desc: 'Quarter, padded', group: 'Ordinal / Quarter' },
  // Hours
  { token: 'H', desc: '24-hour, unpadded', group: 'Hours' },
  { token: 'HH', desc: '24-hour, padded', group: 'Hours' },
  { token: 'h', desc: '12-hour, unpadded', group: 'Hours' },
  { token: 'hh', desc: '12-hour, padded', group: 'Hours' },
  { token: 'a', desc: 'Meridiem (AM/PM)', group: 'Hours' },
  // Minutes
  { token: 'm', desc: 'Minutes, unpadded', group: 'Minutes' },
  { token: 'mm', desc: 'Minutes, padded', group: 'Minutes' },
  // Seconds
  { token: 's', desc: 'Seconds, unpadded', group: 'Seconds' },
  { token: 'ss', desc: 'Seconds, padded', group: 'Seconds' },
  // Subseconds
  { token: 'S', desc: 'Millisecond, unpadded', group: 'Subseconds' },
  { token: 'SSS', desc: 'Millisecond, padded to 3', group: 'Subseconds' },
  { token: 'u', desc: 'Fractional seconds (3 digits)', group: 'Subseconds' },
  { token: 'uu', desc: 'Fractional seconds (2 digits)', group: 'Subseconds' },
  { token: 'uuu', desc: 'Fractional seconds (1 digit)', group: 'Subseconds' },
  // Timestamps
  { token: 'X', desc: 'Unix timestamp (seconds)', group: 'Timestamps' },
  { token: 'x', desc: 'Unix timestamp (ms)', group: 'Timestamps' },
  // Locale Date
  { token: 'D', desc: 'Localized numeric date', group: 'Locale Date' },
  { token: 'DD', desc: 'Localized date with abbreviated month', group: 'Locale Date' },
  { token: 'DDD', desc: 'Localized date with full month', group: 'Locale Date' },
  { token: 'DDDD', desc: 'Localized date with weekday', group: 'Locale Date' },
  // Locale Time (12-hour)
  { token: 't', desc: 'Localized time', group: 'Locale Time (12-hour)' },
  { token: 'tt', desc: 'Localized time with seconds', group: 'Locale Time (12-hour)' },
  { token: 'ttt', desc: 'Localized time with abbreviated offset', group: 'Locale Time (12-hour)' },
  { token: 'tttt', desc: 'Localized time with full offset', group: 'Locale Time (12-hour)' },
  // Locale Time (24-hour)
  { token: 'T', desc: 'Localized 24-hour time', group: 'Locale Time (24-hour)' },
  { token: 'TT', desc: 'Localized 24-hour time with seconds', group: 'Locale Time (24-hour)' },
  { token: 'TTT', desc: 'Localized 24-hour time with abbreviated offset', group: 'Locale Time (24-hour)' },
  { token: 'TTTT', desc: 'Localized 24-hour time with full offset', group: 'Locale Time (24-hour)' },
  // Locale DateTime
  { token: 'f', desc: 'Short localized date & time', group: 'Locale DateTime' },
  { token: 'ff', desc: 'Less short localized date & time', group: 'Locale DateTime' },
  { token: 'fff', desc: 'Verbose localized date & time', group: 'Locale DateTime' },
  { token: 'ffff', desc: 'Extra verbose localized date & time', group: 'Locale DateTime' },
  // Locale DateTime with Seconds
  { token: 'F', desc: 'Short localized date & time with seconds', group: 'Locale DateTime with Seconds' },
  { token: 'FF', desc: 'Less short localized date & time with seconds', group: 'Locale DateTime with Seconds' },
  { token: 'FFF', desc: 'Verbose localized date & time with seconds', group: 'Locale DateTime with Seconds' },
  { token: 'FFFF', desc: 'Extra verbose localized date & time with seconds', group: 'Locale DateTime with Seconds' },
  // Escaping
  { token: "'text'", desc: 'Literal text in single quotes', group: 'Escaping' },
];

class LuxonTokenTable {
  constructor(options = {}) {
    this.container = options.container || null;
    this.navigatorContainer = options.navigatorContainer || null;
    this.sampleDateTime = options.sampleDateTime || DateTime.fromISO('2020-01-01T14:03:02');
    this.currentDateTime = options.currentDateTime || DateTime.now();
    this.tablePath = options.tablePath || 'luxon-tokens';
    this.lookupKeyIdx = options.lookupKeyIdx || 16;
    this.cssPrefix = options.cssPrefix || 'luxon-tokens';
    this.storageKey = options.storageKey || 'luxon_tokens';
    this.onTokenSelected = options.onTokenSelected || (() => {});

    this.table = null;
    this.updateInterval = null;
    this.isDestroyed = false;
  }

  /**
   * Initialize the token table
   */
  async init() {
    if (!this.container) {
      log(Subsystems.UI, Status.ERROR, '[LuxonTokenTable] No container provided');
      return false;
    }

    this.table = new LithiumTable({
      container: this.container,
      navigatorContainer: this.navigatorContainer,
      tablePath: this.tablePath,
      lookupKeyIdx: this.lookupKeyIdx,
      cssPrefix: this.cssPrefix,
      storageKey: this.storageKey,
      app: null,
      readonly: true,
      primaryKeyField: ['token'],
      localSearch: true,
      localSearchFields: ['token', 'description', 'group'],
      useOverlayScrollbars: true,
      onRefresh: () => this.refresh(),
      onRowSelected: () => {
        closeAllPopups();
        this.onTokenSelected();
      },
    });

    await this.table.init();
    this.loadData();
    // Removed automatic updates - table only updates when explicitly requested

    log(Subsystems.UI, Status.DEBUG, '[LuxonTokenTable] Initialized');
    return true;
  }

  /**
   * Load token data into the table
   */
  loadData() {
    if (!this.table) return;

    const data = this.buildTokenData(this.sampleDateTime, this.currentDateTime);
    this.table.loadStaticData(data, { autoSelect: true });
  }

  /**
   * Refresh the table with updated datetime values
   */
  refresh() {
    if (this.isDestroyed || !this.table) return;

    this.currentDateTime = DateTime.now();
    this.loadData();
  }

  /**
   * Update the sample datetime and refresh the table
   */
  updateSample(sampleDateTime) {
    this.sampleDateTime = sampleDateTime;
    this.refresh();
  }

  /**
   * Start automatic updates of current time examples
   */
  startUpdates() {
    if (this.updateInterval) return;

    this.updateInterval = setInterval(() => {
      this.refresh();
    }, 1000);
  }

  /**
   * Stop automatic updates
   */
  stopUpdates() {
    if (this.updateInterval) {
      clearInterval(this.updateInterval);
      this.updateInterval = null;
    }
  }

  /**
   * Build the token data array with examples
   */
  buildTokenData(sample, now) {
    return LUXON_FORMAT_TOKENS.map(t => {
      let example, current;
      try {
        example = sample.toFormat(t.token);
      } catch (_e) {
        example = '-';
      }
      try {
        current = now.toFormat(t.token);
      } catch (_e) {
        current = '-';
      }
      return {
        id: t.token,  // Primary key
        token: t.token,
        description: t.desc,
        group: t.group,
        example,
        current,
      };
    });
  }

  /**
   * Get the current table instance
   */
  getTable() {
    return this.table;
  }

  /**
   * Destroy the component and clean up resources
   */
  destroy() {
    this.isDestroyed = true;
    this.stopUpdates();

    if (this.table) {
      this.table.destroy();
      this.table = null;
    }

    log(Subsystems.UI, Status.DEBUG, '[LuxonTokenTable] Destroyed');
  }
}

/**
 * Profile Manager - Date Formats Page Handler
 *
 * Handles the Date Formats settings page (index: -9)
 * Manages date/time format selection with live preview using Luxon.
 *
 * Uses the ProfileSettingsService for all persistence.
 * Section key: "-9"
 *
 * JSON structure written to the service:
 * {
 *   "_name": "Date Formats",
 *   "sample": "2020-01-01T14:03:02",
 *   "timezone": "America/New_York",
 *   "dates": {
 *     "short": "yyyy-MM-dd",
 *     "long": "MMMM d, y"
 *   },
 *   "times": {
 *     "short": "h:mm a",
 *     "medium": "h:mm:ss a",
 *     "long": "h:mm:ss a zzz"
 *   },
 *   "datetimes": {
 *     "short": "yyyy-MM-dd h:mm a",
 *     "long": "MMMM d, y 'at' h:mm:ss a"
 *   }
 * }
 */


const DEFAULT_SAMPLE = '2020-01-01T14:03:02';

// Default built-in formats
const BUILTIN_FORMATS = {
  dates: {
    'Short Date': 'yyyy-MM-dd',
    'Medium Date': 'yyyy-MMM-dd',
    'Long Date': 'MMMM d, y',
    'Week Number': 'yyyy-\'W\'nn',
  },
  times: {
    'Short Time': 'HH:mm',
    'Medium Time': 'H:mm:ss',
    'Long Time': 'HH:mm:ss.SSS',
  },
  datetimes: {
    'Short DateTime': 'yyyy-MM-dd HH:mm:ss',
    'Medium DateTime': 'yyyy-MMM-dd (EEE) HH:mm:ss',
    'Long DateTime': 'MMMM d, y \'at\' HH:mm:ss',
  },
};



/**
 * Exhaustive list of IANA timezones, organized by region.
 * Generated from the IANA timezone database.
 */
const IANA_TIMEZONES = [
  // Africa
  'Africa/Abidjan', 'Africa/Accra', 'Africa/Addis_Ababa', 'Africa/Algiers', 'Africa/Asmara',
  'Africa/Bamako', 'Africa/Bangui', 'Africa/Banjul', 'Africa/Bissau', 'Africa/Blantyre',
  'Africa/Brazzaville', 'Africa/Bujumbura', 'Africa/Cairo', 'Africa/Casablanca', 'Africa/Ceuta',
  'Africa/Conakry', 'Africa/Dakar', 'Africa/Dar_es_Salaam', 'Africa/Djibouti', 'Africa/Douala',
  'Africa/El_Aaiun', 'Africa/Freetown', 'Africa/Gaborone', 'Africa/Harare', 'Africa/Johannesburg',
  'Africa/Juba', 'Africa/Kampala', 'Africa/Khartoum', 'Africa/Kigali', 'Africa/Kinshasa',
  'Africa/Lagos', 'Africa/Libreville', 'Africa/Lome', 'Africa/Luanda', 'Africa/Lubumbashi',
  'Africa/Lusaka', 'Africa/Malabo', 'Africa/Maputo', 'Africa/Maseru', 'Africa/Mbabane',
  'Africa/Mogadishu', 'Africa/Monrovia', 'Africa/Nairobi', 'Africa/Ndjamena', 'Africa/Niamey',
  'Africa/Nouakchott', 'Africa/Ouagadougou', 'Africa/Porto-Novo', 'Africa/Sao_Tome', 'Africa/Tripoli',
  'Africa/Tunis', 'Africa/Windhoek',
  // America
  'America/Adak', 'America/Anchorage', 'America/Anguilla', 'America/Antigua', 'America/Araguaina',
  'America/Argentina/Buenos_Aires', 'America/Argentina/Catamarca', 'America/Argentina/Cordoba',
  'America/Argentina/Jujuy', 'America/Argentina/La_Rioja', 'America/Argentina/Mendoza',
  'America/Argentina/Rio_Gallegos', 'America/Argentina/Salta', 'America/Argentina/San_Juan',
  'America/Argentina/San_Luis', 'America/Argentina/Tucuman', 'America/Argentina/Ushuaia',
  'America/Aruba', 'America/Asuncion', 'America/Atikokan', 'America/Bahia', 'America/Bahia_Banderas',
  'America/Barbados', 'America/Belem', 'America/Belize', 'America/Blanc-Sablon', 'America/Boa_Vista',
  'America/Bogota', 'America/Boise', 'America/Cambridge_Bay', 'America/Campo_Grande', 'America/Cancun',
  'America/Caracas', 'America/Cayenne', 'America/Cayman', 'America/Chicago', 'America/Chihuahua',
  'America/Costa_Rica', 'America/Creston', 'America/Cuiaba', 'America/Curacao', 'America/Danmarkshavn',
  'America/Dawson', 'America/Dawson_Creek', 'America/Denver', 'America/Detroit', 'America/Dominica',
  'America/Edmonton', 'America/Eirunepe', 'America/El_Salvador', 'America/Fort_Nelson',
  'America/Fortaleza', 'America/Glace_Bay', 'America/Goose_Bay', 'America/Grand_Turk',
  'America/Grenada', 'America/Guadeloupe', 'America/Guatemala', 'America/Guayaquil', 'America/Guyana',
  'America/Halifax', 'America/Havana', 'America/Hermosillo', 'America/Indiana/Indianapolis',
  'America/Indiana/Knox', 'America/Indiana/Marengo', 'America/Indiana/Petersburg',
  'America/Indiana/Tell_City', 'America/Indiana/Vevay', 'America/Indiana/Vincennes',
  'America/Indiana/Winamac', 'America/Inuvik', 'America/Iqaluit', 'America/Jamaica',
  'America/Juneau', 'America/Kentucky/Louisville', 'America/Kentucky/Monticello',
  'America/Kralendijk', 'America/La_Paz', 'America/Lima', 'America/Los_Angeles',
  'America/Lower_Princes', 'America/Maceio', 'America/Managua', 'America/Manaus', 'America/Marigot',
  'America/Martinique', 'America/Matamoros', 'America/Mazatlan', 'America/Menominee',
  'America/Merida', 'America/Metlakatla', 'America/Mexico_City', 'America/Miquelon',
  'America/Moncton', 'America/Monterrey', 'America/Montevideo', 'America/Montserrat',
  'America/Nassau', 'America/New_York', 'America/Nipigon', 'America/Nome', 'America/Noronha',
  'America/North_Dakota/Beulah', 'America/North_Dakota/Center', 'America/North_Dakota/New_Salem',
  'America/Nuuk', 'America/Ojinaga', 'America/Panama', 'America/Pangnirtung', 'America/Paramaribo',
  'America/Phoenix', 'America/Port-au-Prince', 'America/Port_of_Spain', 'America/Porto_Velho',
  'America/Puerto_Rico', 'America/Punta_Arenas', 'America/Rainy_River', 'America/Rankin_Inlet',
  'America/Recife', 'America/Regina', 'America/Resolute', 'America/Rio_Branco', 'America/Santarem',
  'America/Santiago', 'America/Santo_Domingo', 'America/Sao_Paulo', 'America/Scoresbysund',
  'America/Sitka', 'America/St_Barthelemy', 'America/St_Johns', 'America/St_Kitts',
  'America/St_Lucia', 'America/St_Thomas', 'America/St_Vincent', 'America/Swift_Current',
  'America/Tegucigalpa', 'America/Thule', 'America/Thunder_Bay', 'America/Tijuana',
  'America/Toronto', 'America/Tortola', 'America/Vancouver', 'America/Whitehorse', 'America/Winnipeg',
  'America/Yakutat', 'America/Yellowknife',
  // Antarctica
  'Antarctica/Casey', 'Antarctica/Davis', 'Antarctica/DumontDUrville', 'Antarctica/Macquarie',
  'Antarctica/Mawson', 'Antarctica/McMurdo', 'Antarctica/Palmer', 'Antarctica/Rothera',
  'Antarctica/Syowa', 'Antarctica/Troll', 'Antarctica/Vostok',
  // Arctic
  'Arctic/Longyearbyen',
  // Asia
  'Asia/Aden', 'Asia/Almaty', 'Asia/Amman', 'Asia/Anadyr', 'Asia/Aqtau', 'Asia/Aqtobe',
  'Asia/Ashgabat', 'Asia/Atyrau', 'Asia/Baghdad', 'Asia/Bahrain', 'Asia/Baku', 'Asia/Bangkok',
  'Asia/Barnaul', 'Asia/Beirut', 'Asia/Bishkek', 'Asia/Brunei', 'Asia/Chita', 'Asia/Choibalsan',
  'Asia/Colombo', 'Asia/Damascus', 'Asia/Dhaka', 'Asia/Dili', 'Asia/Dubai', 'Asia/Dushanbe',
  'Asia/Famagusta', 'Asia/Gaza', 'Asia/Hebron', 'Asia/Ho_Chi_Minh', 'Asia/Hong_Kong', 'Asia/Hovd',
  'Asia/Irkutsk', 'Asia/Jakarta', 'Asia/Jayapura', 'Asia/Jerusalem', 'Asia/Kabul',
  'Asia/Kamchatka', 'Asia/Karachi', 'Asia/Kathmandu', 'Asia/Khandyga', 'Asia/Kolkata',
  'Asia/Krasnoyarsk', 'Asia/Kuala_Lumpur', 'Asia/Kuching', 'Asia/Kuwait', 'Asia/Macau',
  'Asia/Magadan', 'Asia/Makassar', 'Asia/Manila', 'Asia/Muscat', 'Asia/Nicosia', 'Asia/Novokuznetsk',
  'Asia/Novosibirsk', 'Asia/Omsk', 'Asia/Oral', 'Asia/Phnom_Penh', 'Asia/Pontianak',
  'Asia/Pyongyang', 'Asia/Qatar', 'Asia/Qostanay', 'Asia/Qyzylorda', 'Asia/Riyadh',
  'Asia/Sakhalin', 'Asia/Samarkand', 'Asia/Seoul', 'Asia/Shanghai', 'Asia/Singapore',
  'Asia/Srednekolymsk', 'Asia/Taipei', 'Asia/Tashkent', 'Asia/Tbilisi', 'Asia/Tehran',
  'Asia/Thimphu', 'Asia/Tokyo', 'Asia/Tomsk', 'Asia/Ulaanbaatar', 'Asia/Urumqi', 'Asia/Ust-Nera',
  'Asia/Vientiane', 'Asia/Vladivostok', 'Asia/Yakutsk', 'Asia/Yangon', 'Asia/Yekaterinburg',
  'Asia/Yerevan',
  // Atlantic
  'Atlantic/Azores', 'Atlantic/Bermuda', 'Atlantic/Canary', 'Atlantic/Cape_Verde',
  'Atlantic/Faroe', 'Atlantic/Madeira', 'Atlantic/Reykjavik', 'Atlantic/South_Georgia',
  'Atlantic/St_Helena', 'Atlantic/Stanley',
  // Australia
  'Australia/Adelaide', 'Australia/Brisbane', 'Australia/Broken_Hill', 'Australia/Currie',
  'Australia/Darwin', 'Australia/Eucla', 'Australia/Hobart', 'Australia/Lindeman',
  'Australia/Lord_Howe', 'Australia/Melbourne', 'Australia/Perth', 'Australia/Sydney',
  // Europe
  'Europe/Amsterdam', 'Europe/Andorra', 'Europe/Astrakhan', 'Europe/Athens', 'Europe/Belgrade',
  'Europe/Berlin', 'Europe/Bratislava', 'Europe/Brussels', 'Europe/Bucharest', 'Europe/Budapest',
  'Europe/Busingen', 'Europe/Chisinau', 'Europe/Copenhagen', 'Europe/Dublin', 'Europe/Gibraltar',
  'Europe/Guernsey', 'Europe/Helsinki', 'Europe/Isle_of_Man', 'Europe/Istanbul', 'Europe/Jersey',
  'Europe/Kaliningrad', 'Europe/Kiev', 'Europe/Kirov', 'Europe/Lisbon', 'Europe/Ljubljana',
  'Europe/London', 'Europe/Luxembourg', 'Europe/Madrid', 'Europe/Malta', 'Europe/Mariehamn',
  'Europe/Minsk', 'Europe/Monaco', 'Europe/Moscow', 'Europe/Oslo', 'Europe/Paris',
  'Europe/Podgorica', 'Europe/Prague', 'Europe/Riga', 'Europe/Rome', 'Europe/Samara',
  'Europe/San_Marino', 'Europe/Sarajevo', 'Europe/Saratov', 'Europe/Simferopol', 'Europe/Skopje',
  'Europe/Sofia', 'Europe/Stockholm', 'Europe/Tallinn', 'Europe/Tirane', 'Europe/Ulyanovsk',
  'Europe/Uzhgorod', 'Europe/Vaduz', 'Europe/Vatican', 'Europe/Vienna', 'Europe/Vilnius',
  'Europe/Volgograd', 'Europe/Warsaw', 'Europe/Zagreb', 'Europe/Zaporozhye', 'Europe/Zurich',
  // Indian
  'Indian/Antananarivo', 'Indian/Chagos', 'Indian/Christmas', 'Indian/Cocos', 'Indian/Comoro',
  'Indian/Kerguelen', 'Indian/Mahe', 'Indian/Maldives', 'Indian/Mauritius', 'Indian/Mayotte',
  'Indian/Reunion',
  // Pacific
  'Pacific/Apia', 'Pacific/Auckland', 'Pacific/Bougainville', 'Pacific/Chatham', 'Pacific/Chuuk',
  'Pacific/Easter', 'Pacific/Efate', 'Pacific/Fakaofo', 'Pacific/Fiji', 'Pacific/Funafuti',
  'Pacific/Galapagos', 'Pacific/Gambier', 'Pacific/Guadalcanal', 'Pacific/Guam',
  'Pacific/Honolulu', 'Pacific/Kanton', 'Pacific/Kiritimati', 'Pacific/Kosrae', 'Pacific/Kwajalein',
  'Pacific/Majuro', 'Pacific/Marquesas', 'Pacific/Midway', 'Pacific/Nauru', 'Pacific/Niue',
  'Pacific/Norfolk', 'Pacific/Noumea', 'Pacific/Pago_Pago', 'Pacific/Palau', 'Pacific/Pitcairn',
  'Pacific/Pohnpei', 'Pacific/Port_Moresby', 'Pacific/Rarotonga', 'Pacific/Saipan',
  'Pacific/Tahiti', 'Pacific/Tarawa', 'Pacific/Tongatapu', 'Pacific/Wake', 'Pacific/Wallis',
  // UTC and common abbreviations (mapped to IANA identifiers)
  'UTC', 'GMT',
  // US timezones (using IANA identifiers)
  'America/Los_Angeles', 'America/Denver', 'America/Chicago', 'America/New_York',
  'America/Anchorage', 'Pacific/Honolulu',
  // European timezones (using IANA identifiers)
  'Europe/Paris', 'Europe/London', 'Europe/Berlin', 'Europe/Rome', 'Europe/Madrid',
  // Asian timezones (using IANA identifiers)
  'Asia/Kolkata', 'Asia/Tokyo', 'Asia/Shanghai', 'Asia/Seoul',
  'Asia/Bangkok', 'Asia/Manila', 'Asia/Singapore',
  // Australian timezones (using IANA identifiers)
  'Australia/Sydney', 'Australia/Adelaide', 'Australia/Perth',
  // Other major cities
  'Pacific/Auckland', 'Africa/Johannesburg', 'Europe/Moscow', 'Asia/Yekaterinburg',
  // Etc zones for completeness
  'Etc/UTC', 'Etc/GMT', 'Etc/GMT+0', 'Etc/GMT+1', 'Etc/GMT+2', 'Etc/GMT+3',
  'Etc/GMT+4', 'Etc/GMT+5', 'Etc/GMT+6', 'Etc/GMT+7', 'Etc/GMT+8', 'Etc/GMT+9', 'Etc/GMT+10',
  'Etc/GMT+11', 'Etc/GMT+12', 'Etc/GMT-0', 'Etc/GMT-1', 'Etc/GMT-2', 'Etc/GMT-3', 'Etc/GMT-4',
  'Etc/GMT-5', 'Etc/GMT-6', 'Etc/GMT-7', 'Etc/GMT-8', 'Etc/GMT-9', 'Etc/GMT-10', 'Etc/GMT-11',
  'Etc/GMT-12', 'Etc/GMT-13', 'Etc/GMT-14',
];





class DateFormatsPage extends BaseSettingsPage {
  constructor(options = {}) {
    super({
      ...options,
      index: -9,
    });

    this._currentUpdateInterval = null;
    this._flatpickrPicker = null;
    this._tokenTable = null;
    this._timezonePicker = null;
    this._contentScrollbarInstance = null;
  }

  /**
   * Called when the page is initialized
   */
  async onInit() {
    this._ensureSectionNamed();
    this._setupEventListeners();
    this._initTimezonePicker();
    this._initFlatpickrPicker();
    await this._initTokenTable();
    await this.loadData();
    this._startCurrentTimeUpdates();
    this._initContentScrollbars();

    log(Subsystems.MANAGER, Status.DEBUG, '[DateFormatsPage] Initialized');
  }

  /**
   * Ensure the section has a _name so the JSON is self-describing
   */
  _ensureSectionNamed() {
    const section = this.getSectionData();
    if (!section._name) {
      this.setSetting('_name', 'Date Formats');
    }
  }

  /**
   * Get the effective timezone — either the saved preference or browser local
   */
  _getTimezone() {
    const tz = this.getSetting('timezone', 'local');
    if (tz === 'local') return 'local';
    if (typeof tz === 'object' && tz.name) {
      if (tz.name === 'local') return 'local';
      return tz.name;
    }
    if (typeof tz === 'string') return tz; // backward compatibility
    return 'local';
  }

  /**
   * Get a DateTime in the effective timezone
   */
  _getDateTimeNow() {
    const tz = this._getTimezone();
    if (tz === 'local') {
      return DateTime.now();
    }
    return DateTime.now().setZone(tz);
  }

  /**
   * Setup event listeners for live save
   */
  _setupEventListeners() {
    const container = this.container;
    if (!container) return;

    // Built-in format inputs: live save on input
    container.querySelectorAll('.df-format-input[data-setting]').forEach(input => {
      input.addEventListener('input', () => {
        this._updatePreview(input);
        const settingPath = input.dataset.setting;
        if (settingPath) {
          this.setSetting(settingPath, input.value);
        }
      });
    });

    // Sample input: live save
    const sampleInput = container.querySelector('#df-sample-input');
    if (sampleInput) {
      sampleInput.addEventListener('input', () => {
        this.setSetting('sample', sampleInput.value);
        this._renderAllPreviews();
      });
    }



    // Add custom format buttons
    container.querySelectorAll('.df-add-header-btn').forEach(btn => {
      btn.addEventListener('click', (e) => {
        const type = e.currentTarget.dataset.type;
        this._addCustomFormat(type);
      });
    });

    // Delete buttons (delegated, for builtins these are just visual locks)
    container.querySelectorAll('.df-delete-btn').forEach(btn => {
      btn.addEventListener('click', (e) => {
        const item = e.currentTarget.closest('.df-format-item');
        if (item && item.classList.contains('df-custom-item')) {
          item.remove();
          this._persistCustomFormats();
        }
      });
    });
  }



  /**
   * Create a new FlatPickr instance for the sample input
   */


  /**
   * Initialize FlatpickrPicker on the sample input
   */
  _initFlatpickrPicker() {
    const container = this.container;
    if (!container) return;

    const input = container.querySelector('#df-sample-input');
    const btn = container.querySelector('#df-picker-btn');
    if (!input || !btn) return;

    // Create FlatpickrPicker instance
    this._flatpickrPicker = new FlatpickrPicker({
      onChange: (date, formattedValue) => {
        this.setSetting('sample', formattedValue);
        this._renderAllPreviews();
      },
    });

    // Initialize with trigger button and input
    this._flatpickrPicker.init(btn, input);
  }

  /**
   * Initialize the custom timezone picker
   */
  _initTimezonePicker() {
    const container = this.container;
    if (!container) return;

    const pickerContainer = container.querySelector('#df-timezone-picker');
    if (!pickerContainer) return;

    this._timezonePicker = new TimezonePicker(
      pickerContainer,
      IANA_TIMEZONES,
      (timezone) => {
        this.setSetting('timezone', timezone);
        this._timezonePicker.setTimezone(timezone); // Update picker display
        this._renderAllPreviews();
        // Update token table to reflect timezone change
        if (this._tokenTable) {
          this._tokenTable.refresh();
        }
      },
      (position) => {
        this.setSetting('timezonePopupPosition', position);
      }
    );

    // Load saved popup position
    const savedPosition = this.getSetting('timezonePopupPosition');
    if (savedPosition && typeof savedPosition === 'object') {
      this._timezonePicker.popupWidth = savedPosition.width;
      this._timezonePicker.popupHeight = savedPosition.height;
    }

    // Set initial timezone
    const savedTz = this.getSetting('timezone', 'local');
    this._timezonePicker.setTimezone(savedTz);
  }

  /**
   * Initialize the Luxon tokens LithiumTable
   */
  async _initTokenTable() {
    const container = this.container;
    if (!container) return;

    const tableContainer = container.querySelector('#df-token-table-container');
    const navContainer = container.querySelector('#df-token-navigator');
    if (!tableContainer) return;

    const sample = this._getSampleDateTime();

    this._tokenTable = new LuxonTokenTable({
      container: tableContainer,
      navigatorContainer: navContainer,
      sampleDateTime: sample,
      tablePath: 'profile-manager/luxon-tokens',
      lookupKeyIdx: 16,
      cssPrefix: 'profile-luxon-tokens',
      storageKey: 'profile_luxon_tokens',
      onTokenSelected: () => closeAllPopups(),
    });

    await this._tokenTable.init();
  }

  /**
   * Initialize OverlayScrollbars on the main content area
   */
  _initContentScrollbars() {
    const container = this.container;
    if (!container) return;

    const contentElement = container.querySelector('.df-content');
    if (!contentElement) return;

    // Initialize OSB on the content area using the same styling as other instances
    this._contentScrollbarInstance = scrollbarManager.initGeneric(contentElement);
  }



  /**
   * Start updating current time previews every second
   */
  _startCurrentTimeUpdates() {
    this._currentUpdateInterval = setInterval(() => {
      this._renderCurrentPreviews();
    }, 1000);
  }

  /**
   * Get the browser timezone IANA name
   */
  _getBrowserTimezone() {
    try {
      return Intl.DateTimeFormat().resolvedOptions().timeZone || 'UTC';
    } catch (_e) {
      return 'UTC';
    }
  }

  /**
   * Get the alternate timezone (override if different from browser, else UTC)
   */
  _getAlternateTimezone() {
    const overrideTz = this._getTimezone();
    const browserTz = this._getBrowserTimezone();

    if (overrideTz !== 'local' && overrideTz !== browserTz) {
      return overrideTz;
    }
    return 'UTC';
  }

  /**
   * Render current time previews for format tables
   */
  _renderCurrentPreviews() {
    const container = this.container;
    if (!container) return;

    const browserTz = this._getBrowserTimezone();
    const overrideTz = this._getTimezone();
    const altTz = this._getAlternateTimezone();

    // Primary: always browser timezone
    const nowPrimary = DateTime.now().setZone(browserTz);

    // Alternate: override if different from browser, else UTC
    const nowAlt = altTz === 'UTC' ? DateTime.now().toUTC() : DateTime.now().setZone(altTz);

    // Update the current datetime display at top (primary = browser)
    const currentDisplay = container.querySelector('#df-current-datetime');
    if (currentDisplay) {
      currentDisplay.textContent = nowPrimary.toFormat("yyyy-MM-dd'T'HH:mm:ss.SSSZZ '('ZZZZ')'");
    }

    // Update alternate current datetime
    const currentAltDisplay = container.querySelector('#df-current-datetime-alt');
    if (currentAltDisplay) {
      currentAltDisplay.textContent = nowAlt.toFormat("yyyy-MM-dd'T'HH:mm:ss.SSSZZ '('ZZZZ')'");
    }

    // Update browser timezone display
    const browserTzDisplay = container.querySelector('#df-browser-timezone');
    if (browserTzDisplay) {
      const abbr = nowPrimary.toFormat('ZZZZ');
      browserTzDisplay.textContent = `${browserTz} (${abbr})`;
    }

    // Update sample datetime display
    this._renderSampleDatetime();

    // Format tables - use override timezone for compatibility
    const nowForTables = overrideTz === 'local' ? DateTime.now() : DateTime.now().setZone(overrideTz);
    container.querySelectorAll('.df-current').forEach(el => {
      const format = el.dataset.format;
      if (format) {
        try {
          el.textContent = nowForTables.toFormat(format);
        } catch (_e) {
          el.textContent = 'Invalid';
        }
      }
    });
  }

  /**
   * Render sample datetime display
   */
  _renderSampleDatetime() {
    const container = this.container;
    if (!container) return;

    const sample = this._getSampleDateTime();
    const browserTz = this._getBrowserTimezone();
    const altTz = this._getAlternateTimezone();

    // Get current time in browser timezone for consistent abbreviation
    const nowPrimary = DateTime.now().setZone(browserTz);
    const nowAlt = altTz === 'UTC' ? DateTime.now().toUTC() : DateTime.now().setZone(altTz);

    // Primary: sample in browser timezone
    const dtPrimary = sample.setZone(browserTz);

    // Alternate: sample in alternate timezone
    const dtAlt = altTz === 'UTC' ? sample.toUTC() : sample.setZone(altTz);

    // Primary sample display - use current time's abbreviation for consistency
    const sampleDisplay = container.querySelector('#df-sample-datetime');
    if (sampleDisplay) {
      sampleDisplay.textContent = dtPrimary.toFormat("yyyy-MM-dd'T'HH:mm:ss.SSSZZ") + ' (' + nowPrimary.toFormat('ZZZZ') + ')';
    }

    // Alternate sample display - use current time's abbreviation for consistency
    const sampleAltDisplay = container.querySelector('#df-sample-datetime-alt');
    if (sampleAltDisplay) {
      sampleAltDisplay.textContent = dtAlt.toFormat("yyyy-MM-dd'T'HH:mm:ss.SSSZZ") + ' (' + nowAlt.toFormat('ZZZZ') + ')';
    }
  }

  /**
   * Render example previews for format tables
   */
  _renderExamplePreviews() {
    const container = this.container;
    if (!container) return;

    const sample = this._getSampleDateTime();
    const tz = this._getTimezone();
    const sampleInTz = tz === 'local' ? sample : sample.setZone(tz);

    // Format tables
    container.querySelectorAll('.df-example').forEach(el => {
      const row = el.closest('.df-format-item');
      if (!row) return;
      const input = row.querySelector('.df-format-input');
      if (!input) return;
      try {
        el.textContent = sampleInTz.toFormat(input.value);
      } catch (_e) {
        el.textContent = 'Invalid format';
      }
    });
  }

  /**
   * Update single preview (example + current)
   */
  _updatePreview(input) {
    const row = input.closest('.df-format-item');
    if (!row) return;

    const exampleEl = row.querySelector('.df-example');
    const currentEl = row.querySelector('.df-current');
    const format = input.value;

    const sample = this._getSampleDateTime();
    const tz = this._getTimezone();
    const sampleInTz = tz === 'local' ? sample : sample.setZone(tz);
    const now = this._getDateTimeNow();

    try {
      if (exampleEl) exampleEl.textContent = sampleInTz.toFormat(format);
    } catch (_e) {
      if (exampleEl) exampleEl.textContent = 'Invalid format';
    }

    try {
      if (currentEl) {
        currentEl.textContent = now.toFormat(format);
        currentEl.dataset.format = format;
      }
    } catch (_e) {
      if (currentEl) currentEl.textContent = 'Invalid';
    }
  }

  /**
   * Get sample DateTime
   */
  _getSampleDateTime() {
    const input = this.container?.querySelector('#df-sample-input');
    const sampleStr = input?.value || DEFAULT_SAMPLE;
    return DateTime.fromISO(sampleStr);
  }

  /**
   * Render all previews (example + current)
   */
  _renderAllPreviews() {
    this._renderExamplePreviews();
    this._renderCurrentPreviews();

    // Update token table with new sample datetime
    if (this._tokenTable) {
      const sample = this._getSampleDateTime();
      this._tokenTable.updateSample(sample);
    }
  }

  /**
   * Add custom format
   */
  _addCustomFormat(type) {
    const container = this.container;
    if (!container) return;

    // Copy from the last custom row, or use defaults
    const name = 'Custom Format';
    const format = type === 'dates' ? 'yyyy-MM-dd' : type === 'times' ? 'HH:mm' : 'yyyy-MM-dd HH:mm:ss';

    const listId = `df-${type}-all`;
    const list = container.querySelector(`#${listId}`);
    if (!list) return;

    const sample = this._getSampleDateTime();
    const now = this._getDateTimeNow();

    // Generate unique name
    const settingKey = type === 'datetime' ? 'datetimes' : type;
    const existing = this.getSetting(settingKey, {});
    let uniqueName = name;
    let counter = 1;
    while (existing[uniqueName]) {
      uniqueName = `${name} ${counter}`;
      counter++;
    }

    const item = document.createElement('tr');
    item.className = 'df-format-item df-custom-item';
    item.dataset.id = uniqueName;
    item.innerHTML = `
      <td class="df-col-icon"><button type="button" class="df-delete-btn" title="Delete"><fa fa-trash></fa></button></td>
      <td class="df-col-name"><input type="text" class="df-format-name-input" value="${this._escapeHtml(uniqueName)}" placeholder="Name"></td>
      <td class="df-col-format"><input type="text" class="form-input df-format-input" value="${this._escapeHtml(format)}" placeholder="Luxon format"></td>
      <td class="df-col-example"><span class="df-example">${this._safeFormat(sample, format)}</span></td>
      <td class="df-col-current"><span class="df-current" data-format="${this._escapeHtml(format)}">${this._safeFormat(now, format)}</span></td>
    `;

    const nameInput = item.querySelector('.df-format-name-input');
    const formatInput = item.querySelector('.df-format-input');
    const deleteBtn = item.querySelector('.df-delete-btn');

    nameInput?.addEventListener('input', () => this._persistCustomFormats());
    formatInput?.addEventListener('input', () => {
      this._updatePreview(formatInput);
      this._persistCustomFormats();
    });
    deleteBtn?.addEventListener('click', () => {
      item.remove();
      this._persistCustomFormats();
    });

    list.appendChild(item);
    this._persistCustomFormats();
  }

  /**
   * Persist all custom formats to the settings service
   */
  _persistCustomFormats() {
    const container = this.container;
    if (!container) return;

    ['dates', 'times', 'datetimes'].forEach(type => {
      const custom = this._gatherCustomFormats(type);
      const settingKey = type;

      // Get current stored formats for this type
      const currentStored = this.getSetting(settingKey, {});
      const merged = { ...currentStored };

      // Remove old custom formats (anything not in builtin defaults)
      if (BUILTIN_FORMATS[type]) {
        Object.keys(merged).forEach(key => {
          if (!(key in BUILTIN_FORMATS[type])) {
            delete merged[key];
          }
        });
      }

      // Add current custom formats
      Object.assign(merged, custom);

      // Update the setting
      this.setSetting(settingKey, merged);
    });
  }

  /**
   * Safe format helper
   */
  _safeFormat(dt, format) {
    try {
      return dt.toFormat(format);
    } catch (_e) {
      return 'Invalid';
    }
  }

  /**
   * Escape HTML
   */
  _escapeHtml(str) {
    const div = document.createElement('div');
    div.textContent = str;
    return div.innerHTML;
  }

  /**
   * Load data from settings service
   */
  async loadData() {
    const container = this.container;
    if (!container) return;

    // Sample
    const sampleInput = container.querySelector('#df-sample-input');
    if (sampleInput) {
      sampleInput.value = this.getSetting('sample', DEFAULT_SAMPLE);
    }

    // Timezone picker
    if (this._timezonePicker) {
      const savedTz = this.getSetting('timezone', 'local');
      this._timezonePicker.setTimezone(savedTz);
    }

    // Built-in formats
    this._loadBuiltinFormats('dates');
    this._loadBuiltinFormats('times');
    this._loadBuiltinFormats('datetimes');

    // Custom formats
    this._loadCustomFormats('dates');
    this._loadCustomFormats('times');
    this._loadCustomFormats('datetimes');

    this._renderAllPreviews();
  }

  /**
   * Load built-in formats from settings service
   */
  _loadBuiltinFormats(type) {
    const container = this.container;
    if (!container) return;

    let allFormats = this.getSetting(type, {});

    // If no formats exist for this type, populate with defaults
    if (Object.keys(allFormats).length === 0 && BUILTIN_FORMATS[type]) {
      allFormats = { ...BUILTIN_FORMATS[type] };
      // Note: Defaults are now initialized by Profile Manager, but keep as fallback
      this.setSetting(type, allFormats);
    }

    // Load all formats for this type (both builtin and custom)
    Object.entries(allFormats).forEach(([key, format]) => {
      const input = container.querySelector(`[data-setting="${type}.${key}"]`);
      if (input && typeof format === 'string') {
        input.value = format;
        this._updatePreview(input);
      }
    });
  }

  /**
   * Load custom formats from settings service
   */
   _loadCustomFormats(type) {
    const container = this.container;
    if (!container) return;

    const listId = `df-${type}-all`;
    const list = container.querySelector(`#${listId}`);
    if (!list) return;

    // Remove existing custom items
    list.querySelectorAll('.df-custom-item').forEach(item => item.remove());

    const settingKey = type;
    const allFormats = this.getSetting(settingKey, {});

    const sample = this._getSampleDateTime();
    const now = this._getDateTimeNow();

    // Only load formats that are not builtins
    Object.entries(allFormats).forEach(([name, format]) => {
      if (typeof format !== 'string') return;

      // Skip if this is a builtin format
      if (BUILTIN_FORMATS[type] && name in BUILTIN_FORMATS[type]) {
        return;
      }

      const item = document.createElement('tr');
      item.className = 'df-format-item df-custom-item';
      item.dataset.id = name; // Use name as ID for simplicity
      item.innerHTML = `
        <td class="df-col-icon"><button type="button" class="df-delete-btn" title="Delete"><fa fa-trash></fa></button></td>
        <td class="df-col-name"><input type="text" class="df-format-name-input" value="${this._escapeHtml(name)}" placeholder="Name"></td>
        <td class="df-col-format"><input type="text" class="form-input df-format-input" value="${this._escapeHtml(format)}" placeholder="Luxon format"></td>
        <td class="df-col-example"><span class="df-example">${this._safeFormat(sample, format)}</span></td>
        <td class="df-col-current"><span class="df-current" data-format="${this._escapeHtml(format)}">${this._safeFormat(now, format)}</span></td>
      `;

      const nameInput = item.querySelector('.df-format-name-input');
      const formatInput = item.querySelector('.df-format-input');
      const deleteBtn = item.querySelector('.df-delete-btn');

      nameInput?.addEventListener('input', () => this._persistCustomFormats());
      formatInput?.addEventListener('input', () => {
        this._updatePreview(formatInput);
        this._persistCustomFormats();
      });
      deleteBtn?.addEventListener('click', () => {
        item.remove();
        this._persistCustomFormats();
      });

      list.appendChild(item);
    });
  }

  /**
   * Gather custom formats from DOM
   */
  _gatherCustomFormats(type) {
    const container = this.container;
    const custom = {};

    const listId = `df-${type}-all`;
    const list = container?.querySelector(`#${listId}`);
    if (!list) return custom;

    list.querySelectorAll('.df-custom-item').forEach(item => {
      const nameInput = item.querySelector('.df-format-name-input');
      const input = item.querySelector('.df-format-input');
      if (nameInput && input && nameInput.value.trim()) {
        // Use the name as the key, format as the value
        custom[nameInput.value.trim()] = input.value;
      }
    });

    return custom;
  }

  /**
   * Save — no-op for live-save page, but required by base class
   */
  async save() {
    // All changes are saved live via setSetting
    return { success: true };
  }

  /**
   * Cancel — reload from service
   */
  cancel() {
    this.loadData();
  }

  /**
   * Always clean for live-save page
   */
  isDirty() {
    return false;
  }

  /**
   * On show
   */
  onShow() {
    this._renderCurrentPreviews();
  }

  /**
   * Called when the page is hidden
   */
  onHide() {
    // Close any open popups when switching away from this page
    if (this._timezonePicker && this._timezonePicker.isOpen) {
      this._timezonePicker.close();
    }
    if (this._flatpickrPicker && this._flatpickrPicker.getIsOpen()) {
      this._flatpickrPicker.close();
    }
  }

  /**
   * Destroy the page
   */
  destroy() {
    if (this._currentUpdateInterval) {
      clearInterval(this._currentUpdateInterval);
      this._currentUpdateInterval = null;
    }
    if (this._flatpickrPicker) {
      this._flatpickrPicker.destroy();
      this._flatpickrPicker = null;
    }
    if (this._tokenTable) {
      this._tokenTable.destroy();
      this._tokenTable = null;
    }
    if (this._timezonePicker) {
      this._timezonePicker.destroy();
      this._timezonePicker = null;
    }
    if (this._contentScrollbarInstance) {
      scrollbarManager.destroy(this._contentScrollbarInstance);
      this._contentScrollbarInstance = null;
    }
    super.destroy();
  }
}

/**
 * Profile Manager - Number Formats Page Handler
 *
 * Handles the Number Formats settings page (index: -10)
 */


class NumberFormatsPage extends SimpleSettingsPage {
  constructor(options = {}) {
    super({ ...options, index: -10, formSelector: 'form' });
  }
  async onInit() { await super.onInit(); log(Subsystems.MANAGER, Status.DEBUG, '[NumberFormatsPage] Initialized (stub)'); }
  async loadData() { /* TODO */ }
  async save() { this.setDirty(false); return { success: true }; }
}

/**
 * Profile Manager - Startup Page Handler
 *
 * Handles the Startup settings page (index: -11)
 */


class StartupPage extends SimpleSettingsPage {
  constructor(options = {}) {
    super({ ...options, index: -11, formSelector: 'form' });
  }
  async onInit() { await super.onInit(); log(Subsystems.MANAGER, Status.DEBUG, '[StartupPage] Initialized (stub)'); }
}

/**
 * Camera Popout Component for Photo Capture
 *
 * Provides a popout interface for camera capture with live preview,
 * camera selection, flip/rotate controls, and photo capture.
 */


class CameraPopup {
  constructor(options = {}) {
    this.options = options;
    this.isVisible = false;
    this.stream = null;
    this.currentDeviceId = null;
    this.rotation = 0;
    this.hFlip = false;
    this.vFlip = false;
    this.videoElement = null;
    this.canvas = null;
    this.overlay = null;
    this.popup = null;

    // Bind methods
    this.handleOverlayClick = this.handleOverlayClick.bind(this);
    this.handleKeyDown = this.handleKeyDown.bind(this);
    this.handleResizeStart = this.handleResizeStart.bind(this);
    this.handleDragStart = this.handleDragStart.bind(this);
    this.handleDragMove = this.handleDragMove.bind(this);
    this.handleDragEnd = this.handleDragEnd.bind(this);
    this.handleDeviceChange = this.handleDeviceChange.bind(this);
    this.handleCapture = this.handleCapture.bind(this);
    this.closeAllPopupsHandler = this.closeAllPopupsHandler.bind(this);
  }

  /**
   * Initialize the popup (called on first show)
   */
  init() {
    if (this.popup) return;

    // Create overlay
    this.overlay = document.createElement('div');
    this.overlay.className = 'camera-popout-overlay';
    this.overlay.addEventListener('click', this.handleOverlayClick);
    document.body.appendChild(this.overlay);

    // Create popup
    this.popup = document.createElement('div');
    this.popup.className = 'camera-popout';
    this.popup.innerHTML = `
      <div class="camera-resize-handle camera-resize-handle-tl" data-tooltip="Resize"></div>
      <div class="camera-resize-handle camera-resize-handle-tr" data-tooltip="Resize"></div>
      <div class="camera-header">
        <div class="subpanel-header-group">
          <button type="button" class="camera-header-primary">
            <fa fa-camera></fa>
            <span>Take Photo</span>
          </button>
          <button type="button" class="camera-header-placeholder"></button>
          <button type="button" class="camera-header-close" data-tooltip="Close (ESC)">
            <fa fa-xmark></fa>
          </button>
        </div>
      </div>
      <div class="camera-content">
        <div class="camera-video-container">
          <video id="camera-video" autoplay playsinline muted></video>
        </div>
        <div class="camera-controls">
          <div class="camera-device-selector">
            <label for="camera-select">Camera:</label>
            <select id="camera-select" disabled>
              <option value="">Loading cameras...</option>
            </select>
          </div>
          <div class="camera-iso-selector" id="camera-iso-container" style="display:none;">
            <label for="camera-iso">ISO:</label>
            <select id="camera-iso">
              <option value="auto">Auto</option>
            </select>
          </div>
          <div class="camera-gain-selector" id="camera-gain-container" style="display:none;">
            <label for="camera-gain">Gain:</label>
            <select id="camera-gain">
              <option value="auto">Auto</option>
            </select>
          </div>
          <div class="camera-transform-controls">
            <button id="flip-h-btn" class="camera-flip-btn" disabled data-tooltip="Flip Horizontal">
              <fa fa-left-right></fa>
            </button>
            <button id="flip-v-btn" class="camera-flip-btn" disabled data-tooltip="Flip Vertical">
              <fa fa-up-down></fa>
            </button>
            <button id="rotate-left-btn" class="camera-rotate-btn" disabled data-tooltip="Rotate Left">
              <fa fa-rotate-left></fa>
            </button>
            <button id="rotate-right-btn" class="camera-rotate-btn" disabled data-tooltip="Rotate Right">
              <fa fa-rotate-right></fa>
            </button>
            <button id="reset-transform-btn" disabled data-tooltip="Reset Transforms">
              <fa fa-arrow-rotate-left></fa>
            </button>
          </div>
        </div>
        <div class="camera-actions">
          <button id="camera-start-btn">Start Camera</button>
          <button id="camera-capture-btn" disabled>Capture Photo</button>
          <button id="camera-retake-btn" style="display:none;">Retake</button>
        </div>
      </div>
      <div class="camera-resize-handle camera-resize-handle-bl" data-tooltip="Resize"></div>
      <div class="camera-resize-handle camera-resize-handle-br" data-tooltip="Resize"></div>
    `;

    document.body.appendChild(this.popup);

    // Cache elements
    this.videoElement = this.popup.querySelector('#camera-video');
    this.cameraSelect = this.popup.querySelector('#camera-select');
    this.isoContainer = this.popup.querySelector('#camera-iso-container');
    this.isoSelect = this.popup.querySelector('#camera-iso');
    this.gainContainer = this.popup.querySelector('#camera-gain-container');
    this.gainSelect = this.popup.querySelector('#camera-gain');
    this.startBtn = this.popup.querySelector('#camera-start-btn');
    this.captureBtn = this.popup.querySelector('#camera-capture-btn');
    this.retakeBtn = this.popup.querySelector('#camera-retake-btn');
    this.flipHBtn = this.popup.querySelector('#flip-h-btn');
    this.flipVBtn = this.popup.querySelector('#flip-v-btn');
    this.rotateLeftBtn = this.popup.querySelector('#rotate-left-btn');
    this.rotateRightBtn = this.popup.querySelector('#rotate-right-btn');
    this.resetBtn = this.popup.querySelector('#reset-transform-btn');

    // Create canvas for capture
    this.canvas = document.createElement('canvas');
    this.canvas.style.display = 'none';
    document.body.appendChild(this.canvas);

    // Bind events
    this.bindEvents();

    // Add drag functionality to header
    const header = this.popup.querySelector('.camera-header');
    if (header) {
      header.addEventListener('mousedown', (e) => this.handleDragStart(e));
    }

    // Initialize tooltips
    if (window.initTooltips) {
      window.initTooltips(this.popup);
    }
  }

  /**
   * Bind event listeners
   */
  bindEvents() {
    // Close button
    const closeBtn = this.popup.querySelector('.camera-header-close');
    closeBtn.addEventListener('click', () => this.hide());

    // Camera controls
    this.startBtn.addEventListener('click', () => this.startCamera());
    this.captureBtn.addEventListener('click', () => this.handleCapture());
    this.retakeBtn.addEventListener('click', () => this.retake());

    // Transform controls
    this.flipHBtn.addEventListener('click', () => this.toggleFlip('h'));
    this.flipVBtn.addEventListener('click', () => this.toggleFlip('v'));
    this.rotateLeftBtn.addEventListener('click', () => this.rotate(-90));
    this.rotateRightBtn.addEventListener('click', () => this.rotate(90));
    this.resetBtn.addEventListener('click', () => this.resetTransforms());

    // Camera selection
    this.cameraSelect.addEventListener('change', this.handleDeviceChange);

    // ISO selection
    this.isoSelect.addEventListener('change', () => this.handleIsoChange());

    // Gain selection
    this.gainSelect.addEventListener('change', () => this.handleGainChange());

    // Resize handles
    const resizeHandles = this.popup.querySelectorAll('.camera-resize-handle');
    resizeHandles.forEach(handle => {
      handle.addEventListener('mousedown', (e) => this.handleResizeStart(e, handle.classList[1].split('-')[3]));
    });

    // Listen for close-all-popups
    document.addEventListener('close-all-popups', this.closeAllPopupsHandler);
  }

  /**
   * Handle close-all-popups event
   */
  closeAllPopupsHandler() {
    if (this.isVisible) {
      this.hide();
    }
  }

  /**
   * Show the popup
   */
  async show() {
    if (!this.popup) {
      this.init();
    }

    if (this.isVisible) {
      return;
    }

    // Dispatch close-all-popups first
    document.dispatchEvent(new CustomEvent('close-all-popups'));

    // Position popup (center if no saved position)
    this.centerPopup();

    // Show overlay and popup
    this.overlay.classList.add('visible');
    this.popup.classList.add('visible');
    this.isVisible = true;

    // Add global keydown listener
    document.addEventListener('keydown', this.handleKeyDown);

    // Try to start camera automatically
    try {
      await this.startCamera();
    } catch (error) {
      log(Subsystems.MANAGER, Status.WARN, '[CameraPopout] Auto-start failed:', error.message);
    }

    log(Subsystems.MANAGER, Status.INFO, '[CameraPopout] Shown');
  }

  /**
   * Hide the popup
   */
  hide() {
    if (!this.isVisible) return;

    // Stop camera stream
    this.stopCamera();

    // Hide popup
    this.overlay.classList.remove('visible');
    this.popup.classList.remove('visible');
    this.isVisible = false;

    // Remove global listeners
    document.removeEventListener('keydown', this.handleKeyDown);

    log(Subsystems.MANAGER, Status.INFO, '[CameraPopout] Hidden');
  }

  /**
   * Center the popup on screen
   */
  centerPopup() {
    if (!this.popup) return;

    const viewportWidth = window.innerWidth;
    const viewportHeight = window.innerHeight;
    const popupWidth = 600; // Default width
    const popupHeight = 500; // Default height

    this.popup.style.left = `${Math.round((viewportWidth - popupWidth) / 2)}px`;
    this.popup.style.top = `${Math.round((viewportHeight - popupHeight) / 2)}px`;
    this.popup.style.width = `${popupWidth}px`;
    this.popup.style.height = `${popupHeight}px`;
  }

  /**
   * Start camera with selected device
   */
  async startCamera(deviceId = null) {
    this.stopCamera();

    const baseConstraints = {
      width: { ideal: 4096 },
      height: { ideal: 2160 },
      facingMode: 'user',
      noiseSuppression: false,
      advanced: [
        { exposureMode: 'manual' },
        { iso: { ideal: 100 } },
        { brightness: { ideal: 128 } },
        { exposureCompensation: { ideal: 0 } },
      ],
    };

    const constraints = {
      video: deviceId
        ? { ...baseConstraints, deviceId: { exact: deviceId } }
        : baseConstraints
    };

    try {
      this.stream = await navigator.mediaDevices.getUserMedia(constraints);
      this.videoElement.srcObject = this.stream;

      // Get the actual device ID and applied settings
      const track = this.stream.getVideoTracks()[0];
      const settings = track.getSettings();
      this.currentDeviceId = settings.deviceId || deviceId;

      // Log actual resolution for debugging
      log(Subsystems.MANAGER, Status.INFO,
        `[CameraPopout] Camera started: ${settings.width}x${settings.height}`);

      // Detect and populate ISO / Gain capabilities
      await this._detectIsoCapabilities(track);
      await this._detectGainCapabilities(track);

      // Populate camera list if not already done
      if (this.cameraSelect.options.length <= 1) {
        await this.populateCameraList();
      }

      // Select current camera
      this.cameraSelect.value = this.currentDeviceId || '';

      // Update UI
      this.startBtn.style.display = 'none';
      this.captureBtn.disabled = false;
      this.updateButtonStates();

      // Apply current transforms
      this.applyVideoTransforms();

    } catch (error) {
      log(Subsystems.MANAGER, Status.ERROR, '[CameraPopout] Failed to start camera:', error.message);
      alert('Camera access failed. Please check permissions.');
    }
  }

  /**
   * Stop current camera stream
   */
  stopCamera() {
    if (this.stream) {
      this.stream.getTracks().forEach(track => track.stop());
      this.stream = null;
    }
    this.videoElement.srcObject = null;
  }

  /**
   * Detect ISO capabilities and populate ISO selector
   * @param {MediaStreamTrack} track - Active video track
   * @private
   */
  async _detectIsoCapabilities(track) {
    try {
      const capabilities = track.getCapabilities();
      const settings = track.getSettings();

      if (!capabilities.iso) {
        this.isoContainer.style.display = 'none';
        log(Subsystems.MANAGER, Status.INFO, '[CameraPopout] ISO control not supported');
        return;
      }

      // Build ISO options from capabilities
      const { min, max, step } = capabilities.iso;
      const isoValues = [];
      for (let v = min; v <= max; v = Math.round(v * step > 1 ? v * step : v + step)) {
        isoValues.push(v);
        if (v >= max) break;
      }
      // Add common standard ISO values within range
      const standardIsos = [50, 100, 200, 400, 800, 1600, 3200, 6400, 12800, 25600];
      standardIsos.forEach(iso => {
        if (iso >= min && iso <= max && !isoValues.includes(iso)) {
          isoValues.push(iso);
        }
      });
      isoValues.sort((a, b) => a - b);

      // Populate selector
      this.isoSelect.innerHTML = '<option value="auto">Auto</option>';
      isoValues.forEach(iso => {
        const opt = document.createElement('option');
        opt.value = iso;
        opt.textContent = iso;
        this.isoSelect.appendChild(opt);
      });

      // Set current value
      const currentIso = settings.iso;
      if (currentIso) {
        this.isoSelect.value = currentIso;
      }

      this.isoContainer.style.display = '';
      log(Subsystems.MANAGER, Status.INFO,
        `[CameraPopout] ISO range: ${min}-${max}, current: ${currentIso || 'auto'}`);

    } catch (error) {
      log(Subsystems.MANAGER, Status.WARN, '[CameraPopout] ISO detection failed:', error.message);
      this.isoContainer.style.display = 'none';
    }
  }

  /**
   * Handle ISO selection change
   * @private
   */
  async handleIsoChange() {
    if (!this.stream) return;

    const track = this.stream.getVideoTracks()[0];
    if (!track) return;

    const value = this.isoSelect.value;
    const constraints = {};

    if (value === 'auto') {
      constraints.advanced = [{ iso: 'auto' }];
    } else {
      const isoValue = parseInt(value, 10);
      constraints.advanced = [{ iso: isoValue }];
    }

    try {
      await track.applyConstraints(constraints);
      const newSettings = track.getSettings();
      log(Subsystems.MANAGER, Status.INFO,
        `[CameraPopout] ISO set to: ${newSettings.iso || 'auto'}`);
    } catch (error) {
      log(Subsystems.MANAGER, Status.ERROR, '[CameraPopout] Failed to set ISO:', error.message);
    }
  }

  /**
   * Detect Gain capabilities and populate Gain selector
   * @param {MediaStreamTrack} track - Active video track
   * @private
   */
  async _detectGainCapabilities(track) {
    try {
      const capabilities = track.getCapabilities();
      const settings = track.getSettings();

      if (!capabilities.gain) {
        this.gainContainer.style.display = 'none';
        log(Subsystems.MANAGER, Status.INFO, '[CameraPopout] Gain control not supported');
        return;
      }

      // Build Gain options from capabilities
      const { min, max, step } = capabilities.gain;
      const gainValues = [];
      for (let v = min; v <= max; v = Math.round(v * step > 1 ? v * step : v + step)) {
        gainValues.push(v);
        if (v >= max) break;
      }
      gainValues.sort((a, b) => a - b);

      // Populate selector
      this.gainSelect.innerHTML = '<option value="auto">Auto</option>';
      gainValues.forEach(gain => {
        const opt = document.createElement('option');
        opt.value = gain;
        opt.textContent = gain;
        this.gainSelect.appendChild(opt);
      });

      // Set current value
      const currentGain = settings.gain;
      if (currentGain !== undefined) {
        this.gainSelect.value = currentGain;
      }

      this.gainContainer.style.display = '';
      log(Subsystems.MANAGER, Status.INFO,
        `[CameraPopout] Gain range: ${min}-${max}, current: ${currentGain !== undefined ? currentGain : 'auto'}`);

    } catch (error) {
      log(Subsystems.MANAGER, Status.WARN, '[CameraPopout] Gain detection failed:', error.message);
      this.gainContainer.style.display = 'none';
    }
  }

  /**
   * Handle Gain selection change
   * @private
   */
  async handleGainChange() {
    if (!this.stream) return;

    const track = this.stream.getVideoTracks()[0];
    if (!track) return;

    const value = this.gainSelect.value;
    const constraints = {};

    if (value === 'auto') {
      constraints.advanced = [{ gain: 'auto' }];
    } else {
      const gainValue = parseInt(value, 10);
      constraints.advanced = [{ gain: gainValue }];
    }

    try {
      await track.applyConstraints(constraints);
      const newSettings = track.getSettings();
      log(Subsystems.MANAGER, Status.INFO,
        `[CameraPopout] Gain set to: ${newSettings.gain !== undefined ? newSettings.gain : 'auto'}`);
    } catch (error) {
      log(Subsystems.MANAGER, Status.ERROR, '[CameraPopout] Failed to set Gain:', error.message);
    }
  }

  /**
   * Populate camera selection dropdown
   */
  async populateCameraList() {
    try {
      const devices = await navigator.mediaDevices.enumerateDevices();
      const videoDevices = devices.filter(d => d.kind === 'videoinput');

      this.cameraSelect.innerHTML = '';

      if (videoDevices.length === 0) {
        const opt = document.createElement('option');
        opt.textContent = 'No cameras found';
        this.cameraSelect.appendChild(opt);
        return;
      }

      for (const device of videoDevices) {
        const opt = document.createElement('option');
        opt.value = device.deviceId;

        // Query capabilities to show resolution info
        let resolutionInfo = '';
        try {
          const testStream = await navigator.mediaDevices.getUserMedia({
            video: { deviceId: { exact: device.deviceId }, width: { ideal: 4096 }, height: { ideal: 2160 } }
          });
          const track = testStream.getVideoTracks()[0];
          const settings = track.getSettings();
          resolutionInfo = ` (${settings.width}x${settings.height})`;
          track.stop();
        } catch {
          // Fallback: no resolution info
        }

        opt.textContent = (device.label || `Camera ${videoDevices.indexOf(device) + 1}`) + resolutionInfo;
        this.cameraSelect.appendChild(opt);
      }

    } catch (error) {
      log(Subsystems.MANAGER, Status.ERROR, '[CameraPopout] Failed to enumerate cameras:', error.message);
    }
  }

  /**
   * Apply transforms to video preview
   */
  applyVideoTransforms() {
    if (!this.videoElement) return;

    let transform = `rotate(${this.rotation}deg)`;
    if (this.hFlip) transform += ' scaleX(-1)';
    if (this.vFlip) transform += ' scaleY(-1)';

    this.videoElement.style.transform = transform;
  }

  /**
   * Update button states
   */
  updateButtonStates() {
    const hasStream = !!this.stream;
    this.captureBtn.disabled = !hasStream;
    this.flipHBtn.disabled = !hasStream;
    this.flipVBtn.disabled = !hasStream;
    this.rotateLeftBtn.disabled = !hasStream;
    this.rotateRightBtn.disabled = !hasStream;
    this.resetBtn.disabled = !hasStream || (this.rotation === 0 && !this.hFlip && !this.vFlip);
    this.cameraSelect.disabled = !hasStream;
  }

  /**
   * Toggle horizontal/vertical flip
   */
  toggleFlip(type) {
    if (type === 'h') {
      this.hFlip = !this.hFlip;
    } else if (type === 'v') {
      this.vFlip = !this.vFlip;
    }
    this.applyVideoTransforms();
    this.updateButtonStates();
  }

  /**
   * Rotate video
   */
  rotate(degrees) {
    this.rotation = (this.rotation + degrees + 360) % 360;
    this.applyVideoTransforms();
    this.updateButtonStates();
  }

  /**
   * Reset all transforms
   */
  resetTransforms() {
    this.rotation = 0;
    this.hFlip = false;
    this.vFlip = false;
    this.applyVideoTransforms();
    this.updateButtonStates();
  }

  /**
   * Handle camera device change
   */
  handleDeviceChange() {
    if (this.cameraSelect.value) {
      this.startCamera(this.cameraSelect.value);
    }
  }

  /**
   * Capture photo with current transforms
   */
  async handleCapture() {
    if (!this.stream || !this.videoElement) return;

    try {
      const w = this.videoElement.videoWidth;
      const h = this.videoElement.videoHeight;

      // Determine canvas size (swap for 90°/270° rotation)
      let cw = w;
      let ch = h;
      if (this.rotation % 180 !== 0) {
        cw = h;
        ch = w;
      }

      this.canvas.width = cw;
      this.canvas.height = ch;

      const ctx = this.canvas.getContext('2d', { alpha: false });
      ctx.save();

      // Move origin to center
      ctx.translate(cw / 2, ch / 2);

      // Apply rotation
      ctx.rotate(this.rotation * Math.PI / 180);

      // Apply flips
      ctx.scale(this.hFlip ? -1 : 1, this.vFlip ? -1 : 1);

      // Draw the current video frame centered
      ctx.drawImage(this.videoElement, -w / 2, -h / 2, w, h);

      ctx.restore();

      // Apply mild noise reduction via temporary downscale/upscale
      // Step 1: draw to a small temp canvas (blur/noise average out)
      const tempCanvas = document.createElement('canvas');
      const scaleFactor = 0.5;
      tempCanvas.width = Math.max(1, Math.round(cw * scaleFactor));
      tempCanvas.height = Math.max(1, Math.round(ch * scaleFactor));
      const tempCtx = tempCanvas.getContext('2d', { alpha: false });
      tempCtx.imageSmoothingEnabled = true;
      tempCtx.imageSmoothingQuality = 'high';
      tempCtx.drawImage(this.canvas, 0, 0, tempCanvas.width, tempCanvas.height);

      // Step 2: draw back to original canvas at full size
      ctx.imageSmoothingEnabled = true;
      ctx.imageSmoothingQuality = 'high';
      ctx.drawImage(tempCanvas, 0, 0, cw, ch);

      // Convert to blob at maximum quality
      const blob = await new Promise(resolve => {
        this.canvas.toBlob(resolve, 'image/png');
      });

      if (!blob || blob.size === 0) {
        throw new Error('Failed to capture image');
      }

      // Create file
      const file = new window.File([blob], 'capture.png', { type: 'image/png' });

      // Call callback if provided
      if (this.options.onCapture) {
        this.options.onCapture(file);
      }

      // Hide popout after successful capture

      this.hide();

      log(Subsystems.MANAGER, Status.INFO, '[CameraPopout] Photo captured successfully');

    } catch (error) {
      log(Subsystems.MANAGER, Status.ERROR, '[CameraPopout] Capture failed:', error.message);
      alert('Failed to capture photo. Please try again.');
    }
  }

  /**
   * Retake photo (restart camera)
   */
  async retake() {
    // Show controls again
    this.startBtn.style.display = 'inline-block';
    this.captureBtn.style.display = 'inline-block';
    this.retakeBtn.style.display = 'none';

    // Restart camera
    await this.startCamera(this.currentDeviceId);
  }

  /**
   * Handle overlay click (close popup)
   */
  handleOverlayClick(e) {
    if (e.target === this.overlay) {
      this.hide();
    }
  }

  /**
   * Handle keydown events
   */
  handleKeyDown(e) {
    if (e.key === 'Escape') {
      this.hide();
      e.preventDefault();
    } else if (e.key === ' ' && !this.captureBtn.disabled) {
      // Space to capture
      this.handleCapture();
      e.preventDefault();
    }
  }

  /**
   * Handle resize start
   */
  handleResizeStart(e, corner) {
    // Basic resize implementation (similar to Terminal popup)
    e.preventDefault();

    const startX = e.clientX;
    const startY = e.clientY;
    const startRect = this.popup.getBoundingClientRect();

    const handleMouseMove = (e) => {
      const deltaX = e.clientX - startX;
      const deltaY = e.clientY - startY;

      let newWidth = startRect.width;
      let newHeight = startRect.height;

      if (corner.includes('r')) newWidth = startRect.width + deltaX;
      if (corner.includes('l')) {
        newWidth = startRect.width - deltaX;
        this.popup.style.left = `${startRect.left + deltaX}px`;
      }
      if (corner.includes('b')) newHeight = startRect.height + deltaY;
      if (corner.includes('t')) {
        newHeight = startRect.height - deltaY;
        this.popup.style.top = `${startRect.top + deltaY}px`;
      }

      // Enforce minimum size
      newWidth = Math.max(400, newWidth);
      newHeight = Math.max(300, newHeight);

      this.popup.style.width = `${newWidth}px`;
      this.popup.style.height = `${newHeight}px`;
    };

    const handleMouseUp = () => {
      document.removeEventListener('mousemove', handleMouseMove);
      document.removeEventListener('mouseup', handleMouseUp);
    };

    document.addEventListener('mousemove', handleMouseMove);
    document.addEventListener('mouseup', handleMouseUp);
  }

  /**
   * Destroy the popup (cleanup)
   */
  destroy() {
    this.hide();

    if (this.popup) {
      document.body.removeChild(this.popup);
      this.popup = null;
    }

    if (this.overlay) {
      document.body.removeChild(this.overlay);
      this.overlay = null;
    }

    if (this.canvas) {
      document.body.removeChild(this.canvas);
      this.canvas = null;
    }

    document.removeEventListener('close-all-popups', this.closeAllPopupsHandler);
  }

  /**
   * Handle drag start
   */
  handleDragStart(e) {
    // Allow dragging from the title area and placeholder, but not from actionable controls
    if (e.target.closest([
      '.camera-header-close',
      '#camera-select'
    ].join(', '))) return;

    this.isDragging = true;
    this.dragStartX = e.clientX;
    this.dragStartY = e.clientY;

    const rect = this.popup.getBoundingClientRect();
    this.popupStartX = rect.left;
    this.popupStartY = rect.top;

    this.popup.classList.add('dragging');

    document.addEventListener('mousemove', this.handleDragMove);
    document.addEventListener('mouseup', this.handleDragEnd);

    e.preventDefault();
  }

  /**
   * Handle drag move
   */
  handleDragMove(e) {
    if (!this.isDragging) return;

    const dx = e.clientX - this.dragStartX;
    const dy = e.clientY - this.dragStartY;

    let newX = this.popupStartX + dx;
    let newY = this.popupStartY + dy;

    // Keep some part of the popout visible
    const rect = this.popup.getBoundingClientRect();
    const minVisible = 50;

    newX = Math.max(minVisible - rect.width, Math.min(newX, window.innerWidth - minVisible));
    newY = Math.max(0, Math.min(newY, window.innerHeight - minVisible));

    this.popup.style.left = `${newX}px`;
    this.popup.style.top = `${newY}px`;
  }

  /**
   * Handle drag end
   */
  handleDragEnd() {
    if (!this.isDragging) return;

    this.isDragging = false;
    this.popup.classList.remove('dragging');

    document.removeEventListener('mousemove', this.handleDragMove);
    document.removeEventListener('mouseup', this.handleDragEnd);
  }
}

/**
 * Profile Manager - Photo Page
 *
 * Handles user photo upload, capture, editing, and saving as 200x200 base64 PNG
 * Features: custom sliders with icon thumbs, drag-to-reposition, FloatingUI tooltips
 */


class PhotoPage extends BaseSettingsPage {
  constructor(options = {}) {
    super({ ...options, index: -12 });

    // Editor state
    this.photoData = null;
    this.originalPhotoData = null;
    this.originalWidth = null;
    this.originalHeight = null;
    this.initialScale = 1;  // Scale calculated to fit preview when image is first loaded
    this.xPct = 0;  // Horizontal offset as % of displayed bbox width
    this.yPct = 0;  // Vertical offset as % of displayed bbox height
    this.scale = 1;
    this.rotation = 0;
    this.originalFile = null;

    // DOM elements
    this.elements = {};
    this._customSliders = {};
    this._isDragging = false;
    this._dragStart = { x: 0, y: 0, startX: 0, startY: 0 };
    this._saveTimeout = null;

    // Camera popup
    this.cameraPopup = null;
  }

  /**
   * Initialize the page
   */
  async onInit() {
    this._cacheElements();
    this._createCustomSliders();
    this._bindEvents();
    await this._loadExistingPhoto();
    this._syncToolbarWidth();
    this._initCameraPopup();
  }

  /**
   * Cache DOM elements
   * @private
   */
  _cacheElements() {
    const container = this.container;
    this.elements = {
      uploadInput: container.querySelector('#photo-upload'),
      captureBtn: container.querySelector('#photo-capture-btn'),
      removeBtn: container.querySelector('#photo-remove-btn'),
      resetBtn: container.querySelector('#photo-reset-btn'),
      downloadBtn: container.querySelector('.photo-download-btn'),
      flipHBtn: container.querySelector('#photo-flip-h-btn'),
      flipVBtn: container.querySelector('#photo-flip-v-btn'),
      rotateLeftBtn: container.querySelector('#photo-rotate-left-btn'),
      rotateRightBtn: container.querySelector('#photo-rotate-right-btn'),
      status: container.querySelector('#photo-status'),
      editor: container.querySelector('#photo-editor'),
      imageContainer: container.querySelector('#photo-image-container'),
      image: container.querySelector('#photo-image'),
      dimming: container.querySelector('#photo-dimming'),
      previewOverlay: container.querySelector('#photo-preview-overlay'),
       resizeHandle: container.querySelector('#photo-resize-handle'),
        toolbar: container.querySelector('#photo-toolbar'),
        previews: container.querySelector('.photo-previews'),
        preview40: container.querySelector('#photo-preview-40'),
        preview80: container.querySelector('#photo-preview-80'),
        preview150: container.querySelector('#photo-preview-150'),
    };
  }

  /**
   * Create custom sliders with icon thumbs and FloatingUI tooltips
   * @private
   */
  _createCustomSliders() {
    const sliderConfigs = [
      { id: 'x', min: -100, max: 100, value: 0, step: 1, orientation: 'horizontal', icon: 'fa-left-right', label: 'Left/Right', positionClass: 'photo-slider-top', unit: '%' },
      { id: 'y', min: -100, max: 100, value: 0, step: 1, orientation: 'vertical', icon: 'fa-up-down', label: 'Up/Down', positionClass: 'photo-slider-left', unit: '%' },
      { id: 'scale', min: 0.0, max: 4.0, value: 1.0, step: 0.01, orientation: 'vertical', icon: 'fa-up-down-left-right', label: 'Scale', positionClass: 'photo-slider-right' },
      { id: 'rotation', min: -180, max: 180, value: 0, step: 1, orientation: 'horizontal', icon: 'fa-rotate', label: 'Rotation', positionClass: 'photo-slider-bottom' },
    ];

    sliderConfigs.forEach(config => {
      const sliderEl = this.elements.editor.querySelector(`#slider-${config.id}`);
      if (!sliderEl) return;

      const wrapper = this._createCustomSlider(config);
      sliderEl.parentNode.replaceChild(wrapper, sliderEl);
      this._customSliders[config.id] = wrapper;
    });
  }

  /**
   * Create a custom slider element with FloatingUI tooltip
   * @private
   */
  _createCustomSlider(config) {
    const wrapper = document.createElement('div');
    wrapper.className = `photo-custom-slider photo-custom-slider-${config.orientation} ${config.positionClass}`;
    wrapper.dataset.sliderId = config.id;

    const track = document.createElement('div');
    track.className = 'photo-slider-track';

    const thumb = document.createElement('div');
    thumb.className = 'photo-slider-thumb';
    thumb.innerHTML = `<fa ${config.icon}></fa>`;

    // Create FloatingUI tooltip
    const tooltip = new Tooltip(thumb, this._formatSliderValue(config.id, config.value), {
      placement: config.orientation === 'horizontal' ? 'top' : 'right',
      theme: 'default',
      trigger: 'manual',
    }).init();

    // Append track and thumb as siblings (not nested) so track opacity doesn't affect thumb
    wrapper.appendChild(track);
    wrapper.appendChild(thumb);

    // Store config and references
    wrapper._config = config;
    wrapper._thumb = thumb;
    wrapper._tooltip = tooltip;
    wrapper._value = config.value;

    // Set initial position
    this._updateSliderPosition(wrapper, config.value);

    // Add interaction handlers
    this._bindCustomSliderEvents(wrapper);

    return wrapper;
  }

  /**
   * Format slider value for tooltip display
   * @private
   */
  _formatSliderValue(id, value) {
    switch (id) {
      case 'x': return `Horizontal: ${Math.round(value)}%`;
      case 'y': return `Vertical: ${Math.round(value)}%`;
      case 'scale': return `Scale: ${Math.round(value * 100)}%`;
      case 'rotation': return `Rotate: ${value}°`;
      default: return value;
    }
  }

  /**
   * Update custom slider thumb position and tooltip
   * @private
   */
  _updateSliderPosition(sliderWrapper, value) {
    const config = sliderWrapper._config;
    const thumb = sliderWrapper._thumb;
    const tooltip = sliderWrapper._tooltip;

    let percentage;
    if (config.id === 'scale') {
      // Custom thumb positioning for scale slider
      if (value <= 1.0) {
        // 0-1.0 → 0-50% (top half of vertical slider)
        percentage = (value / 1.0) * 0.5 * 100;
      } else {
        // 1.0-4.0 → 50-100% (bottom half)
        percentage = (0.5 + (value - 1.0) / 3.0 * 0.5) * 100;
      }
    } else {
      percentage = (value - config.min) / (config.max - config.min) * 100;
    }

    if (config.orientation === 'horizontal') {
      thumb.style.left = `${percentage}%`;
    } else {
      thumb.style.top = `${percentage}%`;
    }

    sliderWrapper._value = value;
    tooltip?.updateContent(this._formatSliderValue(config.id, value));
  }

  /**
   * Bind events to custom slider
   * @private
   */
  _bindCustomSliderEvents(wrapper) {
    const config = wrapper._config;
    const tooltip = wrapper._tooltip;
    let isDragging = false;

    const updateValue = (clientX, clientY) => {
      const rect = wrapper.getBoundingClientRect();
      let percentage;

      if (config.orientation === 'horizontal') {
        percentage = (clientX - rect.left) / rect.width;
      } else {
        percentage = (clientY - rect.top) / rect.height;
      }

      percentage = Math.max(0, Math.min(1, percentage));

      let val;
      if (config.id === 'scale') {
        // Custom mapping: 100% (1.0) at 50% slider position (middle)
        if (percentage <= 0.5) {
          // Top half (0-50%): 0-100% (0-1.0), 1% increments
          val = (percentage / 0.5) * 1.0;
          const step = 0.01;
          val = Math.round(val / step) * step;
          val = Math.max(0, Math.min(1.0, val));
        } else {
          // Bottom half (50-100%): 100-400% (1.0-4.0), 3% increments
          val = 1.0 + ((percentage - 0.5) / 0.5) * 3.0;
          const step = 0.03;
          val = Math.round(val / step) * step;
          val = Math.max(1.0, Math.min(4.0, val));
        }
      } else {
        // Default linear mapping for other sliders
        val = config.min + percentage * (config.max - config.min);
        val = Math.round(val / config.step) * config.step;
      }

      this._updateSliderPosition(wrapper, val);
      this._onSliderChange(config.id, val);
    };

    wrapper.addEventListener('mousedown', (e) => {
      isDragging = true;
      updateValue(e.clientX, e.clientY);
      tooltip?.show();
      e.preventDefault();
    });

    document.addEventListener('mousemove', (e) => {
      if (!isDragging) return;
      updateValue(e.clientX, e.clientY);
    });

    document.addEventListener('mouseup', () => {
      if (isDragging) {
        isDragging = false;
        tooltip?.hide();
      }
    });

    // Show tooltip on hover
    wrapper.addEventListener('mouseenter', () => {
      if (!isDragging) tooltip?.show();
    });
    wrapper.addEventListener('mouseleave', () => {
      if (!isDragging) tooltip?.hide();
    });
  }

  /**
   * Handle custom slider value change
   * @private
   */
  _onSliderChange(id, value) {
    switch (id) {
      case 'x':
        this.xPct = value;
        break;
case 'y':
         this.yPct = value;
         break;
      case 'scale':
        this.scale = value;
        break;
      case 'rotation':
        this.rotation = value;
        break;
    }
    this._updateImageTransform();
  }

  /**
   * Debounced save to avoid excessive writes
   * @private
   */
  _invokeSaveDebounced() {
    if (this._saveTimeout) {
      clearTimeout(this._saveTimeout);
    }
    this._saveTimeout = setTimeout(() => {
      this._handleSave();
    }, 1000);
  }

  /**
   * Handle download button click - download original image
   * @param {Event} e - Click event
   * @private
   */
  _handleDownload(e) {
    e.preventDefault();
    if (!this.originalPhotoData) {
      // this._showStatus('No original image to download', 'error');
      return;
    }

    // Extract MIME type and extension from data URL
    const mimeMatch = this.originalPhotoData.match(/^data:(image\/\w+);base64,/);
    const mimeType = mimeMatch ? mimeMatch[1] : 'image/jpeg';
    const extension = mimeType.split('/')[1] || 'jpg';
    const filename = `Profile Photo.${extension}`;

    // Trigger download
    const a = document.createElement('a');
    a.href = this.originalPhotoData;
    a.download = filename;
    document.body.appendChild(a);
    a.click();
    document.body.removeChild(a);

    // this._showStatus('Download started', 'success');
  }

  /**
   * Bind event listeners
   * @private
   */
  _bindEvents() {
    // Upload handler
    this.elements.uploadInput.addEventListener('change', (e) => this._handleUpload(e));

    // Capture handler
    this.elements.captureBtn.addEventListener('click', () => this._handleCapture());

    // Remove handler
    this.elements.removeBtn.addEventListener('click', () => this._handleRemove());

    // Download handler
    this.elements.downloadBtn.addEventListener('click', (e) => this._handleDownload(e));

    // Reset handler
    this.elements.resetBtn.addEventListener('click', () => this._handleReset());

    // Transform handlers
    this.elements.flipHBtn.addEventListener('click', () => this._handleFlipHorizontal());
    this.elements.flipVBtn.addEventListener('click', () => this._handleFlipVertical());
    this.elements.rotateLeftBtn.addEventListener('click', () => this._handleRotateLeft());
    this.elements.rotateRightBtn.addEventListener('click', () => this._handleRotateRight());

    // Image drag handler
    this._setupImageDrag();

    // Scroll wheel zoom
    this._setupScrollZoom();

    // Resize handler (corner only, anchored to top-left)
    this._setupResize();
  }

  /**
   * Setup image drag to reposition
   * @private
   */
  _setupImageDrag() {
    const image = this.elements.image;
    let isDragging = false;
    let startX, startY, startXPct, startYPct;

    image.addEventListener('mousedown', (e) => {
      if (!this.photoData) return;
      isDragging = true;
      startX = e.clientX;
      startY = e.clientY;
      startXPct = this.xPct;
      startYPct = this.yPct;
      e.preventDefault();
    });

    document.addEventListener('mousemove', (e) => {
      if (!isDragging) return;

      const dx = e.clientX - startX;
      const dy = e.clientY - startY;

      const editorRect = this.elements.editor.getBoundingClientRect();
      const previewSize = editorRect.width * 0.6667; // Reference size for percentages

      // Convert pixel movement to percentage of preview size (independent of scale/rotation)
      const dxPct = (dx / previewSize) * 100;
      const dyPct = (dy / previewSize) * 100;

      this.xPct = startXPct + dxPct;
      this.yPct = startYPct + dyPct;

// Update sliders
       this._updateSliderPosition(this._customSliders.x, this.xPct);
       this._updateSliderPosition(this._customSliders.y, this.yPct);

      this._updateImageTransform();
    });

    document.addEventListener('mouseup', () => {
      isDragging = false;
    });
  }

  /**
   * Handle photo upload
   * @param {Event} e - Input change event
   * @private
   */
  async _handleUpload(e) {
    const file = e.target.files[0];
    if (!file) return;

    // Validate file type
    if (!file.type.startsWith('image/')) {
      // this._showStatus('Please select an image file', 'error');
      return;
    }

    // Validate file size (10MB max)
    if (file.size > 10 * 1024 * 1024) {
      // this._showStatus('File size exceeds 10MB limit', 'error');
      return;
    }

    this.originalFile = file;
    await this._loadImageFromFile(file);
    this._updateButtonStates();
    this._resetToDefaultAndScaleImage();
    await this._handleSave();
  }

  /**
   * Initialize camera popup
   * @private
   */
  _initCameraPopup() {
    this.cameraPopup = new CameraPopup({
      onCapture: (file) => this._handleCameraCapture(file)
    });
  }

  /**
   * Handle camera capture callback from popup
   * @param {File} file - Captured image file
   * @private
   */
  async _handleCameraCapture(file) {
    this.originalFile = file;
    await this._loadImageFromFile(file);
    this._updateButtonStates();
    this._resetToDefaultAndScaleImage();
    await this._handleSave();
  }

  /**
   * Handle photo capture from camera (shows popup)
   * @private
   */
  async _handleCapture() {
    if (this.cameraPopup) {
      this.cameraPopup.show();
    }
  }

  /**
   * Load image from file
   * @param {File} file - Image file
   * @private
   */
  async _loadImageFromFile(file) {
    return new Promise((resolve, _reject) => {
      const reader = new window.FileReader();
      reader.onload = (e) => {
      this.photoData = e.target.result;
         this.originalPhotoData = this.photoData; // Store original
         this.elements.image.src = this.photoData;
         this.elements.image.onload = () => {
           this.originalWidth = this.elements.image.naturalWidth;
           this.originalHeight = this.elements.image.naturalHeight;
           this._updateImageTransform();
           resolve();
         };
      };
      reader.onerror = _reject;
      reader.readAsDataURL(file);
    });
  }



  /**
   * Update image transform based on slider values and editor scale
   * Uses left/top to center natural image, sliders control translate() in transform
   * @private
   */
  _updateImageTransform() {
    if (!this.photoData) return;

    const naturalW = this.originalWidth || this.elements.image.naturalWidth;
    const naturalH = this.originalHeight || this.elements.image.naturalHeight;
    if (!naturalW || !naturalH) return;

    const editorRect = this.elements.editor.getBoundingClientRect();
    const centerX = editorRect.width / 2;
    const centerY = editorRect.height / 2;

    // Set left/top to center the natural image size
    const left = centerX - naturalW / 2;
    const top = centerY - naturalH / 2;

    this.elements.image.style.left = `${left}px`;
    this.elements.image.style.top = `${top}px`;

    // Use preview overlay size as reference for translate offsets (independent of scale/rotation)
    const previewSize = editorRect.width * 0.6667; // 400px in 600px editor
    const tx = (previewSize * this.xPct) / 100;
    const ty = (previewSize * this.yPct) / 100;

    const editorScale = editorRect.width / 600;
    const s = this.scale * editorScale;

    // Transform: translate (user control) then scale/rotate
    this.elements.image.style.transform = `translate(${tx}px, ${ty}px) scale(${s}) rotate(${this.rotation}deg)`;

    // Trigger debounced auto-save
    this._invokeSaveDebounced();
  }

  /**
   * Reset sliders to default and scale image to cover preview overlay
   * @private
   */
  _resetToDefaultAndScaleImage() {
    // Reset transform values
    this.xPct = 0;
    this.yPct = 0;
    this.rotation = 0;

    const img = this.elements.image;
    const naturalW = this.originalWidth || img.naturalWidth;
    const naturalH = this.originalHeight || img.naturalHeight;
    if (!naturalW || !naturalH) return;

    // Preview overlay is 66.67% of editor width (square)
    const editorRect = this.elements.editor.getBoundingClientRect();
    const previewSize = editorRect.width * 0.6667;

    const imgAspect = naturalW / naturalH;

    // Cover preview: match height if landscape, match width if portrait
    if (imgAspect >= 1) {
      this.scale = previewSize / naturalH;
    } else {
      this.scale = previewSize / naturalW;
    }

    // Store the initial scale for reset functionality
    this.initialScale = this.scale;

// Update sliders
     this._updateSliderPosition(this._customSliders.x, this.xPct);
     this._updateSliderPosition(this._customSliders.y, this.yPct);
     this._updateSliderPosition(this._customSliders.scale, this.scale);
     this._updateSliderPosition(this._customSliders.rotation, this.rotation);

     this._updateImageTransform();
   }

  /**
   * Handle photo removal
   * @private
   */
  _handleRemove() {
    this.photoData = null;
    this.originalPhotoData = null;
    this.initialScale = 1;
    this.xPct = 0;
    this.yPct = 0;
    this.scale = 1;
    this.rotation = 0;
    this.originalFile = null;

    this.elements.image.src = '';
    this.elements.preview40.src = '';
    this.elements.preview80.src = '';
    this.elements.preview150.src = '';
    this._updateSliderPosition(this._customSliders.x, 0);
    this._updateSliderPosition(this._customSliders.y, 0);
    this._updateSliderPosition(this._customSliders.scale, 1);
    this._updateSliderPosition(this._customSliders.rotation, 0);

    this._updateImageTransform();
    this._updateButtonStates();
    // Remove the photo section from settings
    this.settings.removeSection(this.sectionKey);
  }

  /**
   * Handle save photo
   * @private
   */
  async _handleSave() {
    if (!this.photoData) {
      // this._showStatus('No photo to save', 'error');
      return;
    }

    try {
      // this._showStatus('Processing photo...', 'info');

      // Capture 200x200 PNG
      const base64 = await this._capturePhoto();

      // Save to settings with all data including original dimensions and slider positions
      const photoData = {
        _name: 'Photo',
        photo: base64,
        original: this.originalPhotoData ? this.originalPhotoData.split(',')[1] : null,
        originalWidth: this.originalWidth,
        originalHeight: this.originalHeight,
        timestamp: new Date().toISOString(),
        xPct: this.xPct,
        yPct: this.yPct,
        scale: this.scale,
        rotation: this.rotation,
      };

      // Save entire section to settings service
      this.settings.setSection(this.sectionKey, null, photoData);

      this._updateButtonStates();
      this._updatePreviews();

    } catch (error) {
      log(Subsystems.MANAGER, Status.ERROR, '[PhotoPage] Save failed:', error.message);
      // this._showStatus('Failed to save photo', 'error');
    }
  }

/**
     * Capture photo as 200x200 PNG base64
     * @returns {Promise<string>} Base64 encoded PNG (without data: prefix)
     * @private
     */
   async _capturePhoto() {
     return new Promise((resolve) => {
       const canvas = document.createElement('canvas');
       canvas.width = 200;
       canvas.height = 200;
       const ctx = canvas.getContext('2d');

       const editor = this.elements.editor;

       const editorRect = editor.getBoundingClientRect();
       const naturalW = this.originalWidth || this.elements.image.naturalWidth;
       const naturalH = this.originalHeight || this.elements.image.naturalHeight;
       if (!naturalW || !naturalH) { resolve(''); return; }

       const fullWidth = editorRect.width;
       const centerX = fullWidth / 2;
       const centerY = fullWidth / 2;

       // Use preview overlay size as reference for translate offsets (66.67% of full width)
       const previewSize = fullWidth * 0.6667;
       const tx = (previewSize * this.xPct) / 100;
       const ty = (previewSize * this.yPct) / 100;
       const editorScale = fullWidth / 600;
       const s = this.scale * editorScale;

       // Create temp canvas sized to editor (including border area for exact match)
       const tempCanvas = document.createElement('canvas');
       tempCanvas.width = fullWidth;
       tempCanvas.height = fullWidth;
       const tempCtx = tempCanvas.getContext('2d');

       // CSS: transform: translate(tx, ty) scale(s) rotate(rad) with transform-origin: center
       // The image's natural center starts at (centerX, centerY) due to left/top positioning.
       // In CSS, the transform moves this center to (centerX + tx, centerY + ty), then
       // scale and rotate happen around that center point.
       //
       // In canvas, to get the same result:
       // 1. Move to the final center position: (centerX + tx, centerY + ty)
       // 2. Apply rotation (around the center)
       // 3. Apply scale (around the center)
       // 4. Draw image centered at origin

       tempCtx.translate(centerX + tx, centerY + ty);
       tempCtx.rotate(this.rotation * Math.PI / 180);
       tempCtx.scale(s, s);
       tempCtx.drawImage(
         this.elements.image,
         -naturalW / 2, -naturalH / 2, naturalW, naturalH
       );

       // Crop to preview overlay area (inset 16.6% from edges)
       const cropInset = fullWidth * 0.166;
       const cropSize = fullWidth * 0.6667;

       ctx.drawImage(
         tempCanvas,
         cropInset, cropInset, cropSize, cropSize,
         0, 0, 200, 200
       );

       resolve(canvas.toDataURL('image/png').split(',')[1]);
     });
   }

  /**
   * Load existing photo from settings
   * @private
   */
  async _loadExistingPhoto() {
    const section = this.getSectionData();
    // section IS the photoData object (not nested under photoData)
    if (section?.photo) {
      // Load saved photo
      this.photoData = `data:image/png;base64,${section.photo}`;
      this.originalPhotoData = section.original
        ? `data:image/png;base64,${section.original}`
        : this.photoData;

      // Restore slider positions
      this.scale = section.scale || 1;
      this.rotation = section.rotation || 0;

      // Restore percentages (migrate from old pixel values if needed)
      if (section.xPct !== undefined && section.yPct !== undefined) {
        this.xPct = section.xPct;
        this.yPct = section.yPct;
      } else {
        // Old format: x/y in pixels — convert to percentages relative to preview size
        const editorRect = this.elements.editor.getBoundingClientRect();
        const previewSize = editorRect.width * 0.6667;
        this.xPct = previewSize ? ((section.x || 0) / previewSize) * 100 : 0;
        this.yPct = previewSize ? ((section.y || 0) / previewSize) * 100 : 0;
      }

      // Restore original dimensions
      this.originalWidth = section.originalWidth || null;
      this.originalHeight = section.originalHeight || null;

// Update sliders (y inverted for display)
       this._updateSliderPosition(this._customSliders.x, this.xPct);
       this._updateSliderPosition(this._customSliders.y, this.yPct);
       this._updateSliderPosition(this._customSliders.scale, this.scale);
       this._updateSliderPosition(this._customSliders.rotation, this.rotation);

      this.elements.image.src = this.originalPhotoData;
        this.elements.image.onload = () => {
          // Fall back to image natural dimensions if not stored
          if (!this.originalWidth || !this.originalHeight) {
            this.originalWidth = this.elements.image.naturalWidth;
            this.originalHeight = this.elements.image.naturalHeight;
          }
          this._updateImageTransform();
          this._updateButtonStates();
          this._updatePreviews();
        };
    }
  }

  /**
   * Handle flip horizontal button click
   * @private
   */
  async _handleFlipHorizontal() {
    if (!this.photoData) return;
    await this._transformImage({ hFlip: true });
  }

  /**
   * Handle flip vertical button click
   * @private
   */
  async _handleFlipVertical() {
    if (!this.photoData) return;
    await this._transformImage({ vFlip: true });
  }

  /**
   * Handle rotate left button click
   * @private
   */
  async _handleRotateLeft() {
    if (!this.photoData) return;
    await this._transformImage({ rotation: -90 });
  }

  /**
   * Handle rotate right button click
   * @private
   */
  async _handleRotateRight() {
    if (!this.photoData) return;
    await this._transformImage({ rotation: 90 });
  }

  /**
   * Transform image data using Canvas API and update the display
   * @param {Object} transform - Transform options { hFlip, vFlip, rotation }
   * @private
   */
  async _transformImage(transform) {
    if (!this.photoData) return;

    try {
      // Create image element to get dimensions
      const img = new Image();
      img.src = this.photoData;

      await new Promise((resolve, reject) => {
        img.onload = resolve;
        img.onerror = reject;
      });

      const w = img.width;
      const h = img.height;

      // Determine canvas size (swap for 90°/270° rotation)
      let cw = w;
      let ch = h;
      if (transform.rotation && Math.abs(transform.rotation) % 180 !== 0) {
        cw = h;
        ch = w;
      }

      // Create canvas and apply transforms
      const canvas = document.createElement('canvas');
      canvas.width = cw;
      canvas.height = ch;

      const ctx = canvas.getContext('2d', { alpha: false });
      ctx.save();

      // Move origin to center
      ctx.translate(cw / 2, ch / 2);

      // Apply rotation
      if (transform.rotation) {
        ctx.rotate(transform.rotation * Math.PI / 180);
      }

      // Apply flips
      const hScale = transform.hFlip ? -1 : 1;
      const vScale = transform.vFlip ? -1 : 1;
      ctx.scale(hScale, vScale);

      // Draw the image centered
      ctx.drawImage(img, -w / 2, -h / 2, w, h);

      ctx.restore();

      // Convert to blob and create data URL
      const blob = await new Promise(resolve => {
        canvas.toBlob(resolve, 'image/jpeg', 0.92);
      });

      if (!blob || blob.size === 0) {
        throw new Error('Failed to transform image');
      }

      // Create new data URL
      const reader = new FileReader();
      const dataUrl = await new Promise((resolve, reject) => {
        reader.onload = () => resolve(reader.result);
        reader.onerror = reject;
        reader.readAsDataURL(blob);
      });

      // Update the image data and display
      this.photoData = dataUrl;
      this.elements.image.src = this.photoData;

      // Update original photo data with transformed version
      this.originalPhotoData = dataUrl;

      // Update dimensions if rotated
      if (transform.rotation && Math.abs(transform.rotation) % 180 !== 0) {
        [this.originalWidth, this.originalHeight] = [this.originalHeight, this.originalWidth];
      }

      // Reload image following same rules as new upload
      // This will recalculate initialScale and reset positioning
      this._resetToDefaultAndScaleImage();

      // Update sliders and transform
      this._updateSliderPosition(this._customSliders.x, this.xPct);
      this._updateSliderPosition(this._customSliders.y, this.yPct);
      this._updateSliderPosition(this._customSliders.scale, this.scale);
      this._updateSliderPosition(this._customSliders.rotation, this.rotation);
      this._updateImageTransform();

      // Save changes first (updates the saved 200x200 photo)
      await this._handleSave();

      // Update previews (now shows the newly saved photo)
      this._updatePreviews();

      log(Subsystems.MANAGER, Status.INFO, '[PhotoPage] Image transformed successfully');

    } catch (error) {
      log(Subsystems.MANAGER, Status.ERROR, '[PhotoPage] Image transformation failed:', error.message);
      // this._showStatus('Image transformation failed', 'error');
    }
  }

  /**
   * Update preview images with the saved 200px photo
   * @private
   */
  _updateButtonStates() {
    const hasPhoto = !!this.photoData;
    this.elements.removeBtn.disabled = !hasPhoto;
    this.elements.flipHBtn.disabled = !hasPhoto;
    this.elements.flipVBtn.disabled = !hasPhoto;
    this.elements.rotateLeftBtn.disabled = !hasPhoto;
    this.elements.rotateRightBtn.disabled = !hasPhoto;

    // Download button state
    if (this.originalPhotoData) {
      this.elements.downloadBtn.classList.remove('disabled');
      this.elements.downloadBtn.style.pointerEvents = 'auto';
    } else {
      this.elements.downloadBtn.classList.add('disabled');
      this.elements.downloadBtn.style.pointerEvents = 'none';
    }
  }

  /**
   * Sync toolbar and previews width with editor width
   * @private
   */
  _syncToolbarWidth() {
    const editorWidth = this.elements.editor.getBoundingClientRect().width;
    this.elements.toolbar.style.width = `${editorWidth}px`;
    this.elements.previews.style.width = `${editorWidth}px`;
  }

  /**
   * Setup scroll wheel zoom functionality
   * @private
   */
  _setupScrollZoom() {
    const editor = this.elements.editor;

    editor.addEventListener('wheel', (e) => {
      if (!this.photoData) return;

      e.preventDefault();

      // Zoom in on scroll up, out on scroll down
      const zoomFactor = 0.05; // 5% per scroll step
      const delta = e.deltaY > 0 ? -zoomFactor : zoomFactor;

      // Update scale within bounds
      this.scale = Math.max(0.1, Math.min(4.0, this.scale * (1 + delta)));

      // Update slider and image
      this._updateSliderPosition(this._customSliders.scale, this.scale);
      this._updateImageTransform();
    });
  }

  /**
   * Setup editor resize functionality (anchored to top-left, maintains aspect ratio)
   * Image scales proportionally with the editor
   * @private
   */
  _setupResize() {
    const resizeHandle = this.elements.resizeHandle;
    const editor = this.elements.editor;
    let isResizing = false;
    let startX, startWidth;

    resizeHandle.addEventListener('mousedown', (e) => {
      isResizing = true;
      startX = e.clientX;
      startWidth = parseInt(getComputedStyle(editor).width, 10);
      e.preventDefault();
    });

    document.addEventListener('mousemove', (e) => {
      if (!isResizing) return;

      const dx = e.clientX - startX;

      // Maintain aspect ratio (1:1) when resizing
      const newWidth = Math.max(300, Math.min(800, startWidth + dx));
      const newHeight = newWidth; // Keep 1:1 aspect ratio

      editor.style.width = `${newWidth}px`;
      editor.style.height = `${newHeight}px`;

      // xPct/yPct unchanged; recompute transform with new editor size
      this._updateImageTransform();
      this._syncToolbarWidth();
    });

    document.addEventListener('mouseup', () => {
      isResizing = false;
    });
  }

  /**
   * Handle reset button click - animate sliders to initial position
   * @private
   */
  async _handleReset() {
    const initial = { xPct:0, yPct:0, scale:this.initialScale, rotation:0 };
    const duration = 300; // ms
    const steps = 10;
    const interval = duration / steps;
    let step = 0;

    const intervalId = setInterval(() => {
      step++;
      const progress = step / steps;
      // Ease-in-out quadratic
      const ease = progress < 0.5 ? 2 * progress * progress : 1 - Math.pow(-2 * progress + 2, 2) / 2;

      this.xPct = this.xPct + (initial.xPct - this.xPct) * ease;
      this.yPct = this.yPct + (initial.yPct - this.yPct) * ease;
      this.scale = this.scale + (initial.scale - this.scale) * ease;
      this.rotation = this.rotation + (initial.rotation - this.rotation) * ease;

this._updateSliderPosition(this._customSliders.x, this.xPct);
       this._updateSliderPosition(this._customSliders.y, this.yPct);
       this._updateSliderPosition(this._customSliders.scale, this.scale);
       this._updateSliderPosition(this._customSliders.rotation, this.rotation);
       this._updateImageTransform();

       if (step >= steps) {
         clearInterval(intervalId);
         // Ensure final values are exact
         this.xPct = initial.xPct;
         this.yPct = initial.yPct;
         this.scale = initial.scale;
         this.rotation = initial.rotation;
         this._updateSliderPosition(this._customSliders.x, this.xPct);
         this._updateSliderPosition(this._customSliders.y, this.yPct);
         this._updateSliderPosition(this._customSliders.scale, this.scale);
         this._updateSliderPosition(this._customSliders.rotation, this.rotation);
         this._updateImageTransform();
       }
     }, interval);
   }

  /**
   * Update preview images with the saved 200px photo
   * @private
   */
  _updatePreviews() {
    const photoData = this.getSectionData();
    if (!photoData?.photo) {
      this.elements.preview40.src = '';
      this.elements.preview80.src = '';
      this.elements.preview150.src = '';
      return;
    }
    const src = `data:image/png;base64,${photoData.photo}`;
    this.elements.preview40.src = src;
    this.elements.preview80.src = src;
    this.elements.preview150.src = src;
  }

  /**
   * Show status message
   * @param {string} message - Status message
   * @param {string} type - Message type (success, error, info)
   * @private
   */
  _showStatus(message, type = 'info') {
    const statusEl = this.elements.status;
    statusEl.textContent = message;
    statusEl.className = 'photo-status';
    if (type) statusEl.classList.add(type);

    if (type !== 'info') {
      setTimeout(() => {
        statusEl.textContent = '';
        statusEl.className = 'photo-status';
      }, 3000);
    }
  }

  /**
   * Save method for settings page
   * @returns {Promise<Object>}
   */
  async save() {
    await this._handleSave();
    return { success: true };
  }

  /**
   * Cancel changes
   */
  cancel() {
    this._loadExistingPhoto();
    this.setDirty(false);
  }

  /**
   * Cleanup on destroy
   */
  destroy() {
    super.destroy();
    // Clear pending save timeout
    if (this._saveTimeout) {
      clearTimeout(this._saveTimeout);
      this._saveTimeout = null;
    }
    // Destroy FloatingUI tooltips
    Object.values(this._customSliders).forEach(slider => {
      slider._tooltip?.destroy();
    });
    // Remove download listener
    if (this.elements.downloadBtn) {
      this.elements.downloadBtn.removeEventListener('click', this._handleDownload);
    }
    this.photoData = null;
    this.originalPhotoData = null;
    this.elements = {};
    this._customSliders = {};
  }
}

/**
 * Profile Manager - Concierge Page Handler
 *
 * Handles the Concierge settings page (index: -13)
 */


class ConciergePage extends SimpleSettingsPage {
  constructor(options = {}) {
    super({ ...options, index: -13, formSelector: 'form' });
  }
  async onInit() { await super.onInit(); log(Subsystems.MANAGER, Status.DEBUG, '[ConciergePage] Initialized (stub)'); }
}

/**
 * Profile Manager - Placeholder Page Handler
 *
 * Displays a "settings not implemented" message for managers
 * that don't have specific settings pages yet.
 */


/**
 * Placeholder Page Handler
 * Used when a manager doesn't have specific settings implemented
 */
class PlaceholderPage extends BaseSettingsPage {
  constructor(options = {}) {
    super(options);
    this._managerName = options.managerName || 'This Manager';
  }

  /**
   * Called when the page is initialized
   */
  async onInit() {
    this._renderPlaceholder();
    log(Subsystems.MANAGER, Status.DEBUG, `[PlaceholderPage] Initialized for ${this._managerName}`);
  }

  /**
   * Render the placeholder message
   */
  _renderPlaceholder() {
    if (!this.container) return;

    // Find or create the settings page element
    let pageElement = this.container.querySelector(`.settings-page[data-page-index="${this.index}"]`);
    if (!pageElement) {
      pageElement = document.createElement('div');
      pageElement.className = 'settings-page';
      pageElement.setAttribute('data-page-index', this.index);
      pageElement.id = `settings-page-${this.index}`;
      this.container.appendChild(pageElement);
    }

    // Find or create the section
    let section = pageElement.querySelector('.profile-section');
    if (!section) {
      section = document.createElement('section');
      section.className = 'profile-section';
      pageElement.appendChild(section);
    }

    // Find or create the placeholder content
    let placeholderEl = section.querySelector('.settings-placeholder');
    if (!placeholderEl) {
      section.innerHTML = `
        <h3 class="section-title"><fa fa-wrench></fa> ${this._managerName} Settings</h3>
        <div class="settings-placeholder">
          <div class="placeholder-icon">
            <fa fa-wrench></fa>
          </div>
          <h4 class="placeholder-title">Settings Not Available</h4>
          <p class="placeholder-message">
            No settings are currently available for ${this._managerName}.
          </p>
          <p class="placeholder-hint">
            This feature will be implemented in a future update.
          </p>
        </div>
      `;
    }
  }

  /**
   * Always returns false - placeholder has no changes
   */
  isDirty() {
    return false;
  }

  /**
   * Nothing to save
   */
  async save() {
    return { success: true, message: 'No settings to save' };
  }
}

/**
 * Profile Manager - Training Page Handler
 *
 * Handles the Training settings page (index: -16)
 */


class TrainingPage extends SimpleSettingsPage {
  constructor(options = {}) {
    super({ ...options, index: -16, formSelector: 'form' });
  }
  async onInit() { await super.onInit(); log(Subsystems.MANAGER, Status.DEBUG, '[TrainingPage] Initialized (stub)'); }
}

/**
 * Profile Manager - Login History Page Handler
 *
 * Handles the Login History settings page (index: -17)
 */


class LoginHistoryPage extends BaseSettingsPage {
  constructor(options = {}) {
    super({ ...options, index: -17 });
  }
  async onInit() {
    log(Subsystems.MANAGER, Status.DEBUG, '[LoginHistoryPage] Initialized (stub)');
  }
}

/**
 * Profile Manager - Lookups Manager Settings Page
 *
 * Settings for the Lookups Manager (manager ID: 23)
 * Allows users to configure lookup display preferences,
 * default views, and caching options.
 *
 * Uses the ProfileSettingsService for all persistence.
 * Section key: "23"
 * JSON structure:
 *   {
 *     "_name": "Lookups Manager",
 *     "defaultView": "list",
 *     "pageSize": 50,
 *     "autoRefresh": true,
 *     "refreshInterval": 60,
 *     "showInactive": false,
 *     "cacheLookups": true
 *   }
 */


/**
 * Lookups Manager Settings Page
 */
class LookupsManagerPage extends SimpleSettingsPage {
  constructor(options = {}) {
    super({
      ...options,
      index: 23,
      managerId: 23,
      formSelector: 'form',
    });

    this._defaultValues = {
      defaultView: 'list',
      pageSize: 50,
      autoRefresh: true,
      refreshInterval: 60,
      showInactive: false,
      cacheLookups: true,
    };
  }

  /**
   * Called when the page is initialized
   */
  async onInit() {
    await super.onInit();
    await this.loadData();
    log(Subsystems.MANAGER, Status.DEBUG, '[LookupsManagerPage] Initialized');
  }

  /**
   * Load settings from the Settings Service.
   * Falls back to legacy localStorage for migration.
   */
  async loadData() {
    const section = this.getSectionData();
    const hasData = Object.keys(section).length > 1; // >1 because _name may be present

    let data;
    if (hasData) {
      // Load from Settings Service
      data = { ...this._defaultValues };
      for (const key of Object.keys(this._defaultValues)) {
        if (section[key] !== undefined) {
          data[key] = section[key];
        }
      }
    } else {
      // Fallback: migrate from legacy localStorage
      const stored = localStorage.getItem('lithium_lookups_settings');
      data = { ...this._defaultValues };
      if (stored) {
        try {
          const parsed = JSON.parse(stored);
          data = { ...this._defaultValues, ...parsed };
        } catch (error) {
          log(Subsystems.MANAGER, Status.WARN, '[LookupsManagerPage] Failed to parse legacy settings:', error);
        }
      }
    }

    this.setFormData(data);
    this._originalData = JSON.parse(JSON.stringify(data));
    this.setDirty(false);
  }

  /**
   * Save settings via the Settings Service.
   */
  async save() {
    const data = this.getFormData(this.formSelector);

    // Write to Settings Service under section "23"
    this.setSectionData(data, 'Lookups Manager');

    // Also update legacy localStorage for backward compatibility
    this._updateLegacyStorage(data);

    this._originalData = JSON.parse(JSON.stringify(data));
    this.setDirty(false);

    log(Subsystems.MANAGER, Status.INFO, '[LookupsManagerPage] Settings saved:', data);
    return { success: true, data };
  }

  /**
   * Update legacy localStorage for backward compatibility.
   * @private
   */
  _updateLegacyStorage(data) {
    try {
      localStorage.setItem('lithium_lookups_settings', JSON.stringify(data));
    } catch (_e) {
      // ignore
    }
  }

  /**
   * Cancel changes and reload
   */
  cancel() {
    this.loadData();
    log(Subsystems.MANAGER, Status.DEBUG, '[LookupsManagerPage] Changes cancelled');
  }

  /**
   * Check if page is dirty
   */
  isDirty() {
    if (!this.container) return false;

    const currentData = this.getFormData(this.formSelector);
    return JSON.stringify(currentData) !== JSON.stringify(this._originalData);
  }
}

/**
 * Profile Manager - Tour Manager Page Handler
 *
 * Handles the Tour Manager settings page (index: 6)
 */


class TourManagerPage extends SimpleSettingsPage {
  constructor(options = {}) {
    super({ ...options, index: 6, formSelector: 'form' });
  }
  async onInit() { await super.onInit(); log(Subsystems.MANAGER, Status.DEBUG, '[TourManagerPage] Initialized'); }
}

/**
 * Profile Manager - Settings Page Registry
 *
 * Manages the registration and loading of settings page handlers.
 * Internal pages (negative indices) use the pattern: page-{name}.js
 * Manager pages (positive indices) use the pattern: manager-{id}.js
 */


/**
 * Map of internal page indices to their handler classes
 * Negative indices for internal Lithium pages
 */
const INTERNAL_PAGE_MAP = {
  '-1': AccountPage,
  '-2': NamesPage,
  '-3': AddressesPage,
  '-4': EmailPage,
  '-5': PhonePage,
  '-6': AuthenticationPage,
  '-7': TokensPage,
  '-8': LanguagePage,
  '-9': DateFormatsPage,
  '-10': NumberFormatsPage,
  '-11': StartupPage,
  '-12': PhotoPage,
  '-13': ConciergePage,
  '-14': PlaceholderPage,
  '-16': TrainingPage,
  '-17': LoginHistoryPage,
};

/**
 * Settings Page Registry
 * Manages loading and caching of page handlers
 */
class SettingsPageRegistry {
  constructor(options = {}) {
    this.app = options.app || null;
    this.settingsService = options.settingsService || null;
    this.onDirtyChange = options.onDirtyChange || (() => {});

    // Cache of loaded handlers: index -> handler instance
    this._handlers = new Map();

    // Track which pages are currently loading
    this._loading = new Set();
  }

  /**
   * Get or load a page handler for the given index
   * @param {number} index - Page index (negative for internal, positive for managers)
   * @param {HTMLElement} container - The page container element
   * @param {Object} pageData - Page data from the user options table
   * @returns {Promise<BaseSettingsPage|null>}
   */
  async getHandler(index, container, pageData) {
    log(Subsystems.MANAGER, Status.DEBUG, `[PageRegistry] getHandler called for index ${index}`);

    // Check cache first
    if (this._handlers.has(index)) {
      log(Subsystems.MANAGER, Status.DEBUG, `[PageRegistry] Handler found in cache for ${index}`);
      const handler = this._handlers.get(index);
      // Update container reference in case DOM was rebuilt
      handler.container = container;
      return handler;
    }

    // Prevent duplicate loading
    if (this._loading.has(index)) {
      log(Subsystems.MANAGER, Status.DEBUG, `[PageRegistry] Page ${index} already loading, waiting...`);
      await this._waitForLoad(index);
      return this._handlers.get(index) || null;
    }

    log(Subsystems.MANAGER, Status.DEBUG, `[PageRegistry] Starting load for ${index}`);
    this._loading.add(index);

    try {
      const handler = await this._loadHandler(index, container, pageData);
      if (handler) {
        this._handlers.set(index, handler);
        log(Subsystems.MANAGER, Status.DEBUG, `[PageRegistry] Handler cached for ${index}`);
      }
      return handler;
    } finally {
      this._loading.delete(index);
    }
  }

  /**
   * Load a page handler based on index type
   * @private
   */
  async _loadHandler(index, container, pageData) {
    log(Subsystems.MANAGER, Status.DEBUG, `[PageRegistry] _loadHandler called for index ${index}`);
    if (index < 0) {
      log(Subsystems.MANAGER, Status.DEBUG, `[PageRegistry] Dispatching to _loadInternalHandler for ${index}`);
      return this._loadInternalHandler(index, container, pageData);
    } else {
      log(Subsystems.MANAGER, Status.DEBUG, `[PageRegistry] Dispatching to _loadManagerHandler for ${index}`);
      return this._loadManagerHandler(index, container, pageData);
    }
  }

  /**
   * Load an internal page handler (negative indices)
   * @private
   */
  async _loadInternalHandler(index, container, pageData) {
    log(Subsystems.MANAGER, Status.DEBUG, `[PageRegistry] _loadInternalHandler called for index ${index}`);

    try {
      // Get the handler class from the map
      const HandlerClass = INTERNAL_PAGE_MAP[index];
      if (!HandlerClass) {
        log(Subsystems.MANAGER, Status.ERROR, `[PageRegistry] No handler class found for internal page ${index}`);
        return null;
      }

      // Check if page element already exists (might be embedded in main HTML)
      let pageElement = container.querySelector(`.settings-page[data-page-index="${index}"]`);

      if (!pageElement) {
        // Map index to page name for file paths
        const pageName = this._getPageNameFromIndex(index);
        if (!pageName) {
          log(Subsystems.MANAGER, Status.ERROR, `[PageRegistry] Could not map index ${index} to page name`);
          return null;
        }

        // Load HTML and CSS dynamically
        await this._loadPageAssets(pageName, container);

        // Find the page element in the container
        pageElement = container.querySelector(`.settings-page[data-page-index="${index}"]`);
        if (!pageElement) {
          log(Subsystems.MANAGER, Status.ERROR, `[PageRegistry] Page element not found after loading assets for ${index}`);
          return null;
        }
      } else {
        log(Subsystems.MANAGER, Status.DEBUG, `[PageRegistry] Page element already exists in DOM for ${index}`);
      }

      // Create and initialize the handler
      const handler = new HandlerClass({
        index: index,
        app: this.app,
        settings: this.settingsService,
        onDirtyChange: this.onDirtyChange,
      });

      await handler.init(pageElement, pageData);

      log(Subsystems.MANAGER, Status.DEBUG, `[PageRegistry] Loaded internal handler for ${index}`);
      return handler;
    } catch (error) {
      log(Subsystems.MANAGER, Status.ERROR, `[PageRegistry] Error loading internal page ${index}:`, error.message);
      return null;
    }
  }

  /**
   * Load a manager page handler
   * @private
   */
  async _loadManagerHandler(index, container, pageData) {
    // Get the manager name from page data or use a default
    const managerName = pageData?.label || `Manager ${index}`;

    try {
      // Use a switch to get the appropriate handler class
      const HandlerClass = this._getManagerHandlerClass(index);

      if (!HandlerClass) {
        // No specific handler - return a placeholder
        log(Subsystems.MANAGER, Status.DEBUG, `[PageRegistry] Using placeholder for manager ${index}`);
        const placeholder = new PlaceholderPage({
          index: index,
          managerId: index,
          managerName: managerName,
          app: this.app,
          settings: this.settingsService,
          onDirtyChange: this.onDirtyChange,
        });
        await placeholder.init(container, pageData);
        return placeholder;
      }

      // Load HTML for manager pages
      const pageName = this._getPageNameFromManagerIndex(index);
      if (pageName) {
        await this._loadPageAssets(pageName, container);
      }

      const handler = new HandlerClass({
        index: index,
        managerId: index,
        app: this.app,
        settings: this.settingsService,
        onDirtyChange: this.onDirtyChange,
      });

      await handler.init(container, pageData);

      log(Subsystems.MANAGER, Status.DEBUG, `[PageRegistry] Loaded manager handler for ${index}`);
      return handler;
    } catch (error) {
      log(Subsystems.MANAGER, Status.ERROR, `[PageRegistry] Error loading manager ${index}:`, error.message);
      // Return a placeholder on error
      const placeholder = new PlaceholderPage({
        index: index,
        managerId: index,
        managerName: managerName,
        app: this.app,
        settings: this.settingsService,
        onDirtyChange: this.onDirtyChange,
      });
      await placeholder.init(container, pageData);
      return placeholder;
    }
  }
  _getManagerHandlerClass(index) {
    // Map of manager IDs to their handler classes
    // Add new managers here as they are implemented
    switch (index) {
      case 6: // Tour Manager
        return TourManagerPage;
      case 23: // Lookups Manager
        return LookupsManagerPage;
      default:
        // Return null to use placeholder
        return null;
    }
  }

  /**
   * Map internal page index to page name for file paths
   * @private
   */
  _getPageNameFromIndex(index) {
    const indexToNameMap = {
      '-1': 'account',
      '-2': 'names',
      '-3': 'addresses',
      '-4': 'email',
      '-5': 'phone',
      '-6': 'authentication',
      '-7': 'tokens',
      '-8': 'language',
      '-9': 'date-formats',
      '-10': 'number-formats',
       '-11': 'startup',
       '-12': 'photo',
       '-13': 'concierge',
      '-14': 'annotations',
      '-16': 'training',
      '-17': 'login-history',
    };
    return indexToNameMap[index] || null;
  }

  /**
   * Derive page name from manager name for file paths
   * @private
   */
  _getPageNameFromManagerIndex(index) {
    // All manager pages use the pattern: manager-{id}
    // This ensures consistent numeric referencing as per LITHIUM-INS.md
    return 'manager-' + index;
  }

  /**
   * Load HTML and CSS assets for a page
   * @private
   */
  async _loadPageAssets(pageName, container) {
    try {
      // Load HTML - use absolute path that works in both dev and prod
      const htmlUrl = `/src/managers/profile-manager/pages/${pageName}/page-${pageName}.html`;
      const htmlResponse = await fetch(htmlUrl);
      if (!htmlResponse.ok) {
        throw new Error(`Failed to load HTML for ${pageName}: ${htmlResponse.status}`);
      }
      const htmlContent = await htmlResponse.text();

      // Parse and insert HTML
      const tempDiv = document.createElement('div');
      tempDiv.innerHTML = htmlContent;
      const pageElement = tempDiv.firstElementChild;
      if (pageElement) {
        container.appendChild(pageElement);
      }

      // Load CSS - use absolute path that works in both dev and prod
      const cssUrl = `/src/managers/profile-manager/pages/${pageName}/page-${pageName}.css`;
      const cssResponse = await fetch(cssUrl);
      if (cssResponse.ok) {
        const cssContent = await cssResponse.text();
        const styleElement = document.createElement('style');
        styleElement.textContent = cssContent;
        styleElement.setAttribute('data-page-css', pageName);
        document.head.appendChild(styleElement);
      } else {
        log(Subsystems.MANAGER, Status.DEBUG, `[PageRegistry] No CSS file found for ${pageName}, skipping`);
      }

      log(Subsystems.MANAGER, Status.DEBUG, `[PageRegistry] Loaded assets for page ${pageName}`);
    } catch (error) {
      log(Subsystems.MANAGER, Status.ERROR, `[PageRegistry] Error loading assets for ${pageName}:`, error.message);
      throw error;
    }
  }

  /**
   * Wait for a page to finish loading
   * @private
   */
  async _waitForLoad(index) {
    const startTime = Date.now();
    const timeout = 5000;

    while (this._loading.has(index)) {
      if (Date.now() - startTime > timeout) {
        log(Subsystems.MANAGER, Status.WARN, `[PageRegistry] Timeout waiting for page ${index}`);
        break;
      }
      await new Promise(resolve => setTimeout(resolve, 50));
    }
  }

  /**
   * Check if any loaded page is dirty
   * @returns {boolean}
   */
  isDirty() {
    for (const handler of this._handlers.values()) {
      if (handler?.isDirty?.()) {
        return true;
      }
    }
    return false;
  }

  /**
   * Save all dirty pages
   * @returns {Promise<Array>}
   */
  async saveAll() {
    const results = [];
    for (const [index, handler] of this._handlers.entries()) {
      if (handler?.isDirty?.() && handler?.save) {
        try {
          const result = await handler.save();
          results.push({ index, success: true, result });
        } catch (error) {
          results.push({ index, success: false, error: error.message });
        }
      }
    }
    return results;
  }

  /**
   * Cancel/reset all pages
   */
  cancelAll() {
    for (const handler of this._handlers.values()) {
      if (handler?.cancel) {
        handler.cancel();
      }
    }
  }

  /**
   * Get a specific handler if loaded
   * @param {number} index - Page index
   * @returns {BaseSettingsPage|null}
   */
  getLoadedHandler(index) {
    return this._handlers.get(index) || null;
  }

  /**
   * Check if a handler is loaded
   * @param {number} index - Page index
   * @returns {boolean}
   */
  hasHandler(index) {
    return this._handlers.has(index);
  }

  /**
   * Unload a specific handler
   * @param {number} index - Page index
   */
  unloadHandler(index) {
    const handler = this._handlers.get(index);
    if (handler?.destroy) {
      handler.destroy();
    }
    this._handlers.delete(index);
  }

  /**
   * Destroy all handlers
   */
  destroy() {
    for (const [index, handler] of this._handlers.entries()) {
      if (handler?.destroy) {
        try {
          handler.destroy();
        } catch (error) {
          log(Subsystems.MANAGER, Status.ERROR, `[PageRegistry] Error destroying handler ${index}:`, error);
        }
      }
    }
    this._handlers.clear();
    this._loading.clear();
  }
}

/**
 * Profile Manager - Settings Tab Handler
 *
 * Handles the settings pages with index-based navigation and dynamic page loading.
 * Internal sections use negative indices, manager sections use actual manager IDs.
 */


  /**
   * SettingsTabHandler class
   * Manages the settings pages and crossfade transitions
   */
  class SettingsTabHandler {
    constructor(options) {
      this.container = options.container;
      this.onPageChange = options.onPageChange || (() => {});
      this.onDirtyChange = options.onDirtyChange || (() => {});
      this.app = options.app || null;
      this.settingsService = options.settingsService || null;

      this.currentPageIndex = null;
      this._inPageTransition = false;
      this._transitionDuration = 350;

      // Page registry for loading handlers
      this._pageRegistry = null;

      // Queue for pending page requests during transitions
      this._pendingPageRequest = null;
    }

  /**
   * Initialize the settings tab handler
   */
  init() {
    // Get transition duration from CSS
    const duration = getComputedStyle(document.documentElement)
      .getPropertyValue('--transition-duration')
      .trim();
    this._transitionDuration = parseInt(duration, 10) || 350;

    // Initialize the page registry
    this._pageRegistry = new SettingsPageRegistry({
      app: this.app,
      settingsService: this.settingsService,
      onDirtyChange: this.onDirtyChange,
    });

    log(Subsystems.MANAGER, Status.INFO, '[SettingsTab] Handler initialized');
  }

  /**
   * Show a settings page by index with crossfade transition
   * @param {number} index - Page index (negative for internal, positive for managers)
   * @param {Object} pageData - Data about the page (label, icon, etc.)
   */
  async showPage(index, pageData = {}) {
    // Guard: same page requested
    if (index === this.currentPageIndex) return;

    // If a transition is in progress, queue this request and return
    if (this._inPageTransition) {
      this._pendingPageRequest = { index, pageData };
      return;
    }

    this._inPageTransition = true;

    let targetPage = this.container.querySelector(`.settings-page[data-page-index="${index}"]`);

    if (!targetPage) {
      // Page not loaded yet, try to load it via the registry
      log(Subsystems.MANAGER, Status.DEBUG, `[SettingsTab] Page ${index} not found, attempting to load...`);
      log(Subsystems.MANAGER, Status.DEBUG, `[SettingsTab] Container is: ${this.container.id}, has ${this.container.children.length} children`);
      log(Subsystems.MANAGER, Status.DEBUG, `[SettingsTab] Container children:`, Array.from(this.container.children).map(c => `${c.tagName}${c.id ? '#' + c.id : ''}${c.className ? '.' + c.className : ''}`));

      // The registry will load the HTML/CSS if needed
      const handler = await this._pageRegistry.getHandler(index, this.container, pageData);
      if (!handler) {
        log(Subsystems.MANAGER, Status.ERROR, `[SettingsTab] Failed to load page handler for ${index}`);
        this._inPageTransition = false;
        return;
      }

      // Now try to find the page element again
      targetPage = this.container.querySelector(`.settings-page[data-page-index="${index}"]`);
      log(Subsystems.MANAGER, Status.DEBUG, `[SettingsTab] After loading, found targetPage: ${targetPage ? targetPage.id : 'null'}`);
      if (!targetPage) {
        // Try searching the entire document
        const docTarget = document.querySelector(`.settings-page[data-page-index="${index}"]`);
        log(Subsystems.MANAGER, Status.DEBUG, `[SettingsTab] Document search found: ${docTarget ? docTarget.id : 'null'}`);
        log(Subsystems.MANAGER, Status.ERROR, `[SettingsTab] Page element still not found after loading: ${index}`);
        this._inPageTransition = false;
        return;
      }
    }

    // Find currently active page
    const activePage = this.container.querySelector('.settings-page.active');

    // Prepare new page (hidden initially for fade-in)
    targetPage.style.opacity = '0';
    targetPage.classList.add('active');

    // Force reflow for transition to work
    void targetPage.offsetHeight;

    // Apply transition and fade in
    const durationStr = `${this._transitionDuration}ms`;

    if (activePage) {
      activePage.style.transition = `opacity ${durationStr} ease-in-out`;
      activePage.style.opacity = '0';
    }

    targetPage.style.transition = `opacity ${durationStr} ease-in-out`;
    targetPage.style.opacity = '1';

    // Load page handler if not already loaded
    let handler = this._pageRegistry.getLoadedHandler(index);
    if (!handler) {
      handler = await this._pageRegistry.getHandler(index, targetPage, pageData);
    }

    // Ensure we have a handler
    if (!handler) {
      log(Subsystems.MANAGER, Status.ERROR, `[SettingsTab] No handler available for page ${index}`);
      this._inPageTransition = false;
      return;
    }
    if (handler?.onShow) {
      handler.onShow();
    }

    // After transition completes, clean up
    setTimeout(() => {
      if (activePage) {
        activePage.classList.remove('active');
        activePage.style.transition = '';
        activePage.style.opacity = '';

        // Notify previous handler it's hidden
        const prevHandler = this._pageRegistry.getLoadedHandler(this.currentPageIndex);
        if (prevHandler?.onHide) {
          prevHandler.onHide();
        }
      }
      targetPage.style.transition = '';
      targetPage.style.opacity = '';

      this.currentPageIndex = index;
      this._inPageTransition = false;

      this.onPageChange(index, pageData);
      log(Subsystems.MANAGER, Status.DEBUG, `[SettingsTab] Showing page: ${index}`);

      // Process any pending page request that came in during this transition
      const pending = this._pendingPageRequest;
      this._pendingPageRequest = null;
      if (pending) {
        log(Subsystems.MANAGER, Status.DEBUG, `[SettingsTab] Processing pending page request: ${pending.index}`);
        this.showPage(pending.index, pending.pageData);
      }
    }, this._transitionDuration);
  }

  /**
   * Get the currently active page index
   * @returns {number|null}
   */
  getCurrentPage() {
    return this.currentPageIndex;
  }

  /**
   * Check if a page exists in the DOM
   * @param {number} index - Page index
   * @returns {boolean}
   */
  hasPage(index) {
    return !!this.container.querySelector(`.settings-page[data-page-index="${index}"]`);
  }

  /**
   * Refresh the current page (called when data changes)
   */
  refreshCurrentPage() {
    const handler = this._pageRegistry.getLoadedHandler(this.currentPageIndex);
    if (handler?.refresh) {
      handler.refresh();
    }
  }

  /**
   * Check if any page has unsaved changes
   * @returns {boolean}
   */
  isDirty() {
    return this._pageRegistry.isDirty();
  }

  /**
   * Save all dirty pages
   * @returns {Promise<Array>}
   */
  async save() {
    return this._pageRegistry.saveAll();
  }

  /**
   * Reset/cancel changes on all pages
   */
  cancel() {
    this._pageRegistry.cancelAll();
  }

  /**
   * Get all available page indices from the DOM
   * @returns {Array<number>}
   */
  getAvailablePages() {
    const pages = this.container.querySelectorAll('.settings-page');
    return Array.from(pages)
      .map(p => parseInt(p.dataset.pageIndex, 10))
      .filter(n => !isNaN(n));
  }

  /**
   * Destroy all page handlers
   */
  destroy() {
    this._pageRegistry?.destroy();
    this._pageRegistry = null;
  }
}

/**
 * Profile Manager - Table Management Module
 *
 * Handles all table-related functionality for the Profile Manager.
 */


class ProfileManagerTable {
  constructor(profileManager) {
    this.pm = profileManager; // Reference to parent ProfileManager
    this._optionsTable = null; // Local reference to optionsTable
  }

  /**
   * Initialize the options table
   */
  async initOptionsTable() {
    log(Subsystems.MANAGER, Status.INFO, '[ProfileManager] initOptionsTable() START');
    if (!this.pm.elements.tableContainer || !this.pm.elements.navigatorContainer) return;

    this.pm.optionsTable = new LithiumTable({
      container: this.pm.elements.tableContainer,
      navigatorContainer: this.pm.elements.navigatorContainer,
      tablePath: 'profile-manager/user-options',
      lookupKeyIdx: 4,
      cssPrefix: 'profile-options',
      storageKey: 'profile_options_table',
      app: this.pm.app,
      readonly: true,
      panel: this.pm.elements.leftPanel,
      panelStateManager: this.pm.leftPanelState,
      localSearch: true,
      localSearchFields: ['section', 'label', 'search'],
      tableWidthMode: 'narrow',
      onRowSelected: (rowData) => this.handleOptionSelected(rowData),
      onRowDeselected: () => this.handleOptionDeselected(),
      onRefresh: () => this.loadUserOptions(),
    });

    // Also store reference on this (ProfileManagerTable) so it can be accessed as this.tableManager.optionsTable
    this.optionsTable = this.pm.optionsTable;

    this.pm.editHelper.registerTable(this.pm.optionsTable);
    log(Subsystems.MANAGER, Status.INFO, '[ProfileManager] initOptionsTable() about to call optionsTable.init()');
    await this.pm.optionsTable.init();
    log(Subsystems.MANAGER, Status.INFO, '[ProfileManager] initOptionsTable() optionsTable.init() completed, table=', !!this.pm.optionsTable.table);
    await this.loadUserOptions();
    log(Subsystems.MANAGER, Status.INFO, '[ProfileManager] initOptionsTable() loadUserOptions() completed');
  }

  /**
   * Load user options from Lookup 60 and merge with Main Menu data
   */
  async loadUserOptions() {
    if (!this.pm.app?.api || !this.pm.optionsTable?.table) return;

    try {
      this.pm.optionsTable.showLoading();

      const rows = await authQuery(this.pm.app.api, 26, { INTEGER: { LOOKUPID: 60 } });
      const profileSectionsRow = rows?.find(row => row.key_idx === 0);

      if (profileSectionsRow && profileSectionsRow.collection) {
        let collection = profileSectionsRow.collection;
        if (typeof collection === 'string') {
          try {
            collection = JSON.parse(collection);
          } catch (_e) {
            collection = {};
          }
        }

        const locale = navigator.language || 'en-US';
        const sections = collection[locale] || collection.default || [];

        // Transform sections, merging with menu data for manager entries
        this.pm.userOptions = this.transformSections(sections);
      } else {
        this.pm.userOptions = this.getDefaultUserOptions();
      }

      // Don't auto-select - we'll handle selection manually in _restoreSelectedSection
      this.pm.optionsTable.loadStaticData(this.pm.userOptions, { autoSelect: false });
      log(Subsystems.TABLE, Status.INFO,
        `[ProfileManager] Loaded ${this.pm.userOptions.length} user options`);

      // Collapse all groups initially
      this.pm.optionsTable.collapseAll?.();

      // Process icons after the table has rendered HTML formatter cells.
      requestAnimationFrame(() => {
        requestAnimationFrame(() => {
          processIcons(this.pm.elements.tableContainer);
        });
      });

      // Restore previously selected section after data is loaded
      this._restoreSelectedSection();
    } catch (error) {
      log(Subsystems.TABLE, Status.ERROR,
        `[ProfileManager] Failed to load user options: ${error.message}`);
      this.pm.userOptions = this.getDefaultUserOptions();
      // Don't auto-select on error either
      this.pm.optionsTable.loadStaticData(this.pm.userOptions, { autoSelect: false });

      // Process icons after error recovery once the table DOM is present.
      requestAnimationFrame(() => {
        requestAnimationFrame(() => {
          processIcons(this.pm.elements.tableContainer);
        });
      });

      // Restore previously selected section even on error (using default options)
      this._restoreSelectedSection();
    } finally {
      this.pm.optionsTable.hideLoading();
    }
  }

  /**
   * Restore the previously selected section from localStorage
   * If no saved state (e.g., after localStorage purge), select the first record (Account)
   * Skips restoration if there's a pending external section selection waiting to be applied.
   * @private
   */
  _restoreSelectedSection() {
    // Don't restore if we have a pending external section (external selection takes priority)
    if (this.pm.pendingExternalSection !== null) {
      log(Subsystems.MANAGER, Status.INFO,
        `[ProfileManager] Skipping section restore, pending external section: ${this.pm.pendingExternalSection}`);
      return;
    }

    try {
      const savedIndex = localStorage.getItem(this.pm._selectedSectionStorageKey);
      if (savedIndex === null) {
        log(Subsystems.MANAGER, Status.INFO,
          '[ProfileManager] No saved section, selecting first (Account)');
        this._selectSectionByIndex(-1); // Default to Account page
        return;
      }

      const index = parseInt(savedIndex, 10);
      if (isNaN(index)) {
        log(Subsystems.MANAGER, Status.WARN,
          `[ProfileManager] Invalid saved index "${savedIndex}", selecting first`);
        this._selectSectionByIndex(-1);
        return;
      }

      const table = this.pm.optionsTable?.table;
      if (!table) return;

      // Find row with matching index
      const rows = table.getRows();
      const tabulatorRow = rows.find(row => {
        const data = row.getData();
        return data && data.index === index;
      });

      if (tabulatorRow) {
        table.selectRow(tabulatorRow);
        // Expand the group containing this row
        const group = tabulatorRow.getGroup?.();
        group?.show?.();
        log(Subsystems.MANAGER, Status.INFO,
          `[ProfileManager] Restored section: index ${index}`);
      } else {
        log(Subsystems.MANAGER, Status.WARN,
          `[ProfileManager] Saved section index ${index} not found, selecting first`);
        this._selectSectionByIndex(-1);
      }
    } catch (error) {
      log(Subsystems.MANAGER, Status.ERROR,
        `[ProfileManager] Failed to restore selected section: ${error.message}`);
      this._selectSectionByIndex(-1);
    }
  }

  /**
   * Select a section by its index value
   * @private
   */
  _selectSectionByIndex(index) {
    const table = this.pm.optionsTable?.table;
    if (!table) return;

    const rows = table.getRows();
    const tabulatorRow = rows.find(row => {
      const data = row.getData();
      return data && data.index === index;
    });

    if (tabulatorRow) {
      table.selectRow(tabulatorRow);
      // Expand the group containing this row
      const group = tabulatorRow.getGroup?.();
      group?.show?.();
      log(Subsystems.MANAGER, Status.INFO,
        `[ProfileManager] Selected section by index: ${index}`);
    } else {
      log(Subsystems.MANAGER, Status.WARN,
        `[ProfileManager] Section with index ${index} not found`);
    }
  }

  /**
   * Transform sections from Lookup 60, merging with Main Menu data for manager entries
   */
  transformSections(sections) {
    return sections.map((section, position) => {
      const index = section[3] ?? (position + 1);
      const isInternal = index < 0;
      const sectionIcon = normalizeIconHtml(section[1], '<fa fa-cube></fa>');
      const searchString = section[4] || '';

      // For manager entries (index >= 0), merge with menu data
      if (!isInternal && this.pm._managerIcons[index]) {
        const menuInfo = this.pm._managerIcons[index];
        const groupName = this.pm._getManagerGroup(index);
        return {
          key_idx: position + 1,
          section: groupName,
          icon: normalizeIconHtml(menuInfo.iconHtml, sectionIcon || '<fa fa-cube></fa>') || sectionIcon || '<fa fa-cube></fa>',
          label: menuInfo.name || section[2] || 'Unknown',
          index: index,
          search: searchString,
          status_a1: 1,
        };
      }

      // Internal sections or managers not in menu
      return {
        key_idx: position + 1,
        section: section[0] || '',
        icon: sectionIcon || '',
        label: section[2] || '',
        index: index,
        search: searchString,
        status_a1: 1,
      };
    });
  }

  /**
   * Get default user options when lookup data is not available
   */
  getDefaultUserOptions() {
    const baseOptions = [
      { key_idx: 1, section: 'General', icon: '<fa fa-id-card></fa>', label: 'Account', index: -1, search: '', status_a1: 1 },
      { key_idx: 2, section: 'General', icon: '<fa fa-id-badge></fa>', label: 'Names', index: -2, search: '', status_a1: 1 },
      { key_idx: 3, section: 'General', icon: '<fa fa-address-book></fa>', label: 'Addresses', index: -3, search: '', status_a1: 1 },
      { key_idx: 4, section: 'General', icon: '<fa fa-at></fa>', label: 'E-Mail', index: -4, search: '', status_a1: 1 },
      { key_idx: 5, section: 'General', icon: '<fa fa-phone></fa>', label: 'Phone', index: -5, search: '', status_a1: 1 },
      { key_idx: 6, section: 'Security', icon: '<fa fa-key></fa>', label: 'Authentication', index: -6, search: '', status_a1: 1 },
      { key_idx: 7, section: 'Security', icon: '<fa fa-user-key></fa>', label: 'Tokens', index: -7, search: '', status_a1: 1 },
      { key_idx: 8, section: 'Formatting', icon: '<fa fa-globe></fa>', label: 'Language', index: -8, search: '', status_a1: 1 },
      { key_idx: 9, section: 'Formatting', icon: '<fa fa-calendar></fa>', label: 'Date Formats', index: -9, search: '', status_a1: 1 },
      { key_idx: 10, section: 'Formatting', icon: '<fa fa-00></fa>', label: 'Number Formats', index: -10, search: '', status_a1: 1 },
      { key_idx: 11, section: 'Application', icon: '<fa fa-atom-simple></fa>', label: 'Startup', index: -11, search: '', status_a1: 1 },
      { key_idx: 12, section: 'Application', icon: '<fa fa-bell></fa>', label: 'Notifications', index: -12, search: '', status_a1: 1 },
      { key_idx: 13, section: 'Application', icon: '<fa fa-bell-concierge></fa>', label: 'Concierge', index: -13, search: '', status_a1: 1 },
      { key_idx: 14, section: 'Application', icon: '<fa fa-note-sticky></fa>', label: 'Annotations', index: -14, search: '', status_a1: 1 },
      { key_idx: 15, section: 'Application', icon: '<fa fa-signs-post></fa>', label: 'Tours', index: -15, search: '', status_a1: 1 },
      { key_idx: 16, section: 'Application', icon: '<fa fa-graduation-cap></fa>', label: 'Training', index: -16, search: '', status_a1: 1 },
      { key_idx: 17, section: 'Security', icon: '<fa fa-receipt></fa>', label: 'Login History', index: -17, search: '', status_a1: 1 },
    ];

    // Add manager entries from menu data, using group names from Main Menu
    const managerOptions = Object.entries(this.pm._managerIcons).map(([id, info], position) => {
      const managerId = parseInt(id, 10);
      const groupName = this.pm._getManagerGroup(managerId);
      return {
        key_idx: 100 + position,
        section: groupName,
        icon: info.iconHtml || '<fa fa-cube></fa>',
        label: info.name || `Manager ${id}`,
        index: managerId,
        search: '',
        status_a1: 1,
      };
    });

    return [...baseOptions, ...managerOptions];
  }

  /**
   * Handle when an option is selected from the table
   */
  async handleOptionSelected(rowData) {
    if (!rowData) return;

    log(Subsystems.MANAGER, Status.INFO,
      `[ProfileManager] Option selected: ${rowData.label}, index: ${rowData.index}`);

    // Close any open popups (timezone picker, datepicker, etc.)
    closeAllPopups();

    // Save selected section index to localStorage
    if (rowData.index !== undefined) {
      localStorage.setItem(this.pm._selectedSectionStorageKey, String(rowData.index));
    }

    // Update section label button using the data directly
    if (this.pm.elements.sectionLabelBtn && rowData.label) {
      this.pm.elements.sectionLabelBtn.classList.add('active');
      this.pm.elements.sectionLabelBtn.title = rowData.label;
      const iconHtml = rowData.icon || '<fa fa-cube></fa>';
      this.pm.elements.sectionLabelBtn.innerHTML = `${iconHtml} <span>${rowData.label}</span>`;
      processIcons(this.pm.elements.sectionLabelBtn);
    }

    // Switch to settings tab if not already there
    if (this.pm.currentTab !== 'settings') {
      this.pm.switchTab('settings');
    }

    // Show the settings page for this section
    await this.pm.settingsHandler.showPage(rowData.index, rowData);
  }

  /**
   * Handle when an option is deselected from the table
   */
  handleOptionDeselected() {
    log(Subsystems.MANAGER, Status.DEBUG, '[ProfileManager] Option deselected');

    // Clear section label when nothing is selected
    if (this.pm.elements.sectionLabelBtn) {
      this.pm.elements.sectionLabelBtn.classList.remove('active');
      this.pm.elements.sectionLabelBtn.title = 'Select a Section';
      this.pm.elements.sectionLabelBtn.innerHTML = '<fa fa-cog></fa> <span>Select a Section</span>';
    }
  }
}

/**
 * Profile Manager
 *
 * User preferences and profile settings management.
 * Uses index-based navigation where internal sections have negative indices
 * and manager sections use actual manager IDs.
 */


/**
 * Profile Manager Class
 */
class ProfileManager {
  constructor(app, container) {
    this.app = app;
    this.container = container;
    this.elements = {};
    this.user = null;
    this.preferences = {};
    this.themes = [];
    this.databases = [];
    this.api = null;

    this.managerId = 3;  // Matches lithium.json "003.Profile"
    this.optionsTable = null;
    this.userOptions = [];
    this.pendingExternalSection = null;

    this.leftPanelState = new PanelStateManager('lithium_profile_left');
    this.splitter = null;
    this.leftPanelWidth = this.leftPanelState.loadWidth(314);
    this.isLeftPanelCollapsed = this.leftPanelState.loadCollapsed(false);

    this.editHelper = new ManagerEditHelper({ name: 'Profile' });

    // Initialize sub-modules
    this.tableManager = new ProfileManagerTable(this);

    // Use the global settings service with sectioned access wrapper
    const globalSettings = window.lithiumSettings;
    this.settingsService = {
      getSection: (sectionKey, path, defaultValue) => {
        const fullPath = path ? `${sectionKey}.${path}` : sectionKey;
        return globalSettings.get(fullPath, defaultValue);
      },
      setSection: (sectionKey, path, value) => {
        const fullPath = path ? `${sectionKey}.${path}` : sectionKey;
        globalSettings.set(fullPath, value);
        // Sync to collection tab
        this.collectionHandler?.setData(this.getCollectionData());
      },
      removeSection: (sectionKey) => {
        // Remove by setting to undefined which deletes the key
        globalSettings.removeSection(sectionKey);
        // Sync to collection tab
        this.collectionHandler?.setData(this.getCollectionData());
      },
      get: (path, defaultValue) => globalSettings.get(path, defaultValue),
      set: (path, value) => globalSettings.set(path, value),
      getSetting: (path, defaultValue) => globalSettings.get(path, defaultValue),
      getAll: () => globalSettings.getAll(),
      onChange: (callback) => globalSettings.onChange(callback),
    };

    // Initialize default formats in settings if not present
    this._initializeDefaultFormats();

    // Load saved font settings
    const savedFontSettings = this.settingsService.getSetting('collection.font', {});
    if (savedFontSettings.fontSize) this.editorFontSize = savedFontSettings.fontSize;
    if (savedFontSettings.fontFamily) this.editorFontFamily = savedFontSettings.fontFamily;

    // Storage key for selected section persistence
    this._selectedSectionStorageKey = 'lithium_profile_selected_section';

    this.defaultPreferences = {
      language: 'en-US',
      dateFormat: 'MM/DD/YYYY',
      timeFormat: '12h',
      numberFormat: 'en-US',
      defaultDatabase: '',
      activeTheme: '',
    };

    this.currentTab = 'settings';
    this.collectionHandler = null;
    this.settingsHandler = null;

    // Tab transition state
    this._inTabTransition = false;

    // Font popup
    this.fontPopup = null;
    this.editorFontSize = 13;
    this.editorFontFamily = '"Vanadium Mono", var(--font-mono, monospace)';

    this._menuData = null;
    this._managerIcons = {};
    this._managerGroups = new Map(); // managerId -> groupName
  }

  /**
   * Initialize the profile manager
   */
  async init() {
    log(Subsystems.MANAGER, Status.INFO, '[ProfileManager] init() START');
    this.api = createRequest();

    await this.render();
    this.loadUserInfo();

    // Load menu data first (needed for merging with Lookup 60)
    await this.loadMenuData();

    // Initialize tab handlers BEFORE table so settingsHandler is ready for section restoration
    this.initTabHandlers();

    // Initialize to settings tab to set button states properly
    this.switchTab('settings');

    await this.tableManager.initOptionsTable();
    log(Subsystems.MANAGER, Status.INFO, `[ProfileManager] initOptionsTable completed, tableManager.optionsTable=${!!this.tableManager?.optionsTable}, table=${!!this.tableManager?.optionsTable?.table}`);

    this.setupSplitter();
    this.setupSplitter();

    await Promise.all([this.loadThemes(), this.loadDatabases()]);
    this.loadPreferences();
    this.setupEventListeners();
    this.restorePanelState();
    this.setupFooter();
    this.initFontPopup();
    this.overrideCloseButton();
    this.show();
  }

  /**
   * Load menu data and build manager icons registry and groups
   */
  async loadMenuData() {
    try {
      this._menuData = await getMenu(this.app.api);
      this._managerIcons = buildManagerIconsRegistry(this._menuData);
      this._buildManagerGroups();
      log(Subsystems.MANAGER, Status.INFO,
        `[ProfileManager] Loaded ${Object.keys(this._managerIcons).length} manager icons ` +
        `across ${new Set(this._managerGroups.values()).size} groups`);
    } catch (error) {
      log(Subsystems.MANAGER, Status.WARN, '[ProfileManager] Failed to load menu data:', error);
      this._managerIcons = {};
      this._managerGroups.clear();
    }
  }

  /**
   * Build manager groups mapping from menu data
   */
  _buildManagerGroups() {
    this._managerGroups.clear();

    if (!this._menuData || !Array.isArray(this._menuData)) return;

    this._menuData.forEach((group) => {
      const groupName = group.name || 'Modules';
      group.items?.forEach((item) => {
        if (item.managerId) {
          this._managerGroups.set(item.managerId, groupName);
        }
      });
    });
  }

  /**
   * Get the group name for a manager ID
   * @param {number} managerId - Manager ID
   * @returns {string}
   */
  _getManagerGroup(managerId) {
    return this._managerGroups.get(managerId) || 'Modules';
  }

  /**
   * Render the profile manager template
   */
  async render() {
    const response = await fetch('/src/managers/profile-manager/profile-manager.html');
    this.container.innerHTML = await response.text();

    this.cacheElements();
    processIcons(this.container);
    initToolbars();
  }

  /**
   * Cache DOM element references
   */
  cacheElements() {
    this.elements = {
      page: this.container.querySelector('#profile-manager-page'),
      leftPanel: this.container.querySelector('#profile-left-panel'),
      rightPanel: this.container.querySelector('#profile-right-panel'),
      tableContainer: this.container.querySelector('#profile-table-container'),
      navigatorContainer: this.container.querySelector('#profile-navigator'),
      splitter: this.container.querySelector('#profile-splitter'),
      collapseBtn: this.container.querySelector('#profile-collapse-btn'),
      tabToolbar: this.container.querySelector('#profile-tab-toolbar'),
      tabCollection: this.container.querySelector('#tab-collection'),
      panelSettings: this.container.querySelector('#panel-settings'),
      panelCollection: this.container.querySelector('#panel-collection'),
      collectionEditorContainer: this.container.querySelector('#collection-editor-container'),
      sectionLabelBtn: this.container.querySelector('#btn-section-label'),
      btnUndo: this.container.querySelector('#btn-undo'),
      btnRedo: this.container.querySelector('#btn-redo'),
      btnFold: this.container.querySelector('#btn-fold'),
      btnUnfold: this.container.querySelector('#btn-unfold'),
      btnFont: this.container.querySelector('#btn-font'),
      btnPrettify: this.container.querySelector('#btn-prettify'),
    };
  }

  /**
   * Render fallback if template fails
   */
  renderFallback() {
    this.container.innerHTML = `
      <div id="profile-manager-page">
        <p>Profile Manager loading...</p>
      </div>
    `;
  }

  /**
   * Initialize the User Options table
   */
















  /**
   * Setup the splitter between panels
   */
  setupSplitter() {
    this.splitter = new LithiumSplitter({
      element: this.elements.splitter,
      leftPanel: this.elements.leftPanel,
      minWidth: 157,
      maxWidth: 600,
      tables: this.optionsTable,
      onResize: (width) => {
        this.leftPanelWidth = width;
      },
      onResizeEnd: (width) => {
        this.leftPanelState.saveWidth(width);
        this.optionsTable?.table?.redraw?.();
      },
    });

    this.optionsTable?.setSplitter(this.splitter);
  }

  /**
   * Toggle left panel collapse
   */
  toggleLeftPanel() {
    this.isLeftPanelCollapsed = togglePanelCollapse({
      panel: this.elements.leftPanel,
      splitter: this.splitter,
      collapseBtn: this.elements.collapseBtn,
      panelWidth: this.leftPanelWidth,
      isCollapsed: this.isLeftPanelCollapsed,
      onAfterToggle: () => this.optionsTable?.table?.redraw?.(),
    });

    this.leftPanelState.saveCollapsed(this.isLeftPanelCollapsed);
  }

  /**
   * Restore panel state from persistence
   */
  restorePanelState() {
    this.isLeftPanelCollapsed = this.leftPanelState.loadCollapsed(this.isLeftPanelCollapsed);

    restorePanelState({
      panel: this.elements.leftPanel,
      splitter: this.splitter,
      collapseBtn: this.elements.collapseBtn,
      isCollapsed: this.isLeftPanelCollapsed,
    });
  }

  /**
   * Initialize tab handlers
   */
  initTabHandlers() {
    // Settings tab handler
    this.settingsHandler = new SettingsTabHandler({
      container: this.elements.panelSettings,
      app: this.app,
      settingsService: this.settingsService,
      onPageChange: (index, _data) => {
        log(Subsystems.MANAGER, Status.DEBUG, `[ProfileManager] Page changed to ${index}`);
      },
      onDirtyChange: (dirty) => {
        this.updateFooterButtons(dirty);
      },
    });
    this.settingsHandler.init();

    // Collection tab handler
    this.collectionHandler = new CollectionTabHandler({
      container: this.elements.collectionEditorContainer,
      parent: this.container, // Manager main container (sibling of toolbar)
      onDirtyChange: (dirty) => {
        this.updateFooterButtons(dirty);
      },
      onSave: () => this.handleSaveCollection(),
      onCancel: () => this.handleCancelCollection(),
    });

    // Register collection editor with editHelper for dirty tracking
    this.editHelper.registerEditor('collection', {
      getContent: () => this.collectionHandler?.getContent() || '{}',
      setContent: (content) => {
        try {
          const data = JSON.parse(content);
          this.collectionHandler?.setData(data);
        } catch (e) {
          log(Subsystems.MANAGER, Status.WARN, '[ProfileManager] Failed to set collection content:', e);
        }
      },
      setEditable: (editable) => this.collectionHandler?.setEditable(editable),
      boundTable: null,
    });
  }

  /**
   * Switch between tabs
   */
  switchTab(tabId) {
    if (!['settings', 'collection'].includes(tabId)) return;

    // Guard: same tab requested or already transitioning
    if (tabId === this.currentTab || this._inTabTransition) return;

    this._inTabTransition = true;

    closeAllPopups();

    // Update button states immediately
    this.elements.tabCollection?.classList.toggle('active', tabId === 'collection');
    const hasPageSelected = this.settingsHandler?.getCurrentPage() !== null;
    this.elements.sectionLabelBtn?.classList.toggle('active', tabId === 'settings' && hasPageSelected);

    // Update state
    this.currentTab = tabId;

    // Enable/disable editor controls
    const isCollection = tabId === 'collection';
    this.setEditorButtonsDisabled(!isCollection);

    // Show/hide panels immediately
    this.elements.panelSettings?.classList.toggle('active', tabId === 'settings');
    this.elements.panelCollection?.classList.toggle('active', tabId === 'collection');

    // Init collection editor after panel is active to ensure proper sizing
    if (tabId === 'collection') {
      this.collectionHandler?.init(this.getCollectionData());
      // Request measure after a frame to ensure layout is updated
      requestAnimationFrame(() => {
        this.collectionHandler?.editor?.requestMeasure();
      });
    }

    this._inTabTransition = false;

    // Update footer buttons — on collection tab, dirty state drives save/cancel
    if (isCollection) {
      const canSave = this.collectionHandler?.isDirty() || false;
      this.updateFooterButtons(canSave);
    } else {
      this.updateFooterButtons(false);
    }

    log(Subsystems.MANAGER, Status.DEBUG, `[ProfileManager] Switched to tab: ${tabId}`);
  }

  /**
   * Set disabled state for editor toolbar buttons
   */
  setEditorButtonsDisabled(disabled) {
    const buttons = this.container.querySelectorAll('[data-editor-btn]');
    buttons.forEach(btn => {
      btn.disabled = disabled;
      btn.classList.toggle('disabled', disabled);
    });
  }

  /**
   * Load user info from JWT claims
   */
  loadUserInfo() {
    const claims = getClaims();
    if (!claims) return;

    this.user = {
      id: claims.user_id,
      username: claims.username || 'User',
      email: claims.email || '-',
      roles: claims.roles || [],
      database: claims.database || '-',
    };

    // Update UI elements if they exist
    const usernameEl = this.container.querySelector('#profile-username');
    const emailEl = this.container.querySelector('#profile-email');
    const rolesEl = this.container.querySelector('#profile-roles');
    const dbEl = this.container.querySelector('#profile-database');

    if (usernameEl) usernameEl.textContent = this.user.username;
    if (emailEl) emailEl.textContent = this.user.email;
    if (rolesEl) rolesEl.textContent = Array.isArray(this.user.roles)
      ? this.user.roles.join(', ') : this.user.roles;
    if (dbEl) dbEl.textContent = this.user.database;
  }

  /**
   * Load themes from localStorage or use defaults
   */
  async loadThemes() {
    const storedThemes = localStorage.getItem('lithium_themes');
    if (storedThemes) {
      try {
        this.themes = JSON.parse(storedThemes);
      } catch (error) {
        log(Subsystems.MANAGER, Status.WARN, '[ProfileManager] Failed to parse themes:', error);
      }
    }

    if (!this.themes?.length) {
      this.themes = [
        { id: 'default', name: 'Default Dark' },
        { id: 1, name: 'Midnight Indigo' },
        { id: 2, name: 'Ocean Breeze' },
        { id: 3, name: 'Sunset Glow' },
      ];
    }

    this.populateThemeSelect();
  }

  /**
   * Populate theme select dropdown
   */
  populateThemeSelect() {
    const select = this.container.querySelector('#pref-theme');
    if (!select) return;

    select.innerHTML = '<option value="">Select a theme...</option>';
    this.themes.forEach((theme) => {
      const option = document.createElement('option');
      option.value = theme.id;
      option.textContent = theme.name;
      select.appendChild(option);
    });
  }

  /**
   * Load databases (mock for now)
   */
  async loadDatabases() {
    this.databases = [
      { id: 'Demo_PG', name: 'Demo PostgreSQL' },
      { id: 'Demo_MySQL', name: 'Demo MySQL' },
      { id: 'Demo_SQLite', name: 'Demo SQLite' },
    ];

    const configDefault = getConfigValue('auth.default_database', '');
    if (configDefault && !this.defaultPreferences.defaultDatabase) {
      this.defaultPreferences.defaultDatabase = configDefault;
    }

    this.populateDatabaseSelect();
  }

  /**
   * Populate database select dropdown
   */
  populateDatabaseSelect() {
    const select = this.container.querySelector('#pref-database');
    if (!select) return;

    select.innerHTML = '<option value="">Select a database...</option>';
    this.databases.forEach((db) => {
      const option = document.createElement('option');
      option.value = db.id;
      option.textContent = db.name;
      select.appendChild(option);
    });
  }

  /**
   * Load preferences from localStorage
   */
  loadPreferences() {
    const stored = localStorage.getItem('lithium_preferences');
    if (stored) {
      try {
        const parsed = JSON.parse(stored);
        this.preferences = { ...this.defaultPreferences, ...parsed };
      } catch (error) {
        log(Subsystems.MANAGER, Status.WARN, '[ProfileManager] Failed to parse preferences:', error);
        this.preferences = { ...this.defaultPreferences };
      }
    } else {
      this.preferences = { ...this.defaultPreferences };
    }

    this.applyPreferencesToForm();
  }

  /**
   * Apply preferences to form fields
   */
  applyPreferencesToForm() {
    const fields = {
      '#pref-language': this.preferences.language,
      '#pref-date-format': this.preferences.dateFormat,
      '#pref-time-format': this.preferences.timeFormat,
      '#pref-number-format': this.preferences.numberFormat,
      '#pref-database': this.preferences.defaultDatabase,
      '#pref-theme': this.preferences.activeTheme,
    };

    Object.entries(fields).forEach(([selector, value]) => {
      const el = this.container.querySelector(selector);
      if (el) el.value = value;
    });
  }

  /**
   * Setup event listeners
   */
  setupEventListeners() {
    // Listen for external section selection requests
    this._eventListenerBound = this.handleExternalSectionSelect.bind(this);
    eventBus.on('profile:select-section', this._eventListenerBound);

    this.elements.collapseBtn?.addEventListener('click', () => this.toggleLeftPanel());
    this.elements.sectionLabelBtn?.addEventListener('click', () => this.switchTab('settings'));
    this.elements.tabCollection?.addEventListener('click', () => this.switchTab('collection'));

    // Collection editor toolbar buttons
    this.elements.btnUndo?.addEventListener('click', () => this.collectionHandler?.undo());
    this.elements.btnRedo?.addEventListener('click', () => this.collectionHandler?.redo());
    this.elements.btnFold?.addEventListener('click', () => this.collectionHandler?.foldAll());
    this.elements.btnUnfold?.addEventListener('click', () => this.collectionHandler?.unfoldAll());
    this.elements.btnPrettify?.addEventListener('click', () => this.collectionHandler?.prettify());

    // Font button — toggle font popup
    this.elements.btnFont?.addEventListener('click', (e) => this.toggleFontPopup(e));


  }





  /**
   * Handle external request to select a section
   * @param {Object} data - Event data with index property
   */
  async handleExternalSectionSelect(data) {
    const { index } = data;
    if (index === undefined) return;

    log(Subsystems.MANAGER, Status.INFO, `[ProfileManager] External section select: ${index}`);

    // Store the pending selection
    this.pendingExternalSection = index;

    // If table is already loaded with data, apply immediately
    if (this.tableManager?.optionsTable?.table && this.userOptions?.length > 0) {
      await this._applyExternalSectionSelection();
    }
    // Otherwise, it will be applied after table loading in _restoreSelectedSection
  }

  /**
   * Select a section externally (called from main manager)
   * @param {number} index - The section index to select
   */
  async selectSection(index) {
    log(Subsystems.MANAGER, Status.INFO, `[ProfileManager] External selectSection: ${index}, this=${!!this}, ctor=${this?.constructor?.name}`);
    this.pendingExternalSection = index;

    log(Subsystems.MANAGER, Status.INFO,
      `[ProfileManager] selectSection: tableManager=${!!this.tableManager}, optionsTable=${!!this.tableManager?.optionsTable}, table=${!!this.tableManager?.optionsTable?.table}`);

    // Wait for table to be ready
    let attempts = 0;
    while (!this.tableManager?.optionsTable?.table && attempts < 100) {
      await new Promise(resolve => setTimeout(resolve, 50));
      attempts++;
    }

    log(Subsystems.MANAGER, Status.INFO,
      `[ProfileManager] selectSection: after while, attempts=${attempts}, table=${!!this.tableManager?.optionsTable?.table}`);

    if (!this.tableManager?.optionsTable?.table) {
      log(Subsystems.MANAGER, Status.ERROR, `[ProfileManager] Table still not ready after waiting (${attempts} attempts)`);
      this.pendingExternalSection = null;
      return;
    }

    log(Subsystems.MANAGER, Status.INFO, `[ProfileManager] Table is ready, calling _applyExternalSectionSelection, pending=${this.pendingExternalSection}`);

    // Clear any existing selection first to avoid conflicts
    this.tableManager.optionsTable.table.deselectRow();
    this.settingsHandler.currentPageIndex = null;

    try {
      await this._applyExternalSectionSelection();
      log(Subsystems.MANAGER, Status.INFO, `[ProfileManager] selectSection: _applyExternalSectionSelection completed`);
    } catch (error) {
      log(Subsystems.MANAGER, Status.ERROR, `[ProfileManager] Error in _applyExternalSectionSelection: ${error.message}`);
    }
  }

  /**
   * Apply pending external section selection
   * @private
   */
  async _applyExternalSectionSelection() {
    log(Subsystems.MANAGER, Status.INFO, `[ProfileManager] _applyExternalSectionSelection called, pending: ${this.pendingExternalSection}`);
    if (this.pendingExternalSection === null) {
      log(Subsystems.MANAGER, Status.INFO, `[ProfileManager] No pending external section, returning`);
      return;
    }

    const index = this.pendingExternalSection;
    this.pendingExternalSection = null; // Clear pending

    log(Subsystems.MANAGER, Status.INFO, `[ProfileManager] Applying external section select: ${index}`);

    // Ensure we're on the settings tab
    if (this.currentTab !== 'settings') {
      this.switchTab('settings');
      log(Subsystems.MANAGER, Status.INFO, `[ProfileManager] Switched to settings tab`);
    }

    // Find and select the row in the table
    if (this.tableManager?.optionsTable?.table) {
      const table = this.tableManager.optionsTable.table;
      const rows = table.getRows();

      log(Subsystems.MANAGER, Status.INFO, `[ProfileManager] Table has ${rows.length} rows`);

      // Find the row with matching index
      const targetRow = rows.find(row => row.getData().index === index);

      log(Subsystems.MANAGER, Status.INFO, `[ProfileManager] Target row found: ${!!targetRow}`);

      if (targetRow) {
        const rowData = targetRow.getData();

        log(Subsystems.MANAGER, Status.INFO, `[ProfileManager] Selecting: ${rowData.label} (index ${rowData.index})`);

        // Expand the group if it's collapsed - get group directly from row
        const group = targetRow.getGroup?.();
        if (group) {
          group.show(); // Expand the group
        }
        log(Subsystems.MANAGER, Status.INFO, `[ProfileManager] Expanded group for: ${rowData.label}`);

        // Select the row
        table.deselectRow();
        targetRow.select();

        log(Subsystems.MANAGER, Status.INFO, `[ProfileManager] Selected row`);

        // Scroll into view
        targetRow.getElement().scrollIntoView({ behavior: 'smooth', block: 'nearest' });

        // Trigger the selection handler
        await this.tableManager.handleOptionSelected(rowData);

        log(Subsystems.MANAGER, Status.INFO, `[ProfileManager] Triggered selection handler`);
      } else {
        log(Subsystems.MANAGER, Status.WARN, `[ProfileManager] Section with index ${index} not found`);
        const indices = rows.map(r => r.getData().index);
        log(Subsystems.MANAGER, Status.DEBUG, `[ProfileManager] Available indices:`, indices);
      }
    } else {
      log(Subsystems.MANAGER, Status.WARN, `[ProfileManager] Table not available`);
    }
  }

  /**
   * Setup footer with Save/Cancel buttons
   */
  setupFooter() {
    const slot = this.container.closest('.manager-slot');
    if (!slot) return;

    const footer = slot.querySelector('.manager-slot-footer');
    if (!footer) return;

    const group = footer.querySelector('.subpanel-header-group');
    if (!group) return;

    const placeholder = group.querySelector('.slot-footer-placeholder');

    const footerElements = setupManagerFooterIcons(group, {
      anchor: placeholder,
      showSaveCancel: true,
      onSave: () => this.handleSaveCollection(),
      onCancel: () => this.handleCancelCollection(),
      onPrint: () => this._handleFooterPrint(),
      onEmail: () => this._handleFooterEmail(),
      onDownload: () => this._handleFooterDownload(),
      onExport: (value, label) => this._handleFooterExport(value, label),
      exportOptions: [
        { value: 'pdf', label: 'PDF', icon: 'fa-file-pdf', enabled: true },
        { value: 'csv', label: 'CSV', icon: 'fa-file-csv', enabled: true },
        { value: 'json', label: 'JSON', icon: 'fa-brackets-curly', enabled: true },
        { value: 'html', label: 'HTML', icon: 'fa-file-html', enabled: true },
      ],
    });

    this._footerSaveBtn = footerElements.saveBtn;
    this._footerCancelBtn = footerElements.cancelBtn;

    this.editHelper.wireFooterButtons(
      footerElements.saveBtn,
      footerElements.cancelBtn,
      footerElements.dummyBtn,
    );

    // Override editHelper's activeEditingTable to handle save/cancel for the collection tab.
    // The Profile Manager doesn't use LithiumTable edit mode — save/cancel drive the
    // collection JSON editor's dirty state directly.
    this.editHelper.activeEditingTable = {
      handleSave: () => this.handleSaveCollection(),
      handleCancel: () => this.handleCancelCollection(),
    };
  }

  /**
   * Enable/disable footer Save/Cancel buttons
   */
  updateFooterButtons(enabled) {
    if (this._footerSaveBtn) this._footerSaveBtn.disabled = !enabled;
    if (this._footerCancelBtn) this._footerCancelBtn.disabled = !enabled;
  }

  // ============ FOOTER HANDLERS ============

  /**
   * Handle Print button from footer
   */
  _handleFooterPrint() {
    // Print the current settings page or collection
    if (this.currentTab === 'collection') {
      // Print collection JSON
      const data = this.getCollectionData();
      const printWindow = window.open('', '_blank');
      printWindow.document.write('<pre>' + JSON.stringify(data, null, 2) + '</pre>');
      printWindow.document.close();
      printWindow.print();
    } else {
      // Print the settings page
      window.print();
    }
  }

  /**
   * Handle Email button from footer
   */
  _handleFooterEmail() {
    const data = this.getCollectionData();
    const subject = encodeURIComponent('User Profile Preferences');
    const body = encodeURIComponent(
      'User Profile Preferences:\n\n' +
      Object.entries(data).map(([key, value]) => `${key}: ${value}`).join('\n')
    );
    window.open(`mailto:?subject=${subject}&body=${body}`, '_self');
  }

  /**
   * Handle Download button from footer
   */
  _handleFooterDownload() {
    const data = this.getCollectionData();
    // eslint-disable-next-line no-undef
    const blob = new Blob([JSON.stringify(data, null, 2)], { type: 'application/json' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = 'user-profile-preferences.json';
    document.body.appendChild(a);
    a.click();
    document.body.removeChild(a);
    URL.revokeObjectURL(url);
  }

  /**
   * Handle Export button from footer
   */
  _handleFooterExport(value, label) {
    log(Subsystems.MANAGER, Status.INFO, `[ProfileManager] Export to ${label} (${value})`);
    // The export format is already saved by the footer system
    // Actual export would be implemented based on format
    this.showToast(`Export format set to ${label}`);
  }

  /**
   * Get preferences as collection data.
   * Delegates to the Settings Service for the canonical JSON object.
   */
  getCollectionData() {
    return this.settingsService?.getAll() || { ...this.preferences };
  }

  /**
   * Update a specific preference value and sync to collection JSON.
   * Called by settings pages when a preference changes.
   * Delegates to the Settings Service for structured storage.
   * @param {string} key - Preference key
   * @param {*} value - New value
   */
  setPreference(key, value) {
    this.preferences[key] = value;

    // Also write to settings service under a legacy section for compatibility
    this.settingsService?.set('_legacy', key, value);

    // Sync to collection editor if initialized
    if (this.collectionHandler?.editor) {
      this.collectionHandler.setData(this.getCollectionData());
    }

    // Enable save/cancel buttons since preferences changed
    this.updateFooterButtons(true);
    log(Subsystems.MANAGER, Status.DEBUG, `[ProfileManager] Preference updated: ${key} = ${value}`);
  }

  /**
   * Handle save from Collection footer button
   */
  async handleSaveCollection() {
    if (!this.collectionHandler) return;

    try {
      const data = this.collectionHandler.getData();
      if (!data) throw new Error('Invalid JSON data');

      // Replace the entire profile JSON via the Settings Service
      this.settingsService.setAll(data);

      // Also keep legacy flat preferences in sync
      this.preferences = { ...this.preferences, ...data };

      // Apply updated preferences to form fields
      this.applyPreferencesToForm();

      // Attempt API save (may fail if endpoint not ready — data is still in localStorage)
      try {
        await this.savePreferencesToApi(this.settingsService.getAll());
      } catch (apiError) {
        log(Subsystems.MANAGER, Status.WARN,
          '[ProfileManager] API save failed (expected if endpoint not ready):', apiError.message);
      }

      // Reset dirty state — setInitialData updates the baseline without changing content
      this.collectionHandler.setInitialData(data);

      // Return editor to read-only after save
      this.collectionHandler.setEditable(false);

      this.showToast('Preferences saved successfully');
      this.updateFooterButtons(false);
      log(Subsystems.MANAGER, Status.INFO, '[ProfileManager] Collection saved');
    } catch (error) {
      log(Subsystems.MANAGER, Status.ERROR, '[ProfileManager] Save failed:', error);
      this.showToast('Failed to save: ' + error.message, 'error');
    }
  }

  /**
   * Handle cancel from Collection footer button
   */
  async handleCancelCollection() {
    // Flush any pending debounced writes before reverting
    this.settingsService?.flush();
    await this.collectionHandler?.setData(this.getCollectionData());
    this.updateFooterButtons(false);
    log(Subsystems.MANAGER, Status.INFO, '[ProfileManager] Collection cancelled');
  }

  /**
   * Save preferences to API
   */
  async savePreferencesToApi(preferences) {
    const serverUrl = getConfigValue('server.url', 'http://localhost:8080');
    const apiPrefix = getConfigValue('server.api_prefix', '/api');

    const response = await fetch(`${serverUrl}${apiPrefix}/user/preferences`, {
      method: 'PUT',
      headers: {
        'Content-Type': 'application/json',
        'Accept': 'application/json',
      },
      body: JSON.stringify(preferences),
    });

    if (!response.ok) {
      throw new Error(`API error: ${response.status}`);
    }

    const data = await response.json();
    if (data.token) {
      storeJWT(data.token);
      eventBus.emit(Events.AUTH_RENEWED, { expiresAt: data.expires_at });
    }

    return data;
  }

  // ── Font Popup ───────────────────────────────────────────────────────────

  /**
   * Initialize and show the font popup for the collection editor
   */
  initFontPopup() {
    if (this.fontPopup?.popup) return;

    const { popup, toggle, getState, setState, destroy } = createFontPopup({
      anchor: this.elements.btnFont,
      fontSize: this.editorFontSize,
      fontFamily: this.editorFontFamily,
      fontWeight: 'normal',
      onPreview: (state) => this._updateFontPreview(state),
      onSave: (state) => this._saveFontSettings(state),
      onCancel: () => this._cancelFontChanges(),
      onReset: () => this._resetFontToDefault(),
    });

    this.fontPopup = { popup, destroy };
    this._fontPopupToggle = toggle;
    this._fontPopupGetState = getState;
    this._fontPopupSetState = setState;
  }

  toggleFontPopup(e) {
    e.stopPropagation();
    if (!this.fontPopup?.popup) {
      this.initFontPopup();
    }
    this._fontPopupToggle?.(e);
  }

  _updateFontPreview(state) {
    // Apply font settings to the collection editor for preview
    this.collectionHandler?.setFontSize(state.fontSize);
    this.collectionHandler?.setFontFamily(state.fontFamily);
  }

  _saveFontSettings(state) {
    // Save font settings to profile
    this.editorFontSize = state.fontSize;
    this.editorFontFamily = state.fontFamily;
    // Store in profile settings under "collection.font"
    this.settingsService?.setSetting('collection.font', {
      fontSize: state.fontSize,
      fontFamily: state.fontFamily,
      fontWeight: state.fontWeight,
    });
  }

  _cancelFontChanges() {
    // Revert to saved settings
    const saved = this.settingsService?.getSetting('collection.font', {
      fontSize: 13,
      fontFamily: '"Vanadium Mono", var(--font-mono, monospace)',
      fontWeight: 'normal',
    });
    this.editorFontSize = saved.fontSize;
    this.editorFontFamily = saved.fontFamily;
    this.collectionHandler?.setFontSize(saved.fontSize);
    this.collectionHandler?.setFontFamily(saved.fontFamily);
  }

  _resetFontToDefault() {
    const defaults = {
      fontSize: '13px',
      fontFamily: '"Vanadium Mono", var(--font-mono, monospace)',
      fontWeight: 'normal',
    };
    return defaults;
  }

  /**
   * Initialize default date/time formats in the settings if not already present
   */
  _initializeDefaultFormats() {
    const defaultFormats = {
      dates: {
        'Short Date': 'yyyy-MM-dd',
        'Medium Date': 'yyyy-MMM-dd',
        'Long Date': 'MMMM d, y',
        'Week Number': 'yyyy-\'W\'nn',
      },
      times: {
        'Short Time': 'HH:mm',
        'Medium Time': 'H:mm:ss',
        'Long Time': 'HH:mm:ss.SSS',
      },
      datetimes: {
        'Short DateTime': 'yyyy-MM-dd HH:mm:ss',
        'Medium DateTime': 'yyyy-MMM-dd (EEE) HH:mm:ss',
        'Long DateTime': 'MMMM d, y \'at\' HH:mm:ss',
      },
    };

    // Check if we already have formats initialized under section "-9"
    const currentDates = this.settingsService.getSection('-9', 'dates', {});
    const currentTimes = this.settingsService.getSection('-9', 'times', {});
    const currentDateTimes = this.settingsService.getSection('-9', 'datetimes', {});

    // Only initialize if we don't have any formats yet
    if (Object.keys(currentDates).length === 0) {
      this.settingsService.setSection('-9', 'dates', defaultFormats.dates);
    }
    if (Object.keys(currentTimes).length === 0) {
      this.settingsService.setSection('-9', 'times', defaultFormats.times);
    }
    if (Object.keys(currentDateTimes).length === 0) {
      this.settingsService.setSection('-9', 'datetimes', defaultFormats.datetimes);
    }
  }

  /**
   * Show toast notification using global toast system
   */
  showToast(message, type = 'success') {
    const toastType = type === 'error' ? 'error' : 'success';
    toast[toastType](message, { duration: 3000 });
  }

  /**
   * Override the slot close button to perform global logout instead.
   * Replaces the X icon with a reload icon and changes the click behavior.
   */
  overrideCloseButton() {
    const managerSlot = this.container.closest('.manager-slot');
    if (!managerSlot) return;

    const closeBtn = managerSlot.querySelector('.manager-ui-close-btn');
    if (!closeBtn) return;

    closeBtn.innerHTML = '<fa fa-radiation></fa>';
    closeBtn.dataset.tooltip = 'Global Logout';
    processIcons(closeBtn);

    closeBtn.onclick = async (e) => {
      e.preventDefault();
      e.stopPropagation();
      eventBus.emit('logout:initiate', { logoutType: 'global' });
    };
  }

  /**
   * Show the profile manager
   */
  show() {
    requestAnimationFrame(() => {
      this.elements.page?.classList.add('visible');
    });
  }

  /**
   * Teardown the profile manager
   */
  teardown() {
    this.collectionHandler?.destroy();
    this.collectionHandler = null;

    this.settingsHandler?.destroy();
    this.settingsHandler = null;

    this.settingsService?.destroy();
    this.settingsService = null;

    this.editHelper?.destroy();
    this.splitter?.destroy();
    this.optionsTable?.destroy();

    // Destroy font popup if it exists
    if (this.fontPopup?.destroy && typeof this.fontPopup.destroy === 'function') {
      this.fontPopup.destroy();
    }

    this.elements = {};
  }
}

export { ProfileManager as default };
