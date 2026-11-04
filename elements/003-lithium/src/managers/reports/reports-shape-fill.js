/**
 * Shape fill (Phase 44b). The style and the paint are separate.
 * Grid, dots, and hatch use the report grid spacing. Hatch is one
 * diagonal, down to the right, and its stroke is 0.5 CSS pixels.
 * Dots sit on that lattice. Their radius is 1 CSS pixel, or a
 * quarter of the spacing when that is smaller. Noise is one dot
 * per cell of about one spacing on a side, placed by an LCG seeded
 * from FNV-1a of the item id. Its radius is 0.12 times the spacing,
 * at least 0.75 CSS pixels, and at most 30 percent of the cell.
 * None of those sizes is stored. A line draws no fill.
 * Linear angle 0 runs left to right and grows clockwise. Radial
 * runs from the center of the box to the farthest corner.
 */

import { toCssPx } from './model/units.js';
import { gridSpec } from './grid.js';

const HEX = /^#[0-9A-Fa-f]{6}$/;
const SVG_NS = 'http://www.w3.org/2000/svg';
const PATTERN_SW = 0.5;
const DEFAULT_COLOR = '#d3d3d3';

export const FILL_STYLES = new Set(['none', 'solid', 'grid', 'dots', 'hatched', 'noise']);
export const FILL_PAINT_ROW = 'fill-paint';
export const FILL_ANGLE_ROW = 'fill-angle';
export const FILL_STOPS_ROW = 'fill-stops';

const PAINT_KINDS = ['solid', 'linear', 'radial'];

/** Closed Fill Style cell. The stored value stays the code. */
export function fillStyleChoices() {
  return [
    { value: 'none', label: 'None' },
    { value: 'solid', label: 'Solid' },
    { value: 'grid', label: 'Grid' },
    { value: 'dots', label: 'Dots' },
    { value: 'hatched', label: 'Hatched' },
    { value: 'noise', label: 'Noise' },
  ];
}

/**
 * Spacing the fill uses. A hidden or broken design grid still falls
 * back to 0.125 in. The stored grid is not changed.
 * @param {object|null|undefined} doc
 * @returns {number}
 */
export function shapeSpacingPx(doc) {
  const spec = gridSpec(doc || {});
  if (spec && !spec.error && spec.spacingPx > 0) return spec.spacingPx;
  return toCssPx('0.125 in');
}

/** Same 4-decimal trim as the shape path formatter. */
export function shapeFmt(n) {
  const value = typeof n === 'number' && Number.isFinite(n) ? n : 0;
  const neg = value < 0;
  const rounded = Math.round(Math.abs(value) * 10000) / 10000;
  if (rounded === 0) return '0';
  const text = rounded.toFixed(4).replace(/0+$/, '').replace(/\.$/, '');
  return neg ? `-${text}` : text;
}

/** Low 32 bits of a product. Matches the Lua splitter in Reports.Render. */
export function mul32(a, b) {
  const aLo = a & 0xffff;
  const aHi = (a >>> 16) & 0xffff;
  const bLo = b & 0xffff;
  const bHi = (b >>> 16) & 0xffff;
  const p0 = aLo * bLo;
  const p1 = aLo * bHi + aHi * bLo;
  return (p0 + (p1 & 0xffff) * 65536) % 4294967296;
}

/** FNV-1a, 32 bits, over the UTF-8 bytes of the item id. */
export function fnv1a(text) {
  let hash = 2166136261;
  const bytes = new TextEncoder().encode(String(text ?? ''));
  for (const byte of bytes) {
    hash = mul32((hash ^ byte) >>> 0, 16777619);
  }
  return hash >>> 0;
}

function lcg(seed) {
  let state = seed >>> 0;
  if (state === 0) state = 1;
  return () => {
    state = (mul32(1664525, state) + 1013904223) % 4294967296;
    return state / 4294967296;
  };
}

export function shapeDomId(prefix, id) {
  const cleaned = String(id ?? '').replace(/[^A-Za-z0-9_-]/g, '');
  return `${prefix}${cleaned || 'item'}`;
}

