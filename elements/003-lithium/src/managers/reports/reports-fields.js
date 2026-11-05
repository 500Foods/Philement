/**
 * Phase 15 manager methods: the Field category, field items, and sample
 * data. Phase 19b: Load Sample writes the rows into a json datasource
 * in the definition (the selected band's, else the first json one),
 * so they save, revision, and export.
 */

import { log, Subsystems, Status } from '../../core/log.js';
import { aggregateBandAllows } from './reports-aggregate.js';
import { dropItem } from './reports-drop.js';
import { toast } from '../../shared/toast.js';
import { addBandInOrder, addItem, removeBand, reorderItem, stepItem } from './model/document.js';
import { SHAPE_TYPE, defaultShapeFields, setShapeCatalog } from './reports-shape.js';
import { STATIC_GHOST, STATIC_IMAGE, imageSourcePath, placeImportedImage, staticItemSpec } from './reports-static.js';
import {
  FIELD_TYPES, PARAM_TYPE, SYSTEM_TYPES, bandDatasource, datasourceColumns, parseSample, setDatasourceRows, unboundFields,
} from './reports-sample.js';
import { loadReportingQueries, runQueryDatasources } from './reports-query.js';

/** Profile path for the selected element group. */
export const CATEGORY_PROFILE_PATH = 'reports.category';

function categorySettings(settings) {
  const service = settings === undefined && typeof window !== 'undefined' ? window.lithiumSettings : settings;
  return service && typeof service.get === 'function' && typeof service.set === 'function' ? service : null;
}

/**
 * Saved element group, Static when nothing valid is stored.
 * @param {{ get: Function, set: Function }|null} [settings]
 * @returns {string}
 */
export function loadCategory(settings) {
  const service = categorySettings(settings);
  const stored = service ? service.get(CATEGORY_PROFILE_PATH, null) : null;
  return REPORT_CATEGORIES.includes(stored) ? stored : 'static';
}

/**
 * @param {string} name
 * @param {{ get: Function, set: Function }|null} [settings]
 */
export function saveCategory(name, settings) {
  const service = categorySettings(settings);
  if (service && REPORT_CATEGORIES.includes(name)) service.set(CATEGORY_PROFILE_PATH, name);
}

export const REPORT_CATEGORIES = Object.freeze(['static', 'field', 'aggregate', 'bands', 'system']);

const TYPE_NAMES = Object.freeze({ 5: 'date', 6: 'user', 20: 'text field', 21: 'number field', 24: 'datetime field', 35: 'aggregate', 61: 'page number', 62: 'page count', 64: 'page label' });

