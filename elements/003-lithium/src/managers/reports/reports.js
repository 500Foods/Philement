/**
 * Report Manager — Manager ID 24
 *
 * Starts a blank definition, imports a local JSON file, and downloads it.
 * The stage lists that definition and draws one page of bands.
 * Each band body draws the section grid. Rulers sit on the sheet.
 * The mouse line follows the pointer and hides when it leaves
 * the page. Dragging a band edge inverts that span on the vertical
 * ruler, then commits through the model and pushes history. The
 * corner cycles the ruler through inches, centimetres, and points
 * and leaves the file unit alone. The view scale fits the sheet
 * to the canvas width, or sits at 100%, which is 96 CSS pixels
 * per inch. Zoom steps by a tenth. The scale is not stored on
 * the definition. Undo
 * and redo apply the stored diff. Clear
 * removes that trail. destroy and cleanup release the same
 * listeners, including the mouse line.
 */

import './reports.css';

import { log, Subsystems, Status } from '../../core/log.js';
import { processIcons } from '../../core/icons.js';
import { registerShortcut, unregisterShortcut } from '../../core/manager-ui.js';
import { confirmDialog } from '../../shared/confirm.js';
import { toast } from '../../shared/toast.js';
import { dragBandHeight } from './bands.js';
import { pageFrame, paintReportPage } from './canvas.js';
import { watchBandGrids } from './grid.js';
import { bandHeight, resizeBand } from './model/document.js';
import {
  bindReportCursor, hideRulerSpan, nextRulerUnit, RULER_SIZE, rulerUnit, showRulerSpan,
} from './rulers.js';
import {
  fitWidthScale, formatViewScale, readViewScale, stepViewScale, viewScaleLabel,
} from './scale.js';
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
const REPORTS_MANAGER_ID = '24';

function appendFact(list, label, value) {
  const term = document.createElement('dt');
  term.textContent = label;
  const detail = document.createElement('dd');
  detail.textContent = value;
  list.append(term, detail);
}