function hexColor(value, fallback) {
  return typeof value === 'string' && HEX.test(value) ? value.toLowerCase() : fallback;
}

function validStops(stops) {
  if (!Array.isArray(stops)) return null;
  const out = [];
  for (const stop of stops) {
    if (!stop || typeof stop !== 'object') continue;
    const at = Number(stop.at);
    if (!Number.isFinite(at) || at < 0 || at > 1) continue;
    if (!HEX.test(stop.color || '')) continue;
    out.push({ at, color: stop.color.toLowerCase() });
  }
  return out.length >= 2 ? out : null;
}

/**
 * Paint the renderer can draw. A gradient with fewer than two hex
 * stops falls back to a solid color.
 * @param {object|null|undefined} fillPaint
 * @returns {{ color: string, gradient: object|null }}
 */
export function resolvePaint(fillPaint) {
  const paint = fillPaint && typeof fillPaint === 'object' && !Array.isArray(fillPaint) ? fillPaint : null;
  const color = hexColor(paint && paint.color, DEFAULT_COLOR);
  const kind = paint && typeof paint.kind === 'string' ? paint.kind : 'solid';
  if (kind !== 'linear' && kind !== 'radial') return { color, gradient: null };
  const stops = validStops(paint.stops);
  if (!stops) return { color, gradient: null };
  if (kind === 'radial') return { color, gradient: { kind: 'radial', stops } };
  const angle = Number(paint.angle);
  return { color, gradient: { kind: 'linear', angle: Number.isFinite(angle) ? angle : 0, stops } };
}

/** Stored kind for the panel. An unknown kind is shown as solid. */
export function storedPaintKind(item) {
  const paint = item && item.fillPaint;
  const kind = paint && typeof paint.kind === 'string' ? paint.kind : '';
  return PAINT_KINDS.includes(kind) ? kind : 'solid';
}

/**
 * Linear gradient across the box. Angle 0 is left to right.
 * Clockwise in SVG space, where y grows downward.
 * @param {number} w
 * @param {number} h
 * @param {number} angle
 * @returns {{ x1: number, y1: number, x2: number, y2: number }}
 */
export function linearEnds(w, h, angle) {
  const rad = (Number.isFinite(angle) ? angle : 0) * Math.PI / 180;
  const dx = Math.cos(rad);
  const dy = Math.sin(rad);
  const cx = w / 2;
  const cy = h / 2;
  const extent = Math.abs(dx) * w / 2 + Math.abs(dy) * h / 2;
  return {
    x1: cx - dx * extent,
    y1: cy - dy * extent,
    x2: cx + dx * extent,
    y2: cy + dy * extent,
  };
}

function gridPath(w, h, spacing) {
  const parts = [];
  const nx = Math.floor(w / spacing + 1e-6);
  const ny = Math.floor(h / spacing + 1e-6);
  for (let i = 0; i <= nx; i += 1) {
    const x = i * spacing;
    parts.push(`M ${shapeFmt(x)} 0 L ${shapeFmt(x)} ${shapeFmt(h)}`);
  }
  for (let j = 0; j <= ny; j += 1) {
    const y = j * spacing;
    parts.push(`M 0 ${shapeFmt(y)} L ${shapeFmt(w)} ${shapeFmt(y)}`);
  }
  return parts.join(' ');
}

/**
 * One diagonal, down to the right. The gap between lines, measured
 * perpendicular to them, is the report grid spacing. The family
 * passes through the shape's origin.
 * @param {number} w
 * @param {number} h
 * @param {number} spacing
 * @returns {string}
 */
