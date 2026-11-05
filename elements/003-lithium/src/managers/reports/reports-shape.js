/**
 * Static shapes (typeKey 25). The design surface paints an SVG.
 * Preview HTML comes from Reports.Render. PDF does not draw the shape.
 * Built-in geometry is code. Path kinds use the lookup collection,
 * or these commands before that lookup is applied. A bad command
 * skips the shape. One inch is 96 CSS pixels. The stroke is centered.
 */

import './reports-shape.css';
import { duotoneOpacityRules, prepareDuotone } from './model/images.js';
import { toCssPx } from './model/units.js';
import {
  FILL_STYLES, fillPlan, fillStyleChoices, mountShape, patchShapeFill, shapeDomId, shapeFillRowList,
  shapeMarkup, shapeSpacingPx, storedPaintKind,
} from './reports-shape-fill.js';

export { fillStyleChoices, shapeSpacingPx };

export const SHAPE_TYPE = 25;
const KAPPA = 0.5522847498;
const HEX = /^#[0-9A-Fa-f]{6}$/;
const SIDES = new Set(['top', 'right', 'bottom', 'left']);
const STROKE_STYLES = new Set(['solid', 'dashed', 'dotted', 'none']);
const GEOMETRY = new Set(['line', 'rectangle', 'roundrect', 'ellipse']);
const CORNER_NAMES = ['topLeft', 'topRight', 'bottomRight', 'bottomLeft'];
const CORNER_KEYS = { 48: 'topLeft', 49: 'topRight', 50: 'bottomRight', 51: 'bottomLeft' };

/** Palette order. `code` is the token the item stores. */
export const SHAPE_LABELS = Object.freeze({
  line: 'Line',
  rectangle: 'Rectangle',
  roundrect: 'Rounded Rectangle',
  ellipse: 'Ellipse',
  x: 'X',
  trapezoid: 'Trapezoid',
  star: 'Star',
  pentagon: 'Pentagon',
  hexagon: 'Hexagon',
});

const SHAPE_ORDER = Object.keys(SHAPE_LABELS);

/** Unit-square commands. Polygons point up. The trapezoid's short edge is on top. */
export const SEEDED_PATHS = Object.freeze({
  x: Object.freeze([['M', 0, 0], ['L', 1, 1], ['M', 1, 0], ['L', 0, 1]]),
  trapezoid: Object.freeze([['M', 0.25, 0], ['L', 0.75, 0], ['L', 1, 1], ['L', 0, 1], ['Z']]),
  star: Object.freeze([
    ['M', 0.5, 0], ['L', 0.618034, 0.381966], ['L', 1, 0.381966], ['L', 0.690983, 0.618034],
    ['L', 0.809017, 1], ['L', 0.5, 0.763932], ['L', 0.190983, 1], ['L', 0.309017, 0.618034],
    ['L', 0, 0.381966], ['L', 0.381966, 0.381966], ['Z'],
  ]),
  pentagon: Object.freeze([
    ['M', 0.5, 0], ['L', 1, 0.381966], ['L', 0.809017, 1], ['L', 0.190983, 1], ['L', 0, 0.381966], ['Z'],
  ]),
  hexagon: Object.freeze([
    ['M', 0.5, 0], ['L', 1, 0.25], ['L', 1, 0.75], ['L', 0.5, 1], ['L', 0, 0.75], ['L', 0, 0.25], ['Z'],
  ]),
});

/** Tags Preview HTML adds. Other sanitizeHtml callers stay on the default. */
export const SHAPE_PURIFY = Object.freeze({
  ADD_TAGS: [
    'svg', 'path', 'g', 'defs', 'clipPath', 'linearGradient', 'radialGradient', 'stop', 'circle',
    'image', 'use', 'rect', 'ellipse', 'line', 'polyline', 'polygon',
  ],
  ADD_ATTR: [
    'viewBox', 'viewbox', 'd', 'fill', 'stroke', 'stroke-width', 'stroke-dasharray',
    'stroke-linecap', 'stroke-linejoin', 'transform', 'preserveAspectRatio', 'preserveaspectratio', 'xmlns',
    'id', 'offset', 'stop-color', 'gradientUnits', 'gradientunits', 'x1', 'y1', 'x2', 'y2', 'cx', 'cy', 'r',
    'clip-path', 'width', 'height', 'overflow', 'href', 'xlink:href', 'opacity', 'points', 'rx', 'ry',
  ],
});

