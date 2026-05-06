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

import { log, Subsystems, Status } from '../../core/log.js';
import {
  buildEditorExtensions,
  createReadOnlyCompartment,
  createWordWrapCompartment,
  createBracketMatchCompartment,
  createSelectionHighlightCompartment,
  createCommentContinuationCompartment,
  createWhitespaceCompartment,
  createVirtualColumnsCompartment,
  createIndentUnitCompartment,
  setEditorContentNoHistory,
  setEditorEditable,
  updateUndoRedoButtons,
  foldAllInEditor,
  unfoldAllInEditor,
  formatSortedJson,
  compareJsonIgnoringKeyOrder,
} from '../../core/codemirror-setup.js';
import { LithiumEditorFooter } from '../../core/manager-ui.js';
import { EditorState, EditorView, undo, redo } from '../../core/codemirror.js';

/**
 * CollectionTabHandler class
 * Manages the CodeMirror JSON editor for the Collection tab
 */
export class CollectionTabHandler {
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

export default CollectionTabHandler;
