import { describe, expect, it } from 'vitest';
import { bandFrames, dragBandHeight } from '../../src/managers/reports/bands.js';
import { bandHeight, createBlank } from '../../src/managers/reports/model/document.js';
import { toCssPx } from '../../src/managers/reports/model/units.js';
import { bandRoleName } from '../../src/managers/reports/reports-file.js';

describe('report bands', () => {
  it('names the v1 band roles', () => {
    expect(bandRoleName(13)).toBe('Page header');
    expect(bandRoleName(15)).toBe('Detail');
    expect(bandRoleName(17)).toBe('Page footer');
    expect(bandRoleName(99)).toBe('Type 99');
    expect(bandRoleName(undefined)).toBe('Band');
  });

  it('uses the stored height and the default when the key is omitted', () => {
    expect(bandHeight({ id: 'ph', typeKey: 13 })).toBe('0.4 in');
    expect(bandHeight({ id: 'detail', typeKey: 15 })).toBe('0.25 in');
    expect(bandHeight({ id: 'gh', typeKey: 14, height: '1 in' })).toBe('1 in');
    expect(() => bandHeight({ id: 'x', typeKey: 99 })).toThrow(/needs a height/);
  });

  it('frames bands in array order and refuses a bad length', () => {
    const blank = bandFrames(createBlank());
    expect(blank.map((frame) => frame.id)).toEqual(['ph', 'detail', 'pf']);
    expect(blank.map((frame) => frame.role)).toEqual(['Page header', 'Detail', 'Page footer']);
    expect(blank.map((frame) => frame.heightPx)).toEqual([
      toCssPx('0.4 in'),
      toCssPx('0.25 in'),
      toCssPx('0.4 in'),
    ]);
    expect(bandFrames({ format: 1 })).toEqual([]);

    const bad = bandFrames({
      format: 1,
      bands: [{ id: 'detail', typeKey: 15, height: '4' }],
    });
    expect(bad[0].error).toMatch(/needs a unit/);
    expect(bad[0].heightPx).toBeUndefined();
    expect(bandFrames({ bands: [null] })[0].error).toMatch(/must be an object/);
  });

  it('keeps a dragged band at least 0.15 in', () => {
    const start = toCssPx('0.25 in');
    expect(dragBandHeight(start, 96)).toBe(start + 96);
    expect(dragBandHeight(start, -1000)).toBe(toCssPx('0.15 in'));
    expect(dragBandHeight(start, Number.NaN)).toBe(toCssPx('0.15 in'));
  });
});