const SHAPE_ATTR_KEYS = [41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51];

/**
 * Lookup 075 keys 10–12. sortSeq sits between Object (4) and Position (5).
 * The panel uses these when the lookup has not been applied yet.
 */
export const SHAPE_SECTIONS = Object.freeze([
  { id: 10, label: 'Stroke', sortSeq: 4.2, icon: '<fa fa-pen></fa>' },
  { id: 11, label: 'Fill', sortSeq: 4.4, icon: '<fa fa-fill-drip></fa>' },
  { id: 12, label: 'Corners', sortSeq: 4.6, icon: '<fa fa-vector-square></fa>' },
]);

/** Shown when lookup 054 has not been applied yet. */
export const SHAPE_ATTR_META = Object.freeze({
  41: { Group: 'Object', section: 4, Label: 'Shape', Editor: 'List' },
  42: { Group: 'Object', section: 4, Label: 'Side', Editor: 'List', List: ['top', 'right', 'bottom', 'left'] },
  43: { Group: 'Stroke', section: 10, Label: 'Stroke Width', Editor: 'Number' },
  44: { Group: 'Stroke', section: 10, Label: 'Stroke Style', Editor: 'List', List: ['solid', 'dashed', 'dotted', 'none'] },
  45: { Group: 'Stroke', section: 10, Label: 'Stroke Color', Editor: 'Color' },
  46: { Group: 'Fill', section: 11, Label: 'Fill Style', Editor: 'List', List: ['none', 'solid', 'grid', 'dots', 'hatched', 'noise'] },
  47: { Group: 'Fill', section: 11, Label: 'Fill Color', Editor: 'Color' },
  48: { Group: 'Corners', section: 12, Label: 'Corner Top Left', Editor: 'Number' },
  49: { Group: 'Corners', section: 12, Label: 'Corner Top Right', Editor: 'Number' },
  50: { Group: 'Corners', section: 12, Label: 'Corner Bottom Right', Editor: 'Number' },
  51: { Group: 'Corners', section: 12, Label: 'Corner Bottom Left', Editor: 'Number' },
});

/**
 * Keep Stroke, Fill, and Corners between Object and Position.
 * A loaded lookup row keeps its label and icon.
 * @param {object[]} sections
 * @returns {object[]}
 */
export function withShapeSections(sections) {
  const list = Array.isArray(sections) ? sections.map((section) => ({ ...section })) : [];
  const byId = new Map(list.map((section) => [Number(section.id), section]));
  for (const section of SHAPE_SECTIONS) {
    const found = byId.get(section.id);
    if (found) {
      found.sortSeq = section.sortSeq;
      if (!found.icon) found.icon = section.icon;
      if (!found.label) found.label = section.label;
    } else {
      list.push({ ...section });
    }
  }
  return list;
}

let lookupRows = null;

/**
 * Lookup 078 rows. Empty clears back to the built-in list.
 * The rows are not stored on the definition.
 * @param {object[]|null|undefined} rows
 */
export function setShapeCatalog(rows) {
  lookupRows = Array.isArray(rows) && rows.length ? rows : null;
}

function collectionObject(value) {
  let parsed = value;
  if (typeof value === 'string') {
    try {
      parsed = JSON.parse(value);
    } catch {
      return null;
    }
  }
  if (Array.isArray(parsed)) parsed = parsed.length === 1 ? parsed[0] : null;
  if (!parsed || typeof parsed !== 'object' || Array.isArray(parsed)) return null;
  return parsed;
}

function catalogRows(rows) {
  const source = rows === undefined ? lookupRows : rows;
  if (!Array.isArray(source) || !source.length) return [];
  return [...source].sort((a, b) => Number(a.key_idx) - Number(b.key_idx));
}

function catalogMap(rows) {
  const map = new Map();
  for (const row of catalogRows(rows)) {
    const code = typeof row.code === 'string' ? row.code : '';
    if (!code || map.has(code)) continue;
    map.set(code, row);
  }
  return map;
}

/**
 * One row per code. Built-in codes stay in the list when the lookup omits them.
 * @param {object[]|null|undefined} [rows]
 * @returns {{ code: string, label: string }[]}
 */
