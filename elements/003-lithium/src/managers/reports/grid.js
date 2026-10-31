/**
 * Section grid for a report band.
 * The tile origin is the band body's top-left. A resize changes the
 * svg viewport and leaves that origin in place. An omitted grid uses
 * the Defaults table and is not written.
 */

import { toCssPx } from './model/units.js';

const SVG_NS = 'http://www.w3.org/2000/svg';
const GRID_STYLES = ['dots', 'grid', 'dotted', 'dashed', 'none'];
const DEFAULT_STYLE = 'dots';
const DEFAULT_SPACING = '0.125 in';
const DEFAULT_WEIGHT = 0.5;
const MIN_DOT_RADIUS = 1;
const DEFAULT_COLOR = 'silver';
const DEFAULT_BACKGROUND = 'white';

function written(value) {
  return typeof value === 'string' && value !== '';
}

function gridObject(doc) {
  const grid = doc && doc.grid;
  if (!grid || typeof grid !== 'object' || Array.isArray(grid)) return {};
  return grid;
}

function errorMessage(error, fallback) {
  return error instanceof Error && error.message ? error.message : fallback;
}

function weightOf(value) {
  if (value === undefined || value === null || value === '') return DEFAULT_WEIGHT;
  if (typeof value === 'number') {
    return Number.isFinite(value) && value >= 0 ? value : null;
  }
  if (typeof value !== 'string' || !/^[+-]?(?:\d+(?:\.\d+)?|\.\d+)$/.test(value.trim())) {
    return null;
  }
  const number = Number(value);
  return Number.isFinite(number) && number >= 0 ? number : null;
}

/**
 * Display values for the grid. An omitted key uses the Defaults table.
 * A length or style that cannot be used sets `error` and invents nothing.
 * @param {object} doc
 * @returns {object}
 */
export function gridSpec(doc) {
  const grid = gridObject(doc);
  const style = written(grid.style) ? grid.style : DEFAULT_STYLE;
  if (!GRID_STYLES.includes(style)) {
    return { error: `Unknown grid style "${style}".` };
  }
  const spacing = written(grid.spacing) ? grid.spacing : DEFAULT_SPACING;
  let spacingPx;
  try {
    spacingPx = toCssPx(spacing);
  } catch (error) {
    return { error: errorMessage(error, 'Grid spacing could not be measured.') };
  }
  if (!(spacingPx > 0)) return { error: 'Grid spacing must be greater than zero.' };
  const weight = weightOf(grid.weight);
  if (weight === null) return { error: `Grid weight "${grid.weight}" must be a number.` };
  return {
    visible: grid.visible !== false,
    style,
    spacing,
    spacingPx,
    weight,
    color: written(grid.color) ? grid.color : DEFAULT_COLOR,
    background: written(grid.background) ? grid.background : DEFAULT_BACKGROUND,
  };
}

/**
 * Painted dot radius. A stored weight of 0.5 is a hairline. A circle
 * of that radius, centered on the tile corner, is clipped to a
 * quarter of a pixel and does not show. The floor is 1 CSS pixel.
 * The stored weight is unchanged.
 * @param {number} weight
 * @returns {number}
 */
function dotRadius(weight) {
  return Math.max(weight, MIN_DOT_RADIUS);
}

/**
 * One tile. The lattice starts at the band origin.
 * A dot is drawn on each corner of the tile. The pattern clips each
 * circle, and the neighboring tiles complete the dot, so the mark
 * sits on the spacing point instead of inside the cell.
 * @param {object} spec
 * @returns {{ shape: string, step?: number, r?: number, points?: number[][], d?: string, dash?: string }}
 */
export function gridMarks(spec) {
  if (!spec || spec.error || spec.visible === false || spec.style === 'none') {
    return { shape: 'none' };
  }
  const step = spec.spacingPx;
  if (spec.style === 'dots') {
    return {
      shape: 'circle',
      r: dotRadius(spec.weight),
      step,
      points: [[0, 0], [step, 0], [0, step], [step, step]],
    };
  }
  let dash = '';
  if (spec.style === 'dotted') dash = `${spec.weight} ${step / 4}`;
  if (spec.style === 'dashed') dash = `${step / 2} ${step / 4}`;
  return {
    shape: 'lines',
    step,
    d: `M 0 0 V ${step} M 0 0 H ${step}`,
    dash,
  };
}

