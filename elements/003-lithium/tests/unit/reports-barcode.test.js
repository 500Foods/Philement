import { describe, expect, it } from 'vitest';
import { addItem, createBlank, resizeBand, resizeItem } from '../../src/managers/reports/model/document.js';
import { parseLength } from '../../src/managers/reports/model/units.js';
import {
  BARCODE_SAMPLES, barcodeOpts, barcodePlacement, fittedSvg, svgPixelSize, symbologyOptions,
} from '../../src/managers/reports/model/barcode.js';
import { groupBySection } from '../../src/properties/sections.js';
import { applyProperty, propertyRows } from '../../src/managers/reports/report-properties.js';
import {
  barcodeRows, clearBarcodeCache, fieldBarcodeValue, paintBarcode, rememberBarcodeSvg, withBarcodeSections,
} from '../../src/managers/reports/reports-barcode.js';
import { previewSrcdoc } from '../../src/managers/reports/reports-preview.js';
import { addDatasource, setDatasourceRows, unboundFields } from '../../src/managers/reports/reports-sample.js';

const SVG = '<svg width="190" height="70" version="1.1" xmlns="http://www.w3.org/2000/svg"><rect width="190" height="70" fill="#000000"/></svg>';

function itemOf(doc, id) {
  return doc.bands.flatMap((band) => band.items || []).find((entry) => entry.id === id);
}

function inches(length) {
  return parseLength(length).value;
}

const SYMBOLOGY_ORDER = Object.freeze([
  'aztec', 'code39', 'code128', 'code128ab', 'pdf417comp', 'datamatrix',
  'ean2', 'ean5', 'ean8', 'ean13', 'flat', 'dbarexp', 'dbarexpstk',
  'dbaromn', 'dbaromnstk', 'gs1-128', 'c25inter', 'isbn', 'itf14',
  'maxicode', 'nve18', 'pdf417', 'qr', 'upca', 'upce', 'usps',
]);

describe('barcode symbologies', () => {
  it('sorts by symbology name and fills the built-in names', () => {
    const rows = symbologyOptions([
      { code: 'code11', value_txt: 'Code 11' },
      { code: 'code128', value_txt: 'Code 128' },
      { code: 'aztec', value_txt: 'Aztec Code' },
    ]);
    expect(rows.map((entry) => entry.code)).toEqual(SYMBOLOGY_ORDER);
    expect(rows[0]).toMatchObject({ code: 'aztec', label: 'Aztec Code' });
    expect(rows.some((entry) => entry.code === 'code11')).toBe(false);
    expect(rows.at(-1).code).toBe('usps');
    const labels = rows.map((entry) => entry.label);
    const sorted = labels.slice().sort((a, b) => a.localeCompare(b, 'en', { numeric: true }));
    expect(labels).toEqual(sorted);
  });
});

