/**
 * Report Manager — Manager ID 24
 *
 * Starts a blank definition, imports a local JSON file, and downloads it.
 * The stage lists that definition and draws one page of bands.
 * Each band body draws the section grid. Dragging a band edge
 * commits through the model and pushes history. Undo and redo
 * restore that snapshot.
 * destroy and cleanup release the same listeners.
 */

import './reports.css';

import { log, Subsystems, Status } from '../../core/log.js';
import { processIcons } from '../../core/icons.js';
import { toast } from '../../shared/toast.js';
import { dragBandHeight } from './bands.js';
import { paintReportPage } from './canvas.js';
import { watchBandGrids } from './grid.js';
import { bandHeight, resizeBand } from './model/document.js';
import { createHistory } from './model/history.js';
import { toCssPx } from './model/units.js';
import {
  blankReport,
  definitionText,
  openReportText,
  reportDisplayName,
  reportDownloadName,
  reportOutline,
  saveReportFile,
} from './reports-file.js';

const TEMPLATE = '/src/managers/reports/reports.html';

function appendFact(list, label, value) {
  const term = document.createElement('dt');
  term.textContent = label;
  const detail = document.createElement('dd');
  detail.textContent = value;
  list.append(term, detail);
}

/** Paint the open definition and its page into the stage. */
export function paintReportStage(stage, doc) {
  const outline = reportOutline(doc);
  const body = document.createElement('div');
  body.className = 'reports-stage-body';
  const canvas = document.createElement('div');
  canvas.className = 'reports-canvas';
  const frame = paintReportPage(canvas, doc);
  body.append(canvas);

  const root = document.createElement('div');
  root.className = 'reports-summary';

  const name = document.createElement('h2');
  name.className = 'reports-summary-name';
  name.id = 'reports-summary-name';
  name.textContent = outline.name;
  root.append(name);

  const fileName = document.createElement('p');
  fileName.className = 'reports-summary-file';
  fileName.textContent = outline.fileName;
  root.append(fileName);

  if (outline.nameOmitted) {
    const note = document.createElement('p');
    note.className = 'reports-summary-note';
    note.textContent = 'The file omits the name.';
    root.append(note);
  }

  const facts = document.createElement('dl');
  facts.className = 'reports-summary-facts';
  appendFact(facts, 'Page', outline.pageText);
  appendFact(facts, 'Margins', outline.marginText);
  appendFact(facts, 'Grid', outline.gridText);
  appendFact(facts, 'Font', outline.fontText);
  root.append(facts);

  if (outline.bands.length === 0) {
    const empty = document.createElement('p');
    empty.className = 'reports-summary-note';
    empty.textContent = 'No bands.';
    root.append(empty);
  } else {
    const bands = document.createElement('ul');
    bands.className = 'reports-summary-bands';
    for (const band of outline.bands) {
      const row = document.createElement('li');
      const label = document.createElement('span');
      label.className = 'reports-band-label';
      label.textContent = band.line;
      row.append(label);
      if (band.items.length > 0) {
        const items = document.createElement('ul');
        for (const item of band.items) {
          const itemRow = document.createElement('li');
          itemRow.textContent = item;
          items.append(itemRow);
        }
        row.append(items);
      }
      bands.append(row);
    }
    root.append(bands);
  }

  body.append(root);
  stage.replaceChildren(body);
  return frame;
}

export default class ReportsManager {
  constructor(app, container) {
    this.app = app;
    this.container = container;
    this.elements = {};
    this.doc = null;
    this.history = null;
    this.saveFile = saveReportFile;
    this._listening = false;
    this._closed = false;
    this._drag = null;
    this._gridWatch = null;
  }

  async init() {
    await this._render();
    if (this._closed || !this.elements.name) return;
    this._setDocument(blankReport());
    this._bind();
  }

  async _render() {
    try {
      const response = await fetch(TEMPLATE);
      if (!response.ok) throw new Error(`Report template returned ${response.status}.`);
      const html = await response.text();
      if (!html.includes('id="reports-name"')) {
        throw new Error('Report template is empty.');
      }
      this.container.innerHTML = html;
    } catch (error) {
      const message = error instanceof Error ? error.message : 'Report template failed.';
      log(Subsystems.MANAGER, Status.ERROR, `[Reports] ${message}`);
      toast.error('Failed to load Report Manager', {
        description: message,
        subsystem: 'Manager',
      });
      this.container.innerHTML = '<div class="error">Failed to load Report Manager</div>';
      return;
    }

    this.elements = {
      name: this.container.querySelector('#reports-name'),
      newBtn: this.container.querySelector('#reports-new-btn'),
      importBtn: this.container.querySelector('#reports-import-btn'),
      exportBtn: this.container.querySelector('#reports-export-btn'),
      undoBtn: this.container.querySelector('#reports-undo-btn'),
      redoBtn: this.container.querySelector('#reports-redo-btn'),
      fileInput: this.container.querySelector('#reports-import-input'),
      stage: this.container.querySelector('#reports-stage'),
    };
    processIcons(this.container);
  }