export function shapeKindOptions(rows) {
  const map = catalogMap(rows);
  const options = [];
  if (map.size) {
    for (const [code, row] of map) {
      options.push({ code, label: row.value_txt || SHAPE_LABELS[code] || code });
    }
  }
  for (const code of SHAPE_ORDER) {
    if (!map.has(code)) options.push({ code, label: SHAPE_LABELS[code] });
  }
  return options;
}

export function shapeOutlineLabel(item) {
  const kind = item && typeof item.kind === 'string' && item.kind !== '' ? item.kind : 'rectangle';
  const row = catalogMap().get(kind);
  if (row && row.value_txt) return row.value_txt;
  return SHAPE_LABELS[kind] || kind;
}

function knownKind(kind, rows) {
  if (Object.hasOwn(SHAPE_LABELS, kind)) return true;
  return catalogMap(rows).has(kind);
}

/**
 * Fields a new shape stores. Omitted stroke and fill stay omitted on import.
 * @param {string} kind
 * @returns {object}
 */
export function defaultShapeFields(kind) {
  if (!knownKind(kind)) throw new Error(`Shape kind "${kind}" is not in the list.`);
  const fields = {
    kind,
    strokeWidth: '1 pt',
    strokeStyle: 'solid',
    strokeColor: '#000000',
    fillStyle: kind === 'line' ? 'none' : 'solid',
    fillPaint: { kind: 'solid', color: '#d3d3d3' },
    rotation: 0,
  };
  if (kind === 'line') fields.side = 'bottom';
  if (kind === 'roundrect') {
    fields.corners = {
      topLeft: '0.125 in', topRight: '0.125 in', bottomRight: '0.125 in', bottomLeft: '0.125 in',
    };
  }
  return fields;
}

export function shapeAttrMeta(key) {
  return SHAPE_ATTR_META[key] || null;
}

/** Drop report-only key 24 and Align (key 4). Show side on a line and corners on a rounded rectangle. */
export function shapeAttributeKeys(item, keys) {
  const next = (Array.isArray(keys) ? keys : []).filter((key) => key !== 24 && key !== 4);
  for (const key of SHAPE_ATTR_KEYS) {
    if (!next.includes(key)) next.push(key);
  }
  const kind = item && item.kind;
  return next.filter((key) => {
    if (key === 42) return kind === 'line';
    if (key === 47) return storedPaintKind(item) === 'solid';
    if (key >= 48 && key <= 51) return kind === 'roundrect';
    return true;
  });
}

function selectedItem(doc, sel) {
  const bands = Array.isArray(doc && doc.bands) ? doc.bands : [];
  const band = bands.find((entry) => entry.id === (sel && sel.bandId));
  const items = band && Array.isArray(band.items) ? band.items : [];
  return items.find((entry) => entry.id === (sel && sel.itemId)) || null;
}

function editShape(doc, sel, change) {
  const next = JSON.parse(JSON.stringify(doc));
  const item = selectedItem(next, sel);
  if (!item) throw new Error(`Item "${sel && sel.itemId}" was not found.`);
  change(item);
  return next;
}

function readField(item, key, fallback) {
  if (item && Object.hasOwn(item, key) && item[key] !== '' && item[key] !== null) {
    return { value: item[key], omitted: false };
  }
  return { value: fallback, omitted: true };
}

function applyKind(item, kind) {
  item.kind = kind;
  if (kind === 'line') {
    if (!SIDES.has(item.side)) item.side = 'bottom';
    if (!Object.hasOwn(item, 'fillStyle')) item.fillStyle = 'none';
  } else {
    delete item.side;
  }
  if (kind === 'roundrect') {
    const prev = item.corners && typeof item.corners === 'object' && !Array.isArray(item.corners)
      ? item.corners : {};
    const corners = {};
    for (const name of CORNER_NAMES) {
      corners[name] = typeof prev[name] === 'string' && prev[name] !== '' ? prev[name] : '0.125 in';
    }
    item.corners = corners;
  } else {
    delete item.corners;
  }
}

function requireHex(value, label) {
  const text = String(value ?? '').trim();
  if (!HEX.test(text)) throw new Error(`${label} must be a hex color.`);
  return text.toLowerCase();
}

