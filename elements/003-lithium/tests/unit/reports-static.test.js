import { readFileSync } from 'node:fs';
import { resolve } from 'node:path';
import { describe, expect, it } from 'vitest';
import { addItem, createBlank, deleteItem, moveItem } from '../../src/managers/reports/model/document.js';
import { createHistory } from '../../src/managers/reports/model/history.js';
import {
  displayPixels, imageHash, rasterJobs, rasterKey, svgSubset,
} from '../../src/managers/reports/model/images.js';
import { advancePt, layoutBand, wrapText } from '../../src/managers/reports/model/wrap.js';
import { applyProperty, propertyRows } from '../../src/managers/reports/report-properties.js';
import { formatStaticDate, formatStaticNumber } from '../../src/managers/reports/reports-format.js';
import { previewParams, previewSrcdoc, resolvePreviewParams } from '../../src/managers/reports/reports-preview.js';
import { buildRasterMap } from '../../src/managers/reports/reports-rasters.js';
import { assignItemImage, importFailureMessage } from '../../src/managers/reports/reports-static.js';

const PNG = 'data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mP8z8BQDwAEhQGAhKmMIQAAAABJRU5ErkJggg==';
const OTHER = 'data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR42mNk+A8AAQUBAScY42YAAAAASUVORK5CYII=';
const PARAGRAPH = 'Annual notes cover the narrow column';

const attr = (key, name, collection) => ({
  key_idx: key, value_txt: name, collection: JSON.stringify(collection),
});
const lookups = {
  objectTypes: [
    { key_idx: 0, value_txt: 'Static Text', collection: JSON.stringify({ attributes: [0, 1, 2, 3, 4, 14, 29] }) },
  ],
  attributes: [
    attr(0, 'Top', { Group: 'Position', section: 5, Editor: 'Number' }),
    attr(1, 'Left', { Group: 'Position', section: 5, Editor: 'Number' }),
    attr(2, 'Width', { Group: 'Position', section: 5, Editor: 'Number' }),
    attr(3, 'Height', { Group: 'Position', section: 5, Editor: 'Number' }),
    attr(4, 'Align', { Group: 'Position', section: 5, Editor: 'List', List: ['Left', 'Center', 'Right'] }),
    attr(14, 'Rotation', { Group: 'Position', section: 5, Editor: 'Number' }),
    attr(29, 'Link', { Group: 'Object', section: 4, Label: 'Link', Editor: 'None' }),
  ],
};

function itemOf(doc, id) {
  return doc.bands.flatMap((band) => band.items).find((entry) => entry.id === id);
}

function fieldItem() {
  return {
    id: 'note',
    typeKey: 20,
    field: 'note',
    fitHeight: 'both',
    lineHeight: 1.2,
    align: 'Left',
    box: { left: '0 in', top: '0 in', width: '100 pt', height: '0.2 in' },
  };
}

const docFont = { font: { family: 'Vanadium Sans', size: '10 pt', style: 'Normal' } };

describe('static text measure', () => {
  it('locks Helvetica advances of space, A, and n at 10 pt', () => {
    expect(advancePt(' ', 10, 'Normal')).toBeCloseTo(2.78, 5);
    expect(advancePt('A', 10, 'Normal')).toBeCloseTo(6.67, 5);
    expect(advancePt('n', 10, 'Normal')).toBeCloseTo(5.56, 5);
    expect(advancePt('A', 10, 'Bold')).toBeCloseTo(7.22, 5);
    expect(advancePt('n', 10, 'Bold')).toBeCloseTo(6.11, 5);
    expect(advancePt('A', 10, 'Italic')).toBeCloseTo(6.67, 5);
    expect(advancePt('n', 10, 'Bold + Italic')).toBeCloseTo(6.11, 5);
  });

  it('breaks one paragraph on the shared widths', () => {
    const lines = (width) => wrapText(PARAGRAPH, width, 10, 'Normal', 'pre-line');
    expect(lines(80)).toEqual(['Annual notes', 'cover the narrow', 'column']);
    expect(lines(90)).toEqual(['Annual notes cover', 'the narrow column']);
    expect(lines(100)).toEqual(['Annual notes cover', 'the narrow column']);
    expect(lines(110)).toEqual(['Annual notes cover the', 'narrow column']);
    expect(lines(120)).toEqual(['Annual notes cover the', 'narrow column']);
    expect(lines(140)).toEqual(['Annual notes cover the narrow', 'column']);
  });

  it('gives a one-line row 12 pt and a two-line row 24 pt in Helvetica', () => {
    const band = { id: 'd1', typeKey: 15, height: '0.4 in' };
    const item = fieldItem();
    const short = layoutBand(docFont, band, [{ item, text: 'Hello' }]);
    expect(short.reflow).toBe(true);
    expect(short.usedPt).toBeCloseTo(12, 5);
    expect(short.items[0]).toMatchObject({
      lines: ['Hello'], heightPt: 12, grown: true, font: { family: 'Helvetica' },
    });
    const long = layoutBand(docFont, band, [{ item, text: PARAGRAPH }]);
    expect(long.usedPt).toBeCloseTo(24, 5);
    expect(long.items[0].lines).toEqual(['Annual notes cover', 'the narrow column']);
    expect(long.items[0].heightPt).toBeCloseTo(24, 5);
    expect(long.items[0].font.family).toBe('Helvetica');
    const clip = layoutBand(docFont, band, [{
      item: {
        id: 'text_1', typeKey: 0, align: 'Left',
        box: { left: '0 in', top: '0 in', width: '1.5 in', height: '0.2 in' },
      },
      text: 'Hello',
    }]);
    expect(clip.reflow).toBe(false);
    expect(clip.usedPt).toBeCloseTo(28.8, 5);
    expect(clip.items[0].lines).toBeNull();
    expect(clip.items[0].font.family).toBe('Vanadium Sans');
  });
});

