/**
 * Queries Manager - Editors Module
 *
 * Handles SQL, Summary (Markdown), and Collection (JSON) editor initialization.
 * Uses the shared codemirror-setup.js for consistent configuration across all
 * managers (line numbers, folding, per-language backgrounds, edit-mode gutter).
 */

import { log, Subsystems, Status } from '../../core/log.js';
import {
  EditorState,
  EditorView,
  undo,
  redo,
} from '../../core/codemirror.js';
import {
  buildEditorExtensions,
  createReadOnlyCompartment,
  setEditorEditable,
  setEditorContentNoHistory,
  updateUndoRedoButtons,
  foldAllInEditor,
  unfoldAllInEditor,
  formatSortedJson,
  READONLY_CLASS,
} from '../../core/codemirror-setup.js';

// Constants
const MIN_FONT_SIZE = 10;
const MAX_FONT_SIZE = 24;
const DEFAULT_FONT_SIZE = 14;
const DEFAULT_FONT_FAMILY = '"Vanadium Mono", var(--font-mono, monospace)';

/**
 * EditorManager - Manages all editor instances
 */
export class EditorManager {
  constructor(manager) {
    this.manager = manager;
    this.sqlEditor = null;
    this.summaryEditor = null;
    this.collectionEditor = null;
    this.sqlDialect = 'sqlite';
    this.fontSettings = {
      size: DEFAULT_FONT_SIZE,
      family: DEFAULT_FONT_FAMILY,
      bold: false,
      italic: false,
    };
  }

  /**
   * Initialize SQL editor with CodeMirror
   */
  async initSqlEditor(initialContent = '') {
    const container = this.manager.elements.sqlEditorContainer;
    if (!container || this.sqlEditor) return;

    try {
      this._sqlReadOnlyCompartment = createReadOnlyCompartment();

      const extensions = buildEditorExtensions({
        language: 'sql',
        readOnlyCompartment: this._sqlReadOnlyCompartment,
        readOnly: !this.manager.queryTable?.isEditing,
        fontSize: this.fontSettings.size,
        fontFamily: this.fontSettings.family,
        onUpdate: (update) => {
          if (update.docChanged && this.manager.queryTable?.isEditing) {
            this.manager.editHelper.checkDirtyState();
          }
          // Update undo/redo button state whenever transaction state changes
          if (update.transactions.length > 0) {
            this._updateUndoRedoButtons();
          }
        },
      });

      const startState = EditorState.create({ doc: initialContent, extensions });

      this.sqlEditor = new EditorView({
        state: startState,
        parent: container,
      });

      // Set initial visual state
      const isEditing = this.manager.queryTable?.isEditing || false;
      setEditorEditable(this.sqlEditor, this._sqlReadOnlyCompartment, isEditing, container);

      container.addEventListener('dblclick', () => {
        if (!this.manager.queryTable?.isEditing && this.manager.queryTable?.table) {
          const selected = this.manager.queryTable.table.getSelectedRows();
          if (selected.length > 0) void this.manager.queryTable.enterEditMode(selected[0]);
        }
      });
    } catch (err) {
      console.error('[QueriesManager] Failed to initialize CodeMirror:', err);
    }
  }

  /**
   * Initialize Summary (Markdown) editor with CodeMirror
   */
  async initSummaryEditor(initialContent = '') {
    const container = this.manager.elements.summaryEditorContainer;
    if (!container || this.summaryEditor) return;

    try {
      this._summaryReadOnlyCompartment = createReadOnlyCompartment();

      const extensions = buildEditorExtensions({
        language: 'markdown',
        readOnlyCompartment: this._summaryReadOnlyCompartment,
        readOnly: !this.manager.queryTable?.isEditing,
        fontSize: this.fontSettings.size,
        fontFamily: this.fontSettings.family,
        onUpdate: (update) => {
          if (update.docChanged && this.manager.queryTable?.isEditing) {
            this.manager.editHelper.checkDirtyState();
          }
          if (update.transactions.length > 0) {
            this._updateUndoRedoButtons();
          }
        },
      });

      const startState = EditorState.create({ doc: initialContent, extensions });

      this.summaryEditor = new EditorView({
        state: startState,
        parent: container,
      });

      // Set initial visual state
      const isEditing = this.manager.queryTable?.isEditing || false;
      setEditorEditable(this.summaryEditor, this._summaryReadOnlyCompartment, isEditing, container);

      // Double-click to enter edit mode (same behavior as SQL editor)
      container.addEventListener('dblclick', () => {
        if (!this.manager.queryTable?.isEditing && this.manager.queryTable?.table) {
          const selected = this.manager.queryTable.table.getSelectedRows();
          if (selected.length > 0) void this.manager.queryTable.enterEditMode(selected[0]);
        }
      });
    } catch (error) {
      console.error('[QueriesManager] Failed to initialize Summary editor:', error);
    }
  }

