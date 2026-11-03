/**
 * Property panel rows for the report editor (Phase 14).
 * Rows come from lookup 054. The selection picks the list:
 * no item selected uses lookup 053 key 9 (Report Top), which
 * lists attribute names. A selected item uses its 053 row,
 * which lists 054 keys. Key 24 (Grid Background Color) is a
 * report attribute only and is dropped from item lists here.
 * Keys 21 and 24 use the Color editor by name (lookup says Text).
 * Key 29 (Link) is plain text. Key 30 (Page Size) takes its
 * choices from lookup 057, not from its own List.
 * Page Size copies width, height, margins, and orientation from 057.
 * Values the file omits show the Defaults table value.
 */

import { bandHeight, moveItem, resizeBand, resizeItem } from './model/document.js';
import { parseLength } from './model/units.js';
import { datasourceNames } from './reports-sample.js';

export const REPORT_TYPE_KEY = 9;
const TEXT_KEYS = new Set([29]);
const PAGE_SIZE_KEY = 30;
/** Number keys that hold a length; the rest are plain numbers. */
const LENGTH_KEYS = new Set([0, 1, 2, 3, 16, 23, 31, 32, 35, 36, 37, 38]);
/** Editors the panel knows. Field and Datasource (Phase 15) become Lists. */
export const KNOWN_EDITORS = new Set(['Text', 'Number', 'List', 'Boolean', 'Color', 'PageSize', 'Date', 'None', 'Field', 'Datasource']);
/** Items that take a mask string (Phase 15.5 edits it). */
const MASK_ITEM_TYPES = new Set([5, 21, 24, 29, 32, 35, 38, 61, 62, 63]);
export const DEFAULT_MASK_NOTE = 'No mask: the Value format default applies.';
/** Number items that take a mask: field, calculated, aggregate, page number, page count. */
const NUMBER_TYPES = new Set([21, 29, 35, 61, 62]);
export const MASK_ROW_ID = 'mask';
export const OMITTED_NOTE = 'Not in the file; default shown.';
export const FONT_NOTE = 'No item font in the file; report font shown.';
export const PLAIN_DECIMAL_NOTE = 'No mask: prints as a plain decimal, no thousands separator.';

/** Report keys used when 053 key 9 is not loaded. */
const REPORT_FALLBACK = [6, 18, 30, 33, 31, 32, 35, 36, 37, 38, 15, 16, 17, 20, 22, 21, 24, 23, 40];

const DEFAULTS = {
  name: 'Report', title: 'Report', link: '',
  family: 'Vanadium Sans', size: '10 pt', style: 'Normal',
  orientation: 'portrait', width: '8.5 in', height: '11 in', margin: '0.25 in',
  gridVisible: true, gridStyle: 'dots', gridSpacing: '0.125 in', gridWeight: '0.5',
  gridColor: 'silver', gridBackground: 'white', align: 'Left',
};

function obj(value) {
  return value && typeof value === 'object' && !Array.isArray(value) ? value : null;
}

function own(source, key) {
  return Boolean(source) && Object.hasOwn(source, key) && source[key] !== '' && source[key] !== null;
}

function cap(text) {
  const value = String(text);
  return value.charAt(0).toUpperCase() + value.slice(1);
}

function setIn(doc, path, value) {
  const next = JSON.parse(JSON.stringify(doc));
  let node = next;
  for (const key of path.slice(0, -1)) {
    if (!obj(node[key])) node[key] = {};
    node = node[key];
  }
  node[path[path.length - 1]] = value;
  return next;
}

function pathValue(doc, path, fallback) {
  let node = doc;
  for (const key of path.slice(0, -1)) {
    node = obj(node && node[key]);
    if (!node) return { value: fallback, omitted: true };
  }
  const last = path[path.length - 1];
  return own(node, last) ? { value: node[last], omitted: false } : { value: fallback, omitted: true };
}

function reportField(path, fallback, toFile = (v) => v, toShow = (v) => v) {
  return {
    get: (doc) => {
      const read = pathValue(doc, path, fallback);
      return { ...read, value: toShow(read.value) };
    },
    set: (doc, _sel, value) => setIn(doc, path, toFile(value)),
  };
}