export function hatchPath(w, h, spacing) {
  const step = spacing * Math.SQRT2;
  if (!(step > 0)) return '';
  const n0 = Math.ceil(-w / step - 1e-8);
  const n1 = Math.floor(h / step + 1e-8);
  const parts = [];
  for (let n = n0; n <= n1; n += 1) {
    const b = n * step;
    const points = [];
    const add = (x, y) => {
      if (points.some((p) => Math.abs(p.x - x) < 1e-6 && Math.abs(p.y - y) < 1e-6)) return;
      points.push({ x, y });
    };
    if (b >= -1e-6 && b <= h + 1e-6) add(0, Math.min(h, Math.max(0, b)));
    if (w + b >= -1e-6 && w + b <= h + 1e-6) add(w, Math.min(h, Math.max(0, w + b)));
    if (-b >= -1e-6 && -b <= w + 1e-6) add(Math.min(w, Math.max(0, -b)), 0);
    if (h - b >= -1e-6 && h - b <= w + 1e-6) add(Math.min(w, Math.max(0, h - b)), h);
    if (points.length !== 2) continue;
    const [p, q] = points[0].x < points[1].x - 1e-9
      || (Math.abs(points[0].x - points[1].x) <= 1e-9 && points[0].y <= points[1].y)
      ? [points[0], points[1]] : [points[1], points[0]];
    parts.push(`M ${shapeFmt(p.x)} ${shapeFmt(p.y)} L ${shapeFmt(q.x)} ${shapeFmt(q.y)}`);
  }
  return parts.join(' ');
}

function dotRadius(spacing) {
  return Math.min(1, spacing / 4);
}

function dotCircles(w, h, spacing) {
  const r = dotRadius(spacing);
  const circles = [];
  const nx = Math.floor(w / spacing + 1e-6);
  const ny = Math.floor(h / spacing + 1e-6);
  for (let j = 0; j <= ny; j += 1) {
    for (let i = 0; i <= nx; i += 1) {
      circles.push({ x: i * spacing, y: j * spacing, r });
    }
  }
  return circles;
}

function noiseRadius(spacing, cw, ch) {
  return Math.min(Math.max(spacing * 0.12, 0.75), cw * 0.3, ch * 0.3);
}

/**
 * Dot field for one id. The same id, box, and spacing return the
 * same circles. Row by row from the top, left to right.
 * @param {string} id
 * @param {number} w
 * @param {number} h
 * @param {number} spacing
 * @returns {{ x: number, y: number, r: number }[]}
 */
export function noiseDots(id, w, h, spacing) {
  const next = lcg(fnv1a(id));
  const nx = Math.max(1, Math.round(w / spacing));
  const ny = Math.max(1, Math.round(h / spacing));
  const cw = w / nx;
  const ch = h / ny;
  const r = noiseRadius(spacing, cw, ch);
  const circles = [];
  for (let j = 0; j < ny; j += 1) {
    for (let i = 0; i < nx; i += 1) {
      const rx = next();
      const ry = next();
      circles.push({
        x: (i + 0.2 + 0.6 * rx) * cw,
        y: (j + 0.2 + 0.6 * ry) * ch,
        r,
      });
    }
  }
  return circles;
}

function placedGradient(gradient, w, h) {
  if (!gradient) return null;
  if (gradient.kind === 'radial') {
    return {
      kind: 'radial',
      cx: w / 2,
      cy: h / 2,
      r: Math.hypot(w / 2, h / 2),
      stops: gradient.stops,
    };
  }
  return { kind: 'linear', ...linearEnds(w, h, gradient.angle), stops: gradient.stops };
}

/**
 * Fill for one shape. `marks` is null when the style is solid or none.
 * @param {object} item
 * @param {string} kind
 * @param {number} w
 * @param {number} h
 * @param {number} spacing
 * @returns {object}
 */
export function fillPlan(item, kind, w, h, spacing) {
  let style = item && item.fillStyle;
  if (kind === 'line') style = 'none';
  else if (style == null || style === '') style = 'solid';
  else if (!FILL_STYLES.has(style)) style = 'none';
  const paint = resolvePaint(item && item.fillPaint);
  const step = spacing > 0 ? spacing : toCssPx('0.125 in');
  if (style === 'none') return { style, color: paint.color, gradient: null, marks: null };
  const gradient = placedGradient(paint.gradient, w, h);
  if (style === 'solid') return { style, color: paint.color, gradient, marks: null };
  let marks;
  if (style === 'grid') marks = { kind: 'lines', d: gridPath(w, h, step), sw: PATTERN_SW };
  else if (style === 'hatched') marks = { kind: 'lines', d: hatchPath(w, h, step), sw: PATTERN_SW };
  else if (style === 'dots') marks = { kind: 'circles', circles: dotCircles(w, h, step) };
  else marks = { kind: 'circles', circles: noiseDots(item && item.id, w, h, step) };
  return { style, color: paint.color, gradient, marks };
}