describe('barcode options', () => {
  it('maps UPC check digits and leaves scale, gs1, output, and rotate off', () => {
    const opts = barcodeOpts({
      typeKey: 43,
      rotation: 90,
      dpi: 300,
      barcode: {
        symbology: 'upca', check: 'supplied', data: '123456789012',
        fg: '#112233', scale: 1, gs1: true, rotate: 90,
      },
    });
    expect(opts.barcode).toBe('upcachk');
    expect(opts.data).toBe('123456789012');
    expect(opts.fg).toBe('112233');
    expect(opts.filetype).toBe('SVG');
    expect(opts.scale).toBeUndefined();
    expect(opts.gs1).toBeUndefined();
    expect(opts.output).toBeUndefined();
    expect(opts.rotate).toBeUndefined();
  });

  it('maps GS1-128 and does not pass gs1', () => {
    const opts = barcodeOpts({ barcode: { symbology: 'gs1-128', data: '(01)1', gs1: true } });
    expect(opts.barcode).toBe('gs1128');
    expect(opts.gs1).toBeUndefined();
  });

  it('does not pass gs1 for Code 128', () => {
    const opts = barcodeOpts({ barcode: { symbology: 'code128', data: 'Ab-129', gs1: true } });
    expect(opts.barcode).toBe('code128');
    expect(opts.gs1).toBeUndefined();
  });

  it('maps the added symbologies and their options', () => {
    expect(barcodeOpts({ barcode: { symbology: 'upce', check: 'supplied', data: '1234567' } }).barcode).toBe('upcechk');
    const matrix = barcodeOpts({
      barcode: { symbology: 'datamatrix', data: 'HELLO', vers: 10, square: true, gs1: true, fast: true },
    });
    expect(matrix).toMatchObject({ barcode: 'datamatrix', vers: 10, square: true, gs1: true, fast: true });
    const pdf = barcodeOpts({
      barcode: { symbology: 'pdf417', data: 'PDF417', cols: 4, rows: 8, secure: 2, fast: true, vers: 3 },
    });
    expect(pdf).toMatchObject({ barcode: 'pdf417', cols: 4, rows: 8, secure: 2, fast: true });
    expect(pdf.vers).toBeUndefined();
    const compact = barcodeOpts({
      barcode: { symbology: 'pdf417comp', data: 'Hello', cols: 3, rows: 6, secure: 2, fast: true },
    });
    expect(compact).toMatchObject({ barcode: 'pdf417comp', cols: 3, rows: 6, secure: 2, fast: true });
    const aztec = barcodeOpts({
      barcode: { symbology: 'aztec', data: '123456789012', vers: 5, secure: 2, azfull: true },
    });
    expect(aztec).toMatchObject({ barcode: 'aztec', vers: 5, secure: 2, azfull: true });
    const maxi = barcodeOpts({
      barcode: { symbology: 'maxicode', data: 'A MaxiCode Message in Mode 4', mode: 4, primary: '999999999840012', scmvv: 96 },
    });
    expect(maxi).toMatchObject({ barcode: 'maxicode', mode: 4, primary: '999999999840012', scmvv: 96 });
    expect(barcodeOpts({ barcode: { symbology: 'usps', data: '01234567094987654321' } }).barcode).toBe('uspsimail');
    const stacked = barcodeOpts({
      barcode: { symbology: 'dbarexpstk', data: 'X', cols: 3, rows: 4, gs1: true, gs1parens: true },
    });
    expect(stacked).toMatchObject({ barcode: 'dbarexpstk', cols: 3, rows: 4, gs1parens: true });
    expect(stacked.gs1).toBeUndefined();
    const linear = barcodeOpts({ barcode: { symbology: 'gs1-128', data: '(01)1', gs1parens: true, gs1: true } });
    expect(linear.gs1parens).toBe(true);
    expect(linear.gs1).toBeUndefined();
    expect(barcodeOpts({ barcode: { symbology: 'c25inter', data: '1234567890', vers: 2 } }).vers).toBe(2);
  });
});

describe('barcode frame', () => {
  it('stores the width and height it is given', () => {
    let doc = resizeBand(addItem(createBlank(), 'd1', { typeKey: 43 }), 'd1', '2 in');
    const id = doc.bands.find((band) => band.id === 'd1').items.at(-1).id;
    doc = resizeItem(doc, 'd1', id, { width: '3.5 in', height: '2 in' });
    expect(inches(itemOf(doc, id).box.width)).toBeCloseTo(3.5, 5);
    expect(inches(itemOf(doc, id).box.height)).toBeCloseTo(2, 5);
    doc = resizeItem(doc, 'd1', id, { height: '0.7 in' });
    expect(inches(itemOf(doc, id).box.height)).toBeCloseTo(0.7, 5);
    expect(inches(itemOf(doc, id).box.width)).toBeCloseTo(3.5, 5);
  });

  it('still resizes text width and height independently', () => {
    let doc = resizeBand(addItem(createBlank(), 'd1', { typeKey: 0 }), 'd1', '2 in');
    const id = doc.bands.find((band) => band.id === 'd1').items.at(-1).id;
    doc = resizeItem(doc, 'd1', id, { width: '2 in', height: '0.4 in' });
    expect(inches(itemOf(doc, id).box.width)).toBeCloseTo(2, 5);
    expect(inches(itemOf(doc, id).box.height)).toBeCloseTo(0.4, 5);
  });
});

