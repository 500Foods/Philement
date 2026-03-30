/**
 * Queries Manager - Editors Module
 *
 * Handles SQL, Summary (Markdown), and Collection (JSON) editor initialization.
 * All CodeMirror imports come from the centralized core/codemirror.js module
 * to prevent "multiple instances" errors.
 */

import { log, Subsystems, Status } from '../../core/log.js';
import {
  EditorState,
  Compartment,
  EditorView,
  keymap,
  lineNumbers,
  highlightActiveLineGutter,
  highlightSpecialChars,
  drawSelection,
  highlightActiveLine,
  defaultKeymap,
  history,
  historyKeymap,
  historyField,
  sql,
  markdown,
  json,
  oneDark,
  undo,
  redo
} from '../../core/codemirror.js';

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
      const startState = EditorState.create({
        doc: initialContent,
        extensions: [
          lineNumbers({ formatNumber: (n) => String(n).padStart(4, '0') }),
          highlightActiveLineGutter(),
          highlightSpecialChars(),
          history(),
          drawSelection(),
          highlightActiveLine(),
          keymap.of([...defaultKeymap, ...historyKeymap]),
          sql(),
          oneDark,
          EditorView.theme({
            '&': { height: '100%', fontSize: `${this.fontSettings.size}px` },
            '.cm-scroller': { overflow: 'auto' },
            '.cm-content': { fontFamily: '"Vanadium Mono", var(--font-mono, monospace)' },
          }),
          EditorView.updateListener.of((update) => {
            if (update.docChanged) {
              const isDirty = this.manager.dirtyTracker.checkSqlDirty();
              this.manager.dirtyTracker.setDirty('sql', isDirty);
            }
          }),
        ],
      });

      this.sqlEditor = new EditorView({
        state: startState,
        parent: container,
      });

      // Check if we're currently in edit mode and set state accordingly
      const isEditing = this.manager.queryTable?.isEditing || false;
      const contentEl = this.sqlEditor.dom.querySelector('.cm-content');
      if (contentEl) contentEl.contentEditable = isEditing ? 'true' : 'false';
      container?.classList.toggle('queries-cm-readonly', !isEditing);

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
      const startState = EditorState.create({
        doc: initialContent,
        extensions: [
          lineNumbers(),
          highlightActiveLineGutter(),
          highlightSpecialChars(),
          history(),
          drawSelection(),
          highlightActiveLine(),
          keymap.of([...defaultKeymap, ...historyKeymap]),
          markdown(),
          oneDark,
          EditorView.theme({ '&': { height: '100%' }, '.cm-scroller': { overflow: 'auto' } }),
          EditorView.updateListener.of((update) => {
            if (update.docChanged) {
              const isDirty = this.manager.dirtyTracker.checkSummaryDirty();
              this.manager.dirtyTracker.setDirty('summary', isDirty);
            }
          }),
        ],
      });

      this.summaryEditor = new EditorView({
        state: startState,
        parent: container,
      });

      // Check if we're currently in edit mode and set state accordingly
      const isEditing = this.manager.queryTable?.isEditing || false;
      const contentEl = this.summaryEditor.dom.querySelector('.cm-content');
      if (contentEl) contentEl.contentEditable = isEditing ? 'true' : 'false';
      container?.classList.toggle('queries-summary-readonly', !isEditing);

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
      
      // Create a Compartment for readonly state management
      this._jsonReadOnlyCompartment = new Compartment();
      
      const startState = EditorState.create({
        doc: jsonStr,
        extensions: [
          json(),
          oneDark,
          history(),
          keymap.of([...defaultKeymap, ...historyKeymap]),
          this._jsonReadOnlyCompartment.of(EditorState.readOnly.of(!this.manager.queryTable?.isEditing)),
          EditorView.theme({
            '&': { height: '100%', fontSize: '13px' },
            '.cm-scroller': { overflow: 'auto', fontFamily: 'var(--font-mono, monospace)' },
            '.cm-content': { caretColor: 'var(--accent-primary, #007acc)' },
          }),
          EditorView.updateListener.of((update) => {
            if (update.docChanged && this.manager.queryTable?.isEditing) {
              // Mark dirty when content changes in edit mode
              this.manager.queryTable.isDirty = true;
              this.manager.queryTable.updateSaveCancelButtonState();
              this.manager.queryTable.notifyDirtyChange();
            }
          }),
        ],
      });

      container.innerHTML = '';
      this.collectionEditor = new EditorView({
        state: startState,
        parent: container,
      });

      // Store references on container for compatibility
      container._cmView = this.collectionEditor;
      container._cmReadOnlyCompartment = this._jsonReadOnlyCompartment;

      // Check if we're currently in edit mode and set state accordingly
      const isEditing = this.manager.queryTable?.isEditing || false;
      const contentEl = this.collectionEditor.dom.querySelector('.cm-content');
      if (contentEl) contentEl.contentEditable = isEditing ? 'true' : 'false';
      container?.classList.toggle('queries-json-readonly', !isEditing);

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
   * Set CodeMirror editors editable state
   */
  _setCodeMirrorEditable(editable) {
    const editors = [
      { view: this.sqlEditor, container: this.manager.elements.sqlEditorContainer, readonlyClass: 'queries-cm-readonly' },
      { view: this.summaryEditor, container: this.manager.elements.summaryEditorContainer, readonlyClass: 'queries-summary-readonly' },
    ];

    for (const { view, container, readonlyClass } of editors) {
      if (!view) continue;
      const contentEl = view.dom.querySelector('.cm-content');
      if (contentEl) contentEl.contentEditable = editable ? 'true' : 'false';
      container?.classList.toggle(readonlyClass, !editable);
    }

    this._setJsonEditorEditable(editable);
  }

  /**
   * Set JSON editor editable state.
   * Uses the Compartment stored on the container to reconfigure readOnly
   * via a dispatch effect — no state recreation needed.
   */
  _setJsonEditorEditable(editable) {
    const container = this.manager.elements.collectionEditorContainer;
    if (!container?._cmView) return;

    const view = container._cmView;
    const compartment = container._cmReadOnlyCompartment;

    if (!compartment) return;

    const isReadOnly = !editable;

    // Use EditorState from static import (same module instance)
    view.dispatch({
      effects: compartment.reconfigure(EditorState.readOnly.of(isReadOnly)),
    });

    container._cmReadOnly = isReadOnly;
    container?.classList.toggle('queries-json-readonly', isReadOnly);
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
