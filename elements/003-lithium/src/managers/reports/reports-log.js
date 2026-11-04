/**
 * Generation log for the last Reports.Render run.
 * Compose and Preview HTML both return { level, message } entries.
 * The Log tab shows that array as aligned columns in CodeMirror.
 * The counts are session state. They are not columns on reports.
 */

import './reports-log.css';
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
  setEditorContentNoHistory,
  setEditorEditable,
} from '../../core/codemirror-setup.js';
import { processIcons } from '../../core/icons.js';
import { LithiumEditorFooter } from '../../core/manager-ui.js';

const LEVELS = ['info', 'warn', 'error'];
const FONT = '"Vanadium Mono", var(--font-mono, monospace)';
const GAP = '  ';

const padLeft = (value, width) => {
  const text = String(value);
  return text.length >= width ? text : `${' '.repeat(width - text.length)}${text}`;
};

const padRight = (value, width) => {
  const text = String(value);
  return text.length >= width ? text : `${text}${' '.repeat(width - text.length)}`;
};

/**
 * Entries the script returned. A non-array, including {}, is empty.
 * @param {unknown} log
 * @returns {Array<{ level: string, message: string }>}
 */
export function generationEntries(log) {
  if (!Array.isArray(log)) return [];
  const entries = [];
  for (const entry of log) {
    if (!entry || typeof entry !== 'object') continue;
    entries.push({
      level: typeof entry.level === 'string' ? entry.level : '',
      message: entry.message == null ? '' : String(entry.message),
    });
  }
  return entries;
}

/**
 * @param {Array<{ level: string }>} entries
 * @returns {{ info: number, warn: number, error: number }}
 */
export function generationCounts(entries) {
  const counts = { info: 0, warn: 0, error: 0 };
  for (const entry of entries) {
    if (Object.hasOwn(counts, entry.level)) counts[entry.level] += 1;
  }
  return counts;
}

/**
 * Worst level. An empty log has no flag.
 * @param {{ info: number, warn: number, error: number }} counts
 * @returns {''|'info'|'warn'|'error'}
 */
export function generationFlag(counts) {
  if (counts.error) return 'error';
  if (counts.warn) return 'warn';
  if (counts.info) return 'info';
  return '';
}

/**
 * Aligned text. Counts are right-aligned under info, warn, and error.
 * The flag column is present only when a level fired. Messages share
 * one column, and a newline in a message stays in that column.
 * @param {unknown} log
 * @returns {string}
 */
export function generationText(log) {
  const entries = generationEntries(log);
  const counts = generationCounts(entries);
  const flag = generationFlag(counts);
  const labels = [...LEVELS];
  const values = LEVELS.map((level) => counts[level]);
  if (flag) {
    labels.push('flag');
    values.push(flag);
  }
  const widths = labels.map((label, index) => Math.max(label.length, String(values[index]).length));
  const head = labels.map((label, index) => padRight(label, widths[index])).join(GAP);
  const nums = values.map((value, index) => (
    index < LEVELS.length ? padLeft(value, widths[index]) : padRight(value, widths[index])
  )).join(GAP);
  const levelWidth = Math.max('level'.length, ...entries.map((entry) => entry.level.length));
  const indent = ' '.repeat(levelWidth + GAP.length);
  const rows = [padRight('level', levelWidth) + GAP + 'message'];
  for (const entry of entries) {
    const lines = entry.message.split('\n');
    rows.push(padRight(entry.level, levelWidth) + GAP + (lines[0] ?? ''));
    for (const extra of lines.slice(1)) rows.push(indent + extra);
  }
  return `${head}\n${nums}\n\n${rows.join('\n')}\n`;
}

/**
 * Read-only CodeMirror page. Same host, footer, and font as Definition.
 */
export class GenerationLogPage {
  /**
   * @param {HTMLElement} host
   * @param {HTMLElement} footerEl
   */
  constructor(host, footerEl) {
    this.host = host;
    this.footerEl = footerEl;
    this.view = null;
    this.footer = null;
    this._compartments = null;
  }

  ensure() {
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
      language: 'plain',
      readOnly: true,
      readOnlyCompartment: compartments.readOnly,
      wordWrapCompartment: compartments.wordWrap,
      bracketMatchCompartment: compartments.bracketMatch,
      selectionHighlightCompartment: compartments.selectionHighlight,
      commentContinuationCompartment: compartments.commentContinuation,
      whitespaceCompartment: compartments.whitespace,
      virtualColumnsCompartment: compartments.virtualColumns,
      indentUnitCompartment: compartments.indentUnit,
      fontSize: 13,
      fontFamily: FONT,
      wordWrap: false,
      bracketMatch: false,
      selectionHighlight: true,
      commentContinuation: false,
      showWhitespace: false,
      onUpdate: (update) => {
        if (update.selectionSet) requestAnimationFrame(() => this.footer?.updateCursorPosition());
      },
    });
    this.host.innerHTML = '';
    this.view = new EditorView({
      state: EditorState.create({ doc: generationText([]), extensions }),
      parent: this.host,
    });
    setEditorEditable(this.view, compartments.readOnly, false, this.host);
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
        initialBracketMatch: false,
        initialSelectionHighlight: true,
        initialCommentContinuation: false,
        initialWhitespace: false,
        initialVirtualColumns: true,
      });
      this.footer.init();
      processIcons(this.footerEl);
    }
  }

  /**
   * @param {unknown} log
   */
  open(log) {
    this.ensure();
    if (!this.view) return;
    const text = generationText(log);
    if (this.view.state.doc.toString() !== text) setEditorContentNoHistory(this.view, text);
  }

  destroy() {
    if (this.footer) this.footer.destroy();
    this.footer = null;
    if (this.view) this.view.destroy();
    this.view = null;
  }
}

export const reportLogMethods = {
  _bindLog() {
    const tab = this.elements.logTab;
    if (!tab || !this.elements.logHost || !this.elements.logFooter) return false;
    this._onLogTab = () => this.showPage('log');
    tab.addEventListener('click', this._onLogTab);
    return true;
  },

  _unbindLog() {
    this.elements.logTab?.removeEventListener('click', this._onLogTab);
    if (this._log) this._log.destroy();
    this._log = null;
  },

  _openLog() {
    if (!this.elements.logHost) return;
    if (!this._log) this._log = new GenerationLogPage(this.elements.logHost, this.elements.logFooter);
    this._log.open(this._generationLog || []);
    this._paintGenerationFlag();
  },

  _paintGenerationFlag() {
    const tab = this.elements && this.elements.logTab;
    if (!tab) return;
    const entries = generationEntries(this._generationLog);
    const counts = generationCounts(entries);
    const flag = generationFlag(counts);
    if (flag) tab.dataset.generationFlag = flag;
    else delete tab.dataset.generationFlag;
    const numbers = `${counts.info} info, ${counts.warn} warn, ${counts.error} error`;
    tab.title = flag ? `Log: ${numbers}, ${flag}` : `Log: ${numbers}`;
  },

  /**
   * Keep the last successful Run. Compose and Preview HTML both call this.
   * @param {unknown} log
   */
  showGeneration(log) {
    this._generationLog = generationEntries(log);
    this._generationReportId = this._openReportId || 0;
    this._paintGenerationFlag();
    if (this._log) this._log.open(this._generationLog);
  },

  /** A different report has no generation yet. */
  clearGeneration() {
    this._generationLog = [];
    this._generationReportId = null;
    this._paintGenerationFlag();
    if (this._log) this._log.open([]);
  },
};