  /**
   * Initialize Collection (JSON) editor with CodeMirror
   */
  async initCollectionEditor(initialContent = {}) {
    const container = this.manager.elements.collectionEditorContainer;
    if (!container || this.collectionEditor) return;

    let data = initialContent;
    if (typeof data === 'string') {
      try { data = JSON.parse(data); } catch (e) { data = {}; }
    }

    try {
      const jsonStr = formatSortedJson(data, 2);

      this._jsonReadOnlyCompartment = createReadOnlyCompartment();

      const extensions = buildEditorExtensions({
        language: 'json',
        readOnlyCompartment: this._jsonReadOnlyCompartment,
        readOnly: !this.manager.queryTable?.isEditing,
        fontSize: 13,
        onUpdate: (update) => {
          if (update.docChanged && this.manager.queryTable?.isEditing) {
            this.manager.editHelper.checkDirtyState();
          }
          if (update.transactions.length > 0) {
            this._updateUndoRedoButtons();
          }
        },
      });

      const startState = EditorState.create({ doc: jsonStr, extensions });

      container.innerHTML = '';
      this.collectionEditor = new EditorView({
        state: startState,
        parent: container,
      });

      // Store references on container for compatibility
      container._cmView = this.collectionEditor;
      container._cmReadOnlyCompartment = this._jsonReadOnlyCompartment;

      // Set initial visual state
      const isEditing = this.manager.queryTable?.isEditing || false;
      setEditorEditable(this.collectionEditor, this._jsonReadOnlyCompartment, isEditing, container);

      // Double-click to enter edit mode (same behavior as SQL editor)
      container.addEventListener('dblclick', () => {
        if (!this.manager.queryTable?.isEditing && this.manager.queryTable?.table) {
          const selected = this.manager.queryTable.table.getSelectedRows();
          if (selected.length > 0) void this.manager.queryTable.enterEditMode(selected[0]);
        }
      });

    } catch (error) {
      console.error('[QueriesManager] Failed to initialize JSON editor:', error);
    }
  }

  /**
   * Render markdown preview
   */
  async renderMarkdownPreview() {
    const container = this.manager.elements.previewContainer;
    if (!container) return;

    const markdownContent = this.summaryEditor?.state.doc.toString()
      || this.manager._pendingSummaryContent
      || this.manager.currentQuery?.summary
      || '';

    if (!markdownContent.trim()) {
      container.innerHTML = '<p class="text-muted">No summary content to preview.</p>';
      return;
    }

    try {
      const { marked } = await import('marked');
      const htmlContent = await marked.parse(markdownContent);
      container.innerHTML = `<div class="queries-preview-content">${htmlContent}</div>`;
    } catch (error) {
      console.error('[QueriesManager] Failed to render markdown:', error);
    }
  }

  /**
   * Set CodeMirror editors editable state.
   * Uses the shared setEditorEditable utility for consistent behavior
   * (compartment reconfigure + contentEditable + CSS class toggle).
   */
  _setCodeMirrorEditable(editable) {
    if (this.sqlEditor && this._sqlReadOnlyCompartment) {
      setEditorEditable(
        this.sqlEditor,
        this._sqlReadOnlyCompartment,
        editable,
        this.manager.elements.sqlEditorContainer,
      );
    }

    if (this.summaryEditor && this._summaryReadOnlyCompartment) {
      setEditorEditable(
        this.summaryEditor,
        this._summaryReadOnlyCompartment,
        editable,
        this.manager.elements.summaryEditorContainer,
      );
    }

    this._setJsonEditorEditable(editable);
  }

  /**
   * Set JSON editor editable state.
   * Uses the shared setEditorEditable utility.
   */
  _setJsonEditorEditable(editable) {
    const container = this.manager.elements.collectionEditorContainer;
    if (!container?._cmView) return;

    const view = container._cmView;
    const compartment = container._cmReadOnlyCompartment;
    if (!compartment) return;

    setEditorEditable(view, compartment, editable, container);
    container._cmReadOnly = !editable;
  }

