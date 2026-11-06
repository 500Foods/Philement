/**
 * Barcode items on the design canvas (typeKey 43 and 45).
 * The editor asks the invokable zint.Svg script for an SVG.
 * The library itself is require'd from the Hydrogen payload.
 */

import './reports-barcode.css';
import { log, Subsystems, Status } from '../../core/log.js';
import { invokeScript } from '../../shared/conduit.js';
import {
  BARCODE_FITS, barcodeDataIsSample, barcodeOpts, barcodePlacement, barcodeSample, fittedSvg, isBarcodeType,
  storedSymbology, svgPixelSize, symbologyOptions,
} from './model/barcode.js';
import { boundFieldText, orderBindingKeys } from './reports-sample.js';
import { toCssPx } from './model/units.js';

const SCRIPT = 'zint.Svg';
const OBJECT = 4;
const PLACEMENT = 13;
const TEXT = 14;
const COLORS = 15;
const SYMBOL = 16;
const FIT_ALIGNS = Object.freeze(['Left', 'Center', 'Right']);
const FIT_VALIGNS = Object.freeze(['Top', 'Middle', 'Bottom']);
const FIT_LABELS = Object.freeze({
  contain: 'Contain', cover: 'Cover', none: 'None', 'scale-down': 'Scale down',
});
const CHECKS = Object.freeze(['compute', 'supplied']);

const cache = new Map();
const failed = new Set();
const inflight = new Set();
let host = null;

const BOOL_ROWS = Object.freeze([
  { id: 'barcode-nobackground', key: 'nobackground', label: 'No background', section: COLORS },
  { id: 'barcode-reverse', key: 'reverse', label: 'Reverse', section: COLORS },
  { id: 'barcode-small', key: 'small', label: 'Small text', section: TEXT },
  { id: 'barcode-bold', key: 'bold', label: 'Bold', section: TEXT },
  { id: 'barcode-embedfont', key: 'embedfont', label: 'Embed font', section: TEXT },
  { id: 'barcode-quietzones', key: 'quietzones', label: 'Quiet zones', section: PLACEMENT },
  { id: 'barcode-noquietzones', key: 'noquietzones', label: 'No quiet zones', section: PLACEMENT },
  { id: 'barcode-bind', key: 'bind', label: 'Bind', section: PLACEMENT },
  { id: 'barcode-bindtop', key: 'bindtop', label: 'Bind top', section: PLACEMENT },
  { id: 'barcode-box', key: 'box', label: 'Box', section: PLACEMENT },
  { id: 'barcode-heightperrow', key: 'heightperrow', label: 'Height per row', section: PLACEMENT },
  { id: 'barcode-compliantheight', key: 'compliantheight', label: 'Compliant height', section: PLACEMENT },
  { id: 'barcode-init', key: 'init', label: 'Reader init', section: OBJECT },
  { id: 'barcode-esc', key: 'esc', label: 'Escape', section: OBJECT },
  { id: 'barcode-gs1', key: 'gs1', label: 'GS1', section: SYMBOL, when: ['qr', 'datamatrix', 'aztec'] },
  { id: 'barcode-gs1parens', key: 'gs1parens', label: 'GS1 parentheses', section: SYMBOL, when: ['qr', 'datamatrix', 'aztec', 'gs1-128', 'dbarexp', 'dbarexpstk'] },
  { id: 'barcode-dotty', key: 'dotty', label: 'Dotty', section: SYMBOL, when: ['qr', 'datamatrix', 'aztec'] },
  { id: 'barcode-extraesc', key: 'extraesc', label: 'Extra escape', section: SYMBOL, when: ['code128', 'aztec'] },
  { id: 'barcode-fast', key: 'fast', label: 'Fast', section: SYMBOL, when: ['datamatrix', 'pdf417', 'pdf417comp', 'aztec'] },
  { id: 'barcode-square', key: 'square', label: 'Square', section: SYMBOL, when: ['datamatrix'] },
  { id: 'barcode-dmre', key: 'dmre', label: 'DMRE', section: SYMBOL, when: ['datamatrix'] },
  { id: 'barcode-dmiso144', key: 'dmiso144', label: 'ISO 144', section: SYMBOL, when: ['datamatrix'] },
  { id: 'barcode-gssep', key: 'gssep', label: 'GS separator', section: SYMBOL, when: ['datamatrix'] },
  { id: 'barcode-azfull', key: 'azfull', label: 'Full range', section: SYMBOL, when: ['aztec'] },
]);

