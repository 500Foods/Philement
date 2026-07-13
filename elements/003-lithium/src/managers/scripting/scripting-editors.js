/**
 * Scripting Manager - Editors Module
 *
 * Handles Lua editor initialization.
 * Uses the shared codemirror-setup.js for consistent configuration.
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
  createWordWrapCompartment,
  createBracketMatchCompartment,
  createSelectionHighlightCompartment,
  createCommentContinuationCompartment,
  createWhitespaceCompartment,
  createVirtualColumnsCompartment,
  createIndentUnitCompartment,
  setEditorEditable,
  setEditorContentNoHistory,
  updateUndoRedoButtons,
  foldAllInEditor,
  unfoldAllInEditor,
  READONLY_CLASS,
} from '../../core/codemirror-setup.js';
import { LithiumEditorFooter } from '../../core/manager-ui.js';

// Constants
const MIN_FONT_SIZE = 10;
const MAX_FONT_SIZE = 24;
const DEFAULT_FONT_SIZE = 14;
const DEFAULT_FONT_FAMILY = '"Vanadium Mono", var(--font-mono, monospace)';

export class EditorManager {
  constructor(manager) {
    this.manager = manager;
    this.luaEditor = null;
    this.fontSettings = {
      size: DEFAULT_FONT_SIZE,
      family: DEFAULT_FONT_FAMILY,
      bold: false,
      italic: false,
    };
  }

  async initLuaEditor(initialContent = '') {
    const container = this.manager.elements.luaEditorContainer;
    if (!container || this.luaEditor) return;

    try {
      this._luaReadOnlyCompartment = createReadOnlyCompartment();
      this._luaWordWrapCompartment = createWordWrapCompartment();
      this._luaBracketMatchCompartment = createBracketMatchCompartment();
      this._luaSelectionHighlightCompartment = createSelectionHighlightCompartment();
      this._luaCommentContinuationCompartment = createCommentContinuationCompartment();
      this._luaWhitespaceCompartment = createWhitespaceCompartment();
      this._luaVirtualColumnsCompartment = createVirtualColumnsCompartment();
      this._luaIndentUnitCompartment = createIndentUnitCompartment();

      const extensions = buildEditorExtensions({
        language: 'lua',
        readOnlyCompartment: this._luaReadOnlyCompartment,
        readOnly: !this.manager.scriptTable?.isEditing,
        fontSize: this.fontSettings.size,
        fontFamily: this.fontSettings.family,
        wordWrapCompartment: this._luaWordWrapCompartment,
        bracketMatchCompartment: this._luaBracketMatchCompartment,
        selectionHighlightCompartment: this._luaSelectionHighlightCompartment,
        commentContinuationCompartment: this._luaCommentContinuationCompartment,
        whitespaceCompartment: this._luaWhitespaceCompartment,
        virtualColumnsCompartment: this._luaVirtualColumnsCompartment,
        indentUnitCompartment: this._luaIndentUnitCompartment,
        wordWrap: false,
        bracketMatch: true,
        selectionHighlight: true,
        commentContinuation: true,
        showWhitespace: false,
        onUpdate: (update) => {
          if (update.selectionSet) {
            requestAnimationFrame(() => this.luaEditorFooter?.updateCursorPosition());
          }
          if (update.docChanged && this.manager.scriptTable?.isEditing) {
            this.manager.editHelper.checkDirtyState();
          }
          if (update.transactions.length > 0) {
            this._updateUndoRedoButtons();
          }
        },
        ...this.manager.editHelper.getCodeMirrorKeymapOptions(),
      });

      const startState = EditorState.create({ doc: initialContent, extensions });

      this.luaEditor = new EditorView({
        state: startState,
        parent: container,
      });

      const luaFooterEl = this.manager.container.querySelector('#scripting-lua-editor-footer');
      this.luaEditorFooter = new LithiumEditorFooter({
        container: luaFooterEl,
        editorView: this.luaEditor,
        wordWrapCompartment: this._luaWordWrapCompartment,
        bracketMatchCompartment: this._luaBracketMatchCompartment,
        selectionHighlightCompartment: this._luaSelectionHighlightCompartment,
        commentContinuationCompartment: this._luaCommentContinuationCompartment,
        whitespaceCompartment: this._luaWhitespaceCompartment,
        indentUnitCompartment: this._luaIndentUnitCompartment,
        virtualColumnsCompartment: this._luaVirtualColumnsCompartment,
        initialWordWrap: false,
        initialBracketMatch: true,
        initialSelectionHighlight: true,
        initialCommentContinuation: true,
        initialWhitespace: false,
        initialVirtualColumns: true,
        storageKey: 'editors.scripting.lua',
      });
      this.luaEditorFooter.init();

      const isEditing = this.manager.scriptTable?.isEditing || false;
      setEditorEditable(this.luaEditor, this._luaReadOnlyCompartment, isEditing, container);

      container.addEventListener('dblclick', () => {
        if (!this.manager.scriptTable?.isEditing && this.manager.scriptTable?.table) {
          const selected = this.manager.scriptTable.table.getSelectedRows();
          if (selected.length > 0) void this.manager.scriptTable.enterEditMode(selected[0]);
        }
      });
    } catch (err) {
      console.error('[ScriptingManager] Failed to initialize CodeMirror:', err);
    }
  }

  _setCodeMirrorEditable(editable) {
    if (this.luaEditor && this._luaReadOnlyCompartment) {
      setEditorEditable(
        this.luaEditor,
        this._luaReadOnlyCompartment,
        editable,
        this.manager.elements.luaEditorContainer,
      );
    }
  }

  destroy() {
    if (this.luaEditor) {
      destroyCodeMirrorScrollbars(this.luaEditor);
    }
    if (this.luaEditorFooter) {
      this.luaEditorFooter.destroy();
      this.luaEditorFooter = null;
    }
    if (this.luaEditor) {
      this.luaEditor.destroy();
      this.luaEditor = null;
    }
  }

  handleUndo() {
    if (this.luaEditor) undo(this.luaEditor);
  }

  handleRedo() {
    if (this.luaEditor) redo(this.luaEditor);
  }

  _updateUndoRedoButtons() {
    updateUndoRedoButtons({
      undoBtn: this.manager.elements?.undoBtn,
      redoBtn: this.manager.elements?.redoBtn,
      view: this.luaEditor,
      isEditing: this.manager.scriptTable?.isEditing || false,
    });
  }

  handleFoldAll() {
    if (this.luaEditor) foldAllInEditor(this.luaEditor);
  }

  handleUnfoldAll() {
    if (this.luaEditor) unfoldAllInEditor(this.luaEditor);
  }

  _setContentNoHistory(view, content) {
    setEditorContentNoHistory(view, content);
  }

  async renderLuaPreview() {
    const container = this.manager.elements.previewContainer;
    if (!container) return;

    const luaContent = this.luaEditor?.state.doc.toString()
      || this.manager._pendingLuaContent
      || '';

    if (!luaContent.trim()) {
      container.innerHTML = '<p class="text-muted">No script content to preview.</p>';
      return;
    }

    container.innerHTML = `<pre class="scripting-preview-content">${escapeHtml(luaContent)}</pre>`;
  }

  applyFontSettings(settings) {
    if (settings) {
      this.fontSettings = { ...this.fontSettings, ...settings };
    }

    if (!this.luaEditor) return;

    const editorElement = this.luaEditor.dom;
    if (editorElement) {
      editorElement.style.fontSize = `${this.fontSettings.size}px`;
      editorElement.style.fontFamily = this.fontSettings.family;
      editorElement.style.fontWeight = this.fontSettings.bold ? 'bold' : 'normal';
      editorElement.style.fontStyle = this.fontSettings.italic ? 'italic' : 'normal';
    }

    log(Subsystems.MANAGER, Status.INFO, `Font updated: ${this.fontSettings.size}px ${this.fontSettings.family}`);
  }

  applyFontSettingsToActiveTab(fontSettings, tabId) {
    const fontSize = parseInt(String(fontSettings.fontSize), 10) || DEFAULT_FONT_SIZE;
    const fontFamily = fontSettings.fontFamily || DEFAULT_FONT_FAMILY;
    const fontWeight = fontSettings.fontWeight || 'normal';

    switch (tabId) {
      case 'lua':
        this._applyFontToEditor(this.luaEditor, fontSize, fontFamily, fontWeight);
        break;
      case 'preview':
        this._applyFontToPreviewContainer(fontSize, fontFamily, fontWeight);
        break;
    }
  }

  _applyFontToEditor(editor, fontSize, fontFamily, fontWeight) {
    if (!editor?.dom) return;

    const editorElement = editor.dom;
    editorElement.style.fontSize = `${fontSize}px`;
    editorElement.style.fontFamily = fontFamily;
    editorElement.style.fontWeight = fontWeight;
    editorElement.style.fontStyle = 'normal';

    editor.requestMeasure();
  }

  _applyFontToPreviewContainer(fontSize, fontFamily, fontWeight) {
    const container = this.manager.elements.previewContainer;
    if (!container) return;

    const previewContent = container.querySelector('.scripting-preview-content');
    if (previewContent) {
      previewContent.style.fontSize = `${fontSize}px`;
      previewContent.style.fontFamily = fontFamily;
      previewContent.style.fontWeight = fontWeight;
    }
  }

  clearFontOverride(tabId) {
    if (tabId === 'preview') {
      const container = this.manager.elements.previewContainer;
      if (container) {
        const previewContent = container.querySelector('.scripting-preview-content');
        if (previewContent) {
          previewContent.style.fontSize = '';
          previewContent.style.fontFamily = '';
          previewContent.style.fontWeight = '';
        }
      }
      return;
    }

    if (this.luaEditor?.dom) {
      this.luaEditor.dom.style.fontSize = '';
      this.luaEditor.dom.style.fontFamily = '';
      this.luaEditor.dom.style.fontWeight = '';
      this.luaEditor.dom.style.fontStyle = '';
      this.luaEditor.requestMeasure();
    }
  }

  getLuaContent() {
    return this.luaEditor?.state.doc.toString() || '';
  }
}

function escapeHtml(text) {
  const div = document.createElement('div');
  div.textContent = text;
  return div.innerHTML;
}

function destroyCodeMirrorScrollbars(view) {
  // Custom scrollbars cleanup if needed
}

export function createEditorManager(manager) {
  return new EditorManager(manager);
}
