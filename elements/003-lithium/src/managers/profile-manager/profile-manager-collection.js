/**
 * Profile Manager - Collection Tab Handler
 *
 * Handles the JSON editor for viewing and editing all profile preferences
 * in a single CodeMirror instance.
 */

import { log, Subsystems, Status } from '../../core/log.js';

/**
 * CollectionTabHandler class
 * Manages the CodeMirror JSON editor for the Collection tab
 */
export class CollectionTabHandler {
  constructor(options) {
    this.container = options.container;
    this.onDirtyChange = options.onDirtyChange || (() => {});
    this.onSave = options.onSave || (() => {});

    this.editor = null;
    this.readOnlyCompartment = null;
    this._initialData = null;
  }

  /**
   * Initialize the Collection JSON editor
   * @param {Object} initialData - Initial preferences data
   */
  async init(initialData = {}) {
    if (!this.container || this.editor) return;

    this._initialData = JSON.parse(JSON.stringify(initialData));

    try {
      const { EditorState, EditorView } = await import('../../core/codemirror.js');
      const {
        buildEditorExtensions,
        createReadOnlyCompartment,
        formatSortedJson,
      } = await import('../../core/codemirror-setup.js');

      this.readOnlyCompartment = createReadOnlyCompartment();

      const jsonStr = formatSortedJson(initialData, 2);

      const extensions = buildEditorExtensions({
        language: 'json',
        readOnlyCompartment: this.readOnlyCompartment,
        readOnly: false,
        fontSize: 13,
        onUpdate: (update) => {
          if (update.docChanged) {
            this.onDirtyChange(this.isDirty());
          }
        },
      });

      const startState = EditorState.create({ doc: jsonStr, extensions });

      this.container.innerHTML = '';
      this.editor = new EditorView({
        state: startState,
        parent: this.container,
      });

      // Store reference for external access
      this.container._cmView = this.editor;

      log(Subsystems.MANAGER, Status.INFO, '[CollectionTab] Editor initialized');
    } catch (error) {
      log(Subsystems.MANAGER, Status.ERROR, '[CollectionTab] Failed to initialize editor:', error);
    }
  }

  /**
   * Check if the editor has unsaved changes
   * @returns {boolean}
   */
  isDirty() {
    if (!this.editor || !this._initialData) return false;
    try {
      const currentContent = this.editor.state.doc.toString();
      const currentData = JSON.parse(currentContent);
      return JSON.stringify(currentData) !== JSON.stringify(this._initialData);
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
   * Update editor content without marking as dirty
   * @param {Object} data - New data to display
   */
  async setData(data) {
    if (!this.editor) {
      await this.init(data);
      return;
    }

    this._initialData = JSON.parse(JSON.stringify(data));

    const { formatSortedJson } = await import('../../core/codemirror-setup.js');
    const jsonStr = formatSortedJson(data, 2);

    this.editor.dispatch({
      changes: {
        from: 0,
        to: this.editor.state.doc.length,
        insert: jsonStr,
      },
    });

    this.onDirtyChange(false);
  }

  /**
   * Handle undo action
   */
  async undo() {
    if (!this.editor) return;
    const { undo } = await import('../../core/codemirror.js');
    undo(this.editor);
    log(Subsystems.MANAGER, Status.DEBUG, '[CollectionTab] Undo');
  }

  /**
   * Handle redo action
   */
  async redo() {
    if (!this.editor) return;
    const { redo } = await import('../../core/codemirror.js');
    redo(this.editor);
    log(Subsystems.MANAGER, Status.DEBUG, '[CollectionTab] Redo');
  }

  /**
   * Fold all JSON blocks
   */
  async foldAll() {
    if (!this.editor) return;
    const { foldAllInEditor } = await import('../../core/codemirror-setup.js');
    foldAllInEditor(this.editor);
    log(Subsystems.MANAGER, Status.DEBUG, '[CollectionTab] Fold all');
  }

  /**
   * Unfold all JSON blocks
   */
  async unfoldAll() {
    if (!this.editor) return;
    const { unfoldAllInEditor } = await import('../../core/codemirror-setup.js');
    unfoldAllInEditor(this.editor);
    log(Subsystems.MANAGER, Status.DEBUG, '[CollectionTab] Unfold all');
  }

  /**
   * Prettify/format the JSON content
   */
  async prettify() {
    if (!this.editor) return;
    try {
      const { formatSortedJson } = await import('../../core/codemirror-setup.js');
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
  async setFontSize(size) {
    if (!this.editor) return;
    const { setEditorFontSize } = await import('../../core/codemirror-setup.js');
    setEditorFontSize(this.editor, size);
  }

  /**
   * Destroy the editor instance
   */
  destroy() {
    this.editor?.destroy();
    this.editor = null;
    this.readOnlyCompartment = null;
    this._initialData = null;
  }
}

export default CollectionTabHandler;
