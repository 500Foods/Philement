import { readFileSync } from 'node:fs';
import { resolve } from 'node:path';
import { describe, expect, it } from 'vitest';
import { addItem, createBlank } from '../../src/managers/reports/model/document.js';
import { applyProperty, propertyRows } from '../../src/managers/reports/report-properties.js';
import { previewSrcdoc } from '../../src/managers/reports/reports-preview.js';
import { defaultShapeFields, shapeAttributeKeys, shapeDrawing, shapeSvg } from '../../src/managers/reports/reports-shape.js';
import { fnv1a, hatchPath, linearEnds, mul32, noiseDots } from '../../src/managers/reports/reports-shape-fill.js';

const fixture = JSON.parse(readFileSync(
  resolve(process.cwd(), 'tests/fixtures/reporting/shape-fill.definition.json'),
  'utf8',
));

function box(width, height) {
  return { left: '0 in', top: '0 in', width, height };
}

function shapeItem(kind, extra = {}) {
  return { id: 's', typeKey: 25, kind, box: box('1 in', '0.6 in'), ...extra };
}

const lookups = {
  objectTypes: [{ key_idx: 25, collection: JSON.stringify({ attributes: [0, 1, 2, 3, 4, 14, 24, 29] }) }],
};

describe('shape fill', () => {
  it('multiplies the low 32 bits the way FNV and the noise seed need', () => {
    const oracle = (a, b) => Number((BigInt(a) * BigInt(b)) & 0xffffffffn);
    expect(mul32(2166136261, 16777619)).toBe(oracle(2166136261, 16777619));
    expect(mul32(1664525, 3826008364)).toBe(oracle(1664525, 3826008364));
    expect(fnv1a('')).toBe(2166136261);
    expect(fnv1a('speckle')).toBe(150473780);
    expect(fnv1a('hatch')).toBe(1592057985);
  });

  it('runs a linear gradient left to right, and a clockwise quarter turn downward', () => {
    expect(linearEnds(96, 57.6, 0)).toMatchObject({ x1: 0, y1: 28.8, x2: 96, y2: 28.8 });
    const down = linearEnds(96, 57.6, 90);
    expect(down.x1).toBeCloseTo(48, 6);
    expect(down.x2).toBeCloseTo(48, 6);
    expect(down.y1).toBeCloseTo(0, 6);
    expect(down.y2).toBeCloseTo(57.6, 6);
  });

  it('hatches down to the right on the report grid spacing', () => {
    const path = hatchPath(96, 57.6, 12);
    expect(path.startsWith('M ')).toBe(true);
    const segment = /M ([0-9.]+) ([0-9.]+) L ([0-9.]+) ([0-9.]+)/.exec(path);
    expect(Number(segment[3])).toBeGreaterThan(Number(segment[1]));
    expect(Number(segment[4])).toBeGreaterThan(Number(segment[2]));
    const drawn = shapeDrawing(shapeItem('rectangle', { fillStyle: 'hatched' }), 24);
    expect(drawn.marks.d).not.toBe(path);
  });

  it('draws a grid with a gradient and ignores fill on a line', () => {
    const drawn = shapeDrawing(shapeItem('rectangle', {
      fillStyle: 'grid',
      fillPaint: { kind: 'linear', angle: 0, stops: [{ at: 0, color: '#112233' }, { at: 1, color: '#abcdef' }] },
    }));
    expect(drawn.fill).toBe('none');
    expect(drawn.marks.d).toContain('M 12 0 L 12 57.6');
    expect(drawn.gradient.stops.map((stop) => stop.color)).toEqual(['#112233', '#abcdef']);
    const line = shapeDrawing(shapeItem('line', { fillStyle: 'hatched', fillPaint: { kind: 'linear', angle: 0, stops: [{ at: 0, color: '#112233' }, { at: 1, color: '#abcdef' }] } }));
    expect(line.fill).toBe('none');
    expect(line.marks).toBeNull();
    expect(line.gradient).toBeNull();
  });

  it('repeats the noise field for the same id', () => {
    const first = noiseDots('speckle', 96, 57.6, 12);
    const second = noiseDots('speckle', 96, 57.6, 12);
    expect(second).toEqual(first);
    expect(first.length).toBeGreaterThan(1);
    expect(noiseDots('other', 96, 57.6, 12)).not.toEqual(first);
  });

  it('draws the hatched gradient fixture and the same noise ellipse twice', () => {
    const hatch = fixture.bands[0].items[0];
    const speckle = fixture.bands[0].items[1];
    const html = shapeSvg(hatch, fixture).html;
    expect(html).toContain('<linearGradient');
    expect(html).toContain('stop-color="#1d4e89"');
    expect(html).toContain('stop-color="#f0c808"');
    expect(html).toContain('x1="0"');
    expect(html).toContain('x2="96"');
    expect(html).toContain('stroke="url(#reports-fill-hatch)"');
    const again = shapeSvg(speckle, fixture).html;
    expect(again).toBe(shapeSvg(speckle, fixture).html);
    expect(again).toContain('<circle');
    expect(again).toContain('fill="#2266aa"');
    expect(again.match(/<circle /g).length).toBeGreaterThan(1);
    const src = previewSrcdoc(`${html}${again}<script>alert(1)</script>`);
    expect(src).toContain('<linearGradient');
    expect(src).toContain('stop-color="#1d4e89"');
    expect(src).toContain('<circle');
    expect(src.toLowerCase()).not.toContain('<script');
    expect(src).not.toContain('alert(1)');
    const hostile = previewSrcdoc(
      '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 10 10"><path d="M 0 0 L 10 10" fill="url(javascript:alert(1))" stroke="#000000"/><circle cx="1" cy="1" r="1" fill="url(http://evil.example/a)"/></svg>',
    );
    expect(hostile).not.toContain('javascript');
    expect(hostile).not.toContain('http://evil');
  });

  it('edits the paint without a new lookup key and keeps a gradient when the color row is set', () => {
    let doc = addItem(createBlank(), 'd1', { typeKey: 25, id: 's1', ...defaultShapeFields('rectangle') });
    const sel = { bandId: 'd1', itemId: 's1' };
    const labels = () => propertyRows(doc, sel, lookups).map((row) => row.label);
    expect(labels()).toContain('Paint');
    expect(labels()).not.toContain('Angle');
    expect(propertyRows(doc, sel, lookups).find((row) => row.key === 46).list[2]).toEqual({ value: 'grid', label: 'Grid' });
    doc = applyProperty(doc, sel, 'fill-paint', 'linear');
    const paint = doc.bands[1].items[0].fillPaint;
    expect(paint.kind).toBe('linear');
    expect(paint.angle).toBe(0);
    expect(paint.stops).toEqual([{ at: 0, color: '#d3d3d3' }, { at: 1, color: '#d3d3d3' }]);
    expect(labels()).toEqual(expect.arrayContaining(['Angle', 'Stops']));
    expect(propertyRows(doc, sel, lookups).some((row) => row.key === 47)).toBe(false);
    doc = applyProperty(doc, sel, 'fill-stops', '0 #1d4e89, 1 #f0c808');
    doc = applyProperty(doc, sel, '47', '#abcdef');
    expect(doc.bands[1].items[0].fillPaint.stops[0].color).toBe('#abcdef');
    expect(doc.bands[1].items[0].fillPaint.stops[1].color).toBe('#f0c808');
    expect(doc.bands[1].items[0].fillPaint.kind).toBe('linear');
    expect(shapeAttributeKeys(doc.bands[1].items[0], [0])).not.toContain(47);
    doc = applyProperty(doc, sel, 'fill-paint', 'solid');
    expect(doc.bands[1].items[0].fillPaint).toEqual({ kind: 'solid', color: '#abcdef' });
    expect(() => applyProperty(doc, sel, '46', 'plaid')).toThrow(/Fill style/);
  });
});