  _bind() {
    const {
      newBtn, importBtn, exportBtn, undoBtn, redoBtn, fileInput, stage,
    } = this.elements;
    if (!newBtn || !importBtn || !exportBtn || !undoBtn || !redoBtn || !fileInput || !stage) {
      log(Subsystems.MANAGER, Status.ERROR, '[Reports] Template is missing a control.');
      toast.error('Failed to load Report Manager', {
        description: 'Template is missing a control.',
        subsystem: 'Manager',
      });
      return;
    }
    this._onNew = () => this._newReport();
    this._onImportClick = () => fileInput.click();
    this._onFile = () => {
      this._importSelected();
    };
    this._onExport = () => this._exportReport();
    this._onUndo = () => this._undo();
    this._onRedo = () => this._redo();
    this._onKey = (event) => this._onUndoKey(event);
    this._onBandDown = (event) => this._beginBandDrag(event);
    newBtn.addEventListener('click', this._onNew);
    importBtn.addEventListener('click', this._onImportClick);
    fileInput.addEventListener('change', this._onFile);
    exportBtn.addEventListener('click', this._onExport);
    undoBtn.addEventListener('click', this._onUndo);
    redoBtn.addEventListener('click', this._onRedo);
    document.addEventListener('keydown', this._onKey);
    stage.addEventListener('pointerdown', this._onBandDown);
    this._listening = true;
  }

  _dropGrid() {
    if (this._gridWatch) this._gridWatch.disconnect();
    this._gridWatch = null;
  }

  _watchGrid(frame) {
    this._dropGrid();
    if (this._closed || !this.elements.stage) return;
    if (!frame || frame.omitted || frame.error) return;
    const watch = watchBandGrids(this.elements.stage);
    if (watch.error) {
      log(Subsystems.MANAGER, Status.ERROR, `[Reports] ${watch.error}`);
      toast.error(watch.error, { subsystem: 'Manager' });
    }
    this._gridWatch = watch;
  }

  _release() {
    this._closed = true;
    this._clearBandDrag();
    this._dropGrid();
    if (!this._listening) return;
    const {
      newBtn, importBtn, exportBtn, undoBtn, redoBtn, fileInput, stage,
    } = this.elements;
    newBtn.removeEventListener('click', this._onNew);
    importBtn.removeEventListener('click', this._onImportClick);
    fileInput.removeEventListener('change', this._onFile);
    exportBtn.removeEventListener('click', this._onExport);
    undoBtn.removeEventListener('click', this._onUndo);
    redoBtn.removeEventListener('click', this._onRedo);
    document.removeEventListener('keydown', this._onKey);
    stage.removeEventListener('pointerdown', this._onBandDown);
    this._listening = false;
  }

  destroy() {
    this._release();
  }

  cleanup() {
    this.destroy();
  }

  _setDocument(doc) {
    let history;
    try {
      history = createHistory(doc);
    } catch (error) {
      const message = error instanceof Error && error.message
        ? error.message
        : 'Report file could not be read.';
      log(Subsystems.MANAGER, Status.ERROR, `[Reports] ${message}`);
      toast.error(message, { subsystem: 'Manager' });
      return;
    }
    this.history = history;
    this._showDocument(history.document);
  }

  _showDocument(doc) {
    this.doc = doc;
    if (this.elements.name) this.elements.name.textContent = reportDisplayName(doc);
    if (this.elements.stage) {
      const frame = paintReportStage(this.elements.stage, doc);
      const message = frame.error || frame.gridError;
      if (message) {
        log(Subsystems.MANAGER, Status.ERROR, `[Reports] ${message}`);
        toast.error(message, { subsystem: 'Manager' });
      }
      this._watchGrid(frame);
    }
    this._syncHistory();
  }

  _commitDocument(next) {
    if (!this.history) this.history = createHistory(this.doc);
    this._showDocument(this.history.commit(next));
  }

  _syncHistory() {
    const undoBtn = this.elements.undoBtn;
    const redoBtn = this.elements.redoBtn;
    if (undoBtn) undoBtn.disabled = !(this.history && this.history.canUndo());
    if (redoBtn) redoBtn.disabled = !(this.history && this.history.canRedo());
  }

  _undo() {
    if (this._drag || !this.history || !this.history.canUndo()) return;
    this._showDocument(this.history.undo());
    log(Subsystems.MANAGER, Status.INFO, '[Reports] Undo');
  }

  _redo() {
    if (this._drag || !this.history || !this.history.canRedo()) return;
    this._showDocument(this.history.redo());
    log(Subsystems.MANAGER, Status.INFO, '[Reports] Redo');
  }

