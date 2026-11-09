import { describe, expect, it } from 'vitest';
import { applyProperty } from '../../src/managers/reports/report-properties.js';
import { createBlank, addItem } from '../../src/managers/reports/model/document.js';
import { paintBandItems } from '../../src/managers/reports/items.js';
import { NONE_COLOR, normalizeColorValue } from '../../src/managers/reports/reports-colours.js';

describe('Static Text canvas colours (Phase 77)', () => {
  it('normalizeColorValue maps empty/transparent to none', () => {
    expect(normalizeColorValue('')).toBe(NONE_COLOR);
    expect(normalizeColorValue('transparent')).toBe(NONE_COLOR);
    expect(normalizeColorValue('NONE')).toBe(NONE_COLOR);
    expect(normalizeColorValue('#AbC')).toBe('#abc');
    expect(normalizeColorValue('#AaBbCc')).toBe('#aabbcc');
  });

  it('paints fore/back/background CSS vars after property commit (default Static Text path)', () => {
    let doc = createBlank();
    doc = addItem(doc, 'd1', { typeKey: 0, id: 't1', text: 'Hello' });
    const sel = { bandId: 'd1', itemId: 't1' };
    doc = applyProperty(doc, sel, '55', '#ff0000');
    doc = applyProperty(doc, sel, '56', '#00ff00');
    doc = applyProperty(doc, sel, '57', '#0000ff');
    const item = doc.bands.find((b) => b.id === 'd1').items.find((i) => i.id === 't1');
    expect(item.foreColor).toBe('#ff0000');
    expect(item.backColor).toBe('#00ff00');
    expect(item.backgroundColor).toBe('#0000ff');

    const body = document.createElement('div');
    const band = doc.bands.find((b) => b.id === 'd1');
    paintBandItems(body, band, doc);
    const el = body.querySelector('.reports-item[data-item-id="t1"]');
    expect(el).toBeTruthy();
    expect(el.style.getPropertyValue('--reports-item-fore-color')).toBe('#ff0000');
    expect(el.style.getPropertyValue('--reports-item-background-color')).toBe('#0000ff');
    // Back Color lands on the inline ink span (glyph highlight), not the block face.
    const ink = el.querySelector('.reports-item-text-ink');
    expect(ink).toBeTruthy();
    expect(ink.style.getPropertyValue('--reports-item-back-color')).toBe('#00ff00');
    expect(el.querySelector('.reports-item-text')).toBeTruthy();
    expect(ink.parentElement.className).toBe('reports-item-text');
    expect(ink.textContent).toBe('Hello');
  });

  it('stores none when the colour field is cleared', () => {
    let doc = createBlank();
    doc = addItem(doc, 'd1', { typeKey: 0, id: 't1', text: 'Hello' });
    const sel = { bandId: 'd1', itemId: 't1' };
    doc = applyProperty(doc, sel, '55', '#112233');
    doc = applyProperty(doc, sel, '55', '');
    const item = doc.bands.find((b) => b.id === 'd1').items.find((i) => i.id === 't1');
    expect(item.foreColor).toBe(NONE_COLOR);

    const body = document.createElement('div');
    paintBandItems(body, doc.bands.find((b) => b.id === 'd1'), doc);
    const el = body.querySelector('.reports-item[data-item-id="t1"]');
    expect(el.style.getPropertyValue('--reports-item-fore-color')).toBe('transparent');
  });
});