const bool = (v) => v === true || v === 'True' || v === 'true';
const boolShow = (v) => (v === false || v === 'False' ? 'False' : 'True');

const REPORT_FIELDS = {
  6: reportField(['name'], DEFAULTS.name),
  18: reportField(['title'], DEFAULTS.title),
  29: reportField(['link'], DEFAULTS.link),
  30: {
    get: (doc) => pathValue(doc, ['page', 'sizeKey'], 0),
    set: (doc, _sel, value, ctx) => applyPageSize(doc, value, ctx),
  },
  31: reportField(['page', 'width'], DEFAULTS.width),
  32: reportField(['page', 'height'], DEFAULTS.height),
  33: reportField(['page', 'orientation'], DEFAULTS.orientation, (v) => String(v).toLowerCase(), cap),
  35: reportField(['page', 'margin', 'left'], DEFAULTS.margin),
  36: reportField(['page', 'margin', 'right'], DEFAULTS.margin),
  37: reportField(['page', 'margin', 'top'], DEFAULTS.margin),
  38: reportField(['page', 'margin', 'bottom'], DEFAULTS.margin),
  15: reportField(['font', 'family'], DEFAULTS.family),
  16: reportField(['font', 'size'], DEFAULTS.size),
  17: reportField(['font', 'style'], DEFAULTS.style),
  20: reportField(['grid', 'visible'], DEFAULTS.gridVisible, bool, boolShow),
  21: reportField(['grid', 'color'], DEFAULTS.gridColor),
  22: reportField(['grid', 'style'], DEFAULTS.gridStyle, (v) => String(v).toLowerCase(), cap),
  23: reportField(['grid', 'spacing'], DEFAULTS.gridSpacing),
  24: reportField(['grid', 'background'], DEFAULTS.gridBackground),
  40: reportField(['grid', 'weight'], DEFAULTS.gridWeight, (v) => String(v)),
};

function findSelected(doc, sel) {
  const bands = Array.isArray(doc && doc.bands) ? doc.bands : [];
  const band = bands.find((entry) => entry.id === sel.bandId);
  const items = band && Array.isArray(band.items) ? band.items : [];
  return { band, item: items.find((entry) => entry.id === sel.itemId) };
}

function editItem(doc, sel, change) {
  const next = JSON.parse(JSON.stringify(doc));
  const { item } = findSelected(next, sel);
  if (!item) throw new Error(`Item "${sel.itemId}" was not found.`);
  change(item, next);
  return next;
}

function itemField(key, fallback) {
  return {
    get: (doc, sel) => {
      const { item } = findSelected(doc, sel);
      return own(item, key) ? { value: item[key], omitted: false } : { value: fallback, omitted: true };
    },
    set: (doc, sel, value) => editItem(doc, sel, (item) => { item[key] = value; }),
  };
}

function boxField(side) {
  const resize = side === 'width' || side === 'height';
  return {
    get: (doc, sel) => {
      const { item } = findSelected(doc, sel);
      const box = obj(item && item.box);
      return own(box, side) ? { value: box[side], omitted: false } : { value: '', omitted: true };
    },
    set: (doc, sel, value) => {
      const text = withUnit(value, doc);
      return resize
        ? resizeItem(doc, sel.bandId, sel.itemId, { [side]: text })
        : moveItem(doc, sel.bandId, sel.itemId, { [side]: text });
    },
  };
}

function fontField(part) {
  return {
    get: (doc, sel) => {
      const { item } = findSelected(doc, sel);
      const mine = obj(item && item.font);
      if (own(mine, part)) return { value: mine[part], omitted: false };
      return {
        value: pathValue(doc, ['font', part], DEFAULTS[part]).value,
        omitted: true,
        note: mine ? OMITTED_NOTE : FONT_NOTE,
      };
    },
    set: (doc, sel, value) => editItem(doc, sel, (item, next) => {
      const base = obj(item.font) || obj(next.font) || {};
      item.font = { ...base, [part]: value };
    }),
  };
}

/** Band scope: height only until 053 lists more band attributes. */
const BAND_FIELDS = {
  3: {
    get: (doc, sel) => {
      const band = bandOf(doc, sel);
      return band ? { value: bandHeight(band), omitted: !own(band, 'height') } : { value: '', omitted: true };
    },
    set: (doc, sel, value) => resizeBand(doc, sel.bandId, value),
  },
};