const NUMBER_ROWS = Object.freeze([
  { id: 'barcode-scale', key: 'scale', label: 'Scale', section: PLACEMENT, min: 0.01, max: 200, show: '1', omit: 1 },
  { id: 'barcode-scalexdimdp', key: 'scalexdimdp', label: 'Scale X dim dp', section: PLACEMENT, min: 0.01, max: 200 },
  { id: 'barcode-height', key: 'height', label: 'Bar height', section: PLACEMENT, min: 0.5, max: 2000 },
  { id: 'barcode-border', key: 'border', label: 'Border', section: PLACEMENT, min: 0, max: 1000, show: '0', omit: 0 },
  { id: 'barcode-whitesp', key: 'whitesp', label: 'Whitespace', section: PLACEMENT, min: 0, show: '0', omit: 0 },
  { id: 'barcode-vwhitesp', key: 'vwhitesp', label: 'Vertical whitespace', section: PLACEMENT, min: 0, show: '0', omit: 0 },
  { id: 'barcode-textgap', key: 'textgap', label: 'Text gap', section: TEXT, min: 0, show: '1', omit: 1 },
  { id: 'barcode-guarddescent', key: 'guarddescent', label: 'Guard descent', section: SYMBOL, min: 0, max: 50, show: '5', omit: 5, when: ['upca', 'upce', 'ean8', 'ean13', 'isbn'] },
  { id: 'barcode-dotsize', key: 'dotsize', label: 'Dot size', section: SYMBOL, min: 0.01, when: ['qr', 'datamatrix', 'aztec'] },
  { id: 'barcode-vers', key: 'vers', label: 'Version', section: SYMBOL, min: 1, max: 48, integer: true, when: ['datamatrix'] },
  { id: 'barcode-aztec-vers', key: 'vers', label: 'Size', section: SYMBOL, min: 1, max: 36, integer: true, when: ['aztec'] },
  { id: 'barcode-cols', key: 'cols', label: 'Columns', section: SYMBOL, min: 1, max: 11, integer: true, when: ['dbarexpstk'] },
  { id: 'barcode-pdf-cols', key: 'cols', label: 'Columns', section: SYMBOL, min: 1, max: 30, integer: true, when: ['pdf417', 'pdf417comp'] },
  { id: 'barcode-rows', key: 'rows', label: 'Rows', section: SYMBOL, min: 2, max: 11, integer: true, when: ['dbarexpstk'] },
  { id: 'barcode-pdf-rows', key: 'rows', label: 'Rows', section: SYMBOL, min: 3, max: 90, integer: true, when: ['pdf417', 'pdf417comp'] },
  { id: 'barcode-secure', key: 'secure', label: 'Error correction', section: SYMBOL, min: 0, max: 8, integer: true, when: ['pdf417', 'pdf417comp'] },
  { id: 'barcode-aztec-secure', key: 'secure', label: 'Error correction', section: SYMBOL, min: 1, max: 4, integer: true, when: ['aztec'] },
  { id: 'barcode-mode', key: 'mode', label: 'Mode', section: SYMBOL, min: 2, max: 6, integer: true, when: ['maxicode'] },
  { id: 'barcode-scmvv', key: 'scmvv', label: 'SCM version', section: SYMBOL, min: 0, max: 99, integer: true, when: ['maxicode'] },
]);

const SYMBOL_ORDER = Object.freeze([
  'barcode-guarddescent',
  'barcode-vers',
  'barcode-aztec-vers',
  'barcode-cols',
  'barcode-pdf-cols',
  'barcode-rows',
  'barcode-pdf-rows',
  'barcode-secure',
  'barcode-aztec-secure',
  'barcode-mode',
  'barcode-scmvv',
  'barcode-fast',
  'barcode-square',
  'barcode-dmre',
  'barcode-dmiso144',
  'barcode-gssep',
  'barcode-azfull',
  'barcode-gs1',
  'barcode-gs1parens',
  'barcode-dotty',
  'barcode-dotsize',
  'barcode-extraesc',
]);
const C25_CHECKS = Object.freeze(['none', 'show', 'hide']);

