/**
 * Report Manager — Manager ID 24
 *
 * Starts a blank definition, imports a local JSON file, and downloads it.
 * The stage lists that definition and draws a stack of bands
 * at the content width. Each band body draws the section grid.
 * The first band carries the top ruler. Each band carries a
 * left ruler that starts at 0. The mouse line follows the
 * pointer and hides when it leaves the page. Dragging a band
 * edge grows that band's left ruler and inverts the span that
 * moved, then commits through the model and pushes history. The
 * corner cycles the ruler through inches, centimetres, and points
 * and leaves the file unit alone. The view scale fits the sheet
 * to the canvas width, or sits at 100%, which is 96 CSS pixels
 * per inch. Zoom steps by five percent. The wheel does that
 * same step while Ctrl or Cmd is held and the pointer is over
 * the page. A bare wheel still scrolls. The scale is
 * not stored on the definition. The left pane lists reports
 * through QueryRef 165 and searches through 166. Opening a
 * row loads QueryRef 167. Save appends a revision through
 * QueryRef 168. List delete calls QueryRef 170 and removes
 * every revision. The page stays read-only until the row is
 * being edited. Text can be dragged onto a band.
 * Report is the first page and edits the name, the title, and
 * the list summary. The Editor page is this stage. The JSON
 * page edits the definition with history removed. The
 * Revisions page lists saved revisions of the open report.
 * Undo, redo, and Clear sit on the Editor toolbar. They use the stored diff.
 * The item toolbar adds text, snaps to the grid, clones,
 * and deletes. Snap starts on. A drag moves an item and the
 * corner resizes it. Both commit on pointerup and stay
 * inside the band. destroy and cleanup release the same
 * listeners, including the mouse line.
 */

import '../../styles/vendor-tabulator.css';
import './reports.css';

import { log, Subsystems, Status } from '../../core/log.js';
import { processIcons } from '../../core/icons.js';
import { ManagerEditHelper } from '../../core/manager-edit-helper.js';
import { registerShortcut, unregisterShortcut } from '../../core/manager-ui.js';
import { confirmDialog } from '../../shared/confirm.js';
import { toast } from '../../shared/toast.js';
import { pageFrame, paintReportPage } from './canvas.js';
import { watchBandGrids } from './grid.js';
import { defaultBandId, itemBox, markReportSelection } from './items.js';
import { addItem, cloneItem, deleteItem } from './model/document.js';
import { bindReportCursor, nextRulerUnit, RULER_SIZE, rulerUnit } from './rulers.js';
import { rulerSides } from './report-properties.js';
import {
  fitWidthScale, formatViewScale, readViewScale, stepViewScale, viewScaleLabel,
  wheelViewScaleDirection, wheelZoomHeld,
} from './scale.js';
import { createHistory } from './model/history.js';
import { definitionMeta, withDefinitionMeta } from './reports-meta.js';
import { ReportJsonPage, reportJsonEdit, reportJsonText } from './reports-json.js';
import { reportDropMethods } from './reports-drop.js';
import { REPORT_CATEGORIES, loadCategory, reportFieldMethods, saveCategory } from './reports-fields.js';
import { reportLockMethods } from './reports-lock.js';
import { reportPageMethods } from './reports-pages.js';
import { reportComposeMethods } from './reports-compose.js';
import { reportPreviewMethods } from './reports-preview.js';
import { reportLogMethods } from './reports-log.js';
import { reportFontMethods } from './reports-font.js';
import { reportPanelMethods } from './reports-panel.js';
import { reportPointerMethods } from './reports-pointer.js';
import { bindRevisionControls, reportRevisionMethods, unbindRevisionControls } from './reports-revisions.js';
import { mountReportList } from './reports-table.js';
import {
  blankReport,
  definitionText,
  openReportText,
  reportDisplayName,
  reportDownloadName,
  saveReportFile,
} from './reports-file.js';

const TEMPLATE = '/src/managers/reports/reports.html';
const REPORTS_MANAGER_ID = '24';