describe('barcode svg', () => {
  it('fits the symbol with a meet aspect', () => {
    const fitted = fittedSvg(SVG, { fitAlign: 'Center', fitValign: 'Middle' });
    expect(svgPixelSize(SVG)).toEqual({ width: 190, height: 70 });
    expect(fitted).toContain('width="100%"');
    expect(fitted).toContain('height="100%"');
    expect(fitted).toContain('viewBox="0 0 190 70"');
    expect(fitted).toContain('preserveAspectRatio="xMidYMid meet"');
    expect(fitted.includes('preserveAspectRatio="none"')).toBe(false);
  });

  it('covers with slice and places none at the symbol size', () => {
    const covered = fittedSvg(SVG, { fit: 'cover', fitAlign: 'Left', fitValign: 'Top' });
    expect(covered).toContain('preserveAspectRatio="xMinYMin slice"');
    expect(covered).toContain('width="100%"');
    const own = fittedSvg(SVG, { fit: 'none', fitAlign: 'Right', fitValign: 'Bottom' });
    expect(own).toContain('width="190"');
    expect(own).toContain('height="70"');
    expect(own).toContain('preserveAspectRatio="xMaxYMax meet"');
    const place = barcodePlacement(
      { fit: 'none', fitAlign: 'Right', fitValign: 'Bottom' },
      { width: 190, height: 70 },
      { width: 300, height: 100 },
    );
    expect(place).toEqual({ width: 190, height: 70, x: 110, y: 30 });
    const shrunk = barcodePlacement(
      { fit: 'scale-down' },
      { width: 190, height: 70 },
      { width: 100, height: 100 },
    );
    expect(shrunk).toBeNull();
  });

  it('mounts the symbol and leaves resize handles to the caller', () => {
    clearBarcodeCache();
    const doc = addItem(createBlank(), 'd1', { typeKey: 43 });
    const item = doc.bands.find((band) => band.id === 'd1').items.at(-1);
    rememberBarcodeSvg(barcodeOpts(item), SVG);
    const el = document.createElement('div');
    paintBarcode(el, item, doc);
    const node = el.querySelector('svg');
    expect(node).not.toBeNull();
    expect(node.getAttribute('preserveAspectRatio')).toBe('xMidYMid meet');
    expect(el.querySelector('.reports-item-handle')).toBeNull();
  });
});

describe('barcode preview', () => {
  it('keeps barcode text and a meet aspect, and still allows none', () => {
    const marked = previewSrcdoc(
      '<svg width="100%" height="100%" viewBox="0 0 200 80" preserveAspectRatio="xMidYMid meet" xmlns="http://www.w3.org/2000/svg"><text x="10" y="20" text-anchor="middle" font-family="OCRB, monospace" font-size="10" font-weight="normal">Ab-129</text></svg>',
    );
    expect(marked).toContain('Ab-129');
    expect(marked).toContain('OCRB, monospace');
    expect(marked).toContain('xMidYMid meet');
    const faced = previewSrcdoc(
      '<svg xmlns="http://www.w3.org/2000/svg" width="10" height="10"><style>@font-face {font-family:"OCRB"; src:url(data:font/woff2;base64,QQ==);}</style><text font-family="OCRB">1</text></svg>',
    );
    expect(faced).toContain('@font-face');
    expect(faced).toContain('data:font/woff2;base64,QQ==');
    const plain = previewSrcdoc(
      '<svg width="100%" height="100%" preserveAspectRatio="none" xmlns="http://www.w3.org/2000/svg"><rect width="1" height="1" fill="#000000"/></svg>',
    );
    expect(plain).toContain('preserveAspectRatio="none"');
  });
});

