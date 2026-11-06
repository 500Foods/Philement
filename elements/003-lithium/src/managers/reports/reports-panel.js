/**
 * Property panel and Outline for the report editor (Phase 14).
 * Lookups load through QueryRef 26 (the working lookup path; the
 * plan's "026"). 053 object types, 054 attributes, 057 page sizes,
 * 074 palette groups, 075 sections. Each panel edit is one commit,
 * so one Undo step. Name and title share the definition with the
 * Report page form; both repaint from the document.
 */

import { log, Subsystems, Status } from '../../core/log.js';
import { loadPanelLookups } from './report-lookups.js';
import { toast } from '../../shared/toast.js';
import { LithiumSplitter } from '../../core/lithium-splitter.js';
import { LithiumProperties } from '../../properties/lithium-properties.js';
import { resolveLabel } from '../../properties/sections.js';
import { applyProperty, CLIENT_SECTIONS, parseCollection, propertyRows } from './report-properties.js';
import { reorderItem } from './model/document.js';
import { withBarcodeSections } from './reports-barcode.js';
import { FIELD_SHAPE, SHAPE_TYPE, setShapeCatalog, shapeOutlineLabel, withShapeSections } from './reports-shape.js';

function locales() {
  return typeof navigator !== 'undefined' && navigator.languages ? [...navigator.languages] : [];
}

/** Shape and barcode headers sit with the lookup sections. */
function editorSections(list) {
  return withBarcodeSections(withShapeSections(list));
}

/** Outline label for the End of Report marker. */
export const END_LABEL = 'End of Report';

/** Prefix a real id when the label is a different name. `d1` stays `d1`. */
function withId(id, text) {
  const name = text || id || '';
  if (!id || name === id) return name;
  return `${id} ${name}`;
}

/** Outline entries: report, each band and its items, then End of Report. */
export function outlineEntries(doc) {
  const out = [{ depth: 0, label: (doc && doc.name) || 'Report', bandId: '', itemId: '' }];
  for (const band of Array.isArray(doc && doc.bands) ? doc.bands : []) {
    out.push({ depth: 1, label: withId(band.id, band.name), bandId: band.id, itemId: '' });
    for (const item of Array.isArray(band.items) ? band.items : []) {
      const named = item.name || (typeof item.text === 'string' && item.text !== '' ? item.text : '');
      const shape = item.typeKey === SHAPE_TYPE || item.typeKey === FIELD_SHAPE;
      const text = shape ? shapeOutlineLabel(item) : named;
      out.push({ depth: 2, label: withId(item.id, text), bandId: band.id, itemId: item.id });
    }
  }
  out.push({ depth: 1, label: END_LABEL, bandId: '', itemId: '', end: true });
  return out;
}

/** Index of the selected entry in `outlineEntries`; report is 0. */
export function selectedEntryIndex(entries, sel) {
  const at = entries.findIndex((entry) => {
    if (sel.end) return Boolean(entry.end);
    if (sel.itemId) return entry.itemId === sel.itemId && entry.bandId === sel.bandId;
    if (sel.band) return entry.depth === 1 && !entry.end && entry.bandId === sel.bandId;
    return entry.depth === 0;
  });
  return at < 0 ? 0 : at;
}

/**
 * Property panel prefs live on the user profile (window.lithiumSettings,
 * synced to the account at login) at `24.props.<panel id>`. The old
 * localStorage blob and PanelStateManager width are read once, copied
 * onto the profile, and removed.
 */
export const PROPS_STATE_KEY = 'lithium_reports_props';
export const PROPS_PREFS_KEY = 'lithium_reports_props_prefs';
export const PROPS_WIDTH_KEY = `${PROPS_STATE_KEY}_width`;
export const PROPS_PROFILE_SECTION = '24';
export const PROPS_PANEL_ID = 'designer';

export function propsProfilePath(panelId = PROPS_PANEL_ID) {
  return `${PROPS_PROFILE_SECTION}.props.${panelId}`;
}

