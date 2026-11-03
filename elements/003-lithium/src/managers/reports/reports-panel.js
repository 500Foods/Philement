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

function locales() {
  return typeof navigator !== 'undefined' && navigator.languages ? [...navigator.languages] : [];
}

/** Outline label for the End of Report marker. */
export const END_LABEL = 'End of Report';

/** Outline entries: report, each band and its items, then End of Report. */
export function outlineEntries(doc) {
  const out = [{ depth: 0, label: (doc && doc.name) || 'Report', bandId: '', itemId: '' }];
  for (const band of Array.isArray(doc && doc.bands) ? doc.bands : []) {
    out.push({ depth: 1, label: band.name || band.id, bandId: band.id, itemId: '' });
    for (const item of Array.isArray(band.items) ? band.items : []) {
      const text = item.name || (typeof item.text === 'string' ? item.text : '') || item.id;
      out.push({ depth: 2, label: text, bandId: band.id, itemId: item.id });
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
    this._outlineHost?.addEventListener('click', this._onOutlineClick);
    this._onOutlineKey = (event) => this._outlineKey(event);
    if (this._outlineHost) {
      this._outlineHost.tabIndex = 0;
      this._outlineHost.addEventListener('keydown', this._onOutlineKey);
    }
    this._paintPanel();
    if (this.app && this.app.api) {
      loadPanelLookups(this.app.api).then((lookups) => {
        if (this._closed) return;
        this._lookups = lookups;
        this._props?.setSections([
          ...(lookups.sections || []).map((row) => ({
            id: Number(row.key_idx),
            label: resolveLabel({ label: row.value_txt, collection: parseCollection(row.collection) }, locales()),
            sortSeq: row.sort_seq,
            icon: parseCollection(row.collection).icon || '',
          })),
          ...CLIENT_SECTIONS,
        ]);
        this._paintPanel();
      });
    }
  },

  _dropPanel() {
    this._outlineHost?.removeEventListener('click', this._onOutlineClick);
    this._outlineHost?.removeEventListener('keydown', this._onOutlineKey);
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
    this._paintOutline();
    if (!this._props) return;
    const labelOf = (row, collection) => resolveLabel(
      { label: collection.Label || row.value_txt, collection }, locales(),
    );
    const editing = Boolean(this._recordEditing);
    const unknown = [];
    const rows = propertyRows(this.doc, this._panelSelection(), this._lookups || {}, labelOf, unknown)
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
