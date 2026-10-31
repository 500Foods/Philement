/**
 * Page rulers and the mouse line.
 * The origin is the top-left of the sheet. One inch is 96 CSS pixels.
 * Inches tick at eighths and label each inch. Centimetres tick at
 * millimetres and label each centimetre. Millimetres label every 10.
 * Points label every 72. CSS pixels label every 96. A half step is
 * a longer tick. Tick lengths are three quarters of the first bar.
 * Seven pixels remain outside the major tick. A major label sits
 * beside that tick, and the origin stays unlabeled. The corner
 * cycles the ruler among inches, centimetres, and points. The
 * file keeps its own unit. The mouse line and a drag span are
 * chrome. The definition gains no band.
 */

import { formatLength, parseLength, toCssPx } from './model/units.js';
import { readViewScale } from './scale.js';

const SVG_NS = 'http://www.w3.org/2000/svg';

/** Thickness of the ruler bar, in CSS pixels. The mouse line starts after it. */
export const RULER_SIZE = 28;

const SCALES = {
  in: { minor: 0.125, medium: 0.5, major: 1 },
  cm: { minor: 0.1, medium: 0.5, major: 1 },
  mm: { minor: 1, medium: 5, major: 10 },
  pt: { minor: 9, medium: 36, major: 72 },
  px: { minor: 12, medium: 48, major: 96 },
};

/** Outer end of each tick. The page edge is `RULER_SIZE`. */
const TICK_START = { major: 7, medium: 14.5, minor: 20.5 };

/** Space between a major label and its tick, in CSS pixels. */
const LABEL_GAP = 2;

/** Corner cycle. Millimetres and pixels stay available as the file unit. */
export const RULER_CYCLE = ['in', 'cm', 'pt'];

const UNIT_NAME = {
  in: 'inches',
  cm: 'centimetres',
  pt: 'points',
  mm: 'millimetres',
  px: 'pixels',
};

function svgEl(name) {
  return document.createElementNS(SVG_NS, name);
}

function errorMessage(error, fallback) {
  return error instanceof Error && error.message ? error.message : fallback;
}

/**
 * Document unit for the ruler. An omitted unit is inches.
 * @param {object} page
 * @returns {string|{ error: string }}
 */
export function rulerUnit(page) {
  const raw = page && page.units;
  if (raw === undefined || raw === null || raw === '') return 'in';
  if (typeof raw !== 'string') return { error: 'Ruler units must be a unit name.' };
  try {
    return parseLength(`1 ${raw}`).unit;
  } catch (error) {
    return { error: errorMessage(error, 'Ruler units could not be read.') };
  }
}

function labelFor(value, unit, rank) {
  if (rank !== 'major' || value === 0) return '';
  const text = formatLength(value, unit);
  const suffix = ` ${unit}`;
  return text.endsWith(suffix) ? text.slice(0, -suffix.length) : text;
}

/**
 * Ticks along one edge. `px` agrees with `toCssPx` for that length.
 * A major tick past zero carries the label. The origin is unlabeled.
 * @param {number} lengthPx
 * @param {string} unit
 * @returns {Array<{ px: number, value: number, rank: string, label: string }>}
 */
export function rulerTicks(lengthPx, unit) {
  const scale = SCALES[unit];
  if (!scale || !(lengthPx > 0)) return [];
  const perMedium = Math.round(scale.medium / scale.minor);
  const perMajor = Math.round(scale.major / scale.minor);
  const ticks = [];
  for (let index = 0; index < 20000; index += 1) {
    const value = index * scale.minor;
    const px = toCssPx(formatLength(value, unit));
    if (px > lengthPx + 1e-4) break;
    if (ticks.length > 0 && px <= ticks[ticks.length - 1].px) break;
    let rank = 'minor';
    if (index % perMajor === 0) rank = 'major';
    else if (index % perMedium === 0) rank = 'medium';
    ticks.push({
      px,
      value,
      rank,
      label: labelFor(value, unit, rank),
    });
  }
  return ticks;
}

/**
 * Next unit in the corner cycle. A unit outside the cycle starts at inches.
 * @param {string} unit
 * @returns {string}
 */
export function nextRulerUnit(unit) {
  const index = RULER_CYCLE.indexOf(unit);
  if (index < 0) return RULER_CYCLE[0];
  return RULER_CYCLE[(index + 1) % RULER_CYCLE.length];
}

/**
 * Ruler unit for this paint. A chosen cycle unit wins. Otherwise the
 * file unit is used.
 * @param {string} stored
 * @param {string} [chosen]
 * @returns {string}
 */