export const shapeItemFields = {
  41: {
    get: (doc, sel) => {
      const item = selectedItem(doc, sel);
      const kind = item && typeof item.kind === 'string' ? item.kind : '';
      return kind !== '' ? { value: kind, omitted: false } : { value: 'rectangle', omitted: true };
    },
    set: (doc, sel, value) => {
      const kind = String(value ?? '');
      if (!knownKind(kind)) throw new Error(`Shape kind "${kind}" is not in the list.`);
      return editShape(doc, sel, (item) => applyKind(item, kind));
    },
  },
  42: {
    get: (doc, sel) => readField(selectedItem(doc, sel), 'side', 'bottom'),
    set: (doc, sel, value) => {
      const side = String(value ?? '');
      if (!SIDES.has(side)) throw new Error(`Side must be top, right, bottom, or left.`);
      return editShape(doc, sel, (item) => { item.side = side; });
    },
  },
  43: {
    get: (doc, sel) => readField(selectedItem(doc, sel), 'strokeWidth', '1 pt'),
    set: (doc, sel, value) => editShape(doc, sel, (item) => { item.strokeWidth = value; }),
  },
  44: {
    get: (doc, sel) => readField(selectedItem(doc, sel), 'strokeStyle', 'solid'),
    set: (doc, sel, value) => {
      const style = String(value ?? '');
      if (!STROKE_STYLES.has(style)) throw new Error('Stroke style must be solid, dashed, dotted, or none.');
      return editShape(doc, sel, (item) => { item.strokeStyle = style; });
    },
  },
  45: {
    get: (doc, sel) => readField(selectedItem(doc, sel), 'strokeColor', '#000000'),
    set: (doc, sel, value) => {
      const color = requireHex(value, 'Stroke color');
      return editShape(doc, sel, (item) => { item.strokeColor = color; });
    },
  },
  46: {
    get: (doc, sel) => {
      const item = selectedItem(doc, sel);
      const fallback = item && item.kind === 'line' ? 'none' : 'solid';
      return readField(item, 'fillStyle', fallback);
    },
    set: (doc, sel, value) => {
      const style = String(value ?? '');
      if (!FILL_STYLES.has(style)) throw new Error('Fill style must be none, solid, grid, dots, hatched, or noise.');
      return editShape(doc, sel, (item) => { item.fillStyle = style; });
    },
  },
  47: {
    get: (doc, sel) => {
      const item = selectedItem(doc, sel);
      const paint = item && item.fillPaint && typeof item.fillPaint === 'object' ? item.fillPaint : null;
      const color = paint && typeof paint.color === 'string' ? paint.color : '';
      return color !== '' ? { value: color, omitted: false } : { value: '#d3d3d3', omitted: true };
    },
    set: (doc, sel, value) => {
      const color = requireHex(value, 'Fill color');
      return editShape(doc, sel, (item) => {
        const kind = storedPaintKind(item);
        const paint = item.fillPaint && typeof item.fillPaint === 'object' ? item.fillPaint : {};
        if (kind === 'linear' || kind === 'radial') {
          const stops = Array.isArray(paint.stops) && paint.stops.length
            ? paint.stops.map((stop, index) => (index === 0 ? { ...stop, color } : { ...stop }))
            : [{ at: 0, color }, { at: 1, color }];
          const next = { kind, stops };
          if (kind === 'linear') {
            const angle = Number(paint.angle);
            next.angle = Number.isFinite(angle) ? angle : 0;
          }
          item.fillPaint = next;
          return;
        }
        item.fillPaint = { kind: 'solid', color };
      });
    },
  },
};

/** Paint rows for a shape. Empty for every other selection. */
export function shapeFillRows(doc, sel) {
  const item = selectedItem(doc, sel);
  if (!item || Number(item.typeKey) !== SHAPE_TYPE) return [];
  return shapeFillRowList(item);
}

/** Write Paint, Angle, or Stops. The color row stays key 47. */
export function applyShapeFill(doc, sel, rowId, value) {
  const item = selectedItem(doc, sel);
  if (!item || Number(item.typeKey) !== SHAPE_TYPE) throw new Error('Fill needs a selected shape.');
  return editShape(doc, sel, (next) => patchShapeFill(next, rowId, value));
}

for (const [key, name] of Object.entries(CORNER_KEYS)) {
  shapeItemFields[key] = {
    get: (doc, sel) => {
      const item = selectedItem(doc, sel);
      const corners = item && item.corners && typeof item.corners === 'object' ? item.corners : null;
      const value = corners && typeof corners[name] === 'string' ? corners[name] : '';
      return value !== '' ? { value, omitted: false } : { value: '0.125 in', omitted: true };
    },
    set: (doc, sel, value) => editShape(doc, sel, (item) => {
      const prev = item.corners && typeof item.corners === 'object' && !Array.isArray(item.corners)
        ? item.corners : {};
      item.corners = { ...prev, [name]: value };
    }),
  };
}

