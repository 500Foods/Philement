/**
 * JSON page for the open report definition.
 * The buffer is the definition with history removed. Folds start
 * collapsed. A parsed edit commits one history step on blur.
 * A parse error leaves the definition alone.
 */

import { EditorState, EditorView } from '../../core/codemirror.js';
import {
  buildEditorExtensions,
  createBracketMatchCompartment,
  createCommentContinuationCompartment,
  createIndentUnitCompartment,
  createReadOnlyCompartment,
  createSelectionHighlightCompartment,
  createVirtualColumnsCompartment,
  createWhitespaceCompartment,
  createWordWrapCompartment,
  foldAllInEditor,
  formatSortedJson,
  setEditorContentNoHistory,
  unfoldAllInEditor,
} from '../../core/codemirror-setup.js';
import { processIcons } from '../../core/icons.js';
import { LithiumEditorFooter } from '../../core/manager-ui.js';
import { importDefinition } from './model/document.js';
import { reportBody } from './model/history.js';

/**
 * Definition text for the JSON page. History is omitted.
 * @param {object} doc
 * @returns {string}
 */
export function reportJsonText(doc) {
  return formatSortedJson(reportBody(doc), 2);
}

/**
 * Parse a JSON-page buffer into a definition.
 * A history key in the buffer is dropped. The caller keeps the trail.
 * @param {string} text
 * @returns {{ doc?: object, error?: string }}
 */
export function reportJsonEdit(text) {
  let parsed;
  try {
    parsed = JSON.parse(text);
  } catch {
    return { error: 'Report JSON could not be read.' };
  }
  if (!parsed || typeof parsed !== 'object' || Array.isArray(parsed)) {
    return { error: 'Report JSON must be an object.' };
  }
  delete parsed.history;
  try {
    return { doc: importDefinition(parsed) };
  } catch (error) {
    const message = error instanceof Error && error.message
      ? error.message
      : 'Report JSON could not be read.';
    return { error: message };
  }
}

/**
 * CodeMirror JSON editor for one report definition.
 */
export class ReportJsonPage {
  /**
   * @param {HTMLElement} host
   * @param {HTMLElement} footerEl
   * @param {() => void} onBlur
   */
  constructor(host, footerEl, onBlur) {
    this.host = host;
    this.footerEl = footerEl;
    this.onBlur = onBlur;
    this.view = null;
    this.footer = null;
    this._compartments = null;
  }

  /**
   * @param {object} doc
   */
  ensure(doc) {
    if (this.view || !this.host) return;
    const compartments = {
      readOnly: createReadOnlyCompartment(),
      wordWrap: createWordWrapCompartment(),
      bracketMatch: createBracketMatchCompartment(),
      selectionHighlight: createSelectionHighlightCompartment(),
      commentContinuation: createCommentContinuationCompartment(),
      whitespace: createWhitespaceCompartment(),
      virtualColumns: createVirtualColumnsCompartment(),
      indentUnit: createIndentUnitCompartment(),
    };
    this._compartments = compartments;
    const extensions = buildEditorExtensions({
      language: 'json',
      readOnly: false,
      readOnlyCompartment: compartments.readOnly,
      wordWrapCompartment: compartments.wordWrap,
      bracketMatchCompartment: compartments.bracketMatch,
      selectionHighlightCompartment: compartments.selectionHighlight,
      commentContinuationCompartment: compartments.commentContinuation,
      whitespaceCompartment: compartments.whitespace,
      virtualColumnsCompartment: compartments.virtualColumns,
      indentUnitCompartment: compartments.indentUnit,
      fontSize: 13,
      wordWrap: false,
      bracketMatch: true,
      selectionHighlight: true,
      commentContinuation: true,
      showWhitespace: false,
      onUpdate: (update) => {
        if (update.selectionSet) {
          requestAnimationFrame(() => this.footer?.updateCursorPosition());
        }
      },
    });
    extensions.push(EditorView.domEventHandlers({
      blur: (event, view) => {
        const next = event.relatedTarget;
        if (next && view.dom.contains(next)) return;
        this.onBlur();
      },
    }));
    this.host.innerHTML = '';
    this.view = new EditorView({
      state: EditorState.create({ doc: reportJsonText(doc), extensions }),
      parent: this.host,
    });
    if (this.footerEl) {
      this.footer = new LithiumEditorFooter({
        container: this.footerEl,
        editorView: this.view,
        wordWrapCompartment: compartments.wordWrap,
        bracketMatchCompartment: compartments.bracketMatch,
        selectionHighlightCompartment: compartments.selectionHighlight,
        commentContinuationCompartment: compartments.commentContinuation,
        whitespaceCompartment: compartments.whitespace,
        indentUnitCompartment: compartments.indentUnit,
        virtualColumnsCompartment: compartments.virtualColumns,
        initialWordWrap: false,
        initialBracketMatch: true,
        initialSelectionHighlight: true,
        initialCommentContinuation: true,
        initialWhitespace: false,
        initialVirtualColumns: true,
      });
      this.footer.init();
      processIcons(this.footerEl);
    }
    foldAllInEditor(this.view);
  }

  /**
   * Show the definition. A held buffer is the text the author has
   * not successfully parsed, so it stays. Folds start collapsed.
   * @param {object} doc
   * @param {boolean} held
   */
  open(doc, held) {
    this.ensure(doc);
    if (!this.view) return;
    if (!held) {
      const text = reportJsonText(doc);
      if (this.view.state.doc.toString() !== text) {
        setEditorContentNoHistory(this.view, text);
      }
    }
    foldAllInEditor(this.view);
  }

  /** @returns {string} */
  text() {
    return this.view ? this.view.state.doc.toString() : '';
  }

  fold() {
    if (this.view) foldAllInEditor(this.view);
  }

  unfold() {
    if (this.view) unfoldAllInEditor(this.view);
  }

  destroy() {
    if (this.footer) this.footer.destroy();
    this.footer = null;
    if (this.view) this.view.destroy();
    this.view = null;
  }
}