  /**
   * Reset the collection editor (destroy tree) so it can be cleanly
   * reinitialized with new data via initCollectionEditor.
   */
  resetCollectionEditor() {
    if (this.collectionEditor) {
      this.collectionEditor.destroy();
      this.collectionEditor = null;
    }
  }

  /**
   * Destroy all editors
   */
  destroy() {
    if (this.sqlEditor) {
      this.sqlEditor.destroy();
      this.sqlEditor = null;
    }

    if (this.summaryEditor) {
      this.summaryEditor.destroy();
      this.summaryEditor = null;
    }

    if (this.collectionEditor) {
      this.collectionEditor.destroy();
      this.collectionEditor = null;
    }
  }

  /**
   * Handle undo action — operates on the active tab's editor.
   */
  handleUndo() {
    const view = this._getActiveEditorView();
    if (view) undo(view);
  }

  /**
   * Handle redo action — operates on the active tab's editor.
   */
  handleRedo() {
    const view = this._getActiveEditorView();
    if (view) redo(view);
  }

  // ── Undo/Redo Button State ─────────────────────────────────────────────

  /**
   * Update the enabled/disabled state of the undo/redo toolbar buttons
   * based on the active editor's history depth and whether we're in edit mode.
   * Delegates to the shared `updateUndoRedoButtons()` from codemirror-setup.
   *
   * Called from:
   *   - Each editor's onUpdate listener (after every transaction)
   *   - editHelper.onAfterEditModeChange (entering/exiting edit mode)
   *   - switchTab (active editor may have different history)
   */
  _updateUndoRedoButtons() {
    updateUndoRedoButtons({
      undoBtn: this.manager.elements?.undoBtn,
      redoBtn: this.manager.elements?.redoBtn,
      view: this._getActiveEditorView(),
      isEditing: this.manager.queryTable?.isEditing || false,
    });
  }

  /**
   * Fold all regions in the active editor
   */
  handleFoldAll() {
    const view = this._getActiveEditorView();
    if (view) foldAllInEditor(view);
  }

  /**
   * Unfold all regions in the active editor
   */
  handleUnfoldAll() {
    const view = this._getActiveEditorView();
    if (view) unfoldAllInEditor(view);
  }

  /**
   * Replace the full content of a CodeMirror editor WITHOUT adding to
   * the undo history.  Delegates to the shared `setEditorContentNoHistory()`.
   *
   * @param {EditorView} view     - The CodeMirror EditorView
   * @param {string}     content  - New document content
   */
  _setContentNoHistory(view, content) {
    setEditorContentNoHistory(view, content);
  }

  /**
   * Get the currently active editor view based on active tab
   */
  _getActiveEditorView() {
    const activeTab = this.manager._getActiveTabId?.() || 'sql';
    switch (activeTab) {
      case 'sql': return this.sqlEditor;
      case 'summary': return this.summaryEditor;
      case 'collection': return this.collectionEditor;
      default: return this.sqlEditor;
    }
  }

  /**
   * Handle SQL prettify action
   */
  async handlePrettify() {
    if (!this.sqlEditor) return;

    if (!this.manager.queryTable?.isEditing && this.manager.queryTable?.table) {
      const selected = this.manager.queryTable.table.getSelectedRows();
      if (selected.length > 0) await this.manager.queryTable.enterEditMode(selected[0]);
    }

    const currentContent = this.sqlEditor.state.doc.toString();
    if (!currentContent.trim()) return;

    try {
      const { format } = await import('sql-formatter');

      const formatted = format(currentContent, {
        language: this.sqlDialect,
        tabWidth: 2,
        keywordCase: 'upper',
      });

      this.sqlEditor.dispatch({
        changes: { from: 0, to: this.sqlEditor.state.doc.length, insert: formatted },
      });

      log(Subsystems.CONDUIT, Status.INFO, 'SQL formatted successfully');
    } catch (error) {
      console.error('[QueriesManager] SQL format error:', error);
      const { toast } = await import('../../shared/toast.js');
      toast.error('SQL Format Failed', {
        description: error.message || 'Unknown formatting error',
        subsystem: 'Query',
        duration: 8000,
      });
    }
  }

  /**
   * Apply font settings to editors
   */
  applyFontSettings(settings) {
    if (settings) {
      this.fontSettings = { ...this.fontSettings, ...settings };
    }

    if (!this.sqlEditor) return;

    const editorElement = this.sqlEditor.dom;
    if (editorElement) {
      editorElement.style.fontSize = `${this.fontSettings.size}px`;
      editorElement.style.fontFamily = this.fontSettings.family;
      editorElement.style.fontWeight = this.fontSettings.bold ? 'bold' : 'normal';
      editorElement.style.fontStyle = this.fontSettings.italic ? 'italic' : 'normal';
    }

    log(Subsystems.MANAGER, Status.INFO, `Font updated: ${this.fontSettings.size}px ${this.fontSettings.family}`);
  }