function bandOf(doc, sel) {
  const bands = Array.isArray(doc && doc.bands) ? doc.bands : [];
  return bands.find((band) => band.id === (sel && sel.bandId)) || null;
}

/** Scope of a selection: 'item', 'band' (sel.band with no item) or 'report'. */
export function selectionScope(doc, sel) {
  if (sel && sel.end) return 'end';
  if (sel && sel.itemId && findSelected(doc, sel).item) return 'item';
  if (sel && sel.band && bandOf(doc, sel)) return 'band';
  return 'report';
}

const ITEM_FIELDS = {
  0: boxField('top'),
  1: boxField('left'),
  2: boxField('width'),
  3: boxField('height'),
  4: itemField('align', DEFAULTS.align),
  5: itemField('fit', ''),
  6: itemField('name', ''),
  7: itemField('field', ''),
  8: itemField('datasource', ''),
  14: itemField('rotation', '0'),
  15: fontField('family'),
  16: fontField('size'),
  17: fontField('style'),
  29: itemField('link', ''),
};

/**
 * Client-only sections (no 075 row, no migration). Ids sit far above
 * the 075 key range so they sort last.
 */
export const END_SECTION_ID = 9001;
export const RULER_SECTION_ID = 9002;
export const CLIENT_SECTIONS = [
  { id: END_SECTION_ID, label: 'End of Report', sortSeq: 9001, icon: '<fa fa-flag-checkered></fa>' },
  { id: RULER_SECTION_ID, label: 'Ruler', sortSeq: 9002, icon: '<fa fa-ruler-combined></fa>' },
];

/** Ruler sides; each defaults to shown. Stored as `doc.rulers`. */
export const RULER_SIDES = ['top', 'left', 'right', 'bottom'];
const RULER_ROW_PREFIX = 'ruler-';

/**
 * Ruler sides from the definition. A side is on unless the file
 * says `false`. `on` false (the toolbar toggle) hides every side.
 * @param {object} doc
 * @param {boolean} [on]
 * @returns {{ top: boolean, left: boolean, right: boolean, bottom: boolean }}
 */
export function rulerSides(doc, on = true) {
  const stored = doc && doc.rulers && typeof doc.rulers === 'object' ? doc.rulers : {};
  const out = {};
  for (const side of RULER_SIDES) out[side] = Boolean(on) && stored[side] !== false;
  return out;
}

function rulerRows(doc) {
  const stored = doc && doc.rulers && typeof doc.rulers === 'object' ? doc.rulers : {};
  const sides = rulerSides(doc);
  return RULER_SIDES.map((side) => ({
    id: `${RULER_ROW_PREFIX}${side}`, key: null, label: side[0].toUpperCase() + side.slice(1),
    value: sides[side] ? 'True' : 'False', omitted: !own(stored, side), note: '',
    editor: 'Boolean', list: null, section: RULER_SECTION_ID, group: 'Ruler', readOnly: false,
  }));
}

/** The End of Report marker has no height and nothing to edit. */
function endRows() {
  return [{
    id: 'end-height', key: null, label: 'Height', value: 'None (marker)', omitted: false,
    note: 'Marks where the report ends; it takes no page height.',
    editor: 'None', list: null, section: END_SECTION_ID, group: 'End of Report', readOnly: true,
  }];
}

/** Static text body. Not a 054 key; shown in the Object section. */
const TEXT_ROW_ID = 'text';

/** Number with no unit gets the report unit. */
export function withUnit(value, doc) {
  const text = String(value).trim();
  if (/^-?\d+(\.\d+)?$/.test(text)) {
    const units = pathValue(doc, ['page', 'units'], 'in').value;
    return `${text} ${units}`;
  }
  return text;
}

