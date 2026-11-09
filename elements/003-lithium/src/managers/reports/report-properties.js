/**
 * Property panel rows for the report editor (Phase 14).
 * Rows come from lookup 054. The selection picks the list:
 * no item selected uses lookup 053 key 9 (Report Top), which
 * lists attribute names. A selected item uses its 053 row,
 * which lists 054 keys. Key 24 (Grid Background Color) is a
 * report attribute only and is dropped from item lists here.
 * Keys 21 and 24 use the Color editor by name (lookup says Text).
 * Key 29 (Link) is plain text and the last row in Object.
 * Key 30 (Page Size) takes its
 * choices from lookup 057, not from its own List.
 * Page Size copies width, height, margins, and orientation from 057.
 * Values the file omits show the Defaults table value.
 */

import {
  AGGREGATE_FUNCTION_ROW_ID, AGGREGATE_OP_ROW_ID, AGGREGATE_OPS, AGGREGATE_WEIGHT_ROW_ID,
  aggregateFunctionChoices, isAggregateType, setAggregateField, setAggregateFunction,
  setAggregateOp, setAggregateWeight,
} from './reports-aggregate.js';
import { GROUP_HEADER_TYPE, bandHeight, moveItem, resizeBand, resizeItem } from './model/document.js';
import { DateTime } from 'luxon';
import { displayLength, parseLength } from './model/units.js';
import { NONE_COLOR, normalizeColorValue } from './reports-colours.js';
import {
  addDatasource, datasourceColumns, datasourceKinds, datasourceList, datasourceNames,
  LIVE_DS_CODES, removeDatasource, renameDatasource, setDatasourceKind,
  PAGE_LABEL_DEFAULT, PAGE_LABEL_TYPE, PARAM_TYPE, parameterNames,
  QUERY_VALUE_TYPES, boundFieldText, orderBindingKeys,
} from './reports-sample.js';
import {
  findReportingQuery, queryLabel, setDatasourceParam, setDatasourceParams, setDatasourceQuery,
} from './reports-query.js';
import {
  applyBarcodeProperty, barcodeAttrMeta, barcodeAttributeKeys, barcodeItemFields, barcodeRows, symbologyList,
} from './reports-barcode.js';
import {
  applyShapeFill, fillStyleChoices, shapeAttrMeta, shapeAttributeKeys, shapeFillRows,
  shapeItemFields, shapeKindFromValue, shapeKindOptions,
} from './reports-shape.js';
import {
  LAYOUT_TYPES, STATIC_DATE, STATIC_IMAGE, STATIC_MEMO, STATIC_NUMBER,
  applyStaticProperty, embedFontsRow, imageFitRow, imageRows, layoutRows, numberMaskRow, staticValueRows,
} from './reports-static.js';
import { IMAGE_FITS } from './model/images.js';

export const REPORT_TYPE_KEY = 9;
const TEXT_KEYS = new Set([29]);
const PAGE_SIZE_KEY = 30;
/** Number keys that hold a length; the rest are plain numbers. */
const LENGTH_KEYS = new Set([0, 1, 2, 3, 16, 23, 31, 32, 35, 36, 37, 38, 43, 48, 49, 50, 51]);
/** Page Width/Height/Margins — bare numbers use Page Units. */
const PAGE_LENGTH_KEYS = new Set([31, 32, 35, 36, 37, 38]);
const FONT_SIZE_KEY = 16;
/** Font Size editable combo presets (Phase 76). */
export const FONT_SIZE_LIST = [8, 10, 12, 14, 16, 18, 20, 22, 24, 28, 32, 40, 48, 64];
/** Editors the panel knows. Field and Datasource (Phase 15) become Lists. */
export const KNOWN_EDITORS = new Set(['Text', 'Number', 'List', 'Boolean', 'Color', 'PageSize', 'Date', 'None', 'Field', 'Datasource', 'File', 'FontFamily', 'FontSize', 'FontStyle']);
/** Items that take a mask string (Phase 15.5 edits it). */
const MASK_ITEM_TYPES = new Set([1, 4, 5, 21, 24, 29, 32, 35, 38, 61, 62, 63]);
export const DEFAULT_MASK_NOTE = 'No mask: the Value format default applies.';
/** Number items that take a mask: field, calculated, aggregate, page number, page count. */
const NUMBER_TYPES = new Set([1, 21, 29, 35, 61, 62]);
export const MASK_ROW_ID = 'mask';
export const OMITTED_NOTE = 'Not in the file; default shown.';
export const FONT_NOTE = 'No item font in the file; report font shown.';
export const PLAIN_DECIMAL_NOTE = 'No mask: prints as a plain decimal, no thousands separator.';