function fmt(n) {
  const neg = n < 0;
  const rounded = Math.round(Math.abs(n) * 10000) / 10000;
  if (rounded === 0) return '0';
  const text = rounded.toFixed(4).replace(/0+$/, '').replace(/\.$/, '');
  return neg ? `-${text}` : text;
}

function lengthPx(value, fallback) {
  if (typeof value !== 'string' || value.trim() === '') return fallback;
  try {
    return toCssPx(value);
  } catch {
    return fallback;
  }
}

function hexColor(value, fallback) {
  return typeof value === 'string' && HEX.test(value) ? value.toLowerCase() : fallback;
}

function commandsOk(commands) {
  if (!Array.isArray(commands) || commands.length === 0) return false;
  const arity = { M: 2, L: 2, C: 6, Z: 0 };
  return commands.every((cmd) => {
    if (!Array.isArray(cmd)) return false;
    const need = arity[cmd[0]];
    if (need === undefined || cmd.length !== need + 1) return false;
    return cmd.slice(1).every((n) => typeof n === 'number' && Number.isFinite(n) && n >= 0 && n <= 1);
  });
}

function pathSource(kind) {
  if (GEOMETRY.has(kind)) return { geometry: kind };
  const row = catalogMap().get(kind);
  if (row) {
    const collection = collectionObject(row.collection);
    const commands = collection && Array.isArray(collection.commands) ? collection.commands : null;
    if (!commandsOk(commands)) return { bad: true };
    return { commands, closed: !(collection && collection.closed === false) };
  }
  if (SEEDED_PATHS[kind]) return { commands: SEEDED_PATHS[kind], closed: kind !== 'x' };
  return null;
}

function scaleCommands(commands, w, h) {
  return commands.map((cmd) => {
    if (cmd[0] === 'Z') return ['Z'];
    const out = [cmd[0]];
    for (let i = 1; i < cmd.length; i += 1) out.push((i % 2 === 1 ? w : h) * cmd[i]);
    return out;
  });
}

function commandD(commands) {
  return commands.map((cmd) => (cmd[0] === 'Z' ? 'Z' : `${cmd[0]} ${cmd.slice(1).map((n) => fmt(n)).join(' ')}`)).join(' ');
}

function clampCorners(w, h, radii) {
  let factor = 1;
  const pairs = [[radii.tl + radii.tr, w], [radii.bl + radii.br, w], [radii.tl + radii.bl, h], [radii.tr + radii.br, h]];
  for (const [span, limit] of pairs) {
    if (span > 0) factor = Math.min(factor, limit / span);
  }
  if (!Number.isFinite(factor) || factor < 0) factor = 0;
  if (factor > 1) factor = 1;
  return { tl: radii.tl * factor, tr: radii.tr * factor, br: radii.br * factor, bl: radii.bl * factor };
}

function roundRectCommands(w, h, r) {
  const { tl, tr, br, bl } = r;
  return [
    ['M', tl, 0],
    ['L', w - tr, 0],
    ['C', w - tr * (1 - KAPPA), 0, w, tr * (1 - KAPPA), w, tr],
    ['L', w, h - br],
    ['C', w, h - br * (1 - KAPPA), w - br * (1 - KAPPA), h, w - br, h],
    ['L', bl, h],
    ['C', bl * (1 - KAPPA), h, 0, h - bl * (1 - KAPPA), 0, h - bl],
    ['L', 0, tl],
    ['C', 0, tl * (1 - KAPPA), tl * (1 - KAPPA), 0, tl, 0],
    ['Z'],
  ];
}

function ellipseCommands(w, h) {
  const cx = w / 2;
  const cy = h / 2;
  const rx = w / 2;
  const ry = h / 2;
  return [
    ['M', cx, cy - ry],
    ['C', cx + KAPPA * rx, cy - ry, cx + rx, cy - KAPPA * ry, cx + rx, cy],
    ['C', cx + rx, cy + KAPPA * ry, cx + KAPPA * rx, cy + ry, cx, cy + ry],
    ['C', cx - KAPPA * rx, cy + ry, cx - rx, cy + KAPPA * ry, cx - rx, cy],
    ['C', cx - rx, cy - KAPPA * ry, cx - KAPPA * rx, cy - ry, cx, cy - ry],
    ['Z'],
  ];
}

