/**
 * Barcode geometry and the options passed to zint.
 * The box is a free rectangle. The symbol keeps its aspect
 * and is fitted inside that rectangle. Bar height is the
 * zint `height` option, in modules. Item rotation is CSS
 * and is not sent to zint.
 */

export const BARCODE_STATIC = 43;
export const BARCODE_FIELD = 45;

export const BUILTIN_SYMBOLOGIES = Object.freeze([
  { code: 'aztec', label: 'Aztec' },
  { code: 'code39', label: 'Code 39' },
  { code: 'code128', label: 'Code 128' },
  { code: 'code128ab', label: 'Code 128 (suppress C)' },
  { code: 'pdf417comp', label: 'Compact PDF417' },
  { code: 'datamatrix', label: 'Data Matrix' },
  { code: 'ean2', label: 'EAN-2 add-on' },
  { code: 'ean5', label: 'EAN-5 add-on' },
  { code: 'ean8', label: 'EAN-8' },
  { code: 'ean13', label: 'EAN-13' },
  { code: 'flat', label: 'Flattermarken' },
  { code: 'dbarexp', label: 'GS1 DataBar Expanded' },
  { code: 'dbarexpstk', label: 'GS1 DataBar Expanded Stacked' },
  { code: 'dbaromn', label: 'GS1 DataBar Omnidirectional' },
  { code: 'dbaromnstk', label: 'GS1 DataBar Stacked Omnidirectional' },
  { code: 'gs1-128', label: 'GS1-128' },
  { code: 'c25inter', label: 'Interleaved 2 of 5' },
  { code: 'isbn', label: 'ISBN' },
  { code: 'itf14', label: 'ITF-14' },
  { code: 'maxicode', label: 'MaxiCode' },
  { code: 'nve18', label: 'NVE-18 (SSCC)' },
  { code: 'pdf417', label: 'PDF417' },
  { code: 'qr', label: 'QR Code' },
  { code: 'upca', label: 'UPC-A' },
  { code: 'upce', label: 'UPC-E' },
  { code: 'usps', label: 'USPS Intelligent Mail' },
]);

/** Example data for a new static barcode, one string per stored symbology. */
export const BARCODE_SAMPLES = Object.freeze({
  qr: 'https://500courses.com',
  code39: 'CODE-39',
  code128: 'Ab-129',
  upca: '12345678901',
  'gs1-128': '[01]09501101020917[10]AB-123',
  code128ab: '123456',
  flat: '0123456789',
  ean8: '1234567',
  ean13: '123456789012',
  upce: '123456',
  c25inter: '1234567890',
  itf14: '1234567890',
  nve18: '12345678901234567',
  dbaromn: '09501101530003',
  dbaromnstk: '09501101530003',
  dbarexp: '[01]09501101530003[17]140704[10]AB-123',
  dbarexpstk: '[01]09501101530003[17]140704[10]AB-123',
  datamatrix: 'HELLO',
  ean2: '05',
  ean5: '12345',
  isbn: '9780306406157',
  pdf417: 'PDF417',
  pdf417comp: 'Hello',
  aztec: '123456789012',
  maxicode: 'A MaxiCode Message in Mode 4',
  usps: '01234567094987654321',
});

const SAMPLE_VALUES = new Set(Object.values(BARCODE_SAMPLES));

const LIBRARY = Object.freeze({
  qr: 'qrcode',
  code39: 'code39',
  code128: 'code128',
  upca: 'upca',
  'gs1-128': 'gs1128',
  code128ab: 'code128ab',
  flat: 'flat',
  ean8: 'ean8',
  ean13: 'ean13',
  upce: 'upce',
  c25inter: 'c25inter',
  itf14: 'itf14',
  nve18: 'nve18',
  dbaromn: 'dbaromn',
  dbaromnstk: 'dbaromnstk',
  dbarexp: 'dbarexp',
  dbarexpstk: 'dbarexpstk',
  datamatrix: 'datamatrix',
  ean2: 'ean2',
  ean5: 'ean5',
  isbn: 'isbn',
  pdf417: 'pdf417',
  pdf417comp: 'pdf417comp',
  aztec: 'aztec',
  maxicode: 'maxicode',
  usps: 'uspsimail',
});