/**
 * Lookup 075 keys 13–16. sortSeq sits between Object (4) and Position (5).
 * The panel uses these when the lookup has not been applied yet.
 */
export const BARCODE_SECTIONS = Object.freeze([
  { id: PLACEMENT, label: 'Barcode Placement', sortSeq: 4.2, icon: '<fa fa-expand></fa>' },
  { id: TEXT, label: 'Barcode Text', sortSeq: 4.4, icon: '<fa fa-font></fa>' },
  { id: COLORS, label: 'Barcode Colors', sortSeq: 4.6, icon: '<fa fa-palette></fa>' },
  { id: SYMBOL, label: 'Barcode Symbology', sortSeq: 4.8, icon: '<fa fa-barcode></fa>' },
]);

/**
 * Keep the four barcode headers between Object and Position.
 * A loaded lookup row keeps its label and icon.
 * @param {object[]} sections
 * @returns {object[]}
 */
export function withBarcodeSections(sections) {
  const list = Array.isArray(sections) ? sections.map((section) => ({ ...section })) : [];
  const byId = new Map(list.map((section) => [Number(section.id), section]));
  for (const section of BARCODE_SECTIONS) {
    const found = byId.get(section.id);
    if (found) {
      found.sortSeq = section.sortSeq;
      if (!found.icon) found.icon = section.icon;
      if (!found.label) found.label = section.label;
    } else {
      list.push({ ...section });
    }
  }
  return list;
}

/**
 * The manager that paints and commits. Null detaches it.
 * @param {object|null} next
 */
export function bindBarcodeHost(next) {
  host = next || null;
}

/** Drop remembered symbols. The next paint asks again. */
export function clearBarcodeCache() {
  cache.clear();
  failed.clear();
  inflight.clear();
}

/**
 * @param {object} opts
 * @param {string} svg
 */
export function rememberBarcodeSvg(opts, svg) {
  cache.set(JSON.stringify(opts), svg);
}

/**
 * A log line may name an error number. It does not repeat the symbol data.
 * @param {string} text
 * @returns {string}
 */
export function safeBarcodeMessage(text) {
  if (text === 'zint.barcode is not loaded') return text;
  const match = /^Error \d+/.exec(String(text || ''));
  return match ? match[0] : 'barcode failed';
}

function selectedItem(doc, sel) {
  const bands = Array.isArray(doc && doc.bands) ? doc.bands : [];
  const band = bands.find((entry) => entry.id === (sel && sel.bandId));
  const items = band && Array.isArray(band.items) ? band.items : [];
  return items.find((entry) => entry.id === (sel && sel.itemId)) || null;
}

function findBand(doc, itemId) {
  const bands = Array.isArray(doc && doc.bands) ? doc.bands : [];
  return bands.find((band) => Array.isArray(band.items) && band.items.some((item) => item.id === itemId)) || null;
}

function findItem(doc, itemId) {
  const band = findBand(doc, itemId);
  if (!band) return null;
  return band.items.find((item) => item.id === itemId) || null;
}

function clone(value) {
  return JSON.parse(JSON.stringify(value));
}

function editBarcode(doc, sel, change) {
  const next = clone(doc);
  const item = selectedItem(next, sel);
  if (!item || !isBarcodeType(Number(item.typeKey))) throw new Error('That property needs a barcode.');
  const barcode = item.barcode && typeof item.barcode === 'object' && !Array.isArray(item.barcode)
    ? { ...item.barcode }
    : {};
  change(item, barcode);
  item.barcode = barcode;
  return next;
}

function row(id, label, value, omitted, editor, list, section) {
  const named = BARCODE_SECTIONS.find((entry) => entry.id === section);
  return {
    id, key: null, label, value, omitted, note: '', editor, list,
    section, group: named ? named.label : 'Object', readOnly: false,
  };
}

function shows(spec, sym) {
  if (!spec.when) return true;
  const names = Array.isArray(spec.when) ? spec.when : [spec.when];
  return names.includes(sym);
}