/** Report keys used when 053 key 9 is not loaded. */
const REPORT_FALLBACK = [6, 18, 30, 33, 31, 32, 35, 36, 37, 38, 53, 54, 34, 15, 16, 17, 20, 22, 21, 24, 23, 40];

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
  34: reportField(['page', 'units'], 'in', (v) => String(v).toLowerCase()),
  53: reportField(['page', 'positionUnits'], 'in', (v) => String(v).toLowerCase()),
  54: reportField(['page', 'fontUnits'], 'pt', (v) => String(v).toLowerCase()),
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
      const text = withUnit(value, doc, 'position');
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
  /** Group field: group header only. The footer shares it. */
  7: {
    get: (doc, sel) => {
      const band = bandOf(doc, sel);
      return band && own(band, 'field') ? { value: band.field, omitted: false } : { value: '', omitted: true };
    },
    set: (doc, sel, value) => {
      const next = JSON.parse(JSON.stringify(doc));
      const band = bandOf(next, sel);
      if (!band || band.typeKey !== GROUP_HEADER_TYPE) throw new Error('Group field is set on the group header.');
      band.field = String(value ?? '').trim();
      return next;
    },
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
  55: {
    get: (doc, sel) => {
      const { item } = findSelected(doc, sel);
      return own(item, 'foreColor')
        ? { value: item.foreColor, omitted: false }
        : { value: '', omitted: true };
    },
    set: (doc, sel, value) => {
      const color = normalizeColorValue(value);
      return editItem(doc, sel, (item) => {
        if (color === NONE_COLOR) item.foreColor = NONE_COLOR;
        else item.foreColor = color;
      });
    },
  },
  56: {
    get: (doc, sel) => {
      const { item } = findSelected(doc, sel);
      return own(item, 'backColor')
        ? { value: item.backColor, omitted: false }
        : { value: '', omitted: true };
    },
    set: (doc, sel, value) => {
      const color = normalizeColorValue(value);
      return editItem(doc, sel, (item) => {
        if (color === NONE_COLOR) item.backColor = NONE_COLOR;
        else item.backColor = color;
      });
    },
  },
  57: {
    get: (doc, sel) => {
      const { item } = findSelected(doc, sel);
      if (own(item, 'backgroundColor')) return { value: item.backgroundColor, omitted: false };
      // Shape legacy: show fillPaint colour as the omitted default.
      const paint = item && item.fillPaint && typeof item.fillPaint === 'object' ? item.fillPaint : null;
      if (paint && typeof paint.color === 'string' && paint.color !== '') {
        return { value: paint.color, omitted: true, note: OMITTED_NOTE };
      }
      const stop = paint && Array.isArray(paint.stops) && paint.stops[0];
      if (stop && typeof stop.color === 'string' && stop.color !== '') {
        return { value: stop.color, omitted: true, note: OMITTED_NOTE };
      }
      return { value: '', omitted: true };
    },
    set: (doc, sel, value) => {
      return editItem(doc, sel, (item) => {
        const color = normalizeColorValue(value);
        item.backgroundColor = color;
        // Keep shape fillPaint colour in step with the shared background.
        if (Number(item.typeKey) === 25 || Number(item.typeKey) === 26) {
          const paint = item.fillPaint && typeof item.fillPaint === 'object' ? item.fillPaint : null;
          const kind = paint && typeof paint.kind === 'string' ? paint.kind : 'solid';
          if (kind === 'linear' || kind === 'radial') {
            const stops = Array.isArray(paint.stops) && paint.stops.length
              ? paint.stops.map((stop, index) => (index === 0 ? { ...stop, color } : { ...stop }))
              : [{ at: 0, color }, { at: 1, color }];
            const next = { kind, stops };
            if (kind === 'linear') {
              const angle = Number(paint.angle);
              next.angle = Number.isFinite(angle) ? angle : 0;
            }
            item.fillPaint = next;
          } else {
            item.fillPaint = { kind: 'solid', color };
          }
        }
      });
    },
  },
};

Object.assign(ITEM_FIELDS, shapeItemFields, barcodeItemFields);

/**
 * Client-only sections (no 075 row, no migration). Ids sit far above
 * the 075 key range so they sort last.
 */