const CHECK_LIBRARY = Object.freeze({
  upca: 'upcachk',
  upce: 'upcechk',
});

const CODES = new Set(BUILTIN_SYMBOLOGIES.map((entry) => entry.code));
const GUARD_SYMS = new Set(['upca', 'upcachk', 'upce', 'upcechk', 'ean8', 'ean13', 'isbn']);
const GS1_SYMS = new Set(['qrcode', 'datamatrix', 'aztec']);
const PARENS_SYMS = new Set(['qrcode', 'datamatrix', 'aztec', 'gs1128', 'dbarexp', 'dbarexpstk']);
const DOTTY_SYMS = new Set(['qrcode', 'datamatrix', 'aztec']);
const EXTRA_SYMS = new Set(['code128', 'aztec']);
const VERS_SYMS = new Set(['datamatrix', 'aztec', 'c25inter']);
const COLS_SYMS = new Set(['dbarexpstk', 'pdf417', 'pdf417comp']);
const ROWS_SYMS = new Set(['dbarexpstk', 'pdf417', 'pdf417comp']);
const SECURE_SYMS = new Set(['pdf417', 'pdf417comp', 'aztec']);
const FAST_SYMS = new Set(['datamatrix', 'pdf417', 'pdf417comp', 'aztec']);
const DM_KEYS = new Set(['square', 'dmre', 'dmiso144', 'gssep']);
const BOOL_KEYS = Object.freeze([
  'gs1', 'gs1parens', 'nobackground', 'heightperrow', 'compliantheight',
  'bind', 'bindtop', 'box', 'quietzones', 'noquietzones', 'notext', 'small',
  'bold', 'embedfont', 'dotty', 'reverse', 'init', 'esc', 'extraesc',
  'fast', 'square', 'dmre', 'dmiso144', 'gssep', 'azfull',
]);
const VALUE_KEYS = Object.freeze([
  'fg', 'bg', 'scale', 'scalexdimdp', 'height', 'border', 'whitesp',
  'vwhitesp', 'textgap', 'guarddescent', 'dotsize',
  'vers', 'cols', 'rows', 'secure', 'mode', 'scmvv',
]);
const TEXT_KEYS = Object.freeze(['primary']);
const OMIT_VALUE = Object.freeze({
  scale: 1, textgap: 1, guarddescent: 5, border: 0, whitesp: 0, vwhitesp: 0,
});
const FIT_X = Object.freeze({ Left: 'Min', Center: 'Mid', Right: 'Max' });
const FIT_Y = Object.freeze({ Top: 'Min', Middle: 'Mid', Bottom: 'Max' });
export const BARCODE_FITS = Object.freeze(['contain', 'cover', 'none', 'scale-down']);
const SVG_SIZE = /<svg\b[^>]*\bwidth="(\d+)"[^>]*\bheight="(\d+)"/i;

/**
 * @param {number} typeKey
 * @returns {boolean}
 */
export function isBarcodeType(typeKey) {
  return typeKey === BARCODE_STATIC || typeKey === BARCODE_FIELD;
}

/**
 * Example string for a stored symbology, or '' when the name is unknown.
 * @param {string} symbology
 * @returns {string}
 */
export function barcodeSample(symbology) {
  return BARCODE_SAMPLES[symbology] || '';
}

/**
 * True when Data is missing, blank, or one of the example strings.
 * A typed value that is not an example stays put.
 * @param {unknown} data
 * @returns {boolean}
 */
export function barcodeDataIsSample(data) {
  if (data == null) return true;
  const text = String(data);
  if (text.trim() === '') return true;
  return SAMPLE_VALUES.has(text);
}

/**
 * Names compare with digits as numbers, so Code 39 precedes Code 128.
 * @param {{ label: string }} a
 * @param {{ label: string }} b
 * @returns {number}
 */
function bySymbologyName(a, b) {
  return a.label.localeCompare(b.label, 'en', { numeric: true });
}

/**
 * Lookup 079 supplies a label when it has one. Built-in names fill any gap.
 * A code outside that list is ignored. The finished list is sorted by name.
 * @param {object[]|null|undefined} rows
 * @returns {{ code: string, label: string }[]}
 */