function profileSettings(settings) {
  const service = settings === undefined && typeof window !== 'undefined' ? window.lithiumSettings : settings;
  return service && typeof service.get === 'function' && typeof service.set === 'function' ? service : null;
}

function legacyPrefs(storage) {
  let prefs = {};
  try {
    const parsed = JSON.parse(storage.getItem(PROPS_PREFS_KEY) || 'null');
    if (parsed && typeof parsed === 'object') prefs = parsed;
  } catch {
    // unreadable legacy blob: start fresh.
  }
  const width = parseInt(storage.getItem(PROPS_WIDTH_KEY) || '', 10);
  if (width >= 100 && !(Number(prefs.panelWidth) > 0)) prefs.panelWidth = width;
  return prefs;
}

/**
 * @param {string} [panelId]
 * @param {{ get: Function, set: Function }|null} [settings] defaults to window.lithiumSettings
 * @param {Storage} [storage] legacy localStorage
 */
export function loadPanelPrefs(panelId = PROPS_PANEL_ID, settings = undefined, storage = globalThis.localStorage) {
  const service = profileSettings(settings);
  const stored = service ? service.get(propsProfilePath(panelId), null) : null;
  if (stored && typeof stored === 'object') return { ...stored };
  if (!storage) return {};
  const legacy = legacyPrefs(storage);
  if (service && Object.keys(legacy).length) {
    service.set(propsProfilePath(panelId), legacy);
    storage.removeItem?.(PROPS_PREFS_KEY);
    storage.removeItem?.(PROPS_WIDTH_KEY);
  }
  return legacy;
}

export function savePanelPrefs(state, panelId = PROPS_PANEL_ID, settings = undefined, storage = globalThis.localStorage) {
  const service = profileSettings(settings);
  if (service) {
    service.set(propsProfilePath(panelId), state);
    return;
  }
  try {
    storage?.setItem(PROPS_PREFS_KEY, JSON.stringify(state));
  } catch {
    // localStorage unavailable or full: prefs are best effort.
  }
}

