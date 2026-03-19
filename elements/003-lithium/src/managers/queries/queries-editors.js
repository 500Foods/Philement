/**
 * Queries Manager - Editors Module
 *
 * Handles SQL, Summary (Markdown), and Collection (JSON) editor initialization.
 */

import { log, Subsystems, Status } from '../../core/log.js';
import { initJsonTree, getJsonTreeData, setJsonTreeData, destroyJsonTree } from '../../components/json-tree-component.js';

// Constants
const MIN_FONT_SIZE = 10;
const MAX_FONT_SIZE = 24;
const DEFAULT_FONT_SIZE = 14;
const DEFAULT_FONT_FAMILY = '"Vanadium Mono", var(--font-mono, monospace)';

// History field for CodeMirror undo/redo state
let historyField;

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
    this._collectionChangeInterval = null;
  }

  /**
   * Initialize SQL editor with CodeMirror
   */
  async initSqlEditor(initialContent = '') {
    const container = this.manager.elements.sqlEditorContainer;
    if (!container || this.sqlEditor) return;

    try {
      const [
        { EditorState },
        { EditorView, lineNumbers, highlightActiveLineGutter, highlightSpecialChars, drawSelection, highlightActiveLine, keymap },
        { defaultKeymap, history, historyKeymap },
        { sql },
        { oneDark },
      ] = await Promise.all([
        import('@codemirror/state'),
        import('@codemirror/view'),
        import('@codemirror/commands'),
        import('@codemirror/lang-sql'),
        import('@codemirror/theme-one-dark'),
      ]);

      const historyModule = await import('@codemirror/commands');
      historyField = historyModule.historyField;

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

      const contentEl = this.sqlEditor.dom.querySelector('.cm-content');
      if (contentEl) contentEl.contentEditable = 'false';
      container?.classList.add('queries-cm-readonly');

      container.addEventListener('dblclick', () => {
        if (!this.manager.editModeManager.isEditing() && this.manager.table) {
          const selected = this.manager.table.getSelectedRows();
          if (selected.length > 0) void this.manager.editModeManager.enterEditMode(selected[0]);
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
      const [
        { EditorState },
        { EditorView, lineNumbers, highlightActiveLineGutter, highlightSpecialChars, drawSelection, highlightActiveLine, keymap },
        { defaultKeymap, history, historyKeymap },
        { markdown },
        { oneDark },
      ] = await Promise.all([
        import('@codemirror/state'),
        import('@codemirror/view'),
        import('@codemirror/commands'),
        import('@codemirror/lang-markdown'),
        import('@codemirror/theme-one-dark'),
      ]);

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

      const contentEl = this.summaryEditor.dom.querySelector('.cm-content');
      if (contentEl) contentEl.contentEditable = 'false';
      container?.classList.add('queries-summary-readonly');
    } catch (error) {
      console.error('[QueriesManager] Failed to initialize Summary editor:', error);
    }
  }

  /**
   * Initialize Collection (JSON) editor with JsonTree.js
   */
  async initCollectionEditor(initialContent = {}) {
    const container = this.manager.elements.collectionEditorContainer;
    if (!container || this.collectionEditor) return;

    let data = initialContent;
    if (typeof data === 'string') {
      try { data = JSON.parse(data); } catch (e) { data = {}; }
    }

    try {
      this.collectionEditor = await initJsonTree({
        target: container,
        data: data,
        readOnly: !this.manager.editModeManager.isEditing(),
      });

      const checkDirty = () => {
        try {
          const currentData = getJsonTreeData(container);
          const originalStr = typeof this.manager.dirtyTracker._originalCollectionContent === 'string'
            ? this.manager.dirtyTracker._originalCollectionContent
            : JSON.stringify(this.manager.dirtyTracker._originalCollectionContent || {});
          const isDirty = JSON.stringify(currentData) !== originalStr;
          this.manager.dirtyTracker.setDirty('collection', isDirty);
        } catch {}
      };

      this._collectionChangeInterval = setInterval(checkDirty, 500);

    } catch (error) {
      console.error('[QueriesManager] Failed to initialize JsonTree:', error);
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

    this._setJsonTreeEditable(editable);
  }

  /**
   * Set JsonTree editor editable state
   */
  async _setJsonTreeEditable(editable) {
    if (!this.collectionEditor) return;
    const container = this.manager.elements.collectionEditorContainer;

    try {
      const currentData = getJsonTreeData(container);
      destroyJsonTree(container);

      this.collectionEditor = await initJsonTree({
        target: container,
        data: currentData,
        readOnly: !editable,
      });
    } catch {}

    container?.classList.toggle('queries-jsoneditor-readonly', !editable);
  }

  /**
   * Destroy all editors
   */
  destroy() {
    if (this._collectionChangeInterval) {
      clearInterval(this._collectionChangeInterval);
      this._collectionChangeInterval = null;
    }

    if (this.sqlEditor) {
      this.sqlEditor.destroy();
      this.sqlEditor = null;
    }

    if (this.summaryEditor) {
      this.summaryEditor.destroy();
      this.summaryEditor = null;
    }

    if (this.collectionEditor) {
      destroyJsonTree(this.manager.elements.collectionEditorContainer);
      this.collectionEditor = null;
    }
  }

  /**
   * Handle undo action
   */
  handleUndo() {
    if (this.sqlEditor) {
      import('@codemirror/commands').then(({ undo }) => undo(this.sqlEditor));
    }
  }

  /**
   * Handle redo action
   */
  handleRedo() {
    if (this.sqlEditor) {
      import('@codemirror/commands').then(({ redo }) => redo(this.sqlEditor));
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
    const data = getJsonTreeData(this.manager.elements.collectionEditorContainer);
    return data ? JSON.stringify(data) : '{}';
  }
}

/**
 * Create an editor manager for the manager
 */
export function createEditorManager(manager) {
  return new EditorManager(manager);
}