  /**
   * Apply font settings to the active tab's content
   * @param {Object} fontSettings - Font settings with fontSize, fontFamily, fontWeight
   * @param {string} tabId - The active tab ID ('sql', 'summary', 'preview')
   */
  applyFontSettingsToActiveTab(fontSettings, tabId) {
    const fontSize = parseInt(String(fontSettings.fontSize), 10) || DEFAULT_FONT_SIZE;
    const fontFamily = fontSettings.fontFamily || DEFAULT_FONT_FAMILY;
    const fontWeight = fontSettings.fontWeight || 'normal';

    switch (tabId) {
      case 'sql':
        this._applyFontToEditor(this.sqlEditor, fontSize, fontFamily, fontWeight);
        break;
      case 'summary':
        this._applyFontToEditor(this.summaryEditor, fontSize, fontFamily, fontWeight);
        break;
      case 'preview':
        this._applyFontToPreviewContainer(fontSize, fontFamily, fontWeight);
        break;
      case 'collection':
        this._applyFontToEditor(this.collectionEditor, fontSize, fontFamily, fontWeight, 13);
        break;
    }
  }

  /**
   * Apply font styles to a CodeMirror editor with full redraw
   */
  _applyFontToEditor(editor, fontSize, fontFamily, fontWeight, baseFontSize = DEFAULT_FONT_SIZE) {
    if (!editor?.dom) return;

    const editorElement = editor.dom;
    editorElement.style.fontSize = `${fontSize}px`;
    editorElement.style.fontFamily = fontFamily;
    editorElement.style.fontWeight = fontWeight;
    editorElement.style.fontStyle = 'normal';

    // Force CodeMirror to re-measure and redraw to sync gutter
    editor.requestMeasure();
  }

  /**
   * Apply font styles to the preview container (HTML content)
   */
  _applyFontToPreviewContainer(fontSize, fontFamily, fontWeight) {
    const container = this.manager.elements.previewContainer;
    if (!container) return;

    const previewContent = container.querySelector('.queries-preview-content');
    if (previewContent) {
      previewContent.style.fontSize = `${fontSize}px`;
      previewContent.style.fontFamily = fontFamily;
      previewContent.style.fontWeight = fontWeight;
    }
  }

  /**
   * Clear font override from an editor, reverting to theme defaults
   */
  clearFontOverride(tabId) {
    if (tabId === 'preview') {
      const container = this.manager.elements.previewContainer;
      if (container) {
        const previewContent = container.querySelector('.queries-preview-content');
        if (previewContent) {
          previewContent.style.fontSize = '';
          previewContent.style.fontFamily = '';
          previewContent.style.fontWeight = '';
        }
      }
      return;
    }

    let editor = null;
    switch (tabId) {
      case 'sql':
        editor = this.sqlEditor;
        break;
      case 'summary':
        editor = this.summaryEditor;
        break;
      case 'collection':
        editor = this.collectionEditor;
        break;
    }

    if (editor?.dom) {
      editor.dom.style.fontSize = '';
      editor.dom.style.fontFamily = '';
      editor.dom.style.fontWeight = '';
      editor.dom.style.fontStyle = '';
      // Force CodeMirror to re-measure
      editor.requestMeasure();
    }
  }

  /**
   * Get current SQL content
   */
  getSqlContent() {
    return this.sqlEditor?.state.doc.toString() || '';
  }

  /**
   * Get current summary content
   */
  getSummaryContent() {
    return this.summaryEditor?.state.doc.toString() || '';
  }

  /**
   * Get current collection content.
   * NOTE: Do NOT blur the focused element here — this method is called from
   * editHelper.checkDirtyState() on every keystroke.  Blurring would steal
   * focus from whichever CodeMirror editor the user is actively typing in.
   * The old blur() was needed for a legacy JSON-tree inline editor; with
   * CodeMirror the state is always up-to-date in EditorState.doc.
   */
  getCollectionContent() {
    // Get content directly from CodeMirror editor
    if (this.collectionEditor) {
      return this.collectionEditor.state.doc.toString();
    }
    return '{}';
  }
}

/**
 * Create an editor manager for the manager
 */
export function createEditorManager(manager) {
  return new EditorManager(manager);
}
