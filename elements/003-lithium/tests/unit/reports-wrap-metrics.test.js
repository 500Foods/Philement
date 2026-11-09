import { describe, expect, it } from 'vitest';
import { measurePt, wrapText, layoutBand, ensureFontsReady } from '../../src/managers/reports/model/wrap.js';
import { createBlank, addItem } from '../../src/managers/reports/model/document.js';

describe('Phase 79 wrap metrics', () => {
  it('AFM Helvetica measure still works without a family', () => {
    const w = measurePt('Hello', 10, 'Normal');
    expect(w).toBeGreaterThan(20);
    expect(w).toBeLessThan(40);
  });

  it('ensureFontsReady resolves', async () => {
    await expect(ensureFontsReady()).resolves.toBeUndefined();
  });

  it('layoutBand keeps the item font on grown rows (no Helvetica force)', () => {
    let doc = createBlank();
    doc = addItem(doc, 'd1', {
      typeKey: 0,
      id: 't1',
      text: 'Hello world that wraps across several lines for grow',
      font: { family: 'Vanadium Sans', size: '12 pt', style: 'Bold' },
      fitHeight: 'grow',
      wrap: 'pre-line',
      box: { left: '0 in', top: '0 in', width: '1 in', height: '0.2 in' },
    });
    const band = doc.bands.find((b) => b.id === 'd1');
    const item = band.items.find((i) => i.id === 't1');
    const laid = layoutBand(doc, band, [{ item, text: item.text }]);
    expect(laid.reflow).toBe(true);
    const row = laid.items[0];
    expect(row.grown).toBe(true);
    expect(row.font.family).toBe('Vanadium Sans');
    expect(row.font.family).not.toBe('Helvetica');
  });

  it('wrapText accepts a family argument (canvas or AFM)', () => {
    const lines = wrapText('one two three four five six', 40, 10, 'Normal', 'pre-line', 'Helvetica');
    expect(lines.length).toBeGreaterThan(1);
  });
});