function lineCommands(w, h, side) {
  if (side === 'top') return [['M', 0, 0], ['L', w, 0]];
  if (side === 'right') return [['M', w, 0], ['L', w, h]];
  if (side === 'left') return [['M', 0, 0], ['L', 0, h]];
  return [['M', 0, h], ['L', w, h]];
}

function cornerRadii(item) {
  const corners = item.corners && typeof item.corners === 'object' ? item.corners : {};
  const fallback = toCssPx('0.125 in');
  const read = (name) => {
    const raw = corners[name];
    if (typeof raw !== 'string' || raw.trim() === '') return Math.max(0, fallback);
    return Math.max(0, lengthPx(raw, fallback));
  };
  return { tl: read('topLeft'), tr: read('topRight'), br: read('bottomRight'), bl: read('bottomLeft') };
}

/**
 * Drawing for one shape, in CSS pixels. `warn` means skip the path.
 * `spacingPx` is the report grid spacing. Omitted spacing uses 0.125 in.
 * @param {object} item
 * @param {number} [spacingPx]
 * @returns {object}
 */
export function shapeDrawing(item, spacingPx) {
  const box = item && item.box && typeof item.box === 'object' ? item.box : null;
  if (!box) return { warn: 'Shape box is empty.' };
  let w;
  let h;
  try {
    w = toCssPx(box.width);
    h = toCssPx(box.height);
  } catch {
    return { warn: 'Shape box is empty.' };
  }
  if (!(w > 0) || !(h > 0)) return { warn: 'Shape box is empty.' };
  const kind = item.kind && typeof item.kind === 'string' && item.kind !== '' ? item.kind : 'rectangle';
  const source = pathSource(kind);
  if (!source) return { warn: `Shape kind ${kind} is not in the list.` };
  if (source.bad) return { warn: 'Shape has a bad command.' };
  let commands;
  if (source.geometry === 'line') commands = lineCommands(w, h, SIDES.has(item.side) ? item.side : 'bottom');
  else if (source.geometry === 'rectangle') commands = [['M', 0, 0], ['L', w, 0], ['L', w, h], ['L', 0, h], ['Z']];
  else if (source.geometry === 'roundrect') commands = roundRectCommands(w, h, clampCorners(w, h, cornerRadii(item)));
  else if (source.geometry === 'ellipse') commands = ellipseCommands(w, h);
  else {
    commands = scaleCommands(source.commands, w, h);
    if (source.closed && commands[commands.length - 1][0] !== 'Z') commands = [...commands, ['Z']];
  }
  const sw = lengthPx(item.strokeWidth, toCssPx('1 pt'));
  let style = item.strokeStyle;
  if (!STROKE_STYLES.has(style)) style = 'solid';
  const stroke = style === 'none' || !(sw > 0) ? 'none' : hexColor(item.strokeColor, '#000000');
  let dash = '';
  let linecap = 'butt';
  if (stroke !== 'none' && style === 'dashed') dash = `${fmt(sw * 4)} ${fmt(sw * 2)}`;
  if (stroke !== 'none' && style === 'dotted') {
    dash = `${fmt(sw)} ${fmt(sw * 2)}`;
    linecap = 'round';
  }
  const deg = Number(item.rotation);
  const turn = Number.isFinite(deg) ? deg : 0;
  const spacing = typeof spacingPx === 'number' && spacingPx > 0 ? spacingPx : shapeSpacingPx(null);
  const plan = fillPlan(item, kind, w, h, spacing);
  const paintId = shapeDomId('reports-fill-', item && item.id);
  const paintRef = plan.gradient ? `url(#${paintId})` : plan.color;
  return {
    w, h, d: commandD(commands),
    fill: plan.style === 'solid' ? paintRef : 'none',
    paintRef,
    paintId,
    clipId: shapeDomId('reports-clip-', item && item.id),
    gradient: plan.gradient,
    marks: plan.marks,
    stroke, sw, dash, linecap,
    transform: turn === 0 ? '' : `rotate(${fmt(turn)} ${fmt(w / 2)} ${fmt(h / 2)})`,
  };
}

/** SVG markup for tests and for the same commands Preview HTML draws. */
export function shapeSvg(item, doc) {
  const draw = shapeDrawing(item, shapeSpacingPx(doc));
  if (draw.warn) return { html: '', warn: draw.warn };
  return { html: shapeMarkup(draw), warn: '' };
}

