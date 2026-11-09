import { describe, expect, it } from 'vitest';
import {
  NONE_COLOR,
  backgroundColorFromFillPaint,
  isHexColor,
  isNoneColor,
  normalizeColorValue,
  normalizeDefinitionColours,
  normalizeShapeBackground,
  requireHexColor,
  shapeFillColor,
} from '../../src/managers/reports/reports-colours.js';

describe('reports-colours', () => {
  it('accepts hex colours and none', () => {
    expect(isHexColor('#abc')).toBe(true);
    expect(isHexColor('#AABBCC')).toBe(true);
    expect(isHexColor('red')).toBe(false);
    expect(requireHexColor('#Ff00Aa', 'Fore')).toBe('#ff00aa');
    expect(requireHexColor('', 'Fore')).toBe(NONE_COLOR);
    expect(requireHexColor('transparent', 'Fore')).toBe(NONE_COLOR);
    expect(normalizeColorValue('')).toBe(NONE_COLOR);
    expect(isNoneColor('none')).toBe(true);
    expect(() => requireHexColor('nope', 'Fore')).toThrow(/hex|none/i);
  });

  it('reads solid and gradient stop 0 from fillPaint', () => {
    expect(backgroundColorFromFillPaint({ kind: 'solid', color: '#112233' })).toBe('#112233');
    expect(backgroundColorFromFillPaint({
      kind: 'linear',
      stops: [{ at: 0, color: '#aabbcc' }, { at: 1, color: '#000000' }],
    })).toBe('#aabbcc');
    expect(backgroundColorFromFillPaint(null)).toBe(null);
  });

  it('normalizes shape backgroundColor once from fillPaint', () => {
    const item = { typeKey: 25, fillPaint: { kind: 'solid', color: '#d3d3d3' } };
    normalizeShapeBackground(item);
    expect(item.backgroundColor).toBe('#d3d3d3');
    item.fillPaint.color = '#ffffff';
    normalizeShapeBackground(item);
    expect(item.backgroundColor).toBe('#d3d3d3');
  });

  it('skips non-shapes and leaves fillStyle alone', () => {
    const text = { typeKey: 0, fillPaint: { kind: 'solid', color: '#111111' } };
    normalizeShapeBackground(text);
    expect(text.backgroundColor).toBeUndefined();
    const shape = {
      typeKey: 25,
      fillStyle: 'hatched',
      fillPaint: { kind: 'solid', color: '#abcdef' },
    };
    normalizeShapeBackground(shape);
    expect(shape.backgroundColor).toBe('#abcdef');
    expect(shape.fillStyle).toBe('hatched');
  });

  it('normalizeDefinitionColours walks every band item', () => {
    const doc = normalizeDefinitionColours({
      bands: [{
        items: [
          { typeKey: 0, text: 'x' },
          { typeKey: 25, fillPaint: { kind: 'solid', color: '#010203' } },
        ],
      }],
    });
    expect(doc.bands[0].items[0].backgroundColor).toBeUndefined();
    expect(doc.bands[0].items[1].backgroundColor).toBe('#010203');
  });

  it('shapeFillColor prefers backgroundColor then fillPaint; none yields null', () => {
    expect(shapeFillColor({ backgroundColor: '#aa0000', fillPaint: { color: '#00aa00' } }))
      .toBe('#aa0000');
    expect(shapeFillColor({ fillPaint: { color: '#00aa00' } })).toBe('#00aa00');
    expect(shapeFillColor({})).toBe('#d3d3d3');
    expect(shapeFillColor({ backgroundColor: 'none' })).toBe(null);
  });
});