/** Paint the open definition and its page into the stage. */
export function paintReportStage(stage, doc, rulerUnit, scale) {
  const outline = reportOutline(doc);
  const body = document.createElement('div');
  body.className = 'reports-stage-body';
  const canvas = document.createElement('div');
  canvas.className = 'reports-canvas';
  const frame = paintReportPage(canvas, doc, rulerUnit, scale);
  const notes = [...canvas.querySelectorAll('.reports-page-note')];
  for (const entry of notes) entry.remove();
  if (canvas.childElementCount > 0) body.append(...notes, canvas);
  else body.append(...notes);

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
    this._cursor = null;
    this._rulerDisplay = null;
    this._rulerCorner = null;
    this._scale = null;
    this._scaleChosen = false;
    this._paneWatch = null;
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
      clearBtn: this.container.querySelector('#reports-clear-btn'),
      fitBtn: this.container.querySelector('#reports-fit-btn'),
      zoomOutBtn: this.container.querySelector('#reports-zoom-out-btn'),
      zoomLabel: this.container.querySelector('#reports-zoom-label'),
      zoom100Btn: this.container.querySelector('#reports-zoom-100-btn'),
      zoomInBtn: this.container.querySelector('#reports-zoom-in-btn'),
      fileInput: this.container.querySelector('#reports-import-input'),
      stage: this.container.querySelector('#reports-stage'),
    };
    processIcons(this.container);
  }

  _bind() {
    const {
      newBtn, importBtn, exportBtn, undoBtn, redoBtn, clearBtn,
      fitBtn, zoomOutBtn, zoomLabel, zoom100Btn, zoomInBtn, fileInput, stage,
    } = this.elements;
    if (!newBtn || !importBtn || !exportBtn || !undoBtn || !redoBtn || !clearBtn
      || !fitBtn || !zoomOutBtn || !zoomLabel || !zoom100Btn || !zoomInBtn
      || !fileInput || !stage) {
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
    this._onClear = () => {
      this._clearHistory();
    };
    this._onFit = () => this._fitWidth();
    this._onZoomOut = () => this._zoomBy(-1);
    this._onZoom100 = () => this._zoom100();
    this._onZoomIn = () => this._zoomBy(1);
    this._onBandDown = (event) => this._beginBandDrag(event);
    newBtn.addEventListener('click', this._onNew);
    importBtn.addEventListener('click', this._onImportClick);
    fileInput.addEventListener('change', this._onFile);
    exportBtn.addEventListener('click', this._onExport);
    undoBtn.addEventListener('click', this._onUndo);
    redoBtn.addEventListener('click', this._onRedo);
    clearBtn.addEventListener('click', this._onClear);
    fitBtn.addEventListener('click', this._onFit);
    zoomOutBtn.addEventListener('click', this._onZoomOut);
    zoom100Btn.addEventListener('click', this._onZoom100);
    zoomInBtn.addEventListener('click', this._onZoomIn);
    stage.addEventListener('pointerdown', this._onBandDown);
    this._registerKeys();
    this._watchPane();
    this._listening = true;
  }

  _dropGrid() {
    if (this._gridWatch) this._gridWatch.disconnect();
    this._gridWatch = null;
  }

  _dropCursor() {
    if (this._cursor) this._cursor.disconnect();
    this._cursor = null;
  }

  _dropRulerUnit() {
    if (this._rulerCorner && this._onRulerUnit) {
      this._rulerCorner.removeEventListener('click', this._onRulerUnit);
    }
    this._rulerCorner = null;
    this._onRulerUnit = null;
  }

  _watchRulerUnit() {
    this._dropRulerUnit();
    if (this._closed || !this.elements.stage) return;
    const corner = this.elements.stage.querySelector('.reports-ruler-corner');
    if (!corner) return;
    this._onRulerUnit = () => this._cycleRulerUnit();
    this._rulerCorner = corner;
    corner.addEventListener('click', this._onRulerUnit);
  }

  _cycleRulerUnit() {
    if (this._closed || !this.doc) return;
    const stored = rulerUnit(this.doc.page);
    const current = this._rulerDisplay || (typeof stored === 'string' ? stored : 'in');
    this._rulerDisplay = nextRulerUnit(current);
    this._showDocument(this.doc);
  }

  _watchCursor() {
    this._dropCursor();
    if (this._closed || !this.elements.stage) return;
    const page = this.elements.stage.querySelector('#reports-page');
    const sheet = page && page.closest('.reports-sheet');
    if (!page || !sheet) return;
    this._cursor = bindReportCursor(page);
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

  _paneWidth() {
    const stage = this.elements.stage;
    if (!stage) return 0;
    const body = stage.querySelector('.reports-stage-body');
    const style = body ? getComputedStyle(body) : null;
    const pad = style
      ? (Number.parseFloat(style.paddingLeft) || 0) + (Number.parseFloat(style.paddingRight) || 0)
      : 0;
    return Math.max(0, stage.clientWidth - pad);
  }

  _fitScale(doc) {
    const frame = pageFrame(doc);
    if (!frame || frame.omitted || frame.error || !(frame.widthPx > 0)) return 1;
    return fitWidthScale(this._paneWidth(), frame.widthPx + RULER_SIZE);
  }

  _currentScale(doc) {
    if (this._scale > 0) return this._scale;
    if (!(this._paneWidth() > 0)) return 1;
    this._scale = this._fitScale(doc);
    return this._scale;
  }

  _setScale(scale, chosen) {
    const text = formatViewScale(scale);
    const next = Number(text);
    if (!(next > 0)) return;
    this._scale = next;
    if (chosen) this._scaleChosen = true;
    const canvas = this.elements.stage && this.elements.stage.querySelector('.reports-canvas');
    if (canvas) canvas.style.setProperty('--reports-scale', text);
    this._syncZoom();
  }

  _syncZoom() {
    const canvas = this.elements.stage && this.elements.stage.querySelector('.reports-canvas');
    const hasSheet = Boolean(canvas && canvas.querySelector('.reports-sheet'));
    const scale = this._scale > 0 ? this._scale : readViewScale(canvas);
    if (this.elements.zoomLabel) this.elements.zoomLabel.textContent = viewScaleLabel(scale);
    if (this.elements.zoomOutBtn) {
      this.elements.zoomOutBtn.disabled = !hasSheet || stepViewScale(scale, -1) === scale;
    }
    if (this.elements.zoomInBtn) {
      this.elements.zoomInBtn.disabled = !hasSheet || stepViewScale(scale, 1) === scale;
    }
    if (this.elements.fitBtn) this.elements.fitBtn.disabled = !hasSheet;
    if (this.elements.zoom100Btn) this.elements.zoom100Btn.disabled = !hasSheet;
  }

  _fitWidth() {
    if (this._closed || !this.doc) return;
    this._scaleChosen = false;
    const scale = this._fitScale(this.doc);
    this._setScale(scale);
    log(Subsystems.MANAGER, Status.INFO, `[Reports] View scale ${viewScaleLabel(scale)}`);
  }

  _zoom100() {
    if (this._closed || !this.doc) return;
    this._setScale(1, true);
    log(Subsystems.MANAGER, Status.INFO, '[Reports] View scale 100%');
  }

  _zoomBy(direction) {
    if (this._closed || !this.doc) return;
    const canvas = this.elements.stage && this.elements.stage.querySelector('.reports-canvas');
    const current = this._scale > 0 ? this._scale : readViewScale(canvas);
    const next = stepViewScale(current, direction);
    if (next === current) return;
    this._setScale(next, true);
    log(Subsystems.MANAGER, Status.INFO, `[Reports] View scale ${viewScaleLabel(next)}`);
  }

  _dropPane() {
    if (this._paneWatch) this._paneWatch.disconnect();
    this._paneWatch = null;
  }

  _watchPane() {
    this._dropPane();
    if (this._closed || !this.elements.stage || typeof ResizeObserver !== 'function') return;
    this._paneWatch = new ResizeObserver(() => {
      if (this._closed || this._scaleChosen || !this.doc) return;
      if (!(this._paneWidth() > 0)) return;
      this._setScale(this._fitScale(this.doc));
    });
    this._paneWatch.observe(this.elements.stage);
  }

  _release() {
    this._closed = true;
    this._clearBandDrag();
    this._dropGrid();
    this._dropCursor();
    this._dropRulerUnit();
    this._dropPane();
    if (!this._listening) return;
    const {
      newBtn, importBtn, exportBtn, undoBtn, redoBtn, clearBtn,
      fitBtn, zoomOutBtn, zoom100Btn, zoomInBtn, fileInput, stage,
    } = this.elements;
    newBtn.removeEventListener('click', this._onNew);
    importBtn.removeEventListener('click', this._onImportClick);
    fileInput.removeEventListener('change', this._onFile);
    exportBtn.removeEventListener('click', this._onExport);
    undoBtn.removeEventListener('click', this._onUndo);
    redoBtn.removeEventListener('click', this._onRedo);
    clearBtn.removeEventListener('click', this._onClear);
    fitBtn.removeEventListener('click', this._onFit);
    zoomOutBtn.removeEventListener('click', this._onZoomOut);
    zoom100Btn.removeEventListener('click', this._onZoom100);
    zoomInBtn.removeEventListener('click', this._onZoomIn);
    stage.removeEventListener('pointerdown', this._onBandDown);
    unregisterShortcut('reports-undo');
    unregisterShortcut('reports-redo');
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
    this._rulerDisplay = null;
    this._scale = null;
    this._scaleChosen = false;
    this.history = history;
    this._showDocument(history.document);
  }

  _showDocument(doc) {
    this._dropCursor();
    this._dropRulerUnit();
    this.doc = doc;
    if (this.elements.name) this.elements.name.textContent = reportDisplayName(doc);
    if (this.elements.stage) {
      const frame = paintReportStage(
        this.elements.stage,
        doc,
        this._rulerDisplay,
        this._currentScale(doc),
      );
      const message = frame.error || frame.gridError || frame.rulerError;
      if (message) {
        log(Subsystems.MANAGER, Status.ERROR, `[Reports] ${message}`);
        toast.error(message, { subsystem: 'Manager' });
      }
      this._watchGrid(frame);
      this._watchCursor();
      this._watchRulerUnit();
    }
    this._syncHistory();
    this._syncZoom();
  }

  _commitDocument(next) {
    if (!this.history) this.history = createHistory(this.doc);
    this._showDocument(this.history.commit(next));
  }

  _syncHistory() {
    const undoBtn = this.elements.undoBtn;
    const redoBtn = this.elements.redoBtn;
    const hasUndo = Boolean(this.history && this.history.canUndo());
    const hasRedo = Boolean(this.history && this.history.canRedo());
    if (undoBtn) undoBtn.disabled = !hasUndo;
    if (redoBtn) redoBtn.disabled = !hasRedo;
    if (this.elements.clearBtn) this.elements.clearBtn.disabled = !(hasUndo || hasRedo);
  }

  async _clearHistory() {
    if (this._drag || this._closed || !this.history) return;
    if (!this.history.canUndo() && !this.history.canRedo()) return;
    const accepted = await confirmDialog({
      title: 'Clear history?',
      message: 'The current report stays. Undo and redo for this report are removed.',
      confirmLabel: 'Clear',
      cancelLabel: 'Cancel',
    });
    if (!accepted || this._closed || !this.history) return;
    this._showDocument(this.history.clear());
    log(Subsystems.MANAGER, Status.INFO, '[Reports] Cleared history');
  }

  _restore(step, label) {
    try {
      this._showDocument(step());
    } catch (error) {
      const message = error instanceof Error && error.message
        ? error.message
        : 'Report history could not be applied.';
      log(Subsystems.MANAGER, Status.ERROR, `[Reports] ${message}`);
      toast.error(message, { subsystem: 'Manager' });
      return;
    }
    log(Subsystems.MANAGER, Status.INFO, `[Reports] ${label}`);
  }

  _undo() {
    if (this._drag || !this.history || !this.history.canUndo()) return;
    this._restore(() => this.history.undo(), 'Undo');
  }

  _redo() {
    if (this._drag || !this.history || !this.history.canRedo()) return;
    this._restore(() => this.history.redo(), 'Redo');
  }

  _registerKeys() {
    const options = { managerId: REPORTS_MANAGER_ID, category: 'Report Manager' };
    registerShortcut('reports-undo', 'Ctrl+Z', 'Undo', () => this._undo(), options);
    registerShortcut('reports-redo', 'Ctrl+Y', 'Redo', () => this._redo(), options);
  }

  _clearBandDrag() {
    if (this._drag && this._drag.page) hideRulerSpan(this._drag.page, 'y');
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
    const page = body.closest('#reports-page');
    const scale = readViewScale(page || body);
    const pageTop = page ? page.getBoundingClientRect().top : 0;
    const edgeY = (body.getBoundingClientRect().bottom - pageTop) / scale;
    this._drag = {
      id, startPx, startY: event.clientY, body, page, edgeY, scale,
    };
    this._onBandMove = (move) => this._moveBandDrag(move);
    this._onBandUp = () => this._endBandDrag();
    document.addEventListener('pointermove', this._onBandMove);
    document.addEventListener('pointerup', this._onBandUp);
    document.addEventListener('pointercancel', this._onBandUp);
  }

  _moveBandDrag(event) {
    if (!this._drag) return;
    const scale = this._drag.scale > 0 ? this._drag.scale : 1;
    const next = dragBandHeight(this._drag.startPx, (event.clientY - this._drag.startY) / scale);
    this._drag.previewPx = next;
    this._drag.body.style.height = `${next}px`;
    if (!this._drag.page) return;
    const moved = this._drag.edgeY + (next - this._drag.startPx);
    showRulerSpan(this._drag.page, 'y', this._drag.edgeY, moved);
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