function attrText(attrs) {
  return attrs.map(([name, value]) => `${name}="${value}"`).join(' ');
}

function transformAttr(draw) {
  return draw.transform ? [['transform', draw.transform]] : [];
}

function gradientTag(draw) {
  const g = draw.gradient;
  const attrs = g.kind === 'radial'
    ? [
      ['id', draw.paintId],
      ['gradientUnits', 'userSpaceOnUse'],
      ['cx', shapeFmt(g.cx)],
      ['cy', shapeFmt(g.cy)],
      ['r', shapeFmt(g.r)],
    ]
    : [
      ['id', draw.paintId],
      ['gradientUnits', 'userSpaceOnUse'],
      ['x1', shapeFmt(g.x1)],
      ['y1', shapeFmt(g.y1)],
      ['x2', shapeFmt(g.x2)],
      ['y2', shapeFmt(g.y2)],
    ];
  const stops = g.stops.map((stop) => `<stop offset="${shapeFmt(stop.at)}" stop-color="${stop.color}"/>`).join('');
  const tag = g.kind === 'radial' ? 'radialGradient' : 'linearGradient';
  return `<${tag} ${attrText(attrs)}>${stops}</${tag}>`;
}

/** SVG markup. Preview HTML uses the same commands from Reports.Render. */
export function shapeMarkup(draw) {
  const open = `<svg xmlns="http://www.w3.org/2000/svg" width="100%" height="100%" overflow="visible" viewBox="0 0 ${shapeFmt(draw.w)} ${shapeFmt(draw.h)}" preserveAspectRatio="none">`;
  const chunks = [open];
  if (draw.gradient || draw.marks) {
    chunks.push('<defs>');
    if (draw.gradient) chunks.push(gradientTag(draw));
    if (draw.marks) {
      chunks.push(`<clipPath id="${draw.clipId}"><path ${attrText([['d', draw.d], ...transformAttr(draw)])}/></clipPath>`);
    }
    chunks.push('</defs>');
  }
  if (draw.marks) {
    chunks.push(`<g clip-path="url(#${draw.clipId})">`);
    if (draw.marks.kind === 'lines') {
      chunks.push(`<path ${attrText([
        ['d', draw.marks.d],
        ['fill', 'none'],
        ['stroke', draw.paintRef],
        ['stroke-width', shapeFmt(draw.marks.sw)],
        ...transformAttr(draw),
      ])}/>`);
    } else {
      for (const circle of draw.marks.circles) {
        chunks.push(`<circle ${attrText([
          ['cx', shapeFmt(circle.x)],
          ['cy', shapeFmt(circle.y)],
          ['r', shapeFmt(circle.r)],
          ['fill', draw.paintRef],
          ...transformAttr(draw),
        ])}/>`);
      }
    }
    chunks.push('</g>');
  }
  const path = [
    ['d', draw.d],
    ['fill', draw.fill],
    ['stroke', draw.stroke],
  ];
  if (draw.stroke !== 'none') {
    path.push(['stroke-width', shapeFmt(draw.sw)], ['stroke-linejoin', 'miter'], ['stroke-linecap', draw.linecap]);
    if (draw.dash) path.push(['stroke-dasharray', draw.dash]);
  }
  path.push(...transformAttr(draw));
  chunks.push(`<path ${attrText(path)}/>`);
  chunks.push('</svg>');
  return chunks.join('');
}

function setAttrs(el, attrs) {
  for (const [name, value] of attrs) el.setAttribute(name, value);
}