function readNumber(value, label) {
  const text = String(value ?? '').trim();
  if (!/^-?\d+(\.\d+)?$/.test(text)) throw new Error(`${label} must be a number.`);
  return Number(text);
}

function onFlag(value) {
  return value === true || value === 'True';
}

/**
 * @param {object} lookups
 * @returns {{ value: string, label: string }[]}
 */
export function symbologyList(lookups) {
  return symbologyOptions(lookups && lookups.symbologies).map((entry) => ({
    value: entry.code, label: entry.label,
  }));
}

/** Lookup 054 keys before those rows are applied. */
export function barcodeAttrMeta(key) {
  if (key === 52) return { Group: 'Object', section: 4, Label: 'Symbology', Editor: 'List' };
  if (key === 8) return { Group: 'Object', section: 4, Label: 'Datasource', Editor: 'Datasource' };
  if (key === 7) return { Group: 'Object', section: 4, Label: 'Field', Editor: 'Field' };
  return null;
}

/**
 * Datasource then Field sit in front of Rotation on a field barcode.
 * Symbology is last.
 * @param {number} typeKey
 * @param {number[]} keys
 * @returns {number[]}
 */
export function barcodeAttributeKeys(typeKey, keys) {
  const next = Array.isArray(keys) ? keys.slice() : [];
  if (typeKey === 45) {
    const ordered = orderBindingKeys(next);
    next.length = 0;
    next.push(...ordered);
  }
  if (!next.includes(52)) next.push(52);
  return next;
}

/**
 * First sample row of the item's datasource, for the design canvas.
 * Empty when the datasource or the field is missing, or the cell is blank.
 * @param {object} doc
 * @param {object} item
 * @returns {string}
 */
export function fieldBarcodeValue(doc, item) {
  return boundFieldText(doc, item);
}

export const barcodeItemFields = {
  52: {
    get: (doc, sel) => {
      const name = storedSymbology(selectedItem(doc, sel));
      return name ? { value: name, omitted: false } : { value: 'code128', omitted: true };
    },
    set: (doc, sel, value) => {
      const name = String(value ?? '');
      if (!symbologyOptions(null).some((entry) => entry.code === name)) {
        throw new Error(`Symbology "${name}" is not in the list.`);
      }
      return editBarcode(doc, sel, (item, barcode) => {
        barcode.symbology = name;
        if (Number(item.typeKey) !== 43) return;
        const current = Object.hasOwn(barcode, 'data') ? barcode.data : '';
        if (barcodeDataIsSample(current)) barcode.data = barcodeSample(name);
      });
    },
  },
};

function pushFlags(rows, barcode, sym, section) {
  for (const spec of BOOL_ROWS) {
    if (spec.section !== section || !shows(spec, sym)) continue;
    const present = Object.hasOwn(barcode, spec.key);
    rows.push(row(spec.id, spec.label, barcode[spec.key] === true ? 'True' : 'False', !present, 'Boolean', null, section));
  }
}

function pushNumbers(rows, barcode, sym, section) {
  for (const spec of NUMBER_ROWS) {
    if (spec.section !== section || !shows(spec, sym)) continue;
    const present = Object.hasOwn(barcode, spec.key) && barcode[spec.key] !== '' && barcode[spec.key] !== null;
    rows.push(row(spec.id, spec.label, present ? String(barcode[spec.key]) : (spec.show || ''), !present, 'Number', null, section));
  }
}

/**
 * Client rows. Symbology is lookup key 52. Field is lookup key 7.
 * Placement, text, colors, and symbology options use their own sections.
 * @param {object} item
 * @returns {object[]}
 */