export const END_SECTION_ID = 9001;
export const RULER_SECTION_ID = 9002;
export const DATASOURCE_SECTION_ID = 9003;
/** Free-text notes. No lookup row and no migration. */
export const NOTES_SECTION_ID = 9004;
export const NOTES_ROW_ID = 'notes';
export const ID_ROW_ID = 'object-id';
/** Most datasources a report may declare (Count editor bound). */
export const DS_MAX = 20;
/** Section id for "Datasource N" (0-based index); one section per datasource. */
export const datasourceSectionId = (index) => 9100 + index;
export const DATASOURCE_ITEM_SECTIONS = Array.from({ length: DS_MAX }, (_, index) => ({
  id: datasourceSectionId(index), label: `Datasource ${index + 1}`,
  sortSeq: 8999 + (index + 1) / 100, icon: '<fa fa-table></fa>',
}));
export const CLIENT_SECTIONS = [
  { id: NOTES_SECTION_ID, label: 'Notes', sortSeq: 20, icon: '<fa fa-note-sticky></fa>' },
  { id: DATASOURCE_SECTION_ID, label: 'Datasources', sortSeq: 8999, icon: '<fa fa-database></fa>' },
  ...DATASOURCE_ITEM_SECTIONS,
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

const DS_ROW_PREFIX = 'ds-';
export const DS_COUNT_ROW_ID = 'ds-count';

/**
 * Report-scope Datasources rows (Phase 19b). Count adds or trims from
 * the end; each datasource gets its own panel section "Datasource N"
 * (ids 9100+) with Name, Kind, (Query + one row per parameter) and a
 * multiline Fields list. A blank Name removes that datasource.
 */
function datasourceRows(doc, lookups) {
  const kinds = datasourceKinds(lookups && lookups.dsKinds);
  const list = datasourceList(doc);
  const base = { omitted: false, note: '', list: null, section: DATASOURCE_SECTION_ID, readOnly: false };
  const rows = [{
    ...base, id: DS_COUNT_ROW_ID, key: null, label: 'Count', value: String(list.length), editor: 'Number',
    group: 'Datasources', note: 'The first datasource drives iteration.',
  }];
  list.forEach((ds, index) => {
    const group = `Datasource ${index + 1}`;
    const section = datasourceSectionId(index);
    const kind = kinds.find((k) => k.key === Number(ds.kindKey)) || kinds.find((k) => k.code === (ds.kind || 'json'));
    const count = Array.isArray(ds.rows) ? ds.rows.length : 0;
    const cols = datasourceColumns(doc, ds.name);
    rows.push(
      { ...base, id: `${DS_ROW_PREFIX}${index}-name`, section, key: null, label: 'Name', value: ds.name || '', editor: 'Text', group,
        note: 'One word, unique. Blank removes this datasource.' },
      { ...base, id: `${DS_ROW_PREFIX}${index}-kind`, section, key: null, label: 'Kind', value: kind ? kind.label : String(ds.kind || ''),
        editor: 'List', group,
        list: kinds.map((k) => k.label),
        note: kinds.filter((k) => !LIVE_DS_CODES.includes(k.code)).map((k) => k.label).join(', ') + ' not available yet.' },
      ...queryRows(ds, index, section, group, lookups, base),
      { ...base, id: `${DS_ROW_PREFIX}${index}-fields`, section, key: null, label: 'Fields', editor: 'None', group, readOnly: true,
        multiline: true, value: fieldLines(ds, cols),
        note: ds.kind === 'query'
          ? `${count} row${count === 1 ? '' : 's'} stored. Run (Field toolbar) or Compose fetches rows.`
          : `${count} row${count === 1 ? '' : 's'} stored. Load Sample with this datasource's band selected (or none) stores rows here.` },
    );
  });
  return rows;
}

/** Query and Params rows for a query datasource (Phase 28). */
function queryRows(ds, index, section, group, lookups, base) {
  if (ds.kind !== 'query') return [];
  const queries = (lookups && lookups.reportingQueries) || [];
  const current = ds.queryRef == null ? null : findReportingQuery(queries, ds.queryRef);
  const value = current ? queryLabel(current) : (ds.queryRef == null ? '' : `#${ds.queryRef} ${ds.queryName || ''}`.trim());
  const list = queries.map(queryLabel);
  if (value && !list.includes(value)) list.unshift(value);
  const params = ds.params || {};
  const meta = current ? current.parameters.filter((p) => p.kind !== 'calculated') : [];
  const names = [...meta.map((p) => p.name), ...Object.keys(params).filter((n) => !meta.some((p) => p.name === n))];
  return [
    { ...base, id: `${DS_ROW_PREFIX}${index}-query`, section, key: null, label: 'Query', value, group,
      editor: list.length ? 'List' : 'Text', list: list.length ? list : null,
      note: queries.length ? 'Reporting queries (lookup 028 SQL - Reporting). Picking one resets parameters to their defaults.'
        : 'No Reporting queries loaded. Use Refresh Queries, or type a QueryRef number.' },
    ...names.map((name) => {
      const p = meta.find((m) => m.name === name);
      const type = shortType(p ? p.type : 'string');
      const editor = type === 'int' || type === 'float' ? 'Number' : type === 'bool' ? 'Boolean' : 'Text';
      const raw = params[name];
      return {
        ...base, id: `${DS_ROW_PREFIX}${index}-param-${name}`, section, key: null, label: name, labelType: type, group, editor,
        value: editor === 'Boolean' ? (raw === true || raw === 'true' || raw === 1 ? 'True' : 'False') : (raw == null ? '' : String(raw)),
        note: `Query parameter ${name} (${p ? p.type : 'string'})${p && p.default !== undefined ? `, default ${p.default}` : ''}.`,
      };
    }),
  ];
}

const SHORT_TYPES = Object.freeze({
  integer: 'int', int: 'int', float: 'float', number: 'float', double: 'float', boolean: 'bool', bool: 'bool',
  string: 'str', str: 'str', text: 'str', memo: 'memo', date: 'date', datetime: 'datetime', time: 'time',
});

/** Short datatype tag for panel labels (int, float, str, memo...). */
export function shortType(type) {
  const t = String(type || 'string').toLowerCase();
  return SHORT_TYPES[t] || t;
}

/** Fields value: one "name type" per line from metadata, else columns. */
function fieldLines(ds, cols) {
  const meta = Array.isArray(ds.fields) ? ds.fields : [];
  const types = new Map(meta.map((f) => [f.name, f.type]));
  const names = cols.length ? cols : meta.map((f) => f.name);
  return names.map((n) => (types.has(n) ? `${n}  ${shortType(types.get(n))}` : n)).join('\n');
}

function applyDatasource(doc, rowId, value, ctx) {
  if (rowId === DS_COUNT_ROW_ID) {
    const want = Number.parseInt(String(value).trim(), 10);
    if (!Number.isInteger(want) || want < 0 || want > DS_MAX) throw new Error(`Datasource count must be 0 to ${DS_MAX}.`);
    let next = doc;
    while (datasourceList(next).length < want) next = addDatasource(next);
    while (datasourceList(next).length > want) next = removeDatasource(next, datasourceList(next).length - 1);
    return next;
  }
  const param = /^ds-(\d+)-param-(.+)$/.exec(rowId);
  if (param) return setDatasourceParam(doc, Number(param[1]), param[2], value);
  const match = /^ds-(\d+)-(name|kind|query|params)$/.exec(rowId);
  if (!match) throw new Error(`Property ${rowId} is read only.`);
  const index = Number(match[1]);
  if (index >= datasourceList(doc).length) throw new Error(`Datasource ${index + 1} does not exist.`);
  if (match[2] === 'name') {
    if (String(value ?? '').trim() === '') return removeDatasource(doc, index);
    return renameDatasource(doc, index, value);
  }
  if (match[2] === 'query') return setDatasourceQuery(doc, index, value, ctx && ctx.reportingQueries);
  if (match[2] === 'params') return setDatasourceParams(doc, index, value);
  return setDatasourceKind(doc, index, value, datasourceKinds(ctx && ctx.dsKinds));
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
export const PARAM_ROW_ID = 'param';
/** Page Label (64) template; {n} page number, {c} page count (Phase 23). */
export const TEMPLATE_ROW_ID = 'template';

/** Phase 22: datetime items (System Date 5, Field datetime 24) take a timezone. */
const ZONE_ITEM_TYPES = new Set([5, 24]);
export const TZ_MODES = Object.freeze(['User', 'UTC', 'Explicit']);
export const TZ_ROW_ID = 'timezone';
export const ZONE_ROW_ID = 'zone';
export const PRESET_ROW_ID = 'mask-preset';
export const DEFAULT_ZONE_ROW_ID = 'defaultZone';

/** Lookup 077 rows as { caption, mask }. */
export function datetimePresets(rows) {
  const out = [];
  for (const row of Array.isArray(rows) ? rows : []) {
    const spec = parseCollection(row && row.collection);
    if (spec && typeof spec.mask === 'string' && spec.mask !== '') out.push({ caption: String(row.value_txt || spec.mask), mask: spec.mask });
  }
  return out;
}

/** Picker entry: caption with a live sample at now. */
export function presetLabel(preset, now = DateTime.now()) {
  let sample;
  try { sample = now.toFormat(preset.mask); } catch { sample = preset.mask; }
  return `${preset.caption} — ${sample}`;
}

function zoneRows(item, presets) {
  const typeKey = Number(item.typeKey);
  const mode = TZ_MODES.includes(item.timezone) ? item.timezone : '';
  const rows = [{
    id: TZ_ROW_ID, key: null, label: 'Timezone', value: mode, omitted: !own(item, 'timezone'),
    note: mode ? '' : (typeKey === 24 ? 'Not set: the value prints as stored.' : 'Not set: UTC.'),
    editor: 'List', list: [...TZ_MODES], section: 7, group: 'Formatting', readOnly: false,
  }];
  if (mode === 'Explicit') {
    rows.push({
      id: ZONE_ROW_ID, key: null, label: 'Zone', value: item.zone ?? '', omitted: !own(item, 'zone'),
      note: 'IANA name, e.g. America/Vancouver. Unknown zones print UTC with a warning.',
      editor: 'Text', list: null, section: 7, group: 'Formatting', readOnly: false,
    });
  }
  if (presets.length) {
    rows.push({
      id: PRESET_ROW_ID, key: null, label: 'Mask preset', value: '', omitted: true,
      note: 'Pick one to copy its mask into Mask (lookup 077).',
      editor: 'List', list: presets.map((p) => presetLabel(p)), section: 7, group: 'Formatting', readOnly: false,
    });
  }
  return rows;
}

/** Report Position Units (default in). Changing it does not rewrite stored lengths. */
export function positionUnitsOf(doc) {
  return pathValue(doc, ['page', 'positionUnits'], 'in').value;
}

/** Report Font Units (default pt). */
export function fontUnitsOf(doc) {
  return pathValue(doc, ['page', 'fontUnits'], 'pt').value;
}

/** Report Page Units (054 key 34; default in). */
export function pageUnitsOf(doc) {
  return pathValue(doc, ['page', 'units'], 'in').value;
}

/**
 * Bare number gets the report unit for `kind`: position (default), font, or page.
 * Values that already carry a unit are left alone.
 */
export function withUnit(value, doc, kind = 'position') {
  const text = String(value).trim();
  if (/^-?\d+(\.\d+)?$/.test(text)) {
    const units = kind === 'font' ? fontUnitsOf(doc)
      : kind === 'page' ? pageUnitsOf(doc)
        : positionUnitsOf(doc);
    return `${text} ${units}`;
  }
  return text;
}

/** Display-only `#0.## unit` for a stored length; falls back to the raw text. */
export function displayLengthValue(value) {
  try {
    return displayLength(String(value));
  } catch {
    return value === undefined || value === null ? '' : String(value);
  }
}

/** Which unit kind a length key uses for bare numbers. */
export function lengthUnitKind(key) {
  if (key === FONT_SIZE_KEY) return 'font';
  if (PAGE_LENGTH_KEYS.has(key)) return 'page';
  return 'position';
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
 * gets Position/Font/Page Units as appropriate); a plain key takes a
 * finite number. Saved strings keep full precision.
 */
export function checkNumber(key, value, doc) {
  if (LENGTH_KEYS.has(key)) {
    const text = withUnit(value, doc, lengthUnitKind(key));
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
    const band = bandOf(doc, sel);
    const row = types.find((entry) => Number(entry.key_idx) === Number(band.typeKey));
    const list = attrList(row) || [3];
    const keys = list.map(toKey).filter(Number.isInteger);
    if (band.typeKey === GROUP_HEADER_TYPE) {
      if (!keys.includes(7)) keys.push(7);
    } else {
      return keys.filter((key) => key !== 7);
    }
    return keys;
  }
  if (item) {
    const row = types.find((entry) => Number(entry.key_idx) === Number(item.typeKey));
    const list = attrList(row) || [0, 1, 2, 3, 4, 14, 29];
    const keys = list.map(toKey).filter(Number.isInteger);
    const typeKey = Number(item.typeKey);
    if (typeKey === 25 || typeKey === 26) {
      let view = item;
      if (typeKey === 26) {
        const code = shapeKindFromValue(boundFieldText(doc, item));
        if (code) view = { ...item, kind: code };
      }
      const shaped = shapeAttributeKeys(view, keys);
      return typeKey === 26 ? orderBindingKeys(shaped) : shaped;
    }
    if (typeKey === 43 || typeKey === 45) {
      return barcodeAttributeKeys(typeKey, keys);
    }
    if (QUERY_VALUE_TYPES.includes(typeKey)) {
      const next = orderBindingKeys(keys);
      if (typeKey === 22 || typeKey === 23) {
        for (const key of [5, 15, 16, 17]) {
          if (next.includes(key)) continue;
          const at = next.indexOf(14);
          if (at >= 0) next.splice(at, 0, key);
          else next.push(key);
        }
      }
      return next;
    }
    return keys;
  }
  const top = types.find((entry) => Number(entry.key_idx) === REPORT_TYPE_KEY);
  const list = attrList(top);
  const keys = list ? list.map(toKey).filter(Number.isInteger) : REPORT_FALLBACK.slice();
  if (!keys.includes(6)) keys.unshift(6);
  return keys;
}

/**
 * Choices for a Field or Datasource row. Field lists the band
 * datasource's columns (the caller passes them as sampleColumns); Datasource lists the definition's `datasources`.
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
  if (key === 15) return 'FontFamily';
  if (key === FONT_SIZE_KEY) return 'FontSize';
  if (key === 17) return 'FontStyle';
  // Lookups say Color; keep Color even if a row is stale Text.
  if (key === 55 || key === 56 || key === 57) return 'Color';
  return collection.Editor || 'Text';
}

/** The object the Notes row writes. End of Report has none. */
function notesOwner(doc, sel) {
  const scope = selectionScope(doc, sel);
  if (scope === 'item') return findSelected(doc, sel).item || null;
  if (scope === 'band') return bandOf(doc, sel);
  if (scope === 'report') return doc;
  return null;
}

/**
 * Read-only id first in Object, notes on the object, Link last in Object.
 * A blank notes string is omitted from the file.
 */
function finishPanelRows(rows, doc, sel) {
  const scope = selectionScope(doc, sel);
  const owner = notesOwner(doc, sel);
  if (owner && (scope === 'item' || scope === 'band') && owner.id) {
    rows.unshift({
      id: ID_ROW_ID, key: null, label: 'ID', value: String(owner.id), omitted: false, note: '',
      editor: 'None', list: null, section: 4, group: 'Object', readOnly: true,
    });
  }
  if (owner) {
    const value = typeof owner.notes === 'string' ? owner.notes : '';
    rows.push({
      id: NOTES_ROW_ID, key: null, label: 'Notes', value, omitted: value === '', note: '',
      editor: 'Text', multiline: true, list: null, section: NOTES_SECTION_ID, group: 'Notes', readOnly: false,
    });
  }
  const linkAt = rows.findIndex((row) => row.key === 29);
  if (linkAt >= 0) rows.push(rows.splice(linkAt, 1)[0]);
  return rows;
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
  if (isItem && (findSelected(doc, sel).item.typeKey === 0 || Number(findSelected(doc, sel).item.typeKey) === STATIC_MEMO)) {
    const { item } = findSelected(doc, sel);
    rows.push({
      id: TEXT_ROW_ID, key: null, label: 'Text', value: item.text ?? '', omitted: !own(item, 'text'), note: '',
      editor: 'Text', list: null, section: 4, group: 'Object', readOnly: false,
    });
  }
  if (isItem && (Number(findSelected(doc, sel).item.typeKey) === STATIC_NUMBER || Number(findSelected(doc, sel).item.typeKey) === STATIC_DATE)) {
    rows.push(...staticValueRows(findSelected(doc, sel).item));
  }
  if (isItem && Number(findSelected(doc, sel).item.typeKey) === PARAM_TYPE) {
    const { item } = findSelected(doc, sel);
    const names = parameterNames(doc);
    const value = item.param ?? '';
    rows.push({
      id: PARAM_ROW_ID, key: null, label: 'Parameter', value, omitted: !own(item, 'param'),
      note: names.length ? 'Datasource parameter to print (NAME, or ds.NAME when two datasources share it).'
        : 'No datasource parameters yet. Pick a Query on the report, or type a name.',
      editor: names.length ? 'List' : 'Text', list: names.length ? (names.includes(value) || !value ? names : [value, ...names]) : null,
      section: 4, group: 'Object', readOnly: false,
    });
  }
  if (isItem && isAggregateType(Number(findSelected(doc, sel).item.typeKey))) {
    const { item } = findSelected(doc, sel);
    const op = AGGREGATE_OPS.includes(item.op) ? item.op : 'count';
    const names = aggregateFunctionChoices(doc.functions, op);
    const current = typeof item.function === 'string' ? item.function : '';
    const list = current !== '' && !names.includes(current) ? [current, ...names] : names;
    const columns = (lookups && lookups.sampleColumns) || [];
    rows.push({
      id: AGGREGATE_OP_ROW_ID, key: null, label: 'Op', value: op, omitted: !own(item, 'op'),
      note: 'count, filled, and unique count rows. sum, avg, mean, weighted, and median use numbers. mode, min, max, first, and last keep the value.',
      editor: 'List', list: [...AGGREGATE_OPS], section: 4, group: 'Object', readOnly: false,
    });
    if (op === 'weighted') {
      const weight = typeof item.weight === 'string' ? item.weight : '';
      const weightList = [...columns];
      if (weight !== '' && !weightList.includes(weight)) weightList.unshift(weight);
      rows.push({
        id: AGGREGATE_WEIGHT_ROW_ID, key: null, label: 'Weight', value: weight, omitted: weight === '',
        note: 'Field multiplied by the value. A missing weight prints blank.',
        editor: weightList.length ? 'List' : 'Text', list: weightList.length ? weightList : null,
        section: 4, group: 'Object', readOnly: false,
      });
    }
    rows.push({
      id: AGGREGATE_FUNCTION_ROW_ID, key: null, label: 'Function', value: current, omitted: current === '',
      note: names.length
        ? 'A function whose returns allows this op. Choosing one clears Field. The total of a function waits for the formula compiler.'
        : 'No function in this report allows this op. Bind a field.',
      editor: list.length ? 'List' : 'Text', list: list.length ? list : null,
      section: 4, group: 'Object', readOnly: false,
    });
  }
  if (isItem && Number(findSelected(doc, sel).item.typeKey) === PAGE_LABEL_TYPE) {
    const { item } = findSelected(doc, sel);
    const empty = !own(item, 'template') || item.template === '';
    rows.push({
      id: TEMPLATE_ROW_ID, key: null, label: 'Template', value: empty ? '' : item.template, omitted: empty,
      note: `{n} prints the page number, {c} the page count. Blank uses "${PAGE_LABEL_DEFAULT}".`,
      editor: 'Text', list: null, section: 4, group: 'Object', readOnly: false,
    });
  }
  const itemType = isItem ? Number(findSelected(doc, sel).item.typeKey) : 0;
  for (const key of attributeKeys(doc, sel, lookups)) {
    if (isItem && key === 14 && (itemType === STATIC_NUMBER || itemType === STATIC_DATE)) continue;
    const found = attrs.find((entry) => Number(entry.key_idx) === key);
    const meta = shapeAttrMeta(key) || ((itemType === 43 || itemType === 45) ? barcodeAttrMeta(key) : null);
    const row = found || { key_idx: key, value_txt: meta ? meta.Label : `Key ${key}` };
    const parsed = found ? parseCollection(found.collection) : {};
    const collection = meta
      ? { ...meta, ...parsed, section: meta.section, Group: meta.Group }
      : parsed;
    const field = fields[key];
    const read = field ? field.get(doc, sel) : { value: '', omitted: true };
    const editor = editorFor(key, collection);
    if (!KNOWN_EDITORS.has(editor)) {
      unknown.push({ key, editor });
      continue;
    }
    let choices = bindingChoices(editor, doc, lookups, read.value);
    if (QUERY_VALUE_TYPES.includes(itemType) && key === 7) {
      const bound = findSelected(doc, sel).item;
      const source = typeof bound.datasource === 'string' ? bound.datasource.trim() : '';
      if (source) {
        const names = datasourceColumns(doc, source);
        if (typeof read.value === 'string' && read.value !== '' && !names.includes(read.value)) names.unshift(read.value);
        choices = names;
      }
    }
    // FontFamily/FontStyle/FontSize are List-backed; do not require editor === 'List'.
    const listBacked = editor === 'List' || editor === 'FontFamily'
      || editor === 'FontStyle' || editor === 'FontSize';
    let list = choices || (listBacked && Array.isArray(collection.List) ? collection.List : null);
    if (key === 5 && (itemType === STATIC_IMAGE || itemType === 22)) list = [...IMAGE_FITS];
    if (key === 41) {
      list = shapeKindOptions(lookups && lookups.shapeKinds).map((entry) => ({
        value: entry.code, label: entry.label,
      }));
    }
    if (key === 52) list = symbologyList(lookups);
    if (key === 46) list = fillStyleChoices();
    if (key === FONT_SIZE_KEY) {
      list = FONT_SIZE_LIST.map((n) => ({ value: String(n), label: String(n) }));
    }
    const showDisplay = LENGTH_KEYS.has(key);
    rows.push({
      id: String(key),
      key,
      label: labelOf(row, collection),
      value: read.value,
      display: showDisplay ? displayLengthValue(read.value) : undefined,
      omitted: read.omitted,
      note: read.omitted ? (read.note || OMITTED_NOTE) : '',
      editor: bindingEditor(editor, choices),
      list,
      section: collection.section ?? null,
      group: collection.Group || '',
      readOnly: !field || editor === 'None',
    });
  }
  if (isItem) {
    const { item } = findSelected(doc, sel);
    const typeKey = Number(item.typeKey);
    if (LAYOUT_TYPES.has(typeKey)) rows.push(...layoutRows(item));
    if (typeKey === STATIC_IMAGE || typeKey === 22) {
      if (!rows.some((entry) => entry.id === '5')) rows.push(imageFitRow(item));
      const placement = imageRows(item);
      rows.push(...(typeKey === 22 ? placement.filter((entry) => entry.id !== 'static-file') : placement));
    }
    if (typeKey === 43 || typeKey === 45) rows.push(...barcodeRows(item));
    if (MASK_ITEM_TYPES.has(typeKey)) {
      const empty = !own(item, 'mask');
      rows.push({
        id: MASK_ROW_ID, key: null, label: 'Mask', value: empty ? '' : item.mask, omitted: empty,
        note: empty ? (NUMBER_TYPES.has(typeKey) ? PLAIN_DECIMAL_NOTE : DEFAULT_MASK_NOTE) : '',
        editor: 'Text', list: null, section: 7, group: 'Formatting', readOnly: false,
      });
    }
    if (typeKey === STATIC_NUMBER) rows.push(numberMaskRow());
    const presets = datetimePresets(lookups && lookups.datetimePresets);
    if (typeKey === STATIC_DATE && presets.length) {
      rows.push({
        id: PRESET_ROW_ID, key: null, label: 'Mask preset', value: '', omitted: true,
        note: 'Pick one to copy its mask into Mask (lookup 077).',
        editor: 'List', list: presets.map((preset) => presetLabel(preset)), section: 7, group: 'Formatting', readOnly: false,
      });
    }
    if (ZONE_ITEM_TYPES.has(typeKey)) rows.push(...zoneRows(item, presets));
  }
  if (scope === 'report') {
    rows.push({
      id: DEFAULT_ZONE_ROW_ID, key: null, label: 'Default zone', value: doc.defaultZone ?? '', omitted: !own(doc, 'defaultZone'),
      note: 'IANA zone for User timezones when the caller has none; blank uses the server default, else UTC.',
      editor: 'Text', list: null, section: null, group: 'Report', readOnly: false,
    });
  }
  if (scope === 'report') rows.push(embedFontsRow(doc), ...datasourceRows(doc, lookups), ...rulerRows(doc));
  const fillAt = rows.findIndex((row) => row.key === 46);
  if (fillAt >= 0) rows.splice(fillAt + 1, 0, ...shapeFillRows(doc, sel));
  return finishPanelRows(rows, doc, sel);
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
  const staticDoc = applyStaticProperty(doc, sel, rowId, value);
  if (staticDoc) return staticDoc;
  const barcodeDoc = applyBarcodeProperty(doc, sel, rowId, value);
  if (barcodeDoc) return barcodeDoc;
  const isItem = Boolean(sel && sel.itemId && findSelected(doc, sel).item);
  if (rowId === TEXT_ROW_ID) {
    if (!isItem) throw new Error('Text needs a selected item.');
    return editItem(doc, sel, (item) => { item.text = String(value); });
  }
  if (rowId === PARAM_ROW_ID) {
    if (!isItem) throw new Error('Parameter needs a selected item.');
    return editItem(doc, sel, (item) => { item.param = String(value ?? '').trim(); });
  }
  if (rowId === TEMPLATE_ROW_ID) {
    if (!isItem) throw new Error('Template needs a selected item.');
    const text = String(value ?? '');
    return editItem(doc, sel, (item) => { if (text.trim()) item.template = text; else delete item.template; });
  }
  if (rowId === AGGREGATE_OP_ROW_ID) {
    if (!isItem) throw new Error('Op needs a selected aggregate.');
    return editItem(doc, sel, (item) => setAggregateOp(item, String(value ?? '').trim(), doc.functions));
  }
  if (rowId === AGGREGATE_FUNCTION_ROW_ID) {
    if (!isItem) throw new Error('Function needs a selected aggregate.');
    return editItem(doc, sel, (item) => setAggregateFunction(item, value, doc.functions));
  }
  if (rowId === AGGREGATE_WEIGHT_ROW_ID) {
    if (!isItem) throw new Error('Weight needs a selected aggregate.');
    return editItem(doc, sel, (item) => setAggregateWeight(item, value));
  }
  if (rowId === '7' && isItem && isAggregateType(Number(findSelected(doc, sel).item.typeKey))) {
    return editItem(doc, sel, (item) => setAggregateField(item, value));
  }
  if (rowId === '8' && isItem && QUERY_VALUE_TYPES.includes(Number(findSelected(doc, sel).item.typeKey))) {
    const name = String(value ?? '').trim();
    return editItem(doc, sel, (item, next) => {
      if (name) item.datasource = name;
      else delete item.datasource;
      const columns = datasourceColumns(next, name);
      if (columns.length && typeof item.field === 'string' && item.field !== '' && !columns.includes(item.field)) {
        delete item.field;
      }
    });
  }
  if (rowId === MASK_ROW_ID) {
    if (!isItem) throw new Error('Mask needs a selected item.');
    return editItem(doc, sel, (item) => { item.mask = String(value ?? '').trim(); });
  }
  if (rowId === TZ_ROW_ID || rowId === ZONE_ROW_ID || rowId === PRESET_ROW_ID) {
    if (!isItem) throw new Error('Timezone needs a selected item.');
    const text = String(value ?? '').trim();
    if (rowId === TZ_ROW_ID) {
      if (text && !TZ_MODES.includes(text)) throw new Error(`Timezone must be one of ${TZ_MODES.join(', ')}.`);
      return editItem(doc, sel, (item) => { if (text) item.timezone = text; else delete item.timezone; if (text !== 'Explicit') delete item.zone; });
    }
    if (rowId === ZONE_ROW_ID) return editItem(doc, sel, (item) => { if (text) item.zone = text; else delete item.zone; });
    const presets = datetimePresets(ctx && ctx.datetimePresets);
    const hit = presets.find((p) => presetLabel(p) === text || p.caption === text || text.startsWith(`${p.caption} — `));
    if (!hit) throw new Error('Unknown mask preset.');
    return editItem(doc, sel, (item) => { item.mask = hit.mask; });
  }
  if (rowId === DEFAULT_ZONE_ROW_ID) {
    if (selectionScope(doc, sel) !== 'report') throw new Error('Default zone is set on the report.');
    const next = JSON.parse(JSON.stringify(doc));
    const text = String(value ?? '').trim();
    if (text) next.defaultZone = text; else delete next.defaultZone;
    return next;
  }
  if (rowId === 'fill-paint' || rowId === 'fill-angle' || rowId === 'fill-stops') {
    if (!isItem) throw new Error('Fill needs a selected shape.');
    return applyShapeFill(doc, sel, rowId, value);
  }
  if (rowId === NOTES_ROW_ID) {
    const scopeNow = selectionScope(doc, sel);
    if (scopeNow === 'end') throw new Error('End of Report has no editable properties.');
    const text = String(value ?? '');
    const keep = text.trim() !== '';
    if (scopeNow === 'item') {
      return editItem(doc, sel, (item) => { if (keep) item.notes = text; else delete item.notes; });
    }
    if (scopeNow === 'band') {
      const next = JSON.parse(JSON.stringify(doc));
      const band = bandOf(next, sel);
      if (!band) throw new Error('Notes needs a selected band.');
      if (keep) band.notes = text; else delete band.notes;
      return next;
    }
    const next = JSON.parse(JSON.stringify(doc));
    if (keep) next.notes = text; else delete next.notes;
    return next;
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
  if (typeof rowId === 'string' && rowId.startsWith(DS_ROW_PREFIX)) {
    if (scope !== 'report') throw new Error('Datasources are set on the report.');
    return applyDatasource(doc, rowId, value, ctx);
  }
  const fields = { item: ITEM_FIELDS, band: BAND_FIELDS, report: REPORT_FIELDS }[scope];
  const key = Number(rowId);
  const field = fields[key];
  if (!field) throw new Error(`Property ${rowId} is read only.`);
  return field.set(doc, sel, checkNumber(key, value, doc), ctx);
}