/** Paint one shape into its box. A bad command shows a note and does not log. */
export function paintShape(el, item, doc) {
  const draw = shapeDrawing(item, shapeSpacingPx(doc));
  if (draw.warn) {
    const note = document.createElement('p');
    note.className = 'reports-shape-note';
    note.textContent = draw.warn;
    el.append(note);
    return;
  }
  mountShape(el, draw);
}

const PREVIEW_SHAPE_TAGS = new Set([
  'svg', 'path', 'g', 'defs', 'clippath', 'lineargradient', 'radialgradient', 'stop', 'circle',
  'image', 'use', 'rect', 'ellipse', 'line', 'polyline', 'polygon',
]);
const HEX_COLOR = /^#[0-9A-Fa-f]{6}$/;
const PATH_D = /^[MmLlHhVvCcSsQqTtAaZz0-9eE\s,.+\-]+$/;
const SAFE_ID = /^[A-Za-z][A-Za-z0-9_-]*$/;
const PAINT_REF = /^(?:none|currentColor|#[0-9A-Fa-f]{3}|#[0-9A-Fa-f]{4}|#[0-9A-Fa-f]{6}|#[0-9A-Fa-f]{8}|url\(#[A-Za-z][A-Za-z0-9_-]*\))$/;
const OPACITY_ATTR = /^(?:0(?:\.0+)?|1(?:\.0+)?|0?\.\d+)$/;
const UNIT_NUM = /^-?\d+(?:\.\d+)?$/;

function transformSequenceOk(text) {
  const parts = text.trim().split(/\)\s+(?=[A-Za-z])/);
  if (parts.length === 0 || text.trim() === '') return false;
  const one = /^(?:translate|scale|rotate)\(-?\d+(?:\.\d+)?(?:[\s,]+-?\d+(?:\.\d+)?){0,2}\)$/;
  return parts.every((part, index) => {
    const closed = index === parts.length - 1 ? part.trim() : `${part.trim()})`;
    return one.test(closed);
  });
}