function svgEl(name) {
  return document.createElementNS(SVG_NS, name);
}

/**
 * Paint one band's pattern. Style `none` and a hidden grid draw no marks.
 * @param {HTMLElement} body
 * @param {object} spec
 * @param {string} patternId
 * @returns {SVGSVGElement|null}
 */
export function paintBandGrid(body, spec, patternId) {
  if (!body || !spec || spec.error) return null;
  if (spec.visible !== false) {
    body.style.setProperty('--reports-grid-background', spec.background);
  }
  const marks = gridMarks(spec);
  if (marks.shape === 'none') return null;
  const svg = svgEl('svg');
  svg.classList.add('reports-band-grid');
  svg.setAttribute('aria-hidden', 'true');
  const defs = svgEl('defs');
  const pattern = svgEl('pattern');
  pattern.setAttribute('id', patternId);
  pattern.setAttribute('x', '0');
  pattern.setAttribute('y', '0');
  pattern.setAttribute('width', String(marks.step));
  pattern.setAttribute('height', String(marks.step));
  pattern.setAttribute('patternUnits', 'userSpaceOnUse');
  if (marks.shape === 'circle') {
    for (const [cx, cy] of marks.points) {
      const circle = svgEl('circle');
      circle.setAttribute('cx', String(cx));
      circle.setAttribute('cy', String(cy));
      circle.setAttribute('r', String(marks.r));
      circle.setAttribute('fill', spec.color);
      pattern.append(circle);
    }
  } else {
    const path = svgEl('path');
    path.setAttribute('d', marks.d);
    path.setAttribute('fill', 'none');
    path.setAttribute('stroke', spec.color);
    path.setAttribute('stroke-width', String(spec.weight));
    if (marks.dash) {
      path.setAttribute('stroke-dasharray', marks.dash);
      path.setAttribute('stroke-linecap', 'round');
    }
    pattern.append(path);
  }
  defs.append(pattern);
  const rect = svgEl('rect');
  rect.classList.add('reports-grid-fill');
  rect.setAttribute('x', '0');
  rect.setAttribute('y', '0');
  rect.setAttribute('width', '100%');
  rect.setAttribute('height', '100%');
  rect.setAttribute('fill', `url(#${patternId})`);
  svg.append(defs, rect);
  body.prepend(svg);
  return svg;
}

/**
 * Size the svg to the band and keep the pattern origin at 0,0.
 * @param {Element} svg
 * @param {{ width: number, height: number }} rect
 */
export function redrawBandGrid(svg, rect) {
  if (!svg || !rect) return;
  const { width, height } = rect;
  if (!(width > 0) || !(height > 0)) return;
  svg.setAttribute('width', String(width));
  svg.setAttribute('height', String(height));
  svg.setAttribute('viewBox', `0 0 ${width} ${height}`);
  const fill = svg.querySelector('.reports-grid-fill');
  if (fill) {
    fill.setAttribute('width', String(width));
    fill.setAttribute('height', String(height));
  }
  const pattern = svg.querySelector('pattern');
  if (pattern) {
    pattern.setAttribute('x', '0');
    pattern.setAttribute('y', '0');
  }
}

/**
 * Redraw each band grid when its body changes size.
 * The caller disconnects this from cleanup and destroy.
 * @param {ParentNode} root
 * @returns {{ disconnect: () => void, error?: string }}
 */
export function watchBandGrids(root) {
  if (typeof ResizeObserver !== 'function') {
    return {
      error: 'ResizeObserver is missing.',
      disconnect() {},
    };
  }
  const observer = new ResizeObserver((entries) => {
    for (const entry of entries) {
      const svg = entry.target.querySelector('.reports-band-grid');
      redrawBandGrid(svg, entry.contentRect);
    }
  });
  if (root) {
    for (const body of root.querySelectorAll('.reports-band-body')) {
      observer.observe(body);
    }
  }
  return {
    disconnect() {
      observer.disconnect();
    },
  };
}