describe('barcode samples', () => {
  it('replaces example data when the symbology changes and keeps a typed value', () => {
    let doc = addItem(createBlank(), 'd1', { typeKey: 43 });
    const id = doc.bands.find((band) => band.id === 'd1').items.at(-1).id;
    const sel = { bandId: 'd1', itemId: id };
    expect(itemOf(doc, id).barcode.data).toBe(BARCODE_SAMPLES.upca);
    doc = applyProperty(doc, sel, '52', 'code128');
    expect(itemOf(doc, id).barcode.symbology).toBe('code128');
    expect(itemOf(doc, id).barcode.data).toBe('Ab-129');
    doc = applyProperty(doc, sel, '52', 'gs1-128');
    expect(itemOf(doc, id).barcode.data).toBe('[01]09501101020917[10]AB-123');
    doc = applyProperty(doc, sel, 'barcode-data', 'LOT-42');
    doc = applyProperty(doc, sel, '52', 'qr');
    expect(itemOf(doc, id).barcode.data).toBe('LOT-42');
    doc = applyProperty(doc, sel, 'barcode-data', '');
    doc = applyProperty(doc, sel, '52', 'code39');
    expect(itemOf(doc, id).barcode.data).toBe('CODE-39');
    doc = applyProperty(doc, sel, '52', 'pdf417');
    expect(itemOf(doc, id).barcode.data).toBe('PDF417');
    doc = applyProperty(doc, sel, 'barcode-pdf-cols', '4');
    doc = applyProperty(doc, sel, 'barcode-secure', '0');
    expect(itemOf(doc, id).barcode.cols).toBe(4);
    expect(itemOf(doc, id).barcode.secure).toBe(0);
    doc = applyProperty(doc, sel, '52', 'c25inter');
    doc = applyProperty(doc, sel, 'barcode-c25check', 'show');
    expect(itemOf(doc, id).barcode.vers).toBe(1);
    doc = applyProperty(doc, sel, 'barcode-c25check', 'none');
    expect(itemOf(doc, id).barcode.vers).toBeUndefined();
    doc = applyProperty(doc, sel, '52', 'maxicode');
    doc = applyProperty(doc, sel, 'barcode-primary', '999999999840012');
    expect(itemOf(doc, id).barcode.primary).toBe('999999999840012');
    expect(itemOf(doc, id).barcode.data).toBe('A MaxiCode Message in Mode 4');
  });
});

function barcodeGroups(item) {
  const sections = withBarcodeSections([
    { id: 4, label: 'Object', sortSeq: 4 },
    { id: 5, label: 'Position', sortSeq: 5 },
    { id: 7, label: 'Formatting', sortSeq: 7 },
  ]);
  const rows = barcodeRows(item).map((row, index) => ({
    ...row,
    sortSeq: index,
    collection: { section: row.section, Group: row.group },
  }));
  return groupBySection(rows, sections).map((group) => ({
    label: group.section.label,
    ids: group.rows.map((row) => row.id),
  }));
}