function previewShapeAttrOk(name, value) {
  const attr = String(name).toLowerCase();
  const text = String(value);
  if (attr.startsWith('on')) return false;
  const href = attr === 'href' || attr === 'xlink:href';
  if (text.length > (attr === 'd' ? 200000 : href ? 8000000 : 4000)) return false;
  if (/[<>]/.test(text) || /javascript\s*:/i.test(text)) return false;
  if (href) {
    if (text.startsWith('#')) return SAFE_ID.test(text.slice(1));
    return /^data:image\/(?:png|jpeg);base64,[A-Za-z0-9+/=]+$/.test(text);
  }
  if (attr === 'xmlns') return text === 'http://www.w3.org/2000/svg';
  if (attr === 'width' || attr === 'height') return text === '100%' || text === '0' || /^-?\d+(?:\.\d+)?$/.test(text);
  if (attr === 'overflow') return text === 'visible';
  if (attr === 'viewbox') return /^-?\d+(?:\.\d+)?(?:\s+-?\d+(?:\.\d+)?){3}$/.test(text.trim());
  if (attr === 'preserveaspectratio') return text === 'none';
  if (attr === 'id') return SAFE_ID.test(text);
  if (attr === 'fill' || attr === 'stroke') return PAINT_REF.test(text);
  if (attr === 'opacity' || attr === 'fill-opacity' || attr === 'stroke-opacity') return OPACITY_ATTR.test(text.trim());
  if (attr === 'fill-rule' || attr === 'clip-rule') return /^(?:evenodd|nonzero)$/i.test(text.trim());
  if (attr === 'clip-path') return /^url\(#[A-Za-z][A-Za-z0-9_-]*\)$/.test(text);
  if (attr === 'gradientunits') return text === 'userSpaceOnUse';
  if (attr === 'offset' || attr === 'x1' || attr === 'y1' || attr === 'x2' || attr === 'y2' || attr === 'cx' || attr === 'cy' || attr === 'r') {
    return UNIT_NUM.test(text);
  }
  if (attr === 'stop-color') return HEX_COLOR.test(text);
  if (attr === 'stroke-width') return /^\d+(?:\.\d+)?$/.test(text);
  if (attr === 'stroke-dasharray') return /^\d+(?:\.\d+)?(?:\s+\d+(?:\.\d+)?)+$/.test(text);
  if (attr === 'stroke-linecap') return text === 'butt' || text === 'round' || text === 'square';
  if (attr === 'stroke-linejoin') return text === 'miter' || text === 'round' || text === 'bevel';
  if (attr === 'transform') return transformSequenceOk(text);
  if (attr === 'd') return PATH_D.test(text);
  return false;
}

function prunePreviewShape(el) {
  for (const child of [...el.childNodes]) {
    if (child.nodeType !== 1) {
      child.remove();
      continue;
    }
    const name = String(child.localName || '').toLowerCase();
    if (!PREVIEW_SHAPE_TAGS.has(name)) {
      child.remove();
      continue;
    }
    prunePreviewShape(child);
  }
  for (const attr of [...el.attributes]) {
    if (!previewShapeAttrOk(attr.name, attr.value)) el.removeAttribute(attr.name);
  }
}

function cleanSvgMarkup(markup, rules) {
  if (typeof DOMParser === 'undefined') return '';
  const doc = new DOMParser().parseFromString(markup, 'image/svg+xml');
  const root = doc.documentElement;
  if (!root || String(root.localName).toLowerCase() !== 'svg') return '';
  prepareDuotone(root, rules);
  prunePreviewShape(root);
  const html = root.outerHTML;
  if (/<\s*script\b/i.test(html)) return '';
  return html;
}

/** One top-level svg, including a nested svg, from `start` at `<svg`. */
function takeSvg(html, start) {
  const lower = html.toLowerCase();
  const openEnd = html.indexOf('>', start);
  if (openEnd < 0) return null;
  let depth = 1;
  let scan = openEnd + 1;
  while (depth > 0) {
    const nextOpen = lower.indexOf('<svg', scan);
    const nextClose = lower.indexOf('</svg', scan);
    if (nextClose < 0) return null;
    if (nextOpen >= 0 && nextOpen < nextClose) {
      depth += 1;
      const nested = html.indexOf('>', nextOpen);
      if (nested < 0) return null;
      scan = nested + 1;
      continue;
    }
    depth -= 1;
    const closeEnd = html.indexOf('>', nextClose);
    if (closeEnd < 0) return null;
    scan = closeEnd + 1;
  }
  return { markup: html.slice(start, scan), end: scan };
}

/**
 * Lift each svg out of the fragment before DOMPurify.
 * The markup is parsed as XML. An HTML parse treats `<style>` as
 * raw text and, in happy-dom, drops the paths that follow it.
 * A bare text token is deleted, so the token sits in a span.
 * @param {string} html
 * @returns {{ html: string, shapes: { token: string, clean: string }[] }}
 */
export function shieldPreviewShapes(html) {
  if (typeof html !== 'string' || !/<svg[\s>/]/i.test(html)) return { html, shapes: [] };
  const rules = duotoneOpacityRules(html);
  const shapes = [];
  let nonce = Math.random().toString(36).slice(2);
  let out = '';
  let cursor = 0;
  const lower = html.toLowerCase();
  while (cursor < html.length) {
    const start = lower.indexOf('<svg', cursor);
    if (start < 0) {
      out += html.slice(cursor);
      break;
    }
    const taken = takeSvg(html, start);
    if (!taken) {
      out += html.slice(cursor);
      break;
    }
    out += html.slice(cursor, start);
    const clean = cleanSvgMarkup(taken.markup, rules);
    let token = `[[reports-shape-${nonce}-${shapes.length}]]`;
    if (html.includes(token) || out.includes(token)) {
      nonce = `${nonce}${shapes.length}x`;
      token = `[[reports-shape-${nonce}-${shapes.length}]]`;
    }
    shapes.push({ token, clean });
    out += `<span>${token}</span>`;
    cursor = taken.end;
  }
  const host = document.createElement('div');
  host.innerHTML = out;
  return { html: host.innerHTML, shapes };
}

/**
 * Put the cleaned svg markup back where the tokens were.
 * @param {string} html
 * @param {{ token: string, clean: string }[]} shapes
 * @returns {string}
 */
export function restorePreviewShapes(html, shapes) {
  let out = typeof html === 'string' ? html : '';
  for (const shape of shapes || []) {
    if (!shape || !shape.token || !shape.clean || !out.includes(shape.token)) continue;
    out = out.split(shape.token).join(shape.clean);
  }
  return out;
}