function applyPageSize(doc, value, ctx) {
  const key = Number(value);
  const sizes = (ctx && ctx.pageSizes) || [];
  const row = sizes.find((entry) => Number(entry.key_idx) === key);
  let next = setIn(doc, ['page', 'sizeKey'], key);
  const spec = row && obj(parseCollection(row.collection));
  if (spec) {
    if (spec['Page Width']) next = setIn(next, ['page', 'width'], spec['Page Width']);
    if (spec['Page Height']) next = setIn(next, ['page', 'height'], spec['Page Height']);
    for (const side of ['Left', 'Right', 'Top', 'Bottom']) {
      const margin = spec[`Page Margin ${side}`];
      if (margin) next = setIn(next, ['page', 'margin', side.toLowerCase()], margin);
    }
    if (spec['Page Orientation']) {
      next = setIn(next, ['page', 'orientation'], String(spec['Page Orientation']).toLowerCase());
    }
  }
  return next;
}

/**
 * Number rows: a length key takes a number and a unit (a bare number
 * gets the report unit); a plain key takes a finite number.
 */
export function checkNumber(key, value, doc) {
  if (LENGTH_KEYS.has(key)) {
    const text = withUnit(value, doc);
    parseLength(text);
    return text;
  }
  if (key === 14 || key === 40) {
    const text = String(value).trim();
    if (!/^-?\d+(\.\d+)?$/.test(text)) throw new Error(`"${text}" is not a number.`);
    return text;
  }
  return value;
}

export function parseCollection(collection) {
  if (typeof collection !== 'string') return obj(collection) || {};
  try {
    return obj(JSON.parse(collection)) || {};
  } catch {
    return {};
  }
}

function attrList(row) {
  const list = parseCollection(row && row.collection).attributes;
  return Array.isArray(list) ? list : null;
}

/**
 * 054 keys the panel shows for this selection, in lookup order.
 * @param {object} doc
 * @param {{ bandId?: string, itemId?: string }} sel
 * @param {{ objectTypes?: object[], attributes?: object[] }} lookups
 * @returns {number[]}
 */
export function attributeKeys(doc, sel, lookups) {
  const types = (lookups && lookups.objectTypes) || [];
  const attrs = (lookups && lookups.attributes) || [];
  const byName = new Map(attrs.map((row) => [String(row.value_txt), Number(row.key_idx)]));
  const toKey = (entry) => (typeof entry === 'number' ? entry : byName.get(String(entry)));
  const { item } = sel && sel.itemId ? findSelected(doc, sel) : {};
  if (!item && selectionScope(doc, sel) === 'band') {
    const row = types.find((entry) => Number(entry.key_idx) === Number(bandOf(doc, sel).typeKey));
    const list = attrList(row) || [3];
    return list.map(toKey).filter(Number.isInteger);
  }
  if (item) {
    const row = types.find((entry) => Number(entry.key_idx) === Number(item.typeKey));
    const list = attrList(row) || [0, 1, 2, 3, 4, 14, 29];
    return list.map(toKey).filter(Number.isInteger);
  }
  const top = types.find((entry) => Number(entry.key_idx) === REPORT_TYPE_KEY);
  const list = attrList(top);
  const keys = list ? list.map(toKey).filter(Number.isInteger) : REPORT_FALLBACK.slice();
  if (!keys.includes(6)) keys.unshift(6);
  return keys;
}

/**
 * Choices for a Field or Datasource row. Field lists the session
 * sample's columns; Datasource lists the definition's `datasources`.
 * The current value stays listed. Null when the row is not a binding.
 */
function bindingChoices(editor, doc, lookups, current) {
  let names;
  if (editor === 'Field') names = (lookups && lookups.sampleColumns) || [];
  else if (editor === 'Datasource') names = datasourceNames(doc);
  else return null;
  const list = [...names];
  if (typeof current === 'string' && current !== '' && !list.includes(current)) list.unshift(current);
  return list;
}

/** A binding with nothing to pick from is typed as Text. */
function bindingEditor(editor, choices) {
  if (!choices) return editor;
  return choices.length ? 'List' : 'Text';
}

function editorFor(key, collection) {
  if (TEXT_KEYS.has(key)) return 'Text';
  if (key === PAGE_SIZE_KEY) return 'PageSize';
  return collection.Editor || 'Text';
}

/**
 * Panel rows for the selection.
 * @returns {object[]} `{ id, key, label, value, omitted, editor, list, section, readOnly }`
 */