export function symbologyOptions(rows) {
  const labels = new Map(BUILTIN_SYMBOLOGIES.map((entry) => [entry.code, entry.label]));
  const out = [];
  const seen = new Set();
  for (const row of Array.isArray(rows) ? rows : []) {
    const code = row && typeof row.code === 'string' ? row.code.trim() : '';
    if (!CODES.has(code) || seen.has(code)) continue;
    seen.add(code);
    const text = row && typeof row.value_txt === 'string' ? row.value_txt.trim() : '';
    out.push({ code, label: text || labels.get(code) });
  }
  for (const entry of BUILTIN_SYMBOLOGIES) {
    if (!seen.has(entry.code)) out.push({ code: entry.code, label: entry.label });
  }
  out.sort(bySymbologyName);
  return out;
}

/**
 * Stored token, or '' when the item has not chosen one.
 * @param {object|null|undefined} item
 * @returns {string}
 */
export function storedSymbology(item) {
  const barcode = item && item.barcode;
  const name = barcode && typeof barcode.symbology === 'string' ? barcode.symbology : '';
  return CODES.has(name) ? name : '';
}

function libraryName(barcode) {
  const raw = typeof barcode.symbology === 'string' && barcode.symbology !== ''
    ? barcode.symbology
    : 'code128';
  if (barcode.check === 'supplied' && CHECK_LIBRARY[raw]) return CHECK_LIBRARY[raw];
  return LIBRARY[raw] || null;
}

function keyAllowed(sym, key) {
  if (key === 'guarddescent') return GUARD_SYMS.has(sym);
  if (key === 'gs1') return GS1_SYMS.has(sym);
  if (key === 'gs1parens') return PARENS_SYMS.has(sym);
  if (key === 'dotty' || key === 'dotsize') return DOTTY_SYMS.has(sym);
  if (key === 'extraesc') return EXTRA_SYMS.has(sym);
  if (key === 'vers') return VERS_SYMS.has(sym);
  if (key === 'cols') return COLS_SYMS.has(sym);
  if (key === 'rows') return ROWS_SYMS.has(sym);
  if (key === 'secure') return SECURE_SYMS.has(sym);
  if (key === 'fast') return FAST_SYMS.has(sym);
  if (key === 'azfull') return sym === 'aztec';
  if (DM_KEYS.has(key)) return sym === 'datamatrix';
  if (key === 'mode' || key === 'scmvv' || key === 'primary') return sym === 'maxicode';
  return true;
}

function colorValue(value) {
  const text = String(value);
  const bare = text.startsWith('#') ? text.slice(1) : text;
  return bare === '' ? null : bare;
}

/**
 * Options for zint.barcode. `output` and `rotate` stay off.
 * A field item encodes `bound`, the value from its datasource field.
 * An empty bound value returns `{ pending: true }`.
 * An unknown stored name returns null.
 * @param {object|null|undefined} item
 * @param {unknown} [bound] Field-barcode value. Static items ignore it.
 * @returns {object|null}
 */
export function barcodeOpts(item, bound) {
  const barcode = item && item.barcode && typeof item.barcode === 'object' && !Array.isArray(item.barcode)
    ? item.barcode
    : {};
  const sym = libraryName(barcode);
  if (!sym) return null;
  let data = barcode.data;
  if (Number(item && item.typeKey) === BARCODE_FIELD) {
    data = bound == null || bound === '' ? '' : String(bound);
    if (data === '') return { barcode: sym, filetype: 'SVG', pending: true };
  }
  const opts = { barcode: sym, filetype: 'SVG' };
  if (data !== undefined && data !== null) opts.data = String(data);
  for (const key of BOOL_KEYS) {
    if (keyAllowed(sym, key) && barcode[key] === true) opts[key] = true;
  }
  for (const key of VALUE_KEYS) {
    if (!keyAllowed(sym, key) || !Object.hasOwn(barcode, key)) continue;
    const raw = barcode[key];
    if (raw === '' || raw === null || raw === undefined) continue;
    if (key === 'fg' || key === 'bg') {
      const color = colorValue(raw);
      if (color) opts[key] = color;
      continue;
    }
    const number = typeof raw === 'number' ? raw : Number(raw);
    if (!Number.isFinite(number)) continue;
    if (Object.hasOwn(OMIT_VALUE, key) && number === OMIT_VALUE[key]) continue;
    opts[key] = number;
  }
  for (const key of TEXT_KEYS) {
    if (!keyAllowed(sym, key) || !Object.hasOwn(barcode, key)) continue;
    const text = barcode[key];
    if (typeof text === 'string' && text !== '') opts[key] = text;
  }
  return opts;
}