describe('barcode sections', () => {
  it('keeps the four headers between Object and Position', () => {
    const loaded = withBarcodeSections([
      { id: 4, label: 'Object', sortSeq: 4 },
      { id: 13, label: 'Barcode Placement', sortSeq: 13 },
    ]);
    expect(loaded.find((section) => section.id === 13).sortSeq).toBe(4.2);
    expect(loaded.find((section) => section.id === 14)).toMatchObject({ label: 'Barcode Text', sortSeq: 4.4 });
    expect(loaded.find((section) => section.id === 15)).toMatchObject({ label: 'Barcode Colors', sortSeq: 4.6 });
    expect(loaded.find((section) => section.id === 16)).toMatchObject({ label: 'Barcode Symbology', sortSeq: 4.8 });
    const upca = barcodeGroups({ typeKey: 43, barcode: { symbology: 'upca' } });
    expect(upca.map((group) => group.label)).toEqual([
      'Object', 'Barcode Placement', 'Barcode Text', 'Barcode Colors', 'Barcode Symbology',
    ]);
    expect(upca[0].ids).toEqual(['barcode-data', 'barcode-init', 'barcode-esc']);
    expect(upca[1].ids.slice(0, 4)).toEqual(['barcode-fit', 'barcode-fit-align', 'barcode-fit-vertical', 'barcode-dpi']);
    expect(upca[2].ids).toEqual(['barcode-show-text', 'barcode-small', 'barcode-bold', 'barcode-embedfont', 'barcode-textgap']);
    expect(upca[3].ids).toEqual(['barcode-fg', 'barcode-bg', 'barcode-nobackground', 'barcode-reverse']);
    expect(upca[4].ids).toEqual(['barcode-check', 'barcode-guarddescent']);
    expect(barcodeGroups({ typeKey: 43, barcode: { symbology: 'code39' } }).map((group) => group.label))
      .not.toContain('Barcode Symbology');
    const qr = barcodeGroups({ typeKey: 43, barcode: { symbology: 'qr' } });
    expect(qr.find((group) => group.label === 'Barcode Symbology').ids).toEqual([
      'barcode-gs1', 'barcode-gs1parens', 'barcode-dotty', 'barcode-dotsize',
    ]);
    const code128 = barcodeGroups({ typeKey: 43, barcode: { symbology: 'code128' } });
    expect(code128.find((group) => group.label === 'Barcode Symbology').ids).toEqual(['barcode-extraesc']);
    expect(barcodeGroups({ typeKey: 45, barcode: { symbology: 'gs1-128' } })
      .find((group) => group.label === 'Barcode Symbology').ids).toEqual(['barcode-gs1parens']);
    const matrix = barcodeGroups({ typeKey: 43, barcode: { symbology: 'datamatrix' } });
    expect(matrix.find((group) => group.label === 'Barcode Symbology').ids).toEqual([
      'barcode-vers', 'barcode-fast', 'barcode-square', 'barcode-dmre', 'barcode-dmiso144',
      'barcode-gssep', 'barcode-gs1', 'barcode-gs1parens', 'barcode-dotty', 'barcode-dotsize',
    ]);
    const pdf = barcodeGroups({ typeKey: 43, barcode: { symbology: 'pdf417' } });
    expect(pdf.find((group) => group.label === 'Barcode Symbology').ids).toEqual([
      'barcode-pdf-cols', 'barcode-pdf-rows', 'barcode-secure', 'barcode-fast',
    ]);
    expect(barcodeGroups({ typeKey: 43, barcode: { symbology: 'pdf417comp' } })
      .find((group) => group.label === 'Barcode Symbology').ids).toEqual([
      'barcode-pdf-cols', 'barcode-pdf-rows', 'barcode-secure', 'barcode-fast',
    ]);
    const aztec = barcodeGroups({ typeKey: 43, barcode: { symbology: 'aztec' } });
    expect(aztec.find((group) => group.label === 'Barcode Symbology').ids).toEqual([
      'barcode-aztec-vers', 'barcode-aztec-secure', 'barcode-fast', 'barcode-azfull',
      'barcode-gs1', 'barcode-gs1parens', 'barcode-dotty', 'barcode-dotsize', 'barcode-extraesc',
    ]);
    const maxi = barcodeGroups({ typeKey: 43, barcode: { symbology: 'maxicode' } });
    expect(maxi.find((group) => group.label === 'Barcode Symbology').ids).toEqual([
      'barcode-mode', 'barcode-scmvv', 'barcode-primary',
    ]);
    const stacked = barcodeGroups({ typeKey: 43, barcode: { symbology: 'dbarexpstk' } });
    expect(stacked.find((group) => group.label === 'Barcode Symbology').ids).toEqual([
      'barcode-cols', 'barcode-rows', 'barcode-gs1parens',
    ]);
    const c25 = barcodeGroups({ typeKey: 43, barcode: { symbology: 'c25inter', vers: 1 } });
    expect(c25.find((group) => group.label === 'Barcode Symbology').ids).toEqual(['barcode-c25check']);
    expect(barcodeGroups({ typeKey: 43, barcode: { symbology: 'ean13' } })
      .find((group) => group.label === 'Barcode Symbology').ids).toEqual(['barcode-guarddescent']);
    expect(barcodeGroups({ typeKey: 43, barcode: { symbology: 'usps' } }).map((group) => group.label))
      .not.toContain('Barcode Symbology');
  });
});