export function barcodeRows(item) {
  const barcode = item && item.barcode && typeof item.barcode === 'object' ? item.barcode : {};
  const sym = storedSymbology(item) || 'code128';
  const rows = [];
  if (Number(item && item.typeKey) === 43) {
    const data = typeof barcode.data === 'string' ? barcode.data : '';
    const empty = !Object.hasOwn(barcode, 'data');
    const dataRow = row('barcode-data', 'Data', empty ? '' : data, empty, 'Text', null, OBJECT);
    dataRow.multiline = true;
    rows.push(dataRow);
  }
  pushFlags(rows, barcode, sym, OBJECT);
  const dpi = item && Object.hasOwn(item, 'dpi') ? item.dpi : '';
  const fit = item && typeof item.fit === 'string' ? item.fit : '';
  const fitShown = BARCODE_FITS.includes(fit) ? fit : 'contain';
  const align = item && typeof item.fitAlign === 'string' ? item.fitAlign : '';
  const valign = item && typeof item.fitValign === 'string' ? item.fitValign : '';
  rows.push(row('barcode-fit', 'Fit', FIT_LABELS[fitShown], fit === '', 'List', BARCODE_FITS.map((name) => FIT_LABELS[name]), PLACEMENT));
  rows.push(row('barcode-fit-align', 'Fit align', align || 'Center', align === '', 'List', [...FIT_ALIGNS], PLACEMENT));
  rows.push(row('barcode-fit-vertical', 'Fit vertical', valign || 'Middle', valign === '', 'List', [...FIT_VALIGNS], PLACEMENT));
  rows.push(row('barcode-dpi', 'Dpi', dpi === '' ? '300' : String(dpi), dpi === '', 'Number', null, PLACEMENT));
  pushNumbers(rows, barcode, sym, PLACEMENT);
  pushFlags(rows, barcode, sym, PLACEMENT);
  const hidden = barcode.notext === true;
  rows.push(row('barcode-show-text', 'Show text', hidden ? 'False' : 'True', !hidden, 'Boolean', null, TEXT));
  pushFlags(rows, barcode, sym, TEXT);
  pushNumbers(rows, barcode, sym, TEXT);
  const fg = typeof barcode.fg === 'string' ? barcode.fg : '';
  const bg = typeof barcode.bg === 'string' ? barcode.bg : '';
  rows.push(row('barcode-fg', 'Foreground', fg, fg === '', 'Text', null, COLORS));
  rows.push(row('barcode-bg', 'Background', bg, bg === '', 'Text', null, COLORS));
  pushFlags(rows, barcode, sym, COLORS);
  if (sym === 'upca' || sym === 'upce') {
    const supplied = barcode.check === 'supplied';
    rows.push(row(
      'barcode-check', 'Check digit', supplied ? 'supplied' : 'compute', !supplied,
      'List', [...CHECKS], SYMBOL,
    ));
  }
  if (sym === 'c25inter') {
    const shown = barcode.vers === 1 ? 'show' : barcode.vers === 2 ? 'hide' : 'none';
    rows.push(row('barcode-c25check', 'Check digit', shown, shown === 'none', 'List', [...C25_CHECKS], SYMBOL));
  }
  for (const id of SYMBOL_ORDER) {
    const spec = NUMBER_ROWS.find((entry) => entry.id === id) || BOOL_ROWS.find((entry) => entry.id === id);
    if (!spec || !shows(spec, sym)) continue;
    if (Object.hasOwn(spec, 'min')) {
      const present = Object.hasOwn(barcode, spec.key) && barcode[spec.key] !== '' && barcode[spec.key] !== null;
      rows.push(row(spec.id, spec.label, present ? String(barcode[spec.key]) : (spec.show || ''), !present, 'Number', null, SYMBOL));
    } else {
      const present = Object.hasOwn(barcode, spec.key);
      rows.push(row(spec.id, spec.label, barcode[spec.key] === true ? 'True' : 'False', !present, 'Boolean', null, SYMBOL));
    }
  }
  if (sym === 'maxicode') {
    const text = typeof barcode.primary === 'string' ? barcode.primary : '';
    rows.push(row('barcode-primary', 'Primary', text, text === '', 'Text', null, SYMBOL));
  }
  return rows;
}

function applyNumber(barcode, spec, value) {
  const number = readNumber(value, spec.label);
  if (spec.integer && !Number.isInteger(number)) {
    throw new Error(`${spec.label} must be a whole number.`);
  }
  if (spec.min !== undefined && number < spec.min) {
    const cap = spec.max !== undefined ? ` to ${spec.max}` : ' or more';
    const floor = spec.max !== undefined ? `from ${spec.min}` : `${spec.min}`;
    throw new Error(`${spec.label} must be ${floor}${cap}.`);
  }
  if (spec.max !== undefined && number > spec.max) {
    throw new Error(`${spec.label} must be from ${spec.min} to ${spec.max}.`);
  }
  if (spec.omit !== undefined && number === spec.omit) delete barcode[spec.key];
  else barcode[spec.key] = number;
}