describe('static number and datetime text', () => {
  it('prints the locked masks and leaves a bad number blank', () => {
    expect(formatStaticNumber(1234.5, '#,##0.00')).toBe('1,234.50');
    expect(formatStaticNumber(1234.5, '$#,##0.00')).toBe('$1,234.50');
    expect(formatStaticNumber(-12, '(#,##0.00)')).toBe('(12.00)');
    expect(formatStaticNumber(0.25, '0%')).toBe('25%');
    expect(formatStaticNumber('nope', '#,##0.00')).toBe('');
    expect(formatStaticNumber(null, '#,##0.00')).toBe('');
    expect(formatStaticDate('2026-03-05T14:07:09', 'yyyy-MMM-dd HH:mm')).toBe('2026-Mar-05 14:07');
    expect(formatStaticDate('2026-01-01', '')).toBe('2026-01-01');
    expect(formatStaticDate(Date.UTC(2026, 0, 1) / 1000, 'yyyy-MM-dd')).toBe('2026-01-01');
    expect(formatStaticDate('nope', 'yyyy-MM-dd')).toBe('nope');
    expect(formatStaticDate('', 'yyyy-MM-dd')).toBe('');
  });
});

describe('static item defaults', () => {
  it('stores number, datetime, memo, and image defaults and leaves text bare', () => {
    expect(createBlank().embedFonts).toBe(false);
    const blank = createBlank();
    const text = addItem(blank, 'd1', { typeKey: 0 });
    expect(itemOf(text, 'text_1')).toEqual({
      id: 'text_1', typeKey: 0, align: 'Left', text: 'Text',
      box: { left: '3.25 in', top: '0.025 in', width: '1.5 in', height: '0.2 in' },
    });
    const number = addItem(blank, 'd1', { typeKey: 1 });
    expect(itemOf(number, 'staticnum_1')).toEqual({
      id: 'staticnum_1', typeKey: 1, align: 'Left', value: 0, mask: '',
      wrap: 'nowrap', fitHeight: 'clip', valign: 'top',
      box: { left: '3.25 in', top: '0.025 in', width: '1.5 in', height: '0.2 in' },
    });
    const date = addItem(blank, 'd1', { typeKey: 4 });
    expect(itemOf(date, 'staticdate_1')).toMatchObject({
      value: '2026-01-01', wrap: 'nowrap', fitHeight: 'clip', valign: 'top', mask: '',
    });
    expect(Object.hasOwn(itemOf(date, 'staticdate_1'), 'timezone')).toBe(false);
    const memo = addItem(blank, 'd1', { typeKey: 3 });
    expect(itemOf(memo, 'memo_1')).toEqual({
      id: 'memo_1', typeKey: 3, align: 'Left', text: 'Memo',
      wrap: 'pre-wrap', fitHeight: 'both', valign: 'top',
      box: { left: '3.25 in', top: '0 in', width: '1.5 in', height: '0.6 in' },
    });
    const image = addItem(blank, 'd1', { typeKey: 2 });
    expect(itemOf(image, 'image_1')).toEqual({
      id: 'image_1', typeKey: 2, align: 'Left', dpi: 72, fit: 'contain',
      fitAlign: 'Center', fitValign: 'Middle', flip: 'none', opacity: 1,
      grayscale: false, invert: false,
      box: { left: '3.5 in', top: '0 in', width: '1 in', height: '0.75 in' },
    });
  });

  it('hides rotation on number and datetime and edits the client rows', () => {
    const number = addItem(createBlank(), 'd1', { typeKey: 1, id: 'staticnum_1' });
    const sel = { bandId: 'd1', itemId: 'staticnum_1' };
    const labels = propertyRows(number, sel, lookups).map((row) => row.label);
    expect(labels).not.toContain('Rotation');
    expect(labels).toEqual(expect.arrayContaining(['Value', 'Wrap', 'Fit height', 'Mask preset']));
    const dated = addItem(createBlank(), 'd1', { typeKey: 4, id: 'staticdate_1' });
    expect(propertyRows(dated, { bandId: 'd1', itemId: 'staticdate_1' }, lookups).map((row) => row.label))
      .not.toContain('Rotation');
    const set = applyProperty(number, sel, 'static-value', '12.5');
    expect(itemOf(set, 'staticnum_1').value).toBe(12.5);
    expect(() => applyProperty(set, sel, 'static-value', 'nope')).toThrow(/Value is not a number/);
    const cleared = applyProperty(set, sel, 'static-value', '');
    expect(Object.hasOwn(itemOf(cleared, 'staticnum_1'), 'value')).toBe(false);
    const dateSel = { bandId: 'd1', itemId: 'staticdate_1' };
    const stamped = applyProperty(dated, dateSel, 'static-value', '2026-03-05T14:07:09');
    expect(itemOf(stamped, 'staticdate_1').value).toBe('2026-03-05T14:07:09');
    const unix = applyProperty(stamped, dateSel, 'static-value', '1760000000');
    expect(itemOf(unix, 'staticdate_1').value).toBe(1760000000);
    expect(() => applyProperty(unix, dateSel, 'static-value', 'March')).toThrow(/Value is not a date/);
    const report = propertyRows(createBlank(), {}, lookups);
    expect(report.find((row) => row.id === 'embed-fonts')).toMatchObject({
      label: 'Embed fonts', value: 'False', editor: 'Boolean',
    });
    expect(applyProperty(createBlank(), {}, 'embed-fonts', 'True').embedFonts).toBe(true);
    expect(applyProperty(createBlank(), {}, 'embed-fonts', 'False').embedFonts).toBe(false);
  });
});

