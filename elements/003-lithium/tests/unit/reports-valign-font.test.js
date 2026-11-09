import { describe, expect, it } from 'vitest';
import { applyProperty } from '../../src/managers/reports/report-properties.js';
import { createBlank, addItem } from '../../src/managers/reports/model/document.js';
import { paintBandItems } from '../../src/managers/reports/items.js';
import { paintMeasuredItem } from '../../src/managers/reports/reports-static.js';

describe('Phase 78 V. Align + font on canvas', () => {
  it('Clip Static Text sets data-valign and keeps chosen font family', () => {
    let doc = createBlank();
    doc = addItem(doc, 'd1', {
      typeKey: 0, id: 't1', text: 'Hi',
      font: { family: 'Vanadium Sans', size: '12 pt', style: 'Normal' },
      valign: 'middle', align: 'Center',
      fitHeight: 'clip',
      backgroundColor: '#ddeeff', backColor: '#ffff00',
    });
    const body = document.createElement('div');
    paintBandItems(body, doc.bands.find((b) => b.id === 'd1'), doc);
    const el = body.querySelector('.reports-item[data-item-id="t1"]');
    expect(el.dataset.valign).toBe('middle');
    expect(el.dataset.align).toBe('Center');
    expect(el.style.getPropertyValue('--reports-item-font-family')).toContain('Vanadium Sans');
    expect(el.querySelector('.reports-item-text-ink')).toBeTruthy();
  });

  it('non-Clip (grow) measured path still paints the chosen font, not Helvetica', () => {
    let doc = createBlank();
    doc = addItem(doc, 'd1', {
      typeKey: 0, id: 't1', text: 'Hello world that wraps',
      font: { family: 'Vanadium Sans', size: '12 pt', style: 'Bold' },
      valign: 'bottom', align: 'Right',
      fitHeight: 'grow',
    });
    const body = document.createElement('div');
    const band = doc.bands.find((b) => b.id === 'd1');
    const item = band.items.find((i) => i.id === 't1');
    paintMeasuredItem(body, item, doc);
    const el = body.querySelector('.reports-item[data-item-id="t1"]');
    expect(el.classList.contains('reports-item-wrapped')).toBe(true);
    expect(el.dataset.valign).toBe('bottom');
    expect(el.style.getPropertyValue('--reports-item-font-family')).toContain('Vanadium Sans');
    expect(el.style.getPropertyValue('--reports-item-font-family')).not.toContain('Helvetica');
  });

  it('property commit for V. Align stores valign and repaints data-valign', () => {
    let doc = createBlank();
    doc = addItem(doc, 'd1', { typeKey: 0, id: 't1', text: 'X' });
    const sel = { bandId: 'd1', itemId: 't1' };
    doc = applyProperty(doc, sel, 'static-valign', 'bottom');
    const item = doc.bands.find((b) => b.id === 'd1').items.find((i) => i.id === 't1');
    expect(item.valign).toBe('bottom');
    const body = document.createElement('div');
    paintBandItems(body, doc.bands.find((b) => b.id === 'd1'), doc);
    expect(body.querySelector('.reports-item').dataset.valign).toBe('bottom');
  });
});