/**
 * Client barcode rows. Key 52 and key 7 stay on the lookup fields.
 * Returns null when this row is not a barcode row.
 * @param {object} doc
 * @param {object} sel
 * @param {string} rowId
 * @param {*} value
 * @returns {object|null}
 */
export function applyBarcodeProperty(doc, sel, rowId, value) {
  if (rowId === '52' || rowId === '7') return null;
  if (typeof rowId !== 'string' || !rowId.startsWith('barcode-')) return null;
  if (rowId === 'barcode-data') {
    return editBarcode(doc, sel, (_item, barcode) => { barcode.data = String(value ?? ''); });
  }
  if (rowId === 'barcode-check') {
    const text = String(value ?? '').trim();
    if (!CHECKS.includes(text)) throw new Error('Check digit must be compute or supplied.');
    return editBarcode(doc, sel, (_item, barcode) => {
      if (text === 'supplied') barcode.check = 'supplied';
      else delete barcode.check;
    });
  }
  if (rowId === 'barcode-c25check') {
    const text = String(value ?? '').trim();
    if (!C25_CHECKS.includes(text)) throw new Error('Check digit must be none, show, or hide.');
    return editBarcode(doc, sel, (_item, barcode) => {
      if (text === 'show') barcode.vers = 1;
      else if (text === 'hide') barcode.vers = 2;
      else delete barcode.vers;
    });
  }
  if (rowId === 'barcode-primary') {
    const text = String(value ?? '');
    return editBarcode(doc, sel, (_item, barcode) => {
      if (text) barcode.primary = text;
      else delete barcode.primary;
    });
  }
  if (rowId === 'barcode-fg' || rowId === 'barcode-bg') {
    const key = rowId === 'barcode-fg' ? 'fg' : 'bg';
    const text = String(value ?? '').trim();
    return editBarcode(doc, sel, (_item, barcode) => {
      if (text) barcode[key] = text;
      else delete barcode[key];
    });
  }
  if (rowId === 'barcode-show-text') {
    const on = onFlag(value);
    return editBarcode(doc, sel, (_item, barcode) => {
      if (on) delete barcode.notext;
      else barcode.notext = true;
    });
  }
  const flag = BOOL_ROWS.find((entry) => entry.id === rowId);
  if (flag) {
    const on = onFlag(value);
    return editBarcode(doc, sel, (_item, barcode) => {
      if (on) barcode[flag.key] = true;
      else delete barcode[flag.key];
    });
  }
  const spec = NUMBER_ROWS.find((entry) => entry.id === rowId);
  if (spec) return editBarcode(doc, sel, (_item, barcode) => applyNumber(barcode, spec, value));
  if (rowId === 'barcode-dpi') {
    const number = readNumber(value, 'Dpi');
    if (!(number > 0)) throw new Error('Dpi must be a positive number.');
    return editBarcode(doc, sel, (item) => { item.dpi = number; });
  }
  if (rowId === 'barcode-fit') {
    const label = String(value ?? '').trim();
    const fit = BARCODE_FITS.find((name) => FIT_LABELS[name] === label);
    if (!fit) throw new Error('Fit must be Contain, Cover, None, or Scale down.');
    return editBarcode(doc, sel, (item) => {
      if (fit === 'contain') delete item.fit;
      else item.fit = fit;
    });
  }
  if (rowId === 'barcode-fit-align') {
    const align = String(value ?? '').trim();
    if (!FIT_ALIGNS.includes(align)) throw new Error('Fit align must be Left, Center, or Right.');
    return editBarcode(doc, sel, (item) => { item.fitAlign = align; });
  }
  if (rowId === 'barcode-fit-vertical') {
    const valign = String(value ?? '').trim();
    if (!FIT_VALIGNS.includes(valign)) throw new Error('Fit vertical must be Top, Middle, or Bottom.');
    return editBarcode(doc, sel, (item) => { item.fitValign = valign; });
  }
  throw new Error(`Property ${rowId} is read only.`);
}

