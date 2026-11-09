import { describe, expect, it } from 'vitest';
import {
  defaultOverflow, mapFitHeight, migrateFitHeight, OVERFLOW_MODES, resolveOverflow,
} from '../../src/managers/reports/model/overflow.js';
import { ellipsisLine, layoutBand, measurePt } from '../../src/managers/reports/model/wrap.js';
import { importDefinition } from '../../src/managers/reports/model/document.js';
import { layoutRows, paintsMeasured } from '../../src/managers/reports/reports-static.js';

describe('overflow mapping', () => {
  it('lists five Overflow modes', () => {
    expect([...OVERFLOW_MODES]).toEqual(['Clip', 'Ellipsis', 'Grow', 'Shrink', 'Fit']);
  });

  it('maps legacy fitHeight', () => {
    expect(mapFitHeight('clip')).toBe('Clip');
    expect(mapFitHeight('grow')).toBe('Grow');
    expect(mapFitHeight('shrink')).toBe('Shrink');
    expect(mapFitHeight('both')).toBe('Grow');
    expect(mapFitHeight('nope')).toBeNull();
  });

  it('defaults memo to Grow and text to Clip', () => {
    expect(defaultOverflow(0)).toBe('Clip');
    expect(defaultOverflow(3)).toBe('Grow');
  });

  it('migrates fitHeight on load and drops the old key', () => {
    const item = { typeKey: 0, fitHeight: 'both', id: 't1' };
    migrateFitHeight(item);
    expect(item.overflow).toBe('Grow');
    expect(item.fitHeight).toBeUndefined();
  });

  it('importDefinition migrates every item', () => {
    const doc = importDefinition({
      format: 1,
      name: 't',
      title: 't',
      page: { orientation: 'portrait', units: 'in', width: '8.5 in', height: '11 in',
        margin: { left: '0.25 in', right: '0.25 in', top: '0.25 in', bottom: '0.25 in' } },
      font: { family: 'Helvetica', size: '12 pt', style: 'Normal' },
      bands: [{
        id: 'd', typeKey: 15, height: '0.5 in',
        items: [{ id: 'a', typeKey: 0, text: 'Hi', fitHeight: 'clip',
          box: { left: '0 in', top: '0 in', width: '2 in', height: '0.2 in' } }],
      }],
    });
    expect(doc.bands[0].items[0].overflow).toBe('Clip');
    expect(doc.bands[0].items[0].fitHeight).toBeUndefined();
  });
});

describe('layoutRows / paintsMeasured', () => {
  it('does not show Fit height', () => {
    const rows = layoutRows({ typeKey: 0, wrap: 'nowrap', valign: 'top' });
    expect(rows.map((r) => r.label)).toEqual(['Wrap', 'V. Align', 'Line height']);
  });

  it('paints measured text for Ellipsis', () => {
    expect(paintsMeasured({ typeKey: 0, overflow: 'Ellipsis' })).toBe(true);
    expect(paintsMeasured({ typeKey: 0, overflow: 'Clip' })).toBe(false);
  });
});

describe('ellipsis and shrink layout', () => {
  it('ellipsisLine shortens to the box width', () => {
    const wide = 'ABCDEFGHIJKLMNOPQRSTUVWXYZ';
    const out = ellipsisLine(wide, 40, 12, 'Normal', 'Helvetica');
    expect(out.endsWith('…')).toBe(true);
    expect(measurePt(out, 12, 'Normal', 'Helvetica')).toBeLessThanOrEqual(40 + 1e-6);
  });

  it('layoutBand Grow raises used height', () => {
    const doc = { font: { family: 'Helvetica', size: '12 pt', style: 'Normal' } };
    const band = { height: '0.2 in' };
    const item = {
      id: 'g1', typeKey: 3, overflow: 'Grow', wrap: 'pre-wrap',
      text: 'one\ntwo\nthree\nfour\nfive',
      box: { left: '0 in', top: '0 in', width: '2 in', height: '0.2 in' },
    };
    const laid = layoutBand(doc, band, [{ item, text: item.text }]);
    expect(laid.reflow).toBe(true);
    expect(laid.items[0].heightPt).toBeGreaterThan(0.2 * 72 - 1);
  });

  it('layoutBand Shrink returns a smaller font when needed', () => {
    const doc = { font: { family: 'Helvetica', size: '24 pt', style: 'Normal' } };
    const band = { height: '0.25 in' };
    const item = {
      id: 's1', typeKey: 0, overflow: 'Shrink', wrap: 'nowrap', minFontSize: '6 pt',
      text: 'ABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZ',
      box: { left: '0 in', top: '0 in', width: '1 in', height: '0.25 in' },
      font: { family: 'Helvetica', size: '24 pt', style: 'Normal' },
    };
    const laid = layoutBand(doc, band, [{ item, text: item.text }]);
    const size = Number(String(laid.items[0].font.size).replace(/[^0-9.]/g, ''));
    expect(size).toBeLessThan(24);
    expect(size).toBeGreaterThanOrEqual(6);
    // Long nowrap string never fits 1 in even at 6 pt → floors to Min Font Size.
    expect(size).toBeCloseTo(6, 0);
  });

  it('resolveOverflow prefers overflow over fitHeight', () => {
    const seen = {};
    expect(resolveOverflow({ typeKey: 0, overflow: 'Fit', fitHeight: 'clip' }, seen)).toBe('Fit');
  });
});