describe('field barcode', () => {
  it('reads Data from the datasource field and does not store a sample', () => {
    let doc = addDatasource(createBlank());
    const name = doc.datasources.at(-1).name;
    doc = setDatasourceRows(doc, name, [{ sku: 'ABC', qty: 1 }]).doc;
    doc.bands.find((band) => band.id === 'd1').datasource = name;
    doc = addItem(doc, 'd1', { typeKey: 45 });
    const item = doc.bands.find((band) => band.id === 'd1').items.at(-1);
    const sel = { bandId: 'd1', itemId: item.id };
    expect(item.datasource).toBe(name);
    expect(item.field).toBe('');
    expect(item.barcode.data).toBeUndefined();
    expect(item.barcode.symbology).toBe('upca');
    expect(unboundFields(doc)).toEqual([{ bandId: 'd1', itemId: item.id, typeKey: 45 }]);
    const rows = propertyRows(doc, sel, { objectTypes: [], attributes: [] });
    const keys = rows.map((entry) => entry.key).filter((key) => key === 7 || key === 8);
    expect(keys).toEqual([8, 7]);
    expect(rows.some((entry) => entry.id === 'barcode-data')).toBe(false);
    expect(rows.find((entry) => entry.key === 7).list).toEqual(['sku', 'qty']);
    expect(fieldBarcodeValue(doc, item)).toBe('');
    expect(barcodeOpts(item, fieldBarcodeValue(doc, item)).pending).toBe(true);
    doc = applyProperty(doc, sel, '7', 'sku');
    const bound = itemOf(doc, item.id);
    expect(fieldBarcodeValue(doc, bound)).toBe('ABC');
    expect(barcodeOpts(bound, 'ABC').data).toBe('ABC');
    expect(unboundFields(doc)).toEqual([]);
    doc = applyProperty(doc, sel, '52', 'code128');
    expect(itemOf(doc, item.id).barcode.data).toBeUndefined();
    expect(itemOf(doc, item.id).barcode.symbology).toBe('code128');
    doc = addDatasource(doc, 'other');
    doc = setDatasourceRows(doc, 'other', [{ code: 'Z' }]).doc;
    doc = applyProperty(doc, sel, '8', 'other');
    expect(itemOf(doc, item.id).datasource).toBe('other');
    expect(itemOf(doc, item.id).field).toBeUndefined();
    expect(unboundFields(doc)).toEqual([{ bandId: 'd1', itemId: item.id, typeKey: 45 }]);
  });
});

describe('barcode items', () => {
  it('adds a UPC with the default frame', () => {
    const doc = addItem(createBlank(), 'd1', { typeKey: 43 });
    const item = doc.bands.find((band) => band.id === 'd1').items.at(-1);
    expect(item.id.startsWith('barcode_')).toBe(true);
    expect(item.dpi).toBe(300);
    expect(item.barcode.symbology).toBe('upca');
    expect(item.barcode.data).toBe('12345678901');
    expect(inches(item.box.width)).toBeCloseTo(1.75, 5);
    expect(inches(item.box.height)).toBeCloseTo(0.7, 5);
    const rows = propertyRows(doc, { bandId: 'd1', itemId: item.id }, { objectTypes: [], attributes: [] });
    expect(rows.some((entry) => entry.key === 52 && entry.label === 'Symbology')).toBe(true);
    const dataRow = rows.find((entry) => entry.id === 'barcode-data');
    expect(dataRow).toMatchObject({ label: 'Data', multiline: true, editor: 'Text' });
    const edited = applyProperty(doc, { bandId: 'd1', itemId: item.id }, 'barcode-data', 'https://500courses.com\nseat 4');
    expect(itemOf(edited, item.id).barcode.data).toBe('https://500courses.com\nseat 4');
  });
});