/** Paint the same markup into the design surface. */
export function mountShape(el, draw) {
  const svg = document.createElementNS(SVG_NS, 'svg');
  setAttrs(svg, [
    ['width', '100%'],
    ['height', '100%'],
    ['overflow', 'visible'],
    ['viewBox', `0 0 ${shapeFmt(draw.w)} ${shapeFmt(draw.h)}`],
    ['preserveAspectRatio', 'none'],
  ]);
  if (draw.gradient || draw.marks) {
    const defs = document.createElementNS(SVG_NS, 'defs');
    if (draw.gradient) {
      const g = draw.gradient;
      const grad = document.createElementNS(SVG_NS, g.kind === 'radial' ? 'radialGradient' : 'linearGradient');
      const attrs = g.kind === 'radial'
        ? [['id', draw.paintId], ['gradientUnits', 'userSpaceOnUse'], ['cx', shapeFmt(g.cx)], ['cy', shapeFmt(g.cy)], ['r', shapeFmt(g.r)]]
        : [['id', draw.paintId], ['gradientUnits', 'userSpaceOnUse'], ['x1', shapeFmt(g.x1)], ['y1', shapeFmt(g.y1)], ['x2', shapeFmt(g.x2)], ['y2', shapeFmt(g.y2)]];
      setAttrs(grad, attrs);
      for (const stop of g.stops) {
        const node = document.createElementNS(SVG_NS, 'stop');
        setAttrs(node, [['offset', shapeFmt(stop.at)], ['stop-color', stop.color]]);
        grad.append(node);
      }
      defs.append(grad);
    }
    if (draw.marks) {
      const clip = document.createElementNS(SVG_NS, 'clipPath');
      clip.setAttribute('id', draw.clipId);
      const outline = document.createElementNS(SVG_NS, 'path');
      setAttrs(outline, [['d', draw.d], ...transformAttr(draw)]);
      clip.append(outline);
      defs.append(clip);
    }
    svg.append(defs);
  }
  if (draw.marks) {
    const group = document.createElementNS(SVG_NS, 'g');
    group.setAttribute('clip-path', `url(#${draw.clipId})`);
    if (draw.marks.kind === 'lines') {
      const mark = document.createElementNS(SVG_NS, 'path');
      setAttrs(mark, [
        ['d', draw.marks.d],
        ['fill', 'none'],
        ['stroke', draw.paintRef],
        ['stroke-width', shapeFmt(draw.marks.sw)],
        ...transformAttr(draw),
      ]);
      group.append(mark);
    } else {
      for (const circle of draw.marks.circles) {
        const node = document.createElementNS(SVG_NS, 'circle');
        setAttrs(node, [
          ['cx', shapeFmt(circle.x)],
          ['cy', shapeFmt(circle.y)],
          ['r', shapeFmt(circle.r)],
          ['fill', draw.paintRef],
          ...transformAttr(draw),
        ]);
        group.append(node);
      }
    }
    svg.append(group);
  }
  const path = document.createElementNS(SVG_NS, 'path');
  const attrs = [['d', draw.d], ['fill', draw.fill], ['stroke', draw.stroke]];
  if (draw.stroke !== 'none') {
    attrs.push(['stroke-width', shapeFmt(draw.sw)], ['stroke-linejoin', 'miter'], ['stroke-linecap', draw.linecap]);
    if (draw.dash) attrs.push(['stroke-dasharray', draw.dash]);
  }
  attrs.push(...transformAttr(draw));
  setAttrs(path, attrs);
  svg.append(path);
  el.append(svg);
}

function paintObject(item) {
  const paint = item.fillPaint;
  return paint && typeof paint === 'object' && !Array.isArray(paint) ? paint : {};
}

function currentColor(item) {
  const paint = paintObject(item);
  if (HEX.test(paint.color || '')) return paint.color.toLowerCase();
  const stops = validStops(paint.stops);
  return stops ? stops[0].color : DEFAULT_COLOR;
}

function formatAt(at) {
  const rounded = Math.round(at * 1000000) / 1000000;
  return String(rounded);
}

export function formatStops(stops) {
  return stops.map((stop) => `${formatAt(stop.at)} ${stop.color}`).join(', ');
}

