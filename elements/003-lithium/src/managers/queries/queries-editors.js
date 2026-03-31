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
  foldAllInEditor,
  unfoldAllInEditor,
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
          if (update.docChanged && this.manager.editModeManager?.isEditing()) {
            const isDirty = this.manager.dirtyTracker.checkSqlDirty();
            this.manager.dirtyTracker.setDirty('sql', isDirty);
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
          if (update.docChanged && this.manager.editModeManager?.isEditing()) {
            const isDirty = this.manager.dirtyTracker.checkSummaryDirty();
            this.manager.dirtyTracker.setDirty('summary', isDirty);
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
      const jsonStr = JSON.stringify(data, null, 2);

      this._jsonReadOnlyCompartment = createReadOnlyCompartment();

      const extensions = buildEditorExtensions({
        language: 'json',
        readOnlyCompartment: this._jsonReadOnlyCompartment,
        readOnly: !this.manager.queryTable?.isEditing,
        fontSize: 13,
        onUpdate: (update) => {
          if (update.docChanged && this.manager.editModeManager?.isEditing()) {
            const isDirty = this.manager.dirtyTracker.checkCollectionDirty();
            this.manager.dirtyTracker.setDirty('collection', isDirty);
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
   * Handle undo action
   */
  handleUndo() {
    if (this.sqlEditor) {
      undo(this.sqlEditor);
    }
  }

  /**
   * Handle redo action
   */
  handleRedo() {
    if (this.sqlEditor) {
      redo(this.sqlEditor);
    }
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

    if (!this.manager.editModeManager.isEditing() && this.manager.table) {
      const selected = this.manager.table.getSelectedRows();
      if (selected.length > 0) await this.manager.editModeManager.enterEditMode(selected[0]);
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
   * Get current collection content
   */
  getCollectionContent() {
    const container = this.manager.elements.collectionEditorContainer;

    // Force-commit any active inline edit before reading
    if (container) {
      const activeEl = container.querySelector(':focus');
      if (activeEl) activeEl.blur();
    }

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
