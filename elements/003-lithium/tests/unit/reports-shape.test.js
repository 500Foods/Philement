import { readFileSync } from 'node:fs';
import { resolve } from 'node:path';
import { describe, expect, it, afterEach } from 'vitest';
import { addItem, createBlank, reorderItem, stepItem } from '../../src/managers/reports/model/document.js';
import { paintBandItems } from '../../src/managers/reports/items.js';
import { applyProperty, propertyRows } from '../../src/managers/reports/report-properties.js';
import { outlineEntries } from '../../src/managers/reports/reports-panel.js';
import { previewSrcdoc } from '../../src/managers/reports/reports-preview.js';
import {
  SEEDED_PATHS, defaultShapeFields, setShapeCatalog, shapeAttributeKeys, shapeDrawing, shapeKindOptions, shapeSvg,
  withShapeSections,
} from '../../src/managers/reports/reports-shape.js';

const fixture = JSON.parse(readFileSync(
  resolve(process.cwd(), 'tests/fixtures/reporting/shapes.definition.json'),
  'utf8',
));
const html = readFileSync(resolve(process.cwd(), 'src/managers/reports/reports.html'), 'utf8');

function box(width, height) {
  return { left: '0 in', top: '0 in', width, height };
}

function shapeItem(kind, extra = {}) {
  return { id: 's', typeKey: 25, kind, box: box('1 in', '0.6 in'), ...extra };
}

afterEach(() => setShapeCatalog(null));

describe('shape geometry', () => {
  it('keeps the point-up polygon commands', () => {
    expect(SEEDED_PATHS.pentagon).toEqual([
      ['M', 0.5, 0], ['L', 1, 0.381966], ['L', 0.809017, 1], ['L', 0.190983, 1], ['L', 0, 0.381966], ['Z'],
    ]);
    expect(SEEDED_PATHS.hexagon[0]).toEqual(['M', 0.5, 0]);
    expect(SEEDED_PATHS.star[1]).toEqual(['L', 0.618034, 0.381966]);
    expect(SEEDED_PATHS.x).toEqual([['M', 0, 0], ['L', 1, 1], ['M', 1, 0], ['L', 0, 1]]);
    expect(SEEDED_PATHS.trapezoid[0]).toEqual(['M', 0.25, 0]);
  });

  it('draws a rectangle, a cubic roundrect, a bottom line, and a rotated hexagon', () => {
    const rect = shapeSvg(shapeItem('rectangle', { fillPaint: { kind: 'solid', color: '#d3d3d3' } }));
    expect(rect.html).toContain('fill="#d3d3d3"');
    expect(rect.html).toContain('stroke="#000000"');
    expect(rect.html).toContain('M 0 0');
    expect(rect.html).toContain('Z');
    const round = shapeDrawing(shapeItem('roundrect', {
      corners: { topLeft: '0.2 in', topRight: '0.05 in', bottomRight: '0.15 in', bottomLeft: '0 in' },
    }));
    expect(round.d).toContain('C ');
    const line = shapeDrawing(shapeItem('line', { side: 'bottom', box: box('1 in', '0.25 in') }));
    expect(line.d).toBe('M 0 24 L 96 24');
    expect(line.fill).toBe('none');
    const hex = shapeDrawing(shapeItem('hexagon', { rotation: 30, box: box('0.8 in', '0.8 in') }));
    expect(hex.transform).toBe('rotate(30 38.4 38.4)');
    expect(hex.d).toContain('Z');
  });

  it('clamps a corner so it cannot swallow the next one', () => {
    const drawn = shapeDrawing(shapeItem('roundrect', {
      box: box('10 px', '10 px'),
      corners: { topLeft: '8 px', topRight: '8 px', bottomRight: '8 px', bottomLeft: '8 px' },
    }));
    expect(drawn.d.startsWith('M 5 0')).toBe(true);
  });

  it('uses the defaults when stroke and fill are omitted, and draws no fill for a line', () => {
    const drawn = shapeDrawing({ kind: 'rectangle', box: box('1 in', '1 in') });
    expect(drawn.fill).toBe('#d3d3d3');
    expect(drawn.stroke).toBe('#000000');
    expect(drawn.sw).toBeCloseTo(96 / 72, 6);
    const line = shapeDrawing({ kind: 'line', box: box('1 in', '0.25 in') });
    expect(line.fill).toBe('none');
    expect(line.d).toBe('M 0 24 L 96 24');
  });

  it('dashes at four stroke widths and dots with a round cap', () => {
    const dashed = shapeDrawing(shapeItem('rectangle', { strokeStyle: 'dashed' }));
    expect(dashed.dash).toBe('5.3333 2.6667');
    expect(dashed.linecap).toBe('butt');
    const dotted = shapeDrawing(shapeItem('rectangle', { strokeStyle: 'dotted' }));
    expect(dotted.dash).toBe('1.3333 2.6667');
    expect(dotted.linecap).toBe('round');
  });

  it('draws no fill for an unknown fill style and keeps a bad color on the default', () => {
    const grid = shapeDrawing(shapeItem('rectangle', { fillStyle: 'plaid' }));
    expect(grid.fill).toBe('none');
    expect(grid.marks).toBeNull();
    const color = shapeDrawing(shapeItem('rectangle', { strokeColor: 'black' }));
    expect(color.stroke).toBe('#000000');
    expect(color.warn).toBeUndefined();
  });

  it('warns and skips a bad command, and draws a user path from a one-element collection', () => {
    setShapeCatalog([{
      key_idx: 10, code: 'bad', value_txt: 'Bad',
      collection: [{ closed: true, commands: [['Q', 0, 0]] }],
    }]);
    expect(shapeDrawing(shapeItem('bad')).warn).toMatch(/bad command/);
    setShapeCatalog([{
      key_idx: 10, code: 'triangle', value_txt: 'Triangle',
      collection: JSON.stringify([{ closed: true, commands: [['M', 0.5, 0], ['L', 1, 1], ['L', 0, 1], ['Z']] }]),
    }]);
    const drawn = shapeDrawing(shapeItem('triangle'));
    expect(drawn.warn).toBeUndefined();
    expect(drawn.d).toContain('Z');
    const options = shapeKindOptions();
    expect(options.filter((entry) => entry.code === 'rectangle')).toHaveLength(1);
    expect(options.some((entry) => entry.code === 'triangle')).toBe(true);
  });

  it('keeps built-in geometry when a lookup row collides', () => {
    setShapeCatalog([{
      key_idx: 1, code: 'rectangle', value_txt: 'Box',
      collection: [{ commands: [['L', 2, 2]] }],
    }]);
    const drawn = shapeDrawing(shapeItem('rectangle'));
    expect(drawn.warn).toBeUndefined();
    expect(drawn.d).toContain('M 0 0');
  });
});