export const reportFieldMethods = {
  _fieldElements() {
    const root = this.container;
    return {
      fieldBtn: root.querySelector('#reports-field-btn'),
      aggregateBtn: root.querySelector('#reports-aggregate-btn'),
      aggItemBtn: root.querySelector('#reports-aggregate-number-btn'),
      itemBtns: [...root.querySelectorAll('#reports-items-toolbar [data-reports-category="field"][data-type-key]')],
      bandsBtn: root.querySelector('#reports-bands-btn'),
      bandBtns: [...root.querySelectorAll('#reports-items-toolbar [data-reports-category="bands"][data-band-type]')],
      sampleBtn: root.querySelector('#reports-sample-btn'),
      pasteBtn: root.querySelector('#reports-sample-paste-btn'),
      sampleInput: root.querySelector('#reports-sample-input'),
      runBtn: root.querySelector('#reports-query-run-btn'),
      refreshBtn: root.querySelector('#reports-query-refresh-btn'),
      systemBtn: root.querySelector('#reports-system-btn'),
      paramBtn: root.querySelector('#reports-param-btn'),
      sysBtns: [...root.querySelectorAll('#reports-sysdate-btn, #reports-sysuser-btn, #reports-pagenum-btn, #reports-pagecount-btn, #reports-pagelabel-btn')],
      shapeBtn: root.querySelector('#reports-shape-btn'),
      raiseBtn: root.querySelector('#reports-raise-btn'),
      lowerBtn: root.querySelector('#reports-lower-btn'),
      staticBtns: [...root.querySelectorAll('#reports-static-number-btn, #reports-static-date-btn, #reports-static-image-btn, #reports-static-memo-btn')],
      imageInput: root.querySelector('#reports-image-file'),
    };
  },

  _bindFields() {
    const els = this._fieldElements();
    this._fieldEls = els;
    this._onFieldCategory = () => this.selectCategory('field');
    this._onAggregateCategory = () => this.selectCategory('aggregate');
    this._onAggregateItem = () => this._addAggregate();
    this._onFieldItem = (event) => this._addField(Number(event.currentTarget.dataset.typeKey));
    this._onBandsCategory = () => this.selectCategory('bands');
    this._onBandAdd = (event) => this._addBandType(Number(event.currentTarget.dataset.bandType));
    this._onSampleClick = () => els.sampleInput?.click();
    this._onSampleFile = () => this._importSampleFile();
    this._onSamplePaste = () => this._pasteSample();
    els.fieldBtn?.addEventListener('click', this._onFieldCategory);
    els.aggregateBtn?.addEventListener('click', this._onAggregateCategory);
    els.aggItemBtn?.addEventListener('click', this._onAggregateItem);
    els.itemBtns.forEach((btn) => btn.addEventListener('click', this._onFieldItem));
    els.bandsBtn?.addEventListener('click', this._onBandsCategory);
    els.bandBtns.forEach((btn) => btn.addEventListener('click', this._onBandAdd));
    els.sampleBtn?.addEventListener('click', this._onSampleClick);
    els.sampleInput?.addEventListener('change', this._onSampleFile);
    els.pasteBtn?.addEventListener('click', this._onSamplePaste);
    this._onQueryRun = () => this.runQueries();
    this._onQueryRefresh = () => this.refreshReportingQueries();
    els.runBtn?.addEventListener('click', this._onQueryRun);
    els.refreshBtn?.addEventListener('click', this._onQueryRefresh);
    this._onSystemCategory = () => this.selectCategory('system');
    this._onParamAdd = () => this._addParameter();
    els.systemBtn?.addEventListener('click', this._onSystemCategory);
    els.paramBtn?.addEventListener('click', this._onParamAdd);
    this._onSystemAdd = (event) => this._addSystem(Number(event.currentTarget.dataset.typeKey));
    els.sysBtns.forEach((btn) => btn.addEventListener('click', this._onSystemAdd));
    this._fillShapes();
    this._onShapeClick = () => {
      if (this._textDragged) {
        this._textDragged = false;
        return;
      }
      this._addShape();
    };
    this._onRaise = () => this._stepShape(1);
    this._onLower = () => this._stepShape(-1);
    els.shapeBtn?.addEventListener('click', this._onShapeClick);
    els.raiseBtn?.addEventListener('click', this._onRaise);
    els.lowerBtn?.addEventListener('click', this._onLower);
    this._onStaticAdd = (event) => {
      if (this._textDragged) {
        this._textDragged = false;
        return;
      }
      this._addStatic(Number(event.currentTarget.dataset.typeKey));
    };
    els.staticBtns.forEach((btn) => btn.addEventListener('click', this._onStaticAdd));
    this._onImageFile = (event) => {
      const input = event.target;
      const file = input.files && input.files[0];
      const sourcePath = imageSourcePath(file, input && input.value);
      if (input) input.value = '';
      if (file) placeImportedImage(this, file, sourcePath);
    };
    els.imageInput?.addEventListener('change', this._onImageFile);
    this._onImagePaste = (event) => {
      if (!this._recordEditing || !this.doc) return;
      const items = event.clipboardData && event.clipboardData.items;
      if (!items) return;
      const found = [...items].find((entry) => entry.kind === 'file' && String(entry.type || '').startsWith('image/'));
      if (!found) return;
      event.preventDefault();
      const file = found.getAsFile();
      if (file) placeImportedImage(this, file);
    };
    this.container?.addEventListener('paste', this._onImagePaste);
  },

  _unbindFields() {
    const els = this._fieldEls;
    if (!els) return;
    els.fieldBtn?.removeEventListener('click', this._onFieldCategory);
    els.aggregateBtn?.removeEventListener('click', this._onAggregateCategory);
    els.aggItemBtn?.removeEventListener('click', this._onAggregateItem);
    els.itemBtns.forEach((btn) => btn.removeEventListener('click', this._onFieldItem));
    els.bandsBtn?.removeEventListener('click', this._onBandsCategory);
    els.bandBtns.forEach((btn) => btn.removeEventListener('click', this._onBandAdd));
    els.sampleBtn?.removeEventListener('click', this._onSampleClick);
    els.sampleInput?.removeEventListener('change', this._onSampleFile);
    els.pasteBtn?.removeEventListener('click', this._onSamplePaste);
    els.runBtn?.removeEventListener('click', this._onQueryRun);
    els.refreshBtn?.removeEventListener('click', this._onQueryRefresh);
    els.systemBtn?.removeEventListener('click', this._onSystemCategory);
    els.paramBtn?.removeEventListener('click', this._onParamAdd);
    els.sysBtns.forEach((btn) => btn.removeEventListener('click', this._onSystemAdd));
    els.shapeBtn?.removeEventListener('click', this._onShapeClick);
    els.raiseBtn?.removeEventListener('click', this._onRaise);
    els.lowerBtn?.removeEventListener('click', this._onLower);
    (els.staticBtns || []).forEach((btn) => btn.removeEventListener('click', this._onStaticAdd));
    els.imageInput?.removeEventListener('change', this._onImageFile);
    this.container?.removeEventListener('paste', this._onImagePaste);
    this._fieldEls = null;
  },

  /**
   * Show the third-toolbar buttons for one category.
   * @param {string} name
   */
  _showCategory(name) {
    const root = this.container;
    if (!root) return;
    for (const btn of root.querySelectorAll('#reports-categories .reports-category-btn')) {
      const own = btn.id === `reports-${name}-btn`;
      btn.setAttribute('aria-pressed', String(own));
    }
    for (const btn of root.querySelectorAll('#reports-items-toolbar [data-reports-category]')) {
      btn.hidden = btn.dataset.reportsCategory !== name;
    }
  },

  /** Field item buttons follow the Text button's enabled rule. */
  _syncFieldButtons(editing) {
    const els = this._fieldEls;
    if (!els) return;
    els.itemBtns.forEach((btn) => { btn.disabled = !editing || !this._selectedBandId; });
    if (els.aggItemBtn) els.aggItemBtn.disabled = !editing || !this._aggregateBandSelected();
    els.bandBtns.forEach((btn) => { btn.disabled = !editing || !this.doc; });
    if (els.paramBtn) els.paramBtn.disabled = !editing || !this._selectedBandId;
    els.sysBtns.forEach((btn) => { btn.disabled = !editing || !this._selectedBandId; });
    const bands = this.doc && Array.isArray(this.doc.bands) ? this.doc.bands : [];
    const band = bands.find((entry) => entry.id === this._selectedBandId);
    const items = band && Array.isArray(band.items) ? band.items : [];
    const index = items.findIndex((item) => item.id === this._selectedItemId);
    if (els.raiseBtn) els.raiseBtn.disabled = !editing || index < 0 || index >= items.length - 1;
    if (els.lowerBtn) els.lowerBtn.disabled = !editing || index <= 0;
    if (els.shapeBtn) els.shapeBtn.disabled = !editing || !this._selectedBandId;
    (els.staticBtns || []).forEach((btn) => { btn.disabled = !editing || !this._selectedBandId; });
    const canBack = Boolean(editing && index > 0);
    const canFront = Boolean(editing && index >= 0 && index < items.length - 1);
    this._props?.setOrderEnabled({
      backward: canBack, back: canBack, forward: canFront, front: canFront,
    });
  },

  /** Shape catalog for the property list. The toolbar button always adds a rectangle. */
  _fillShapes() {
    setShapeCatalog(this._lookups && this._lookups.shapeKinds);
  },

  /** Add a rectangle on the selected band. The kind changes in the property panel. */
  _addShape() {
    if (!this._recordEditing || this._drag || this._closed || !this.doc || !this._selectedBandId) return;
    let next;
    try {
      next = addItem(this.doc, this._selectedBandId, { typeKey: SHAPE_TYPE, ...defaultShapeFields('rectangle') });
    } catch (error) {
      this._failEdit(error, 'Shape could not be added.');
      return;
    }
    const band = next.bands.find((entry) => entry.id === this._selectedBandId);
    const added = band && band.items[band.items.length - 1];
    if (added) this._selectedItemId = added.id;
    this._commitDocument(next);
    log(Subsystems.MANAGER, Status.INFO, `[Reports] Added shape rectangle ${added ? added.id : ''} on ${this._selectedBandId}`);
  },

  /** Add a static number, datetime, image, or memo on the selected band. */
  _addStatic(typeKey) {
    if (!this._recordEditing || this._drag || this._closed || !this.doc || !this._selectedBandId) return;
    if (!STATIC_GHOST[typeKey]) return;
    let next;
    try {
      next = addItem(this.doc, this._selectedBandId, staticItemSpec(typeKey));
    } catch (error) {
      this._failEdit(error, `${STATIC_GHOST[typeKey]} could not be added.`);
      return;
    }
    const band = next.bands.find((entry) => entry.id === this._selectedBandId);
    const added = band && band.items[band.items.length - 1];
    if (added) this._selectedItemId = added.id;
    this._commitDocument(next);
    log(Subsystems.MANAGER, Status.INFO, `[Reports] Added ${STATIC_GHOST[typeKey].toLowerCase()} ${added ? added.id : ''} on ${this._selectedBandId}`);
    if (typeKey === STATIC_IMAGE) this._fieldEls?.imageInput?.click();
  },

  /** Raise (+1, paints later) or lower (-1) the selected item one step. */
  _stepShape(delta) {
    if (!this._recordEditing || this._drag || this._closed || !this.doc || !this._selectedItemId) return;
    const itemId = this._selectedItemId;
    let next;
    try {
      next = stepItem(this.doc, this._selectedBandId, itemId, delta);
    } catch (error) {
      this._failEdit(error, 'Item could not be reordered.');
      return;
    }
    if (next === this.doc) return;
    this._commitDocument(next);
    log(Subsystems.MANAGER, Status.INFO, `[Reports] ${delta > 0 ? 'Raised' : 'Lowered'} ${itemId}`);
  },

  /**
   * Stack order from the property header.
   * backward and forward move one step. back and front move to an end.
   * @param {string} action
   */
  _orderItem(action) {
    if (!this._recordEditing || this._drag || this._closed || !this.doc || !this._selectedItemId) return;
    const band = (this.doc.bands || []).find((entry) => entry.id === this._selectedBandId);
    const items = band && Array.isArray(band.items) ? band.items : [];
    const index = items.findIndex((item) => item.id === this._selectedItemId);
    if (index < 0) return;
    const itemId = this._selectedItemId;
    let next;
    try {
      if (action === 'backward') next = stepItem(this.doc, this._selectedBandId, itemId, -1);
      else if (action === 'forward') next = stepItem(this.doc, this._selectedBandId, itemId, 1);
      else if (action === 'back') next = reorderItem(this.doc, this._selectedBandId, itemId, 0);
      else if (action === 'front') next = reorderItem(this.doc, this._selectedBandId, itemId, items.length - 1);
      else return;
    } catch (error) {
      this._failEdit(error, 'Item could not be reordered.');
      return;
    }
    if (!next || next === this.doc) return;
    this._commitDocument(next);
    log(Subsystems.MANAGER, Status.INFO, `[Reports] Moved ${itemId} ${action}`);
  },

  /** True when the selected band can show a group, page, or report total. */
  _aggregateBandSelected() {
    if (!this.doc || !this._selectedBandId) return false;
    const band = (this.doc.bands || []).find((entry) => entry.id === this._selectedBandId);
    return Boolean(band && aggregateBandAllows(band.typeKey));
  },

  /** Aggregate number (35) on a group, page, or report header or footer. */
  _addAggregate() {
    if (!this._recordEditing || this._drag || this._closed || !this.doc || !this._selectedBandId) return;
    let next;
    try {
      next = addItem(this.doc, this._selectedBandId, dropItem(35));
    } catch (error) {
      this._failEdit(error, 'Aggregate could not be added.');
      return;
    }
    const band = next.bands.find((entry) => entry.id === this._selectedBandId);
    const added = band && band.items[band.items.length - 1];
    if (added) this._selectedItemId = added.id;
    this._commitDocument(next);
    log(Subsystems.MANAGER, Status.INFO, `[Reports] Added aggregate ${added ? added.id : ''} on ${this._selectedBandId}`);
  },

  _addField(typeKey) {
    if (!FIELD_TYPES.includes(typeKey)) return;
    if (!this._recordEditing || this._drag || this._closed || !this.doc || !this._selectedBandId) return;
    let next;
    try {
      next = addItem(this.doc, this._selectedBandId, dropItem(typeKey));
    } catch (error) {
      this._failEdit(error, 'Field could not be added.');
      return;
    }
    const band = next.bands.find((entry) => entry.id === this._selectedBandId);
    const added = band && band.items[band.items.length - 1];
    if (added) this._selectedItemId = added.id;
    this._commitDocument(next);
    log(Subsystems.MANAGER, Status.INFO, `[Reports] Added ${TYPE_NAMES[typeKey]} ${added ? added.id : ''} on ${this._selectedBandId}`);
  },

  /** System palette: add a Parameter item (053 key 63) to the selected band. */
  _addParameter() {
    if (!this._recordEditing || this._drag || this._closed || !this.doc || !this._selectedBandId) return;
    let next;
    try {
      next = addItem(this.doc, this._selectedBandId, { typeKey: PARAM_TYPE, param: '' });
    } catch (error) {
      this._failEdit(error, 'Parameter could not be added.');
      return;
    }
    const band = next.bands.find((entry) => entry.id === this._selectedBandId);
    const added = band && band.items[band.items.length - 1];
    if (added) this._selectedItemId = added.id;
    this._commitDocument(next);
    log(Subsystems.MANAGER, Status.INFO, `[Reports] Added parameter ${added ? added.id : ''} on ${this._selectedBandId}`);
  },

  /** System palette: System Date (5), System User (6), Page Number (61), Page Count (62) or Page Label (64), as a drop would create. */
  _addSystem(typeKey) {
    if (!SYSTEM_TYPES.includes(typeKey)) return;
    if (!this._recordEditing || this._drag || this._closed || !this.doc || !this._selectedBandId) return;
    let next;
    try {
      next = addItem(this.doc, this._selectedBandId, dropItem(typeKey));
    } catch (error) {
      this._failEdit(error, 'System item could not be added.');
      return;
    }
    const band = next.bands.find((entry) => entry.id === this._selectedBandId);
    const added = band && band.items[band.items.length - 1];
    if (added) this._selectedItemId = added.id;
    this._commitDocument(next);
    log(Subsystems.MANAGER, Status.INFO, `[Reports] Added system ${TYPE_NAMES[typeKey] || typeKey} ${added ? added.id : ''} on ${this._selectedBandId}`);
  },

  /** Bands palette: add one band (14 adds a header and footer pair). */
  _addBandType(typeKey) {
    if (!this._recordEditing || this._drag || this._closed || !this.doc) return;
    const before = new Set(this.doc.bands.map((band) => band.id));
    let next;
    try {
      next = addBandInOrder(this.doc, typeKey);
    } catch (error) {
      this._failEdit(error, 'Band could not be added.');
      return;
    }
    const added = next.bands.find((band) => !before.has(band.id));
    if (added) {
      this._selectedBandId = added.id;
      this._selectedItemId = '';
    }
    this._commitDocument(next);
    log(Subsystems.MANAGER, Status.INFO, `[Reports] Added band ${added ? added.id : ''}`);
  },

  /** Delete a band from its strip. A group header or footer removes its pair. */
  _removeBand(id) {
    if (!this._recordEditing || this._drag || this._closed || !this.doc) return;
    let next;
    try {
      next = removeBand(this.doc, id);
    } catch (error) {
      this._failEdit(error, 'Band could not be deleted.');
      return;
    }
    if (!next.bands.some((band) => band.id === this._selectedBandId)) {
      this._selectedBandId = '';
      this._selectedItemId = '';
    }
    this._commitDocument(next);
    log(Subsystems.MANAGER, Status.INFO, `[Reports] Deleted band ${id}`);
  },

  /** Datasource the sample targets: the selected band's, else ''. */
  _sampleTarget() {
    if (!this.doc || !this._selectedBandId) return '';
    const band = (this.doc.bands || []).find((entry) => entry.id === this._selectedBandId);
    if (!band || ![14, 15, 16].includes(band.typeKey)) return '';
    return band.datasource || '';
  },

  /**
   * Store parsed rows on a json datasource in the definition.
   * @param {string} text
   * @returns {boolean}
   */
  loadSample(text) {
    if (!this.doc) return false;
    if (!this._recordEditing) {
      toast.error('Sample data is stored in the report. Start editing first.', { subsystem: 'Manager' });
      return false;
    }
    let sample;
    let result;
    try {
      sample = parseSample(text);
      result = setDatasourceRows(this.doc, this._sampleTarget(), sample.rows);
    } catch (error) {
      const message = error instanceof Error ? error.message : 'Sample data could not be read.';
      toast.error(message, { subsystem: 'Manager' });
      return false;
    }
    this._commitDocument(result.doc);
    toast.success('Sample data loaded', {
      description: `${sample.rows.length} rows, ${sample.columns.length} columns into datasource ${result.name}.`,
      duration: 3000,
    });
    log(Subsystems.MANAGER, Status.INFO, `[Reports] Sample data into ${result.name}: ${sample.rows.length} rows, columns ${sample.columns.join(', ')}`);
    this._paintPanel?.();
    return true;
  },

  /**
   * Columns of the selected band's datasource (the first one by default).
   * @returns {string[]}
   */
  sampleColumns() {
    if (!this.doc) return [];
    const band = (this.doc.bands || []).find((entry) => entry.id === this._selectedBandId);
    return datasourceColumns(this.doc, bandDatasource(this.doc, band));
  },

  /**
   * Phase 28: reload the Reporting query list (QueryRef 173).
   * @param {{ quiet?: boolean }} [opts]
   */
  async refreshReportingQueries(opts = {}) {
    if (!this.app || !this.app.api) return [];
    try {
      const list = await loadReportingQueries(this.app.api);
      if (this._closed) return list;
      this._lookups = { ...(this._lookups || {}), reportingQueries: list };
      if (!opts.quiet) toast.success('Reporting queries refreshed', { description: `${list.length} found.`, duration: 2500 });
      log(Subsystems.MANAGER, Status.INFO, `[Reports] Reporting queries: ${list.length}`);
      this._paintPanel?.();
      return list;
    } catch (error) {
      const message = error instanceof Error ? error.message : 'Reporting queries could not be loaded.';
      log(Subsystems.MANAGER, Status.WARN, `[Reports] Reporting query list failed: ${message}`);
      if (!opts.quiet) toast.error('Reporting queries could not be loaded', { description: message, subsystem: 'Manager' });
      return [];
    }
  },

  /** Phase 28: run every Query datasource and store its rows in the definition. */
  async runQueries() {
    if (!this.doc || !this.app || !this.app.api) return false;
    if (!this._recordEditing) {
      toast.error('Query rows are stored in the report. Start editing first.', { subsystem: 'Manager' });
      return false;
    }
    const queries = (this._lookups && this._lookups.reportingQueries) || [];
    const result = await runQueryDatasources(this.app.api, this.doc, queries);
    if (this._closed) return false;
    for (const err of result.errors) {
      toast.error(`Query datasource ${err.name} failed`, { description: err.message, subsystem: 'Manager' });
      log(Subsystems.MANAGER, Status.ERROR, `[Reports] Query ${err.name}: ${err.message}`);
    }
    if (!result.ran.length) {
      if (!result.errors.length) toast.error('No Query datasource names a QueryRef.', { subsystem: 'Manager' });
      return false;
    }
    this._commitDocument(result.doc);
    toast.success('Query rows loaded', {
      description: result.ran.map((r) => `${r.name}: ${r.rows}`).join(', '), duration: 3000,
    });
    log(Subsystems.MANAGER, Status.INFO, `[Reports] Ran queries ${result.ran.map((r) => `${r.name}=${r.rows}`).join(', ')}`);
    this._paintPanel?.();
    return true;
  },

  async _importSampleFile() {
    const input = this._fieldEls?.sampleInput;
    const file = input?.files?.[0];
    if (!file) return;
    try {
      this.loadSample(await file.text());
    } finally {
      input.value = '';
    }
  },

  async _pasteSample() {
    let text;
    try {
      text = await navigator.clipboard.readText();
    } catch {
      toast.error('Clipboard could not be read.', { subsystem: 'Manager' });
      return;
    }
    this.loadSample(text);
  },

  /**
   * Export is refused while a field item has no binding (rule 1: no
   * silent fallback). Returns true when export may go ahead.
   * @returns {boolean}
   */
  _checkFieldBindings() {
    const missing = unboundFields(this.doc);
    if (!missing.length) return true;
    const first = missing[0];
    toast.error('Export refused: a field has no binding', {
      description: `${missing.length} field item${missing.length === 1 ? '' : 's'} unbound (first: ${first.itemId} on ${first.bandId}). Pick a field in the property panel.`,
      subsystem: 'Manager',
    });
    log(Subsystems.MANAGER, Status.WARN, `[Reports] Export refused; unbound fields: ${missing.map((m) => m.itemId).join(', ')}`);
    return false;
  },
};