  _onUndoKey(event) {
    if (!(event.ctrlKey || event.metaKey) || event.altKey) return;
    const key = event.key;
    const undo = (key === 'z' || key === 'Z') && !event.shiftKey;
    const redo = ((key === 'z' || key === 'Z') && event.shiftKey)
      || key === 'y' || key === 'Y';
    if (!undo && !redo) return;
    const target = event.target;
    const editable = target && typeof target.closest === 'function'
      && (target.closest('input, textarea, select') || target.isContentEditable);
    if (editable) return;
    event.preventDefault();
    if (redo) this._redo();
    else this._undo();
  }

  _clearBandDrag() {
    if (this._onBandMove) {
      document.removeEventListener('pointermove', this._onBandMove);
      document.removeEventListener('pointerup', this._onBandUp);
      document.removeEventListener('pointercancel', this._onBandUp);
    }
    this._onBandMove = null;
    this._onBandUp = null;
    this._drag = null;
  }

  _beginBandDrag(event) {
    if (this._drag || !this.doc || !Array.isArray(this.doc.bands)) return;
    if (event.button !== undefined && event.button !== 0) return;
    const edge = event.target && event.target.closest && event.target.closest('.reports-band-edge');
    if (!edge || !this.elements.stage || !this.elements.stage.contains(edge)) return;
    const bandEl = edge.closest('.reports-band');
    const id = bandEl && bandEl.dataset.bandId;
    const body = bandEl && bandEl.querySelector('.reports-band-body');
    const band = id && this.doc.bands.find((entry) => entry.id === id);
    if (!band || !body) return;
    let startPx;
    try {
      startPx = toCssPx(bandHeight(band));
    } catch {
      return;
    }
    event.preventDefault();
    this._drag = {
      id, startPx, startY: event.clientY, body,
    };
    this._onBandMove = (move) => this._moveBandDrag(move);
    this._onBandUp = () => this._endBandDrag();
    document.addEventListener('pointermove', this._onBandMove);
    document.addEventListener('pointerup', this._onBandUp);
    document.addEventListener('pointercancel', this._onBandUp);
  }

  _moveBandDrag(event) {
    if (!this._drag) return;
    const next = dragBandHeight(this._drag.startPx, event.clientY - this._drag.startY);
    this._drag.previewPx = next;
    this._drag.body.style.height = `${next}px`;
  }

  _endBandDrag() {
    const drag = this._drag;
    this._clearBandDrag();
    if (!drag || this._closed || !this.doc) return;
    if (drag.previewPx === undefined) return;
    let next;
    try {
      next = resizeBand(this.doc, drag.id, `${drag.previewPx} px`);
    } catch (error) {
      const message = error instanceof Error && error.message
        ? error.message
        : 'Band could not be resized.';
      log(Subsystems.MANAGER, Status.ERROR, `[Reports] ${message}`);
      toast.error(message, { subsystem: 'Manager' });
      this._showDocument(this.doc);
      return;
    }
    const before = this.doc.bands.find((entry) => entry.id === drag.id);
    const after = next.bands.find((entry) => entry.id === drag.id);
    if (before && after && before.height === after.height) {
      this._showDocument(this.doc);
      return;
    }
    this._commitDocument(next);
    log(Subsystems.MANAGER, Status.INFO, `[Reports] Resized ${drag.id} to ${after.height}`);
  }

  /** Replace the open definition and redraw the page. */
  loadReport(doc) {
    this._setDocument(doc);
  }

  _failImport(message) {
    log(Subsystems.MANAGER, Status.ERROR, `[Reports] Import failed: ${message}`);
    toast.error(message, { subsystem: 'Manager' });
  }

  _newReport() {
    this._setDocument(blankReport());
    log(Subsystems.MANAGER, Status.INFO, '[Reports] New report');
  }

  async _importSelected() {
    const input = this.elements.fileInput;
    const file = input && input.files && input.files[0];
    if (input) input.value = '';
    if (!file || this._closed) return;
    await this._importFile(file);
  }

  async _importFile(file) {
    let text;
    try {
      text = await file.text();
    } catch (error) {
      if (this._closed) return;
      const message = error instanceof Error && error.message
        ? error.message
        : 'Report file could not be read.';
      this._failImport(message);
      return;
    }
    if (this._closed) return;
    this._importText(text);
  }

  _importText(text) {
    const opened = openReportText(this.doc, text);
    if (opened.error) {
      this._failImport(opened.error);
      return;
    }
    this._setDocument(opened.doc);
    log(Subsystems.MANAGER, Status.INFO, `[Reports] Opened ${reportDisplayName(opened.doc)}`);
  }

  _exportReport() {
    if (!this.doc) return;
    const json = definitionText(this.doc);
    const filename = reportDownloadName(this.doc);
    this.saveFile(filename, json);
    log(Subsystems.MANAGER, Status.INFO, `[Reports] Exported ${filename}`);
  }
}