function readSvg(envelope) {
  const status = envelope && envelope.status ? String(envelope.status) : '';
  if (status !== 'completed') return { error: 'barcode failed' };
  let body = envelope.result;
  if (typeof body === 'string') {
    try {
      body = JSON.parse(body);
    } catch {
      return { error: 'barcode failed' };
    }
  }
  if (!body || typeof body !== 'object') return { error: 'barcode failed' };
  if (body.error) return { error: String(body.error) };
  if (typeof body.svg !== 'string' || body.svg === '') return { error: 'barcode failed' };
  return { svg: body.svg };
}

function placeholder(el) {
  el.classList.add('reports-barcode-placeholder');
  const note = document.createElement('span');
  note.className = 'reports-barcode-note';
  note.textContent = 'Barcode';
  el.replaceChildren(note);
}

function framePx(item) {
  const box = item && item.box;
  if (!box) return null;
  return { width: toCssPx(box.width), height: toCssPx(box.height) };
}

function mountSvg(el, svg, item) {
  const frame = framePx(item);
  const fitted = fittedSvg(svg, item, frame);
  if (!fitted || typeof DOMParser === 'undefined') {
    placeholder(el);
    return;
  }
  const parsed = new DOMParser().parseFromString(fitted, 'image/svg+xml');
  const root = parsed.documentElement;
  if (!root || String(root.localName).toLowerCase() !== 'svg') {
    placeholder(el);
    return;
  }
  const node = document.importNode(root, true);
  const place = barcodePlacement(item, svgPixelSize(svg), frame);
  if (place) {
    node.style.setProperty('--reports-barcode-svg-width', `${place.width}px`);
    node.style.setProperty('--reports-barcode-svg-height', `${place.height}px`);
    node.style.setProperty('--reports-barcode-svg-x', `${place.x}px`);
    node.style.setProperty('--reports-barcode-svg-y', `${place.y}px`);
  }
  const turn = Number(item && item.rotation);
  if (Number.isFinite(turn) && turn !== 0) {
    node.style.setProperty('--reports-barcode-transform', `rotate(${turn}deg)`);
  }
  el.classList.remove('reports-barcode-placeholder');
  el.replaceChildren(node);
}

async function fetchBarcode(bound, itemId, opts) {
  const key = JSON.stringify(opts);
  if (!bound || !bound.app || !bound.app.api) return;
  if (cache.has(key) || failed.has(key) || inflight.has(key)) return;
  inflight.add(key);
  try {
    const envelope = await invokeScript(bound.app.api, { script: SCRIPT, params: opts, wait: true });
    const parsed = readSvg(envelope);
    if (!parsed.svg) {
      failed.add(key);
      log(Subsystems.MANAGER, Status.WARN, `[Reports] ${safeBarcodeMessage(parsed.error)}`);
      return;
    }
    cache.set(key, parsed.svg);
    if (host !== bound || !bound.doc) return;
    const current = findItem(bound.doc, itemId);
    if (!current || JSON.stringify(barcodeOpts(current, fieldBarcodeValue(bound.doc, current))) !== key) return;
    if (typeof bound._showDocument === 'function') bound._showDocument(bound.doc);
  } catch {
    failed.add(key);
    log(Subsystems.MANAGER, Status.WARN, '[Reports] barcode failed');
  } finally {
    inflight.delete(key);
  }
}

/**
 * Paint one barcode. The caller adds the corner after this returns.
 * @param {HTMLElement} el
 * @param {object} item
 * @param {object} doc
 */
export function paintBarcode(el, item, doc) {
  const opts = barcodeOpts(item, fieldBarcodeValue(doc, item));
  if (!opts || opts.pending) {
    placeholder(el);
    return;
  }
  const key = JSON.stringify(opts);
  const svg = cache.get(key);
  if (svg) {
    mountSvg(el, svg, item);
    return;
  }
  placeholder(el);
  if (failed.has(key) || !host || !host.app || !host.app.api) return;
  fetchBarcode(host, item.id, opts);
}