describe('static image registry', () => {
  it('stores the original filename and offers it as a File row', () => {
    const named = assignItemImage(
      addItem(createBlank(), 'd1', { typeKey: 2, id: 'image_1' }),
      'd1', 'image_1', PNG, 'photo.png',
    );
    expect(itemOf(named, 'image_1').fileName).toBe('photo.png');
    expect(named.images['21b32f6d']).toBe(PNG);
    const replaced = assignItemImage(named, 'd1', 'image_1', OTHER, 'next.png');
    expect(itemOf(replaced, 'image_1').fileName).toBe('next.png');
    const kept = assignItemImage(replaced, 'd1', 'image_1', PNG, '  ');
    expect(itemOf(kept, 'image_1').fileName).toBe('next.png');
    const rows = propertyRows(named, { bandId: 'd1', itemId: 'image_1' }, lookups);
    expect(rows.find((row) => row.id === 'static-file')).toMatchObject({
      label: 'File', value: 'photo.png', editor: 'File', section: 4, group: 'Object', omitted: false,
    });
    const empty = propertyRows(
      addItem(createBlank(), 'd1', { typeKey: 2, id: 'image_1' }),
      { bandId: 'd1', itemId: 'image_1' },
      lookups,
    ).find((row) => row.id === 'static-file');
    expect(empty).toMatchObject({ value: 'Choose a file', omitted: true, editor: 'File' });
    const failure = { data: { success: false, error: 'Reporting disabled', details: 'Reporting subsystem is not enabled' } };
    expect(importFailureMessage(failure)).toBe('Reporting subsystem is not enabled');
    expect(importFailureMessage(new Error('Server error. Please try again later.'))).toBe('Server error. Please try again later.');
  });

  it('hashes the fixture PNG and shares one raster for two placements', () => {
    expect(imageHash(PNG)).toBe('21b32f6d');
    expect(displayPixels('1 in', 72)).toBe(72);
    expect(displayPixels('0.75 in', 72)).toBe(54);
    const key = '21b32f6d|72|54|contain|Center|Middle|false|false';
    expect(rasterKey('21b32f6d', 72, 54, 'contain', 'Center', 'Middle', false, false)).toBe(key);
    let doc = addItem(createBlank(), 'ph1', { typeKey: 2, id: 'image_a' });
    doc = assignItemImage(doc, 'ph1', 'image_a', PNG);
    doc = addItem(doc, 'ph1', { typeKey: 2, id: 'image_b' });
    doc = assignItemImage(doc, 'ph1', 'image_b', PNG);
    expect(doc.images).toEqual({ '21b32f6d': PNG });
    expect(rasterJobs(doc)).toHaveLength(1);
    expect(rasterJobs(doc)[0].key).toBe(key);
  });

  it('keeps bytes out of a move, restores them on undo, and prunes on clear', () => {
    const placed = assignItemImage(addItem(createBlank(), 'd1', { typeKey: 2, id: 'image_1' }), 'd1', 'image_1', PNG);
    const history = createHistory(placed);
    history.commit(moveItem(history.document, 'd1', 'image_1', { left: '0.5 in' }));
    expect(JSON.stringify(history.document.history.undo.at(-1))).not.toContain('data:image');
    history.commit(assignItemImage(history.document, 'd1', 'image_1', OTHER));
    expect(history.document.images[imageHash(PNG)]).toBeUndefined();
    expect(history.document.images[imageHash(OTHER)]).toBe(OTHER);
    history.undo();
    expect(history.document.images[imageHash(PNG)]).toBe(PNG);
    expect(itemOf(history.document, 'image_1').image).toBe(imageHash(PNG));
    const removed = deleteItem(history.document, 'd1', 'image_1');
    expect(removed.images[imageHash(PNG)]).toBe(PNG);
    const dropped = createHistory(removed);
    dropped.clear();
    expect(dropped.document.images).toBeUndefined();
    expect(dropped.document.history).toBeUndefined();
    const kept = createHistory(placed);
    kept.clear();
    expect(kept.document.images[imageHash(PNG)]).toBe(PNG);
  });

  it('drops script from an SVG and keeps a shared image in the preview', () => {
    const subset = svgSubset('<svg xmlns="http://www.w3.org/2000/svg"><script>alert(1)</script><rect width="1" height="1" fill="#000000"/></svg>');
    expect(subset.ok).toBe(false);
    expect(subset.markup).not.toContain('script');
    expect(subset.markup).toContain('<rect');
    const html = [
      `<svg xmlns="http://www.w3.org/2000/svg" width="0" height="0"><defs><image id="rptimg1" href="${PNG}" width="72" height="54"/></defs></svg>`,
      '<svg xmlns="http://www.w3.org/2000/svg" width="100%" height="100%" viewBox="0 0 72 54" preserveAspectRatio="none"><use href="#rptimg1" transform="rotate(90 36 27) translate(36 27) scale(-1 1) translate(-36 -27)"/></svg>',
    ].join('');
    const src = previewSrcdoc(html);
    expect(src).toContain(`href="${PNG}"`);
    expect(src).toContain('href="#rptimg1"');
    expect(src).toContain('rotate(90 36 27) translate(36 27) scale(-1 1) translate(-36 -27)');
    expect(src).toContain('<image');
    expect(src).toContain('<use');
  });

  it('leaves rasters off an imageless preview and attaches one display PNG', async () => {
    expect(Object.hasOwn(previewParams(createBlank()), 'rasters')).toBe(false);
    const empty = await resolvePreviewParams({}, createBlank(), { draw: async () => PNG });
    expect(Object.hasOwn(empty.params, 'rasters')).toBe(false);
    const doc = assignItemImage(
      addItem(createBlank(), 'd1', { typeKey: 2, id: 'image_1' }),
      'd1', 'image_1', PNG,
    );
    const painted = 'data:image/png;base64,QQ==';
    const resolved = await resolvePreviewParams({}, doc, { draw: async () => painted });
    const key = '21b32f6d|72|54|contain|Center|Middle|false|false';
    expect(resolved.params.rasters).toEqual({ [key]: { data: painted } });
    expect(await buildRasterMap(doc, async () => painted)).toEqual({ [key]: { data: painted } });
  });
});

describe('static palette markup', () => {
  it('orders the static toolbar number, datetime, text, memo, shape, image', () => {
    const html = readFileSync(resolve(process.cwd(), 'src/managers/reports/reports.html'), 'utf8');
    const ids = [
      'reports-static-number-btn',
      'reports-static-date-btn',
      'reports-text-btn',
      'reports-static-memo-btn',
      'reports-shape-btn',
      'reports-static-image-btn',
    ];
    const at = ids.map((id) => html.indexOf(`id="${id}"`));
    expect(at.every((index) => index >= 0)).toBe(true);
    expect([...at].sort((left, right) => left - right)).toEqual(at);
    expect(html.indexOf('id="reports-image-file"')).toBeGreaterThan(at[at.length - 1]);
    expect(html).toContain('data-type-key="1"');
    expect(html).toContain('data-type-key="4"');
    expect(html).toContain('data-type-key="2"');
    expect(html).toContain('data-type-key="3"');
  });
});
