/**
 * Report Manager — Manager ID 24
 *
 * Starts a blank definition, imports a local JSON file, and downloads it.
 * The stage lists that definition and draws one page.
 * destroy and cleanup release the same listeners.
 */

import './reports.css';

import { log, Subsystems, Status } from '../../core/log.js';
import { processIcons } from '../../core/icons.js';
import { toast } from '../../shared/toast.js';
import { paintReportPage } from './canvas.js';
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
    this.saveFile = saveReportFile;
    this._listening = false;
    this._closed = false;
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
      fileInput: this.container.querySelector('#reports-import-input'),
      stage: this.container.querySelector('#reports-stage'),
    };
    processIcons(this.container);
  }

  _bind() {
    const { newBtn, importBtn, exportBtn, fileInput } = this.elements;
    if (!newBtn || !importBtn || !exportBtn || !fileInput) {
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
    newBtn.addEventListener('click', this._onNew);
    importBtn.addEventListener('click', this._onImportClick);
    fileInput.addEventListener('change', this._onFile);
    exportBtn.addEventListener('click', this._onExport);
    this._listening = true;
  }

  _release() {
    this._closed = true;
    if (!this._listening) return;
    const { newBtn, importBtn, exportBtn, fileInput } = this.elements;
    newBtn.removeEventListener('click', this._onNew);
    importBtn.removeEventListener('click', this._onImportClick);
    fileInput.removeEventListener('change', this._onFile);
    exportBtn.removeEventListener('click', this._onExport);
    this._listening = false;
  }

  destroy() {
    this._release();
  }

  cleanup() {
    this.destroy();
  }

  _setDocument(doc) {
    this.doc = doc;
    if (this.elements.name) this.elements.name.textContent = reportDisplayName(doc);
    if (!this.elements.stage) return;
    const frame = paintReportStage(this.elements.stage, doc);
    if (frame.error) {
      log(Subsystems.MANAGER, Status.ERROR, `[Reports] ${frame.error}`);
      toast.error(frame.error, { subsystem: 'Manager' });
    }
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