describe('shape items', () => {
  it('defaults a new shape to a rectangle and leaves omitted stroke keys omitted', () => {
    const doc = addItem(createBlank(), 'd1', { typeKey: 25 });
    const item = doc.bands.find((band) => band.id === 'd1').items[0];
    expect(item.kind).toBe('rectangle');
    expect(item.strokeWidth).toBeUndefined();
    expect(item.rotation).toBeUndefined();
    const line = addItem(createBlank(), 'd1', { typeKey: 25, kind: 'line' });
    expect(line.bands.find((band) => band.id === 'd1').items[0].side).toBeUndefined();
    expect(defaultShapeFields('line')).toMatchObject({ side: 'bottom', fillStyle: 'none', rotation: 0 });
    expect(defaultShapeFields('roundrect').corners.topLeft).toBe('0.125 in');
    expect(() => defaultShapeFields('nope')).toThrow(/not in the list/);
  });

  it('reorders by taking the target index, and steps stop at the ends', () => {
    let doc = createBlank();
    doc = addItem(doc, 'd1', { typeKey: 0, id: 'A', text: 'A' });
    doc = addItem(doc, 'd1', { typeKey: 0, id: 'B', text: 'B' });
    doc = addItem(doc, 'd1', { typeKey: 0, id: 'C', text: 'C' });
    const ids = (next) => next.bands.find((band) => band.id === 'd1').items.map((item) => item.id);
    expect(ids(reorderItem(doc, 'd1', 'A', 1))).toEqual(['B', 'A', 'C']);
    expect(ids(reorderItem(doc, 'd1', 'A', 2))).toEqual(['B', 'C', 'A']);
    expect(ids(reorderItem(doc, 'd1', 'C', 0))).toEqual(['C', 'A', 'B']);
    expect(ids(reorderItem(doc, 'd1', 'C', 1))).toEqual(['A', 'C', 'B']);
    expect(reorderItem(doc, 'd1', 'B', 1)).toBe(doc);
    expect(stepItem(doc, 'd1', 'A', -1)).toBe(doc);
    expect(ids(stepItem(doc, 'd1', 'A', 1))).toEqual(['B', 'A', 'C']);
    expect(stepItem(doc, 'd1', 'C', 1)).toBe(doc);
  });

  it('paints the later shape on top and selects by the box', () => {
    const band = fixture.bands.find((entry) => entry.id === 'detail');
    const body = document.createElement('div');
    paintBandItems(body, band, fixture);
    const items = [...body.querySelectorAll('.reports-item')];
    expect(items.map((el) => el.dataset.itemId)).toEqual(['rect', 'round', 'rule', 'hex', 'back', 'front']);
    const fills = [...body.querySelectorAll('path')].map((path) => path.getAttribute('fill'));
    expect(fills.indexOf('#112233')).toBeLessThan(fills.indexOf('#abcdef'));
    expect(body.querySelector('[data-item-id="hex"] path').getAttribute('transform')).toContain('rotate(30');
    expect(body.querySelector('[data-item-id="round"] path').getAttribute('d')).toContain('C ');
    expect(body.querySelector('[data-item-id="rule"] path').getAttribute('fill')).toBe('none');
    expect(items[0].querySelector('.reports-item-corner')).not.toBeNull();
    expect(items[0].querySelector('svg').getAttribute('style')).toBeNull();
  });

  it('shows the shape rows and hides side and corners until the kind needs them', () => {
    const doc = addItem(createBlank(), 'd1', { typeKey: 25, id: 's1', ...defaultShapeFields('rectangle') });
    const sel = { bandId: 'd1', itemId: 's1' };
    const lookups = {
      objectTypes: [{ key_idx: 25, collection: JSON.stringify({ attributes: [0, 1, 2, 3, 4, 14, 24, 29] }) }],
    };
    const labels = propertyRows(doc, sel, lookups).map((row) => row.label);
    expect(labels).toContain('Shape');
    expect(labels).not.toContain('Align');
    expect(labels).not.toContain('Side');
    expect(Object.hasOwn(doc.bands[1].items[0], 'align')).toBe(false);
    expect(labels).not.toContain('Corner Top Left');
    expect(labels).not.toContain('Key 41');
    expect(propertyRows(doc, sel, lookups).some((row) => row.key === 24)).toBe(false);
    const shape = propertyRows(doc, sel, lookups).find((row) => row.key === 41);
    expect(shape.list[1]).toEqual({ value: 'rectangle', label: 'Rectangle' });
    const line = applyProperty(doc, sel, '41', 'line');
    const lined = line.bands[1].items[0];
    expect(lined.kind).toBe('line');
    expect(lined.side).toBe('bottom');
    expect(lined.fillStyle).toBe('solid');
    expect(lined.strokeWidth).toBe('1 pt');
    const bare = applyProperty(addItem(createBlank(), 'd1', { typeKey: 25, id: 's1' }), sel, '41', 'line');
    expect(bare.bands[1].items[0].fillStyle).toBe('none');
    expect(propertyRows(line, sel, lookups).some((row) => row.key === 42)).toBe(true);
    const round = applyProperty(doc, sel, '41', 'roundrect');
    expect(round.bands[1].items[0].corners.bottomLeft).toBe('0.125 in');
    expect(propertyRows(round, sel, lookups).some((row) => row.key === 51)).toBe(true);
    const lookedUp = propertyRows(doc, sel, {
      ...lookups,
      attributes: [{
        key_idx: 43,
        value_txt: 'Stroke Width',
        collection: JSON.stringify({ Group: 'Object', section: 4, Label: 'Stroke Width', Editor: 'Number' }),
      }],
    });
    expect(lookedUp.find((row) => row.key === 43)).toMatchObject({ section: 10, group: 'Stroke' });
    expect(lookedUp.find((row) => row.key === 46)).toMatchObject({ section: 11, group: 'Fill' });
    expect(lookedUp.find((row) => row.key === 41)).toMatchObject({ section: 4, group: 'Object' });
    expect(propertyRows(round, sel, lookups).find((row) => row.key === 51)).toMatchObject({ section: 12, group: 'Corners' });
    const sections = withShapeSections([
      { id: 4, label: 'Object', sortSeq: 4 },
      { id: 10, label: 'Stroke', sortSeq: 10, icon: '<fa fa-pen></fa>' },
    ]);
    expect(sections.find((section) => section.id === 10).sortSeq).toBe(4.2);
    expect(sections.find((section) => section.id === 11)).toMatchObject({ label: 'Fill', sortSeq: 4.4 });
    expect(sections.find((section) => section.id === 12).sortSeq).toBe(4.6);
    const painted = applyProperty(doc, sel, '47', '#ABCDEF');
    expect(painted.bands[1].items[0].fillPaint).toEqual({ kind: 'solid', color: '#abcdef' });
    expect(() => applyProperty(doc, sel, '47', 'red')).toThrow(/hex color/);
    expect(shapeAttributeKeys({ kind: 'rectangle' }, [0, 4, 24])).toEqual([0, 41, 43, 44, 45, 46, 47]);
    expect(outlineEntries(doc).map((entry) => entry.label)).toContain('s1 Rectangle');
    expect(propertyRows(doc, sel, lookups).find((row) => row.id === 'object-id')).toMatchObject({ value: 's1', readOnly: true });
    expect(propertyRows(doc, sel, lookups).at(-1).key).toBe(29);
  });

  it('keeps the shape svg in the preview sanitizer and still strips a script', () => {
    const src = previewSrcdoc(`${shapeSvg(shapeItem('rectangle')).html}<script>alert(1)</script>`);
    expect(src).toContain('<svg');
    expect(src).toContain('<path');
    expect(src.toLowerCase()).not.toContain('<script');
    const inside = previewSrcdoc(
      '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 10 10"><path d="M 0 0 L 10 10" fill="none" stroke="#000000" onclick="alert(1)"/><script>alert(1)</script></svg>',
    );
    expect(inside).toContain('<svg');
    expect(inside).toContain('<path');
    expect(inside.toLowerCase()).not.toContain('<script');
    expect(inside.toLowerCase()).not.toContain('onclick');
    expect(inside).not.toContain('alert(1)');
    const sized = previewSrcdoc(
      '<svg xmlns="http://www.w3.org/2000/svg" width="100%" height="100%" overflow="visible" viewBox="0 0 96 24" preserveAspectRatio="none"><path d="M 0 24 L 96 24" fill="none" stroke="#000000"/></svg>',
    );
    expect(sized).toContain('width="100%"');
    expect(sized).toContain('height="100%"');
    expect(sized).toContain('overflow="visible"');
    expect(sized).toContain('d="M 0 24 L 96 24"');
  });

  it('does not put the hidden shape types on the toolbar', () => {
    for (const key of [26, 33, 39, 53]) {
      expect(html).not.toContain(`data-type-key="${key}"`);
    }
    expect(html).not.toContain('reports-shape-kind');
    const shapeLine = html.split('\n').find((line) => line.includes('id="reports-shape-btn"'));
    expect(shapeLine).toContain('draggable="true"');
    expect(shapeLine).toContain('data-reports-category="static"');
    expect(html).toContain('id="reports-raise-btn"');
    expect(html).toContain('fa-bring-forward');
    expect(html).toContain('fa-send-backward');
    expect(html).not.toContain('fa-arrow-up"></fa>\n        <span>Raise');
  });
});

describe('notes editor', () => {
  it('keeps light text on a transparent field when the cell colour is black', () => {
    document.documentElement.style.setProperty('--text-primary', 'rgb(224, 224, 224)');
    const css = readFileSync(resolve(process.cwd(), 'src/managers/reports/reports-shape.css'), 'utf8');
    const style = document.createElement('style');
    style.textContent = css;
    document.head.append(style);
    const root = document.createElement('div');
    root.className = 'lithium-properties';
    const editing = document.createElement('div');
    editing.className = 'tabulator-cell tabulator-editing';
    editing.style.color = 'rgb(0, 0, 0)';
    editing.style.backgroundColor = 'rgb(26, 26, 26)';
    const area = document.createElement('textarea');
    area.className = 'lithium-properties-memo';
    editing.append(area);
    root.append(editing);
    document.body.append(root);
    const computed = getComputedStyle(area);
    expect(computed.color).toBe('rgb(224, 224, 224)');
    expect(computed.backgroundColor).toBe('transparent');
    expect(computed.borderTopStyle).toBe('none');
    expect(computed.whiteSpace).toBe('pre-wrap');
    root.remove();
    style.remove();
    document.documentElement.style.removeProperty('--text-primary');
  });
});
