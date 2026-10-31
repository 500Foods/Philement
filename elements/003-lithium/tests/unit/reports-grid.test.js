import { afterEach, describe, expect, it, vi } from 'vitest';
import {
  gridMarks,
  gridSpec,
  paintBandGrid,
  redrawBandGrid,
  watchBandGrids,
} from '../../src/managers/reports/grid.js';
import { createBlank } from '../../src/managers/reports/model/document.js';
import { toCssPx } from '../../src/managers/reports/model/units.js';

afterEach(() => {
  document.body.replaceChildren();
  vi.unstubAllGlobals();
});

describe('grid spec', () => {
  it('uses dots at 0.125 in when the grid is written or omitted', () => {
    const written = gridSpec(createBlank());
    const omitted = gridSpec({ format: 1 });
    expect(written).toMatchObject({
      visible: true,
      style: 'dots',
      spacing: '0.125 in',
      spacingPx: 12,
      weight: 0.5,
      color: 'silver',
      background: 'white',
    });
    expect(written.spacingPx).toBe(toCssPx('0.125 in'));
    expect(omitted).toMatchObject({
      style: 'dots',
      spacing: '0.125 in',
      spacingPx: 12,
      weight: 0.5,
    });
  });

  it('follows the spacing unit and the five styles', () => {
    const metric = gridSpec({ grid: { style: 'grid', spacing: '2 mm' } });
    expect(metric.spacingPx).toBe(toCssPx('2 mm'));
    expect(gridMarks(metric)).toMatchObject({
      shape: 'lines',
      d: `M 0 0 V ${metric.spacingPx} M 0 0 H ${metric.spacingPx}`,
      dash: '',
    });

    const dots = gridMarks(gridSpec(createBlank()));
    expect(dots).toEqual({
      shape: 'circle',
      r: 1,
      step: 12,
      points: [[0, 0], [12, 0], [0, 12], [12, 12]],
    });
    expect(gridMarks(gridSpec({ grid: { weight: '2' } })).r).toBe(2);
    expect(gridMarks(gridSpec({ grid: { style: 'dotted' } })).dash).toBe('0.5 3');
    expect(gridMarks(gridSpec({ grid: { style: 'dashed' } })).dash).toBe('6 3');
    expect(gridMarks(gridSpec({ grid: { style: 'dotted' } })).d).toBe('M 0 0 V 12 M 0 0 H 12');
    expect(gridMarks(gridSpec({ grid: { style: 'none' } }))).toEqual({ shape: 'none' });
    expect(gridMarks(gridSpec({ grid: { visible: false, style: 'dots' } }))).toEqual({ shape: 'none' });
  });

  it('reports a bad spacing, a zero spacing, an unknown style, and a bad weight', () => {
    expect(gridSpec({ grid: { spacing: '4' } }).error).toMatch(/needs a unit/);
    expect(gridSpec({ grid: { spacing: '0 in' } }).error).toMatch(/greater than zero/);
    expect(gridSpec({ grid: { style: 'cross' } }).error).toBe('Unknown grid style "cross".');
    expect(gridSpec({ grid: { weight: 'thick' } }).error).toBe('Grid weight "thick" must be a number.');
  });
});

describe('grid paint', () => {
  it('paints a pattern on the band origin and a resize keeps that origin', () => {
    const body = document.createElement('div');
    document.body.append(body);
    const svg = paintBandGrid(body, gridSpec(createBlank()), 'reports-grid-0');
    const pattern = svg.querySelector('pattern');
    expect(pattern.getAttribute('x')).toBe('0');
    expect(pattern.getAttribute('y')).toBe('0');
    expect(pattern.getAttribute('width')).toBe('12');
    expect(pattern.getAttribute('patternUnits')).toBe('userSpaceOnUse');
    const circles = [...svg.querySelectorAll('circle')];
    expect(circles.map((circle) => circle.getAttribute('cx'))).toEqual(['0', '12', '0', '12']);
    expect(circles.map((circle) => circle.getAttribute('cy'))).toEqual(['0', '0', '12', '12']);
    expect(circles[0].getAttribute('r')).toBe('1');
    expect(circles[0].getAttribute('fill')).toBe('silver');
    expect(body.style.getPropertyValue('--reports-grid-background')).toBe('white');

    redrawBandGrid(svg, { width: 400, height: 80 });
    expect(svg.getAttribute('width')).toBe('400');
    expect(svg.getAttribute('height')).toBe('80');
    expect(svg.getAttribute('viewBox')).toBe('0 0 400 80');
    expect(pattern.getAttribute('x')).toBe('0');
    expect(pattern.getAttribute('y')).toBe('0');
    expect(pattern.getAttribute('width')).toBe('12');
  });

  it('draws lines for a solid grid and nothing for none', () => {
    const body = document.createElement('div');
    const svg = paintBandGrid(body, gridSpec({ grid: { style: 'grid' } }), 'reports-grid-1');
    const path = svg.querySelector('path');
    expect(path.getAttribute('d')).toBe('M 0 0 V 12 M 0 0 H 12');
    expect(path.getAttribute('stroke')).toBe('silver');
    expect(path.getAttribute('stroke-dasharray')).toBeNull();

    const empty = document.createElement('div');
    expect(paintBandGrid(empty, gridSpec({ grid: { style: 'none' } }), 'reports-grid-2')).toBeNull();
    expect(empty.querySelector('svg')).toBeNull();
    expect(empty.style.getPropertyValue('--reports-grid-background')).toBe('white');
  });

  it('redraws from ResizeObserver and disconnects', () => {
    const observed = [];
    let callback = null;
    let disconnected = false;
    class FakeObserver {
      constructor(fn) {
        callback = fn;
      }

      observe(el) {
        observed.push(el);
      }

      disconnect() {
        disconnected = true;
      }
    }
    vi.stubGlobal('ResizeObserver', FakeObserver);
    const root = document.createElement('div');
    const body = document.createElement('div');
    body.className = 'reports-band-body';
    root.append(body);
    document.body.append(root);
    const svg = paintBandGrid(body, gridSpec(createBlank()), 'reports-grid-0');
    const watch = watchBandGrids(root);
    expect(observed).toEqual([body]);
    callback([{ target: body, contentRect: { width: 220, height: 36 } }]);
    expect(svg.getAttribute('width')).toBe('220');
    expect(svg.querySelector('pattern').getAttribute('x')).toBe('0');
    expect(svg.querySelector('pattern').getAttribute('width')).toBe('12');
    watch.disconnect();
    expect(disconnected).toBe(true);
  });
});