/** Paint the open definition and its page into the stage. */
export function paintReportStage(stage, doc, rulerUnit, scale, sides) {
  const body = document.createElement('div');
  body.className = 'reports-stage-body';
  const canvas = document.createElement('div');
  canvas.className = 'reports-canvas';
  const frame = paintReportPage(canvas, doc, rulerUnit, scale, sides);
  const notes = [...canvas.querySelectorAll('.reports-page-note')];
  for (const entry of notes) entry.remove();
  if (canvas.childElementCount > 0) body.append(...notes, canvas);
  else body.append(...notes);

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
    this._selectedBandId = '';
    this._selectedItemId = '';
    this._bandScope = false;
    this._endScope = false;
    this._rulersOn = true;
    this._page = 'editor';
    this._category = 'static';
    this._json = null;
    this._pdfPage = null;
    this._jsonHeld = false;
    this._jsonLock = false;
    this._jsonErrorText = '';
    this._metaLock = false;
    this._openReportId = 0;
    this._openRevision = null;
    this._reportOpenToken = 0;
    this._recordEditing = false;
    this._textDragged = false;
    this.reportTable = null;
    this.splitter = null;
    this.editHelper = new ManagerEditHelper({ name: 'Reports' });
  }

  async init() {
    await this._render();
    if (this._closed || !this.elements.metaName) return;
    this._setDocument(blankReport());
    this._bind();
    if (!this._listening) return;
    await mountReportList(this);
    this._mountPanel();
    this.setRecordEditing(false);
    this.showPage('editor');
  }

  async _render() {
    try {
      const response = await fetch(TEMPLATE);
      if (!response.ok) throw new Error(`Report template returned ${response.status}.`);
      const html = await response.text();
      if (!html.includes('id="reports-meta-name"')) {
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
      collapseBtn: this.container.querySelector('#reports-collapse-btn'),
      reportTab: this.container.querySelector('#reports-report-tab'),
      editorTab: this.container.querySelector('#reports-editor-tab'),
      jsonTab: this.container.querySelector('#reports-json-tab'),
      revisionsTab: this.container.querySelector('#reports-revisions-tab'),
      composeTab: this.container.querySelector('#reports-compose-tab'),
      htmlTab: this.container.querySelector('#reports-html-tab'),
      logTab: this.container.querySelector('#reports-log-tab'),
      pdfTab: this.container.querySelector('#reports-pdf-tab'),
      pdfToolbar: this.container.querySelector('#reports-pdf-toolbar'),
      pdfHost: this.container.querySelector('#reports-pdf'),
      composeBtn: this.container.querySelector('#reports-compose-btn'),
      previewBtn: this.container.querySelector('#reports-preview-btn'),
      previewFrame: this.container.querySelector('#reports-preview-frame'),
      composeHost: this.container.querySelector('#reports-compose-editor'),
      composeFooter: this.container.querySelector('#reports-compose-footer'),
      logHost: this.container.querySelector('#reports-log-editor'), logFooter: this.container.querySelector('#reports-log-footer'),
      revisionSelect: this.container.querySelector('#reports-revision-select'),
      revisionCopy: this.container.querySelector('#reports-revision-copy'),
      revisionRevert: this.container.querySelector('#reports-revision-revert'),
      revisionStatus: this.container.querySelector('#reports-revisions-status'),
      revisionForm: this.container.querySelector('#reports-revisions'),
      metaName: this.container.querySelector('#reports-meta-name'),
      metaTitle: this.container.querySelector('#reports-meta-title'),
      metaSummary: this.container.querySelector('#reports-meta-summary'),
      metaForm: this.container.querySelector('#reports-meta'),
      importBtn: this.container.querySelector('#reports-import-btn'),
      exportBtn: this.container.querySelector('#reports-export-btn'),
      undoBtn: this.container.querySelector('#reports-undo-btn'),
      redoBtn: this.container.querySelector('#reports-redo-btn'),
      clearBtn: this.container.querySelector('#reports-clear-btn'),
      foldBtn: this.container.querySelector('#reports-fold-btn'),
      unfoldBtn: this.container.querySelector('#reports-unfold-btn'),
      fontBtn: this.container.querySelector('#reports-font-btn'),
      viewControls: this.container.querySelector('#reports-view-controls'),
      deleteBtn: this.container.querySelector('#reports-delete-btn'),
      cloneBtn: this.container.querySelector('#reports-clone-btn'),
      snapBtn: this.container.querySelector('#reports-snap-btn'),
      gridBtn: this.container.querySelector('#reports-grid-btn'),
      rulerBtn: this.container.querySelector('#reports-ruler-btn'),
      crosshairBtn: this.container.querySelector('#reports-crosshair-btn'),
      bordersBtn: this.container.querySelector('#reports-borders-btn'),
      staticBtn: this.container.querySelector('#reports-static-btn'),
      categories: this.container.querySelector('#reports-categories'),
      itemsToolbar: this.container.querySelector('#reports-items-toolbar'),
      textBtn: this.container.querySelector('#reports-text-btn'),
      fitBtn: this.container.querySelector('#reports-fit-btn'),
      zoomOutBtn: this.container.querySelector('#reports-zoom-out-btn'),
      zoomLabel: this.container.querySelector('#reports-zoom-label'),
      zoom100Btn: this.container.querySelector('#reports-zoom-100-btn'),
      zoomInBtn: this.container.querySelector('#reports-zoom-in-btn'),
      fileInput: this.container.querySelector('#reports-import-input'),
      stage: this.container.querySelector('#reports-stage'),
      jsonHost: this.container.querySelector('#reports-json-editor'),
      jsonFooter: this.container.querySelector('#reports-json-footer'),
      leftPanel: this.container.querySelector('.reports-left-panel'),
      tableContainer: this.container.querySelector('#reports-table-container'),
      navigatorContainer: this.container.querySelector('#reports-navigator-container'),
      splitter: this.container.querySelector('#reports-splitter'),
    };
    processIcons(this.container);
  }

  _missingControl() {
    const required = [
      'collapseBtn', 'reportTab', 'editorTab', 'jsonTab', 'revisionsTab',
      'composeTab', 'htmlTab', 'logTab', 'pdfTab', 'pdfToolbar', 'pdfHost', 'composeBtn', 'previewBtn', 'previewFrame', 'composeHost', 'composeFooter', 'logHost', 'logFooter',
      'revisionSelect', 'revisionCopy', 'revisionRevert', 'revisionStatus', 'revisionForm',
      'importBtn', 'exportBtn',
      'undoBtn', 'redoBtn', 'clearBtn', 'foldBtn', 'unfoldBtn', 'fontBtn', 'viewControls',
      'metaName', 'metaTitle', 'metaSummary', 'metaForm',
      'deleteBtn', 'cloneBtn', 'snapBtn', 'gridBtn', 'staticBtn', 'categories', 'itemsToolbar', 'textBtn',
      'fitBtn', 'zoomOutBtn', 'zoomLabel', 'zoom100Btn', 'zoomInBtn', 'fileInput',
      'stage', 'jsonHost', 'jsonFooter', 'leftPanel', 'tableContainer',
      'navigatorContainer', 'splitter',
    ];
    return required.some((key) => !this.elements[key]);
  }

  _bind() {
    if (this._missingControl()) {
      log(Subsystems.MANAGER, Status.ERROR, '[Reports] Template is missing a control.');
      toast.error('Failed to load Report Manager', {
        description: 'Template is missing a control.',
        subsystem: 'Manager',
      });
      return;
    }
    const {
      importBtn, exportBtn, undoBtn, redoBtn, clearBtn, deleteBtn, cloneBtn, textBtn,
      staticBtn, reportTab, editorTab, jsonTab, foldBtn, unfoldBtn,
      metaName, metaTitle, metaSummary, metaForm,
      fitBtn, zoomOutBtn, zoom100Btn, zoomInBtn, fileInput, stage,
    } = this.elements;
    this._onImportClick = () => fileInput.click();
    this._onFile = () => this._importSelected();
    this._onExport = () => this._exportReport();
    this._onUndo = () => this._undo();
    this._onRedo = () => this._redo();
    this._onClear = () => this._clearHistory();
    this._onDelete = () => this._deleteItem();
    this._onClone = () => this._cloneItem();
    this._onText = () => {
      if (this._textDragged) {
        this._textDragged = false;
        return;
      }
      this._addText();
    };
    this._onStatic = () => this.selectCategory('static');
    this._onReport = () => this.showPage('report');
    this._onEditor = () => this.showPage('editor');
    this._onJson = () => this.showPage('json');
    this._onComposeTab = () => this.showPage('compose');
    this._onHtmlTab = () => this.showPage('html');
    this._onPdfTab = () => this.showPage('pdf');
    this._bindLog();
    this._onComposeRun = () => this.runCompose();
    this._onMetaChange = () => this._commitMetaFields();
    this._onMetaSubmit = (event) => event.preventDefault();
    this._onFold = () => (this._page === 'compose' ? this._compose : this._json)?.fold();
    this._onUnfold = () => (this._page === 'compose' ? this._compose : this._json)?.unfold();
    this._onFont = (event) => this._handleFontClick(event);
    this._onFit = () => this._fitWidth();
    this._onZoomOut = () => this._zoomBy(-1);
    this._onZoom100 = () => this._zoom100();
    this._onZoomIn = () => this._zoomBy(1);
    this._onWheel = (event) => this._wheelZoom(event);
    this._onPointerDown = (event) => this._beginPointer(event);
    importBtn.addEventListener('click', this._onImportClick);
    fileInput.addEventListener('change', this._onFile);
    exportBtn.addEventListener('click', this._onExport);
    undoBtn.addEventListener('click', this._onUndo);
    redoBtn.addEventListener('click', this._onRedo);
    clearBtn.addEventListener('click', this._onClear);
    deleteBtn.addEventListener('click', this._onDelete);
    cloneBtn.addEventListener('click', this._onClone);
    textBtn.addEventListener('click', this._onText);
    this._bindTextDrop();
    staticBtn.addEventListener('click', this._onStatic);
    this._bindFields();
    this._category = loadCategory();
    this._showCategory(this._category);
    reportTab.addEventListener('click', this._onReport);
    editorTab.addEventListener('click', this._onEditor);
    jsonTab.addEventListener('click', this._onJson);
    this.elements.composeTab.addEventListener('click', this._onComposeTab);
    this.elements.htmlTab.addEventListener('click', this._onHtmlTab);
    this.elements.pdfTab.addEventListener('click', this._onPdfTab);
    this.elements.composeBtn.addEventListener('click', this._onComposeRun);
    this._bindPreview();
    bindRevisionControls(this);
    metaName.addEventListener('change', this._onMetaChange);
    metaTitle.addEventListener('change', this._onMetaChange);
    metaSummary.addEventListener('change', this._onMetaChange);
    metaForm.addEventListener('submit', this._onMetaSubmit);
    foldBtn.addEventListener('click', this._onFold);
    unfoldBtn.addEventListener('click', this._onUnfold);
    this.elements.fontBtn.addEventListener('click', this._onFont);
    fitBtn.addEventListener('click', this._onFit);
    zoomOutBtn.addEventListener('click', this._onZoomOut);
    zoom100Btn.addEventListener('click', this._onZoom100);
    zoomInBtn.addEventListener('click', this._onZoomIn);
    stage.addEventListener('wheel', this._onWheel, { passive: false });
    stage.addEventListener('pointerdown', this._onPointerDown);
    this._bindSnap();
    this._bindGrid();
    this._bindRuler();
    this._bindCrosshair();
    this._bindBorders();
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
    if (!frame || frame.omitted || frame.error || !(frame.contentWidthPx > 0)) return 1;
    const sides = this._rulerSides(doc);
    const bars = (sides.left ? RULER_SIZE : 0) + (sides.right ? RULER_SIZE : 0);
    return fitWidthScale(this._paneWidth(), frame.contentWidthPx + bars);
  }

  /** Ruler sides shown: the definition's Ruler section, gated by the toolbar toggle. */
  _rulerSides(doc) {
    return rulerSides(doc, this._rulersOn !== false);
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

  _wheelZoom(event) {
    if (this._closed || !this.doc || !event.target || !event.target.closest) return;
    if (!event.target.closest('#reports-page')) return;
    if (!wheelZoomHeld(event)) return;
    const direction = wheelViewScaleDirection(event.deltaY);
    if (!direction) return;
    event.preventDefault();
    this._zoomBy(direction);
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

  async openPdf() {
    if (this._closed) return;
    try {
      if (!this._pdfPage) {
        const { ReportPdfPage } = await import('./reports-pdf.js');
        if (this._closed) return;
        this._pdfPage = new ReportPdfPage(this);
      }
      if (this._closed) return;
      await this._pdfPage.open();
    } catch (error) {
      if (this._closed) return;
      const message = error instanceof Error ? error.message : 'Preview PDF failed.';
      log(Subsystems.MANAGER, Status.ERROR, `[Reports] ${message}`);
      toast.error('Preview PDF failed', { description: message, subsystem: 'Manager' });
    }
  }

  _release() {
    this._closed = true;
    this._clearItemDrag();
    this._clearBandDrag();
    this._dropGrid();
    this._dropCursor();
    this._dropRulerUnit();
    this._dropPane();
    if (this._json) this._json.destroy();
    this._json = null;
    if (this._pdfPage) this._pdfPage.destroy();
    this._pdfPage = null;
    this._destroyReportFont();
    if (this._compose) this._compose.destroy();
    this._compose = null;
    this._unbindLog();
    if (this.auditFooter) this.auditFooter.destroy();
    this.auditFooter = null;
    this.updateAuditFooter = null;
    if (this.reportTable) this.reportTable.destroy();
    this.reportTable = null;
    if (this.splitter) this.splitter.destroy();
    this._dropPanel();
    this.splitter = null;
    if (!this._listening) return;
    const {
      importBtn, exportBtn, undoBtn, redoBtn, clearBtn, deleteBtn, cloneBtn, textBtn,
      staticBtn, reportTab, editorTab, jsonTab, foldBtn, unfoldBtn, collapseBtn,
      metaName, metaTitle, metaSummary, metaForm,
      fitBtn, zoomOutBtn, zoom100Btn, zoomInBtn, fileInput, stage,
    } = this.elements;
    importBtn.removeEventListener('click', this._onImportClick);
    fileInput.removeEventListener('change', this._onFile);
    exportBtn.removeEventListener('click', this._onExport);
    undoBtn.removeEventListener('click', this._onUndo);
    redoBtn.removeEventListener('click', this._onRedo);
    clearBtn.removeEventListener('click', this._onClear);
    deleteBtn.removeEventListener('click', this._onDelete);
    cloneBtn.removeEventListener('click', this._onClone);
    textBtn.removeEventListener('click', this._onText);
    this._unbindTextDrop();
    staticBtn.removeEventListener('click', this._onStatic);
    this._unbindFields();
    reportTab.removeEventListener('click', this._onReport);
    editorTab.removeEventListener('click', this._onEditor);
    jsonTab.removeEventListener('click', this._onJson);
    this.elements.composeTab.removeEventListener('click', this._onComposeTab);
    this.elements.htmlTab.removeEventListener('click', this._onHtmlTab);
    this.elements.pdfTab.removeEventListener('click', this._onPdfTab);
    this.elements.composeBtn.removeEventListener('click', this._onComposeRun);
    this._unbindPreview();
    unbindRevisionControls(this);
    metaName.removeEventListener('change', this._onMetaChange);
    metaTitle.removeEventListener('change', this._onMetaChange);
    metaSummary.removeEventListener('change', this._onMetaChange);
    metaForm.removeEventListener('submit', this._onMetaSubmit);
    foldBtn.removeEventListener('click', this._onFold);
    unfoldBtn.removeEventListener('click', this._onUnfold);
    this.elements.fontBtn.removeEventListener('click', this._onFont);
    collapseBtn.removeEventListener('click', this._onCollapse);
    fitBtn.removeEventListener('click', this._onFit);
    zoomOutBtn.removeEventListener('click', this._onZoomOut);
    zoom100Btn.removeEventListener('click', this._onZoom100);
    zoomInBtn.removeEventListener('click', this._onZoomIn);
    stage.removeEventListener('wheel', this._onWheel);
    stage.removeEventListener('pointerdown', this._onPointerDown);
    this._unbindSnap();
    this._unbindGrid();
    this._unbindRuler();
    this._unbindCrosshair();
    this._unbindBorders();
    unregisterShortcut('reports-undo');
    unregisterShortcut('reports-redo');
    unregisterShortcut('reports-delete');
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
    this._selectedBandId = '';
    this._selectedItemId = '';
    this._bandScope = false;
    this._endScope = false;
    this.history = history;
    this._showDocument(history.document);
  }

  _showDocument(doc) {
    this._jsonHeld = false;
    this._dropCursor();
    this._dropRulerUnit();
    this.doc = doc;
    if (this.elements.stage) {
      const frame = paintReportStage(
        this.elements.stage,
        doc,
        this._rulerDisplay,
        this._currentScale(doc),
        this._rulerSides(doc),
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
    this._retainSelection();
    this._markSelection();
    this._syncHistory();
    this._syncZoom();
    this._paintMeta();
    this._paintPanel();
    this._syncRowName();
    this._refreshJson();
  }

  _paintMeta() {
    const meta = definitionMeta(this.doc);
    const nameInput = this.elements.metaName;
    const titleInput = this.elements.metaTitle;
    if (nameInput && document.activeElement !== nameInput) nameInput.value = meta.name;
    if (titleInput && document.activeElement !== titleInput) titleInput.value = meta.title;
  }

  /**
   * Summary lives on the list row, not the definition.
   * @param {string} value
   */
  _paintSummary(value) {
    const input = this.elements.metaSummary;
    if (!input || document.activeElement === input) return;
    input.value = typeof value === 'string' ? value : '';
  }

  _selectedRow() {
    const grid = this.reportTable && this.reportTable.table;
    if (!grid || typeof grid.getSelectedRows !== 'function') return null;
    const rows = grid.getSelectedRows();
    return rows.length ? rows[0] : null;
  }

  _syncRowName() {
    if (this._metaLock || !this.doc) return;
    const row = this._selectedRow();
    if (!row || typeof row.getData !== 'function' || typeof row.update !== 'function') return;
    const name = definitionMeta(this.doc).name;
    const data = row.getData() || {};
    if (data.name === name) return;
    this._writeRowFields({ name });
  }

  /**
   * @param {object} fields
   */
  _writeRowFields(fields) {
    const row = this._selectedRow();
    if (!row || typeof row.update !== 'function') return;
    const held = this._metaLock;
    this._metaLock = true;
    try {
      row.update(fields);
    } finally {
      this._metaLock = held;
    }
  }

  _commitMetaFields() {
    if (!this._recordEditing || this._closed || !this.doc || this._metaLock) return;
    const current = definitionMeta(this.doc);
    const name = this.elements.metaName ? this.elements.metaName.value : current.name;
    const title = this.elements.metaTitle ? this.elements.metaTitle.value : current.title;
    const summary = this.elements.metaSummary ? this.elements.metaSummary.value : '';
    const changed = name !== current.name || title !== current.title;
    this._metaLock = true;
    try {
      if (changed) this._commitDocument(withDefinitionMeta(this.doc, { name, title }));
      this._writeRowFields({ name, summary });
    } finally {
      this._metaLock = false;
    }
  }

  _commitDocument(next) {
    if (!this.history) this.history = createHistory(this.doc);
    this._showDocument(this.history.commit(next));
    this.editHelper?.checkDirtyState();
  }

  _syncHistory() {
    const undoBtn = this.elements.undoBtn;
    const redoBtn = this.elements.redoBtn;
    const hasUndo = Boolean(this.history && this.history.canUndo());
    const hasRedo = Boolean(this.history && this.history.canRedo());
    const locked = !this._recordEditing;
    if (undoBtn) undoBtn.disabled = locked || !hasUndo;
    if (redoBtn) redoBtn.disabled = locked || !hasRedo;
    if (this.elements.clearBtn) this.elements.clearBtn.disabled = locked || !(hasUndo || hasRedo);
  }

  async _clearHistory() {
    if (!this._recordEditing || this._drag || this._closed || !this.history) return;
    if (!this.history.canUndo() && !this.history.canRedo()) return;
    const accepted = await confirmDialog({
      title: 'Clear history?',
      message: 'The current report stays. Undo and redo for this report are removed.',
      confirmLabel: 'Clear',
      cancelLabel: 'Cancel',
    });
    if (!accepted || this._closed || !this.history) return;
    this._showDocument(this.history.clear());
    this.editHelper?.checkDirtyState();
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
    this.editHelper?.checkDirtyState();
    log(Subsystems.MANAGER, Status.INFO, `[Reports] ${label}`);
  }

  _undo() {
    if (!this._recordEditing || this._drag || !this.history || !this.history.canUndo()) return;
    this._restore(() => this.history.undo(), 'Undo');
  }

  _redo() {
    if (!this._recordEditing || this._drag || !this.history || !this.history.canRedo()) return;
    this._restore(() => this.history.redo(), 'Redo');
  }

  _registerKeys() {
    const options = { managerId: REPORTS_MANAGER_ID, category: 'Report Manager' };
    registerShortcut('reports-undo', 'Ctrl+Z', 'Undo', () => this._undo(), options);
    registerShortcut('reports-redo', 'Ctrl+Y', 'Redo', () => this._redo(), options);
    registerShortcut('reports-delete', 'Delete', 'Delete the selected item', () => this._deleteItem(), options);
  }

  _retainSelection() {
    const bands = Array.isArray(this.doc && this.doc.bands) ? this.doc.bands : [];
    const band = bands.find((entry) => entry.id === this._selectedBandId);
    if (!band) {
      this._selectedBandId = defaultBandId(this.doc);
      this._selectedItemId = '';
    }
  }

  _markSelection() {
    if (this._selectedItemId) this._endScope = false;
    const stage = this.elements.stage;
    markReportSelection(stage, this._endScope ? '' : this._selectedBandId, this._selectedItemId);
    stage?.querySelector?.('.reports-band-end')?.classList.toggle('reports-band-selected', Boolean(this._endScope));
    this._syncItems();
    this._paintPanel();
  }

  _syncItems() {
    const editing = Boolean(this._recordEditing);
    const live = Boolean(itemBox(this.doc, this._selectedBandId, this._selectedItemId));
    if (this.elements.textBtn) this.elements.textBtn.disabled = !editing || !this._selectedBandId;
    if (this.elements.deleteBtn) this.elements.deleteBtn.disabled = !editing || !live;
    if (this.elements.cloneBtn) this.elements.cloneBtn.disabled = !editing || !live;
    this._syncFieldButtons(editing);
  }

  _failEdit(error, fallback) {
    const message = error instanceof Error && error.message ? error.message : fallback;
    log(Subsystems.MANAGER, Status.ERROR, `[Reports] ${message}`);
    toast.error(message, { subsystem: 'Manager' });
  }

  _addText() {
    if (!this._recordEditing || this._drag || this._closed || !this.doc || !this._selectedBandId) return;
    let next;
    try {
      next = addItem(this.doc, this._selectedBandId, { typeKey: 0 });
    } catch (error) {
      this._failEdit(error, 'Static text could not be added.');
      return;
    }
    const band = next.bands.find((entry) => entry.id === this._selectedBandId);
    const added = band && band.items[band.items.length - 1];
    if (added) this._selectedItemId = added.id;
    this._commitDocument(next);
    log(Subsystems.MANAGER, Status.INFO, `[Reports] Added static text ${added ? added.id : ''} on ${this._selectedBandId}`);
  }

  _deleteItem() {
    if (!this._recordEditing || this._drag || this._closed || !this.doc || !this._selectedItemId) return;
    if (!itemBox(this.doc, this._selectedBandId, this._selectedItemId)) return;
    const itemId = this._selectedItemId;
    let next;
    try {
      next = deleteItem(this.doc, this._selectedBandId, itemId);
    } catch (error) {
      this._failEdit(error, 'Item could not be deleted.');
      return;
    }
    this._commitDocument(next);
    log(Subsystems.MANAGER, Status.INFO, `[Reports] Deleted ${itemId}`);
  }

  _cloneItem() {
    if (!this._recordEditing || this._drag || this._closed || !this.doc || !this._selectedItemId) return;
    if (!itemBox(this.doc, this._selectedBandId, this._selectedItemId)) return;
    const sourceId = this._selectedItemId;
    let next;
    try {
      next = cloneItem(this.doc, this._selectedBandId, sourceId);
    } catch (error) {
      this._failEdit(error, 'Item could not be cloned.');
      return;
    }
    const band = next.bands.find((entry) => entry.id === this._selectedBandId);
    const added = band && band.items[band.items.length - 1];
    if (added) this._selectedItemId = added.id;
    this._commitDocument(next);
    log(Subsystems.MANAGER, Status.INFO, `[Reports] Cloned ${sourceId} as ${added ? added.id : ''} on ${this._selectedBandId}`);
  }

  /**
   * Category buttons change the third toolbar: Static shows Text,
   * Field shows Text / Number / Datetime fields and the sample data
   * buttons. The definition is left alone.
   * @param {string} name
   */
  selectCategory(name) {
    if (!REPORT_CATEGORIES.includes(name)) return;
    this._category = name;
    this._showCategory(name);
    saveCategory(name);
  }

  _openJson() {
    if (!this.elements.jsonHost || !this.doc) return;
    if (!this._json) {
      this._json = new ReportJsonPage(
        this.elements.jsonHost,
        this.elements.jsonFooter,
        () => this.commitJson(),
        () => this.editHelper?.checkDirtyState(),
      );
    }
    this._json.open(this.doc, this._jsonHeld);
    this._json.setEditable(Boolean(this._recordEditing));
  }

  _refreshJson() {
    if (this._page !== 'json' || !this._json || this._jsonHeld || this._jsonLock) return;
    this._json.open(this.doc, false);
  }

  /**
   * Commit the JSON buffer as one history step.
   * A parse error toasts and leaves the definition.
   * @param {{ quiet?: boolean }} [options]
   * @returns {boolean}
   */
  commitJson(options = {}) {
    if (!this._recordEditing || this._jsonLock || !this._json || !this.doc) return true;
    const edited = reportJsonEdit(this._json.text());
    if (edited.error) {
      this._jsonHeld = true;
      const text = this._json.text();
      if (this._jsonErrorText !== text) {
        this._jsonErrorText = text;
        log(Subsystems.MANAGER, Status.ERROR, `[Reports] ${edited.error}`);
        if (!options.quiet) toast.error(edited.error, { subsystem: 'Manager' });
      }
      return false;
    }
    this._jsonErrorText = '';
    this._jsonHeld = false;
    if (reportJsonText(edited.doc) === reportJsonText(this.doc)) return true;
    this._jsonLock = true;
    try {
      this._commitDocument(edited.doc);
    } finally {
      this._jsonLock = false;
    }
    if (this._page === 'json' && this._json) this._json.open(this.doc, false);
    return true;
  }

  /** Replace the open definition and redraw the page. */
  loadReport(doc) { this._setDocument(doc); }

  _failImport(message) {
    log(Subsystems.MANAGER, Status.ERROR, `[Reports] Import failed: ${message}`);
    toast.error(message, { subsystem: 'Manager' });
  }

  newReport() {
    if (this._closed) return;
    this._jsonHeld = false;
    this._openReportId = 0;
    this.clearGeneration();
    this._openRevision = null;
    this._reportOpenToken = (this._reportOpenToken || 0) + 1;
    this._setDocument(blankReport());
    this._paintSummary('');
    this.editHelper?.checkDirtyState();
    if (typeof this.refreshRevisions === 'function') this.refreshRevisions();
    log(Subsystems.MANAGER, Status.INFO, '[Reports] New report');
  }

  async _importSelected() {
    if (!this._recordEditing) return;
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
    this.editHelper?.checkDirtyState();
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
    if (!this._checkFieldBindings()) return;
    const json = definitionText(this.doc);
    const filename = reportDownloadName(this.doc);
    this.saveFile(filename, json);
    log(Subsystems.MANAGER, Status.INFO, `[Reports] Exported ${filename}`);
  }
}

Object.assign(
  ReportsManager.prototype,
  reportPointerMethods,
  reportLockMethods,
  reportDropMethods,
  reportFieldMethods,
  reportPageMethods,
  reportComposeMethods,
  reportPreviewMethods,
  reportLogMethods,
  reportFontMethods,
  reportRevisionMethods,
  reportPanelMethods,
);