export function propertyRows(doc, sel, lookups, labelOf = (row, c) => c.Label || row.value_txt, unknown = []) {
  const attrs = (lookups && lookups.attributes) || [];
  const scope = selectionScope(doc, sel);
  if (scope === 'end') return endRows();
  const isItem = scope === 'item';
  const fields = { item: ITEM_FIELDS, band: BAND_FIELDS, report: REPORT_FIELDS }[scope];
  const rows = [];
  if (isItem && findSelected(doc, sel).item.typeKey === 0) {
    const { item } = findSelected(doc, sel);
    rows.push({
      id: TEXT_ROW_ID, key: null, label: 'Text', value: item.text ?? '', omitted: !own(item, 'text'), note: '',
      editor: 'Text', list: null, section: 4, group: 'Object', readOnly: false,
    });
  }
  for (const key of attributeKeys(doc, sel, lookups)) {
    const row = attrs.find((entry) => Number(entry.key_idx) === key) || { key_idx: key, value_txt: `Key ${key}` };
    const collection = parseCollection(row.collection);
    const field = fields[key];
    const read = field ? field.get(doc, sel) : { value: '', omitted: true };
    const editor = editorFor(key, collection);
    if (!KNOWN_EDITORS.has(editor)) {
      unknown.push({ key, editor });
      continue;
    }
    const choices = bindingChoices(editor, doc, lookups, read.value);
    rows.push({
      id: String(key),
      key,
      label: labelOf(row, collection),
      value: read.value,
      omitted: read.omitted,
      note: read.omitted ? (read.note || OMITTED_NOTE) : '',
      editor: bindingEditor(editor, choices),
      list: choices || (editor === 'List' && Array.isArray(collection.List) ? collection.List : null),
      section: collection.section ?? null,
      group: collection.Group || '',
      readOnly: !field || editor === 'None',
    });
  }
  if (isItem) {
    const { item } = findSelected(doc, sel);
    const typeKey = Number(item.typeKey);
    if (MASK_ITEM_TYPES.has(typeKey)) {
      const empty = !own(item, 'mask');
      rows.push({
        id: MASK_ROW_ID, key: null, label: 'Mask', value: empty ? '' : item.mask, omitted: empty,
        note: empty ? (NUMBER_TYPES.has(typeKey) ? PLAIN_DECIMAL_NOTE : DEFAULT_MASK_NOTE) : '',
        editor: 'Text', list: null, section: 7, group: 'Formatting', readOnly: false,
      });
    }
  }
  if (scope === 'report') rows.push(...rulerRows(doc));
  return rows;
}

/**
 * New document after one panel edit.
 * @param {object} doc
 * @param {object} sel
 * @param {string} rowId 054 key as text, or 'text'.
 * @param {*} value
 * @param {{ pageSizes?: object[] }} [ctx]
 * @returns {object}
 */
export function applyProperty(doc, sel, rowId, value, ctx) {
  const isItem = Boolean(sel && sel.itemId && findSelected(doc, sel).item);
  if (rowId === TEXT_ROW_ID) {
    if (!isItem) throw new Error('Text needs a selected item.');
    return editItem(doc, sel, (item) => { item.text = String(value); });
  }
  if (rowId === MASK_ROW_ID) {
    if (!isItem) throw new Error('Mask needs a selected item.');
    return editItem(doc, sel, (item) => { item.mask = String(value ?? '').trim(); });
  }
  const scope = selectionScope(doc, sel);
  if (scope === 'end') throw new Error('End of Report has no editable properties.');
  if (typeof rowId === 'string' && rowId.startsWith(RULER_ROW_PREFIX)) {
    const side = rowId.slice(RULER_ROW_PREFIX.length);
    if (scope !== 'report' || !RULER_SIDES.includes(side)) throw new Error(`Property ${rowId} is read only.`);
    const next = JSON.parse(JSON.stringify(doc));
    const rulers = next.rulers && typeof next.rulers === 'object' ? next.rulers : {};
    rulers[side] = value === true || value === 'True';
    next.rulers = rulers;
    return next;
  }
  const fields = { item: ITEM_FIELDS, band: BAND_FIELDS, report: REPORT_FIELDS }[scope];
  const key = Number(rowId);
  const field = fields[key];
  if (!field) throw new Error(`Property ${rowId} is read only.`);
  return field.set(doc, sel, checkNumber(key, value, doc), ctx);
}