export function rulerDisplayUnit(stored, chosen) {
  if (RULER_CYCLE.includes(chosen)) return chosen;
  return stored;
}

/**
 * Both rulers for a measured page. A unit the model rejects sets `error`.
 * `chosen` selects the corner cycle without writing the file.
 * @param {object} page
 * @param {number} widthPx
 * @param {number} heightPx
 * @param {string} [chosen]
 * @returns {object}
 */
export function rulerSpec(page, widthPx, heightPx, chosen) {
  const stored = rulerUnit(page);
  if (stored.error) return stored;
  const unit = rulerDisplayUnit(stored, chosen);
  if (!SCALES[unit]) return { error: `Unknown unit "${unit}".` };
  if (!(widthPx > 0) || !(heightPx > 0)) return { error: 'Page could not be measured.' };
  return {
    unit,
    widthPx,
    heightPx,
    x: rulerTicks(widthPx, unit),
    y: rulerTicks(heightPx, unit),
  };
}

function drawTicks(svg, ticks, axis) {
  for (const tick of ticks) {
    const line = svgEl('line');
    const start = TICK_START[tick.rank];
    if (axis === 'x') {
      line.setAttribute('x1', String(tick.px));
      line.setAttribute('x2', String(tick.px));
      line.setAttribute('y1', String(start));
      line.setAttribute('y2', String(RULER_SIZE));
    } else {
      line.setAttribute('y1', String(tick.px));
      line.setAttribute('y2', String(tick.px));
      line.setAttribute('x1', String(start));
      line.setAttribute('x2', String(RULER_SIZE));
    }
    line.setAttribute('data-rank', tick.rank);
    svg.append(line);
    if (!tick.label) continue;
    const text = svgEl('text');
    text.textContent = tick.label;
    if (axis === 'x') {
      text.setAttribute('x', String(tick.px - LABEL_GAP));
      text.setAttribute('y', String(TICK_START.major));
      text.setAttribute('text-anchor', 'end');
      text.setAttribute('dominant-baseline', 'text-before-edge');
    } else {
      text.setAttribute('x', String(TICK_START.major));
      text.setAttribute('y', String(tick.px - LABEL_GAP));
      text.setAttribute('text-anchor', 'start');
    }
    svg.append(text);
  }
}

function spanId(axis) {
  return `reports-ruler-span-${axis}`;
}

function setSpanBox(node, axis, start, size) {
  if (axis === 'x') {
    node.setAttribute('x', String(start));
    node.setAttribute('y', '0');
    node.setAttribute('width', String(size));
    node.setAttribute('height', String(RULER_SIZE));
    return;
  }
  node.setAttribute('x', '0');
  node.setAttribute('y', String(start));
  node.setAttribute('width', String(RULER_SIZE));
  node.setAttribute('height', String(size));
}

function spanLayer(axis) {
  const defs = svgEl('defs');
  const clip = svgEl('clipPath');
  clip.id = spanId(axis);
  const clipRect = svgEl('rect');
  clipRect.classList.add('reports-ruler-span-clip');
  setSpanBox(clipRect, axis, 0, 0);
  clip.append(clipRect);
  defs.append(clip);
  const bar = svgEl('rect');
  bar.classList.add('reports-ruler-span');
  setSpanBox(bar, axis, 0, 0);
  return { defs, bar };
}

function rulerSvg(className, ticks, axis, lengthPx) {
  const svg = svgEl('svg');
  svg.classList.add('reports-ruler', className);
  svg.setAttribute('aria-hidden', 'true');
  const along = axis === 'x' ? lengthPx : RULER_SIZE;
  const across = axis === 'x' ? RULER_SIZE : lengthPx;
  svg.setAttribute('width', String(along));
  svg.setAttribute('height', String(across));
  svg.setAttribute('viewBox', `0 0 ${along} ${across}`);
  const span = spanLayer(axis);
  const marks = svgEl('g');
  marks.classList.add('reports-ruler-marks');
  drawTicks(marks, ticks, axis);
  const invert = marks.cloneNode(true);
  invert.classList.add('reports-ruler-marks-invert');
  invert.setAttribute('clip-path', `url(#${spanId(axis)})`);
  svg.append(span.defs, span.bar, marks, invert);
  return svg;
}

function cursorLine(className) {
  const line = document.createElement('div');
  line.className = `reports-cursor ${className}`;
  line.hidden = true;
  return line;
}

function cornerButton(unit) {
  const button = document.createElement('button');
  button.type = 'button';
  button.className = 'reports-ruler-corner';
  button.textContent = unit;
  const next = nextRulerUnit(unit);
  const name = UNIT_NAME[next] || next;
  button.setAttribute('aria-label', `Show ruler in ${name}`);
  return button;
}

