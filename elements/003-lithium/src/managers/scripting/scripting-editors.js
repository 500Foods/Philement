/**
 * Scripting Manager - Editors Module
 *
 * Handles Lua editor initialization.
 * Uses the shared codemirror-setup.js for consistent configuration.
 */

import { log, Subsystems, Status } from '../../core/log.js';
import { toast } from '../../shared/toast.js';
import { formatErrorMessage, formatLuaSource, formatWithStyLua } from './scripting-format.js';
import { buildScriptSummaryHtml, scriptPreviewLua } from './scripting-preview.js';
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

  /**
   * Format the Lua buffer. Enters edit mode when a row is selected,
   * replaces the whole document so undo can restore it, and leaves
   * the buffer unchanged when formatting fails.
   *
   * @param {(source: string) => string | Promise<string>} [formatFn]
   */
  async handlePrettify(formatFn = formatWithStyLua) {
    if (!this.luaEditor || this._prettifyBusy) return;
    this._prettifyBusy = true;
    try {
      const table = this.manager.scriptTable;
      if (table && !table.isEditing) {
        const selected = table.table?.getSelectedRows?.() || [];
        if (selected.length === 0) {
          log(Subsystems.MANAGER, Status.WARN, '[Scripting] Lua format skipped: no script selected');
          toast.warning('Select a script before formatting', {
            subsystem: 'Scripting',
            duration: 4000,
          });
          return;
        }
        await table.enterEditMode(selected[0]);
      }
      if (!this.luaEditor) return;

      const currentContent = this.luaEditor.state.doc.toString();
      const result = await formatLuaSource(currentContent, formatFn);
      if (!this.luaEditor) return;
      if (this.luaEditor.state.doc.toString() !== currentContent) return;

      if (!result.ok) {
        this._reportFormatFailure(result.message);
        return;
      }
      if (result.empty || !result.changed) return;

      this.luaEditor.dispatch({
        changes: {
          from: 0,
          to: this.luaEditor.state.doc.length,
          insert: result.text,
        },
      });
      log(Subsystems.MANAGER, Status.INFO, '[Scripting] Lua formatted');
    } catch (error) {
      this._reportFormatFailure(formatErrorMessage(error));
    } finally {
      this._prettifyBusy = false;
    }
  }

  _reportFormatFailure(message) {
    const description = message || 'Unknown formatting error';
    log(Subsystems.MANAGER, Status.ERROR, `[Scripting] Lua format failed: ${description}`);
    toast.error('Lua Format Failed', {
      description,
      subsystem: 'Scripting',
      duration: 8000,
    });
  }

  _setContentNoHistory(view, content) {
    setEditorContentNoHistory(view, content);
  }

  async renderSummaryPreview() {
    const container = this.manager.elements.previewContainer;
    if (!container) return;

    const summary = this.manager.currentScript?.summary ?? '';
    const built = await buildScriptSummaryHtml(summary);
    const lua = scriptPreviewLua({
      editorText: this.luaEditor ? this.luaEditor.state.doc.toString() : null,
      pending: this.manager._pendingLuaContent,
      code: this.manager.currentScript?.code,
    });

    const preview = document.createElement('div');
    preview.className = 'scripting-preview-content';
    preview.append(previewLabel('Summary'));

    if (built.empty) {
      preview.append(previewNote('No summary to preview.'));
    } else if (built.failed) {
      log(Subsystems.MANAGER, Status.ERROR, `[Scripting] Summary preview failed: ${built.message}`);
      toast.error('Summary preview failed', {
        description: built.message || 'Unknown preview error',
        subsystem: 'Scripting',
        duration: 8000,
      });
      preview.append(previewNote('Summary preview failed.'));
    } else {
      const summaryNode = document.createElement('div');
      summaryNode.className = 'scripting-preview-summary';
      summaryNode.innerHTML = built.html;
      preview.append(summaryNode);
    }

    preview.append(previewLabel('Lua'));
    preview.append(previewLua(lua));
    container.replaceChildren(preview);
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

function previewLabel(text) {
  const label = document.createElement('p');
  label.className = 'scripting-preview-label';
  label.textContent = text;
  return label;
}

function previewNote(message) {
  const note = document.createElement('p');
  note.className = 'text-muted';
  note.textContent = message;
  return note;
}

function previewLua(source) {
  const block = document.createElement('pre');
  block.className = 'scripting-preview-lua';
  if (!String(source).trim()) {
    block.classList.add('text-muted');
    block.textContent = 'No script content to preview.';
    return block;
  }
  block.textContent = source;
  return block;
}

function destroyCodeMirrorScrollbars(_view) {
  // buildEditorExtensions does not install customScrollbars().
  // EditorView.destroy() drops the editor. There is no scrollbar instance.
}

export function createEditorManager(manager) {
  return new EditorManager(manager);
}