export function parseStops(value) {
  const parts = String(value ?? '').split(/[\n,]+/).map((part) => part.trim()).filter(Boolean);
  const stops = [];
  for (const part of parts) {
    const match = /^(\d+(?:\.\d+)?)\s+(#[0-9A-Fa-f]{6})$/.exec(part);
    if (!match) throw new Error('A stop is a position from 0 to 1 and a hex color.');
    const at = Number(match[1]);
    if (at < 0 || at > 1) throw new Error('A stop is a position from 0 to 1 and a hex color.');
    stops.push({ at, color: match[2].toLowerCase() });
  }
  if (stops.length < 2) throw new Error('A gradient needs at least two stops.');
  return stops;
}

function ensureStops(item) {
  const stops = validStops(paintObject(item).stops);
  if (stops) return stops;
  const color = currentColor(item);
  return [{ at: 0, color }, { at: 1, color }];
}

/**
 * Panel rows for the paint. Color stays the lookup row and is hidden
 * while the paint is a gradient. These rows are not lookup keys.
 * @param {object|null} item
 * @returns {object[]}
 */
export function shapeFillRowList(item) {
  if (!item) return [];
  const paint = paintObject(item);
  const kind = storedPaintKind(item);
  const rows = [{
    id: FILL_PAINT_ROW,
    key: null,
    label: 'Paint',
    value: kind,
    omitted: typeof paint.kind !== 'string' || paint.kind === '',
    note: 'Solid, linear, or radial. The fill style uses this paint.',
    editor: 'List',
    list: [
      { value: 'solid', label: 'Solid' },
      { value: 'linear', label: 'Linear' },
      { value: 'radial', label: 'Radial' },
    ],
    section: 11,
    group: 'Fill',
    readOnly: false,
  }];
  if (kind === 'linear') {
    const angle = Number(paint.angle);
    const written = Number.isFinite(angle);
    rows.push({
      id: FILL_ANGLE_ROW,
      key: null,
      label: 'Angle',
      value: written ? String(angle) : '0',
      omitted: !written,
      note: 'Degrees. 0 runs left to right and grows clockwise.',
      editor: 'Text',
      list: null,
      section: 11,
      group: 'Fill',
      readOnly: false,
    });
  }
  if (kind === 'linear' || kind === 'radial') {
    const stops = validStops(paint.stops);
    rows.push({
      id: FILL_STOPS_ROW,
      key: null,
      label: 'Stops',
      value: stops ? formatStops(stops) : '',
      omitted: !stops,
      note: 'Position from 0 to 1, then a hex color. At least two.',
      editor: 'Text',
      list: null,
      section: 11,
      group: 'Fill',
      readOnly: false,
    });
  }
  return rows;
}

/**
 * Write one paint row onto the item. A switch to linear or radial
 * keeps the current color on both stops.
 * @param {object} item
 * @param {string} rowId
 * @param {*} value
 */
export function patchShapeFill(item, rowId, value) {
  const paint = { ...paintObject(item) };
  if (rowId === FILL_PAINT_ROW) {
    const kind = String(value ?? '');
    if (!PAINT_KINDS.includes(kind)) throw new Error('Paint must be solid, linear, or radial.');
    if (kind === 'solid') {
      item.fillPaint = { kind: 'solid', color: currentColor(item) };
      return;
    }
    const next = { kind, stops: ensureStops(item) };
    if (kind === 'linear') {
      const angle = Number(paint.angle);
      next.angle = Number.isFinite(angle) ? angle : 0;
    }
    item.fillPaint = next;
    return;
  }
  if (rowId === FILL_ANGLE_ROW) {
    if (storedPaintKind(item) !== 'linear') throw new Error('Angle belongs on a linear paint.');
    const text = String(value ?? '').trim();
    if (!/^-?\d+(?:\.\d+)?$/.test(text)) throw new Error(`"${text}" is not a number.`);
    item.fillPaint = { kind: 'linear', angle: Number(text), stops: ensureStops(item) };
    return;
  }
  if (rowId === FILL_STOPS_ROW) {
    const kind = storedPaintKind(item);
    if (kind !== 'linear' && kind !== 'radial') throw new Error('Stops belong on a linear or radial paint.');
    const stops = parseStops(value);
    const next = { kind, stops };
    if (kind === 'linear') {
      const angle = Number(paint.angle);
      next.angle = Number.isFinite(angle) ? angle : 0;
    }
    item.fillPaint = next;
    return;
  }
  throw new Error(`Property ${rowId} is read only.`);
}