/**
 * Paint the corner, both rulers, and a hidden mouse line into the sheet.
 * The page itself is appended by the caller. `chosen` is the corner cycle.
 * @param {HTMLElement} sheet
 * @param {object} page
 * @param {{ widthPx: number, heightPx: number }} frame
 * @param {string} [chosen]
 * @returns {{ error?: string, unit?: string }}
 */
export function paintReportRulers(sheet, page, frame, chosen) {
  const spec = rulerSpec(page, frame.widthPx, frame.heightPx, chosen);
  if (spec.error) return spec;
  sheet.style.setProperty('--reports-ruler-size', `${RULER_SIZE}px`);
  const corner = cornerButton(spec.unit);
  sheet.append(
    corner,
    rulerSvg('reports-ruler-top', spec.x, 'x', spec.widthPx),
    rulerSvg('reports-ruler-left', spec.y, 'y', spec.heightPx),
    cursorLine('reports-cursor-x'),
    cursorLine('reports-cursor-y'),
  );
  return { unit: spec.unit };
}

/**
 * Move the crosshair. `x` and `y` are CSS pixels from the page origin.
 * The lines also cross the ruler bars.
 * @param {HTMLElement} page
 * @param {number} x
 * @param {number} y
 */
export function placeReportCursor(page, x, y) {
  const sheet = page && page.closest ? page.closest('.reports-sheet') : null;
  if (!sheet) return;
  const vertical = sheet.querySelector('.reports-cursor-x');
  const horizontal = sheet.querySelector('.reports-cursor-y');
  if (!vertical || !horizontal) return;
  vertical.hidden = false;
  horizontal.hidden = false;
  vertical.style.left = `${RULER_SIZE + x}px`;
  horizontal.style.top = `${RULER_SIZE + y}px`;
}

/**
 * Invert one slice of a ruler. `fromPx` and `toPx` are CSS pixels
 * from the page origin along that edge. A zero span draws nothing.
 * @param {HTMLElement} page
 * @param {'x'|'y'} axis
 * @param {number} fromPx
 * @param {number} toPx
 */
export function showRulerSpan(page, axis, fromPx, toPx) {
  const sheet = page && page.closest ? page.closest('.reports-sheet') : null;
  if (!sheet || (axis !== 'x' && axis !== 'y')) return;
  if (!Number.isFinite(fromPx) || !Number.isFinite(toPx)) return;
  const svg = sheet.querySelector(axis === 'x' ? '.reports-ruler-top' : '.reports-ruler-left');
  if (!svg) return;
  const clip = svg.querySelector('.reports-ruler-span-clip');
  const bar = svg.querySelector('.reports-ruler-span');
  if (!clip || !bar) return;
  const start = Math.min(fromPx, toPx);
  const size = Math.abs(toPx - fromPx);
  setSpanBox(clip, axis, start, size);
  setSpanBox(bar, axis, start, size);
}

/** Clear a ruler span. An omitted axis clears both edges. */
export function hideRulerSpan(page, axis) {
  const axes = axis ? [axis] : ['x', 'y'];
  for (const name of axes) showRulerSpan(page, name, 0, 0);
}

/** Hide the crosshair. */
export function hideReportCursor(page) {
  const sheet = page && page.closest ? page.closest('.reports-sheet') : null;
  if (!sheet) return;
  const vertical = sheet.querySelector('.reports-cursor-x');
  const horizontal = sheet.querySelector('.reports-cursor-y');
  if (vertical) vertical.hidden = true;
  if (horizontal) horizontal.hidden = true;
}

/**
 * Follow the pointer while it is inside the page. `mouseleave` hides
 * the line. The caller disconnects this from cleanup and destroy.
 * @param {HTMLElement} page
 * @returns {{ disconnect: () => void }}
 */
export function bindReportCursor(page) {
  if (!page) return { disconnect() {} };
  const onMove = (event) => {
    const rect = page.getBoundingClientRect();
    const scale = readViewScale(page);
    const x = (event.clientX - rect.left) / scale;
    const y = (event.clientY - rect.top) / scale;
    if (x < 0 || y < 0 || x > rect.width / scale || y > rect.height / scale) {
      hideReportCursor(page);
      return;
    }
    placeReportCursor(page, x, y);
  };
  const onLeave = () => hideReportCursor(page);
  page.addEventListener('mousemove', onMove);
  page.addEventListener('mouseleave', onLeave);
  return {
    disconnect() {
      page.removeEventListener('mousemove', onMove);
      page.removeEventListener('mouseleave', onLeave);
      hideReportCursor(page);
    },
  };
}