export const reportPanelMethods = {
  _mountPanel() {
    const host = this.container && this.container.querySelector('#reports-props');
    this._outlineHost = this.container && this.container.querySelector('#reports-outline');
    if (!host || this._props) return;
    this._lookups = this._lookups || {};
    const prefs = loadPanelPrefs();
    try {
      this._props = new LithiumProperties({
        element: host,
        outline: this._outlineHost,
        onNavigate: (delta) => this._stepSelection(delta),
        onEdit: (id, value) => this._onPanelEdit(id, value),
        onFile: () => this._fieldEls?.imageInput?.click(),
        onOrder: (action) => this._orderItem(action),
        state: prefs,
        onStateChange: (state) => savePanelPrefs(state),
        pageSizes: () => (this._lookups.pageSizes || []).map((row) => ({
          value: Number(row.key_idx), label: row.value_txt,
        })),
      });
    } catch (error) {
      log(Subsystems.MANAGER, Status.ERROR, `[Reports] Property panel failed: ${error.message}`);
      return;
    }
    const bar = this.container.querySelector('#reports-props-splitter');
    const aside = this.container.querySelector('#reports-props');
    if (bar && aside) {
      const saved = Number(prefs.panelWidth) > 0 ? Number(prefs.panelWidth) : 280;
      aside.style.width = `${Math.min(600, Math.max(180, saved))}px`;
      this._propsSplitter = new LithiumSplitter({
        element: bar, leftPanel: aside, minWidth: 180, maxWidth: 600,
        onResizeEnd: (width) => {
          if (width) this._props?.setPanelWidth(width);
          this._props?.table?.redraw?.();
        },
      });
    }
    this._onOutlineClick = (event) => this._pickOutline(event);
    this._onOutlineKey = (event) => this._outlineKey(event);
    this._onOutlineDragStart = (event) => this._outlineDragStart(event);
    this._onOutlineDragOver = (event) => this._outlineDragOver(event);
    this._onOutlineDrop = (event) => this._outlineDrop(event);
    this._props.setSections(editorSections(CLIENT_SECTIONS));
    this._outlineHost?.addEventListener('click', this._onOutlineClick);
    if (this._outlineHost) {
      this._outlineHost.tabIndex = 0;
      this._outlineHost.addEventListener('keydown', this._onOutlineKey);
      this._outlineHost.addEventListener('dragstart', this._onOutlineDragStart);
      this._outlineHost.addEventListener('dragover', this._onOutlineDragOver);
      this._outlineHost.addEventListener('drop', this._onOutlineDrop);
    }
    this._paintPanel();
    if (this.app && this.app.api) {
      loadPanelLookups(this.app.api).then((lookups) => {
        if (this._closed) return;
        this._lookups = lookups;
        this._props?.setSections(editorSections([
          ...(lookups.sections || []).map((row) => ({
            id: Number(row.key_idx),
            label: resolveLabel({ label: row.value_txt, collection: parseCollection(row.collection) }, locales()),
            sortSeq: row.sort_seq,
            icon: parseCollection(row.collection).icon || '',
          })),
          ...CLIENT_SECTIONS,
        ]));
        setShapeCatalog(lookups.shapeKinds);
        if (this.doc) this._showDocument(this.doc);
        else this._paintPanel();
      });
    }
  },

  _dropPanel() {
    this._outlineHost?.removeEventListener('click', this._onOutlineClick);
    this._outlineHost?.removeEventListener('keydown', this._onOutlineKey);
    this._outlineHost?.removeEventListener('dragstart', this._onOutlineDragStart);
    this._outlineHost?.removeEventListener('dragover', this._onOutlineDragOver);
    this._outlineHost?.removeEventListener('drop', this._onOutlineDrop);
    this._propsSplitter?.destroy();
    this._propsSplitter = null;
    this._props?.destroy();
    this._props = null;
  },

  _panelSelection() {
    return {
      bandId: this._selectedBandId, itemId: this._selectedItemId,
      band: Boolean(this._bandScope), end: Boolean(this._endScope),
    };
  },

  _paintPanel() {
    if (!this.doc) return;
    setShapeCatalog(this._lookups && this._lookups.shapeKinds);
    this._fillShapes?.();
    this._paintOutline();
    if (!this._props) return;
    const labelOf = (row, collection) => resolveLabel(
      { label: collection.Label || row.value_txt, collection }, locales(),
    );
    const editing = Boolean(this._recordEditing);
    const unknown = [];
    const rows = propertyRows(
      this.doc, this._panelSelection(), { ...(this._lookups || {}), sampleColumns: this.sampleColumns?.() || [] }, labelOf, unknown,
    )
      .map((row) => (editing ? row : { ...row, readOnly: true }));
    this._props.setRows(rows);
    this._toastUnknown(unknown);
  },

  /** An unknown editor toasts once per key and its row is skipped. */
  _toastUnknown(unknown) {
    this._unknownToasted = this._unknownToasted || new Set();
    for (const entry of unknown) {
      if (this._unknownToasted.has(entry.key)) continue;
      this._unknownToasted.add(entry.key);
      const message = `Property ${entry.key} has unknown editor "${entry.editor}" and is skipped.`;
      log(Subsystems.MANAGER, Status.WARN, `[Reports] ${message}`);
      toast.error(message, { subsystem: 'Manager' });
    }
  },

  _paintOutline() {
    const host = this._outlineHost;
    if (!host) return;
    const list = document.createElement('ul');
    list.className = 'reports-outline-list';
    const entries = outlineEntries(this.doc);
    const picked = selectedEntryIndex(entries, this._panelSelection());
    entries.forEach((entry, index) => {
      const row = document.createElement('li');
      row.className = `reports-outline-row reports-outline-depth-${entry.depth}`;
      if (entry.end) {
        row.classList.add('reports-outline-end');
        row.dataset.end = 'true';
      }
      row.dataset.bandId = entry.bandId;
      row.dataset.itemId = entry.itemId;
      if (entry.itemId && this._recordEditing) row.draggable = true;
      row.textContent = entry.label;
      if (index === picked) row.classList.add('reports-outline-selected');
      list.append(row);
    });
    host.replaceChildren(list);
    host.querySelector('.reports-outline-selected')?.scrollIntoView?.({ block: 'nearest' });
  },

  /** Select one outline entry: report, band, item, or End of Report. */
  _selectEntry(entry) {
    if (!entry) return;
    this._endScope = Boolean(entry.end);
    if (entry.bandId) this._selectedBandId = entry.bandId;
    this._selectedItemId = entry.itemId || '';
    this._bandScope = Boolean(entry.bandId) && !entry.itemId;
    this._markSelection();
  },

  /** Previous (-1) / next (+1) object in outline order; stops at the ends. */
  _stepSelection(delta) {
    if (!this.doc) return;
    const entries = outlineEntries(this.doc);
    const at = selectedEntryIndex(entries, this._panelSelection()) + delta;
    if (at < 0 || at >= entries.length) return;
    this._selectEntry(entries[at]);
  },

  _outlineKey(event) {
    if (event.key !== 'ArrowUp' && event.key !== 'ArrowDown') return;
    event.preventDefault();
    this._stepSelection(event.key === 'ArrowDown' ? 1 : -1);
  },

  _outlineDragStart(event) {
    const row = event.target.closest?.('.reports-outline-row');
    if (!row || !row.dataset.itemId || !this._recordEditing) {
      event.preventDefault();
      return;
    }
    event.dataTransfer?.setData('text/plain', `${row.dataset.bandId}\t${row.dataset.itemId}`);
    if (event.dataTransfer) event.dataTransfer.effectAllowed = 'move';
  },

  _outlineDragOver(event) {
    const row = event.target.closest?.('.reports-outline-row');
    if (!row || !row.dataset.itemId || !this._recordEditing) return;
    event.preventDefault();
  },

  /** Drop takes the target row's index. The same band only. */
  _outlineDrop(event) {
    const row = event.target.closest?.('.reports-outline-row');
    if (!row || !row.dataset.itemId || !this._recordEditing || !this.doc) return;
    event.preventDefault();
    const raw = event.dataTransfer ? event.dataTransfer.getData('text/plain') : '';
    const parts = raw.split('\t');
    const bandId = parts[0];
    const itemId = parts[1];
    if (!bandId || !itemId || bandId !== row.dataset.bandId) return;
    const band = (this.doc.bands || []).find((entry) => entry.id === bandId);
    const to = band && Array.isArray(band.items) ? band.items.findIndex((item) => item.id === row.dataset.itemId) : -1;
    if (to < 0) return;
    try {
      const next = reorderItem(this.doc, bandId, itemId, to);
      if (next === this.doc) return;
      this._commitDocument(next);
      log(Subsystems.MANAGER, Status.INFO, `[Reports] Reordered ${itemId}`);
    } catch (error) {
      this._failEdit(error, 'Item could not be reordered.');
    }
  },

  _pickOutline(event) {
    const row = event.target.closest('.reports-outline-row');
    if (!row) return;
    this._selectEntry({
      bandId: row.dataset.bandId, itemId: row.dataset.itemId, end: row.dataset.end === 'true',
    });
    this._outlineHost?.focus?.({ preventScroll: true });
  },

  _onPanelEdit(id, value) {
    if (!this._recordEditing || !this.doc) {
      this._paintPanel();
      return;
    }
    try {
      const next = applyProperty(this.doc, this._panelSelection(), id, value, {
        pageSizes: (this._lookups && this._lookups.pageSizes) || [],
        dsKinds: (this._lookups && this._lookups.dsKinds) || [],
        reportingQueries: (this._lookups && this._lookups.reportingQueries) || [],
        datetimePresets: (this._lookups && this._lookups.datetimePresets) || [],
      });
      this._commitDocument(next);
    } catch (error) {
      const message = error instanceof Error ? error.message : 'Property edit failed.';
      log(Subsystems.MANAGER, Status.ERROR, `[Reports] ${message}`);
      toast.error(message, { subsystem: 'Manager' });
      this._paintPanel();
    }
  },
};
