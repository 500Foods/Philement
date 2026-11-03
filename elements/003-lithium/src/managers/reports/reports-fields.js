/**
 * Phase 15 manager methods: the Field category, field items, and the
 * session-only sample datasource. The sample sits on the manager as
 * `_sample` and never enters the definition, so export cannot carry it.
 */

import { log, Subsystems, Status } from '../../core/log.js';
import { toast } from '../../shared/toast.js';
import { addItem } from './model/document.js';
import { FIELD_TYPES, parseSample, unboundFields } from './reports-sample.js';

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

export const REPORT_CATEGORIES = Object.freeze(['static', 'field']);

const TYPE_NAMES = Object.freeze({ 20: 'text field', 21: 'number field', 24: 'datetime field' });

export const reportFieldMethods = {
  _fieldElements() {
    const root = this.container;
    return {
      fieldBtn: root.querySelector('#reports-field-btn'),
      itemBtns: [...root.querySelectorAll('#reports-items-toolbar [data-reports-category="field"][data-type-key]')],
      sampleBtn: root.querySelector('#reports-sample-btn'),
      pasteBtn: root.querySelector('#reports-sample-paste-btn'),
      sampleInput: root.querySelector('#reports-sample-input'),
    };
  },

  _bindFields() {
    const els = this._fieldElements();
    this._fieldEls = els;
    this._onFieldCategory = () => this.selectCategory('field');
    this._onFieldItem = (event) => this._addField(Number(event.currentTarget.dataset.typeKey));
    this._onSampleClick = () => els.sampleInput?.click();
    this._onSampleFile = () => this._importSampleFile();
    this._onSamplePaste = () => this._pasteSample();
    els.fieldBtn?.addEventListener('click', this._onFieldCategory);
    els.itemBtns.forEach((btn) => btn.addEventListener('click', this._onFieldItem));
    els.sampleBtn?.addEventListener('click', this._onSampleClick);
    els.sampleInput?.addEventListener('change', this._onSampleFile);
    els.pasteBtn?.addEventListener('click', this._onSamplePaste);
  },

  _unbindFields() {
    const els = this._fieldEls;
    if (!els) return;
    els.fieldBtn?.removeEventListener('click', this._onFieldCategory);
    els.itemBtns.forEach((btn) => btn.removeEventListener('click', this._onFieldItem));
    els.sampleBtn?.removeEventListener('click', this._onSampleClick);
    els.sampleInput?.removeEventListener('change', this._onSampleFile);
    els.pasteBtn?.removeEventListener('click', this._onSamplePaste);
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
  },

  _addField(typeKey) {
    if (!FIELD_TYPES.includes(typeKey)) return;
    if (!this._recordEditing || this._drag || this._closed || !this.doc || !this._selectedBandId) return;
    let next;
    try {
      next = addItem(this.doc, this._selectedBandId, { typeKey, field: '' });
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

  /**
   * Keep a parsed sample for this session. Never written to the definition.
   * @param {string} text
   * @returns {boolean}
   */
  loadSample(text) {
    let sample;
    try {
      sample = parseSample(text);
    } catch (error) {
      const message = error instanceof Error ? error.message : 'Sample data could not be read.';
      toast.error(message, { subsystem: 'Manager' });
      return false;
    }
    this._sample = sample;
    toast.success('Sample data loaded', {
      description: `${sample.rows.length} rows, ${sample.columns.length} columns. Session only; not exported.`,
      duration: 3000,
    });
    log(Subsystems.MANAGER, Status.INFO, `[Reports] Sample data: ${sample.rows.length} rows, columns ${sample.columns.join(', ')}`);
    this._paintPanel?.();
    return true;
  },

  /** @returns {string[]} */
  sampleColumns() {
    return this._sample ? this._sample.columns.slice() : [];
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