/**
 * Pixel size from the zint SVG opening tag.
 * @param {string} svg
 * @returns {{ width: number, height: number }|null}
 */
export function svgPixelSize(svg) {
  if (typeof svg !== 'string') return null;
  const match = SVG_SIZE.exec(svg);
  if (!match) return null;
  return { width: Number(match[1]), height: Number(match[2]) };
}

/**
 * Fit inside the box. Stretch is not a barcode fit.
 * @param {object|null|undefined} item
 * @returns {string}
 */
export function barcodeFit(item) {
  const fit = item && item.fit;
  return BARCODE_FITS.includes(fit) ? fit : 'contain';
}

function anchorFactor(name, horizontal) {
  if (horizontal) {
    if (name === 'Left') return 0;
    if (name === 'Right') return 1;
    return 0.5;
  }
  if (name === 'Top') return 0;
  if (name === 'Bottom') return 1;
  return 0.5;
}

/**
 * @param {object|null|undefined} item
 * @returns {string}
 */
export function preserveAspect(item) {
  const x = FIT_X[item && item.fitAlign] || 'Mid';
  const y = FIT_Y[item && item.fitValign] || 'Mid';
  const mode = barcodeFit(item) === 'cover' ? 'slice' : 'meet';
  return `x${x}Y${y} ${mode}`;
}

/**
 * True when the symbol is drawn at its own pixel size.
 * Scale-down does that only while the symbol fits in the box.
 * @param {object|null|undefined} item
 * @param {{ width: number, height: number }} size
 * @param {{ width?: number, height?: number }|null|undefined} frame
 * @returns {boolean}
 */
export function barcodeIntrinsic(item, size, frame) {
  const fit = barcodeFit(item);
  if (fit === 'none') return true;
  if (fit !== 'scale-down') return false;
  const frameW = frame && frame.width > 0 ? frame.width : 0;
  const frameH = frame && frame.height > 0 ? frame.height : 0;
  if (!(frameW > 0) || !(frameH > 0) || !size) return false;
  return size.width <= frameW && size.height <= frameH;
}

/**
 * Where an intrinsic symbol sits in the box. Null means the
 * symbol scales to the box with meet or slice.
 * @param {object|null|undefined} item
 * @param {{ width: number, height: number }} size
 * @param {{ width?: number, height?: number }|null|undefined} frame
 * @returns {{ width: number, height: number, x: number, y: number }|null}
 */
export function barcodePlacement(item, size, frame) {
  if (!size || !barcodeIntrinsic(item, size, frame)) return null;
  const frameW = frame && frame.width > 0 ? frame.width : size.width;
  const frameH = frame && frame.height > 0 ? frame.height : size.height;
  return {
    width: size.width,
    height: size.height,
    x: (frameW - size.width) * anchorFactor(item && item.fitAlign, true),
    y: (frameH - size.height) * anchorFactor(item && item.fitValign, false),
  };
}

/**
 * The symbol keeps its aspect inside the box.
 * Contain letterboxes. Cover fills and crops. None and a
 * scale-down that fits use the symbol's own pixel size.
 * @param {string} svg
 * @param {object|null|undefined} item
 * @param {{ width?: number, height?: number }|null|undefined} [frame]
 * @returns {string}
 */
export function fittedSvg(svg, item, frame) {
  const size = svgPixelSize(svg);
  if (!size) return '';
  const own = barcodeIntrinsic(item, size, frame);
  const aspectItem = own ? { ...item, fit: 'contain' } : item;
  const width = own ? String(size.width) : '100%';
  const height = own ? String(size.height) : '100%';
  const open = `<svg width="${width}" height="${height}" viewBox="0 0 ${size.width} ${size.height}" preserveAspectRatio="${preserveAspect(aspectItem)}" xmlns="http://www.w3.org/2000/svg">`;
  return String(svg).replace(/<svg\b[^>]*>/i, open);
}
