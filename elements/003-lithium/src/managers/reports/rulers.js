/**
 * Band rulers and the mouse line.
 * The top ruler is the first row of the first measured band and
 * measures the content width from 0. The role strip sits under
 * it, in the content column. Each band has its own left ruler.
 * That ruler starts at 0 at the top of the band body, flush
 * under the corner on the first band, and runs for that band's
 * height. One inch is 96 CSS pixels. Inches tick at
 * eighths and label each inch. Centimetres tick at millimetres
 * and label each centimetre. Millimetres label every 10.
 * Points label every 72. CSS pixels label every 96. A half step
 * is a longer tick. Tick lengths are three quarters of the first
 * bar. Seven pixels remain outside the major tick. A major label
 * sits beside that tick, and the origin stays unlabeled. The
 * corner cycles the ruler among inches, centimetres, and points.
 * The file keeps its own unit. The mouse line and a drag span
 * are chrome. The definition gains no band.
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
 * Ticks for one edge. A unit the model rejects sets `error`.
 * `chosen` selects the corner cycle without writing the file.
 * @param {object} page
 * @param {number} lengthPx
 * @param {string} [chosen]
 * @returns {{ unit: string, ticks: Array<object> }|{ error: string }}
 */
export function rulerAxis(page, lengthPx, chosen) {
  const stored = rulerUnit(page);
  if (stored.error) return stored;
  const unit = rulerDisplayUnit(stored, chosen);
  if (!SCALES[unit]) return { error: `Unknown unit "${unit}".` };
  if (!(lengthPx > 0)) return { error: 'Length could not be measured.' };
  return { unit, ticks: rulerTicks(lengthPx, unit) };
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

function spanId(key) {
  return `reports-ruler-span-${key}`;
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

function spanLayer(axis, key) {
  const defs = svgEl('defs');
  const clip = svgEl('clipPath');
  clip.id = spanId(key);
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

function rulerSvg(className, ticks, axis, lengthPx, key, unit) {
  const svg = svgEl('svg');
  svg.classList.add('reports-ruler', className);
  svg.setAttribute('aria-hidden', 'true');
  if (unit) svg.dataset.unit = unit;
  const along = axis === 'x' ? lengthPx : RULER_SIZE;
  const across = axis === 'x' ? RULER_SIZE : lengthPx;
  svg.setAttribute('width', String(along));
  svg.setAttribute('height', String(across));
  svg.setAttribute('viewBox', `0 0 ${along} ${across}`);
  // The bar is border-box. A 1px edge border shrinks the SVG
  // content box. Default `meet` then scales the ticks and
  // centers them, so 0 drifts as the bar gets longer. `none`
  // keeps the origin on the content edge and the along-axis
  // at 1 user unit per CSS pixel.
  svg.setAttribute('preserveAspectRatio', 'none');
  const span = spanLayer(axis, key);
  const marks = svgEl('g');
  marks.classList.add('reports-ruler-marks');
  drawTicks(marks, ticks, axis);
  const invert = marks.cloneNode(true);
  invert.classList.add('reports-ruler-marks-invert');
  invert.setAttribute('clip-path', `url(#${spanId(key)})`);
  svg.append(span.defs, span.bar, marks, invert);
  return svg;
}

/**
 * Grow or shrink one ruler while a band edge is dragged.
 * The view stays 1 user unit per CSS pixel, and the ticks are
 * drawn again for the new length so the grown region is ruled
 * before the pointer stops.
 * @param {SVGElement|null} svg
 * @param {number} lengthPx
 */
export function setRulerLength(svg, lengthPx) {
  if (!svg || !(lengthPx > 0)) return;
  const across = svg.classList.contains('reports-ruler-top');
  if (across) {
    svg.setAttribute('width', String(lengthPx));
    svg.setAttribute('viewBox', `0 0 ${lengthPx} ${RULER_SIZE}`);
    svg.style.width = `${lengthPx}px`;
  } else {
    svg.setAttribute('height', String(lengthPx));
    svg.setAttribute('viewBox', `0 0 ${RULER_SIZE} ${lengthPx}`);
    svg.style.height = `${lengthPx}px`;
  }
  const unit = svg.dataset.unit;
  if (!unit) return;
  const marks = svg.querySelector('.reports-ruler-marks');
  const invert = svg.querySelector('.reports-ruler-marks-invert');
  if (!marks) return;
  marks.replaceChildren();
  drawTicks(marks, rulerTicks(lengthPx, unit), across ? 'x' : 'y');
  if (!invert) return;
  invert.replaceChildren(...[...marks.childNodes].map((node) => node.cloneNode(true)));
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
 * Kept for a ruler drawn as one pair. The designer uses
 * `attachBandRulers`. `chosen` is the corner cycle.
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
    rulerSvg('reports-ruler-top', spec.x, 'x', spec.widthPx, 'x', spec.unit),
    rulerSvg('reports-ruler-left', spec.y, 'y', spec.heightPx, 'y', spec.unit),
    cursorLine('reports-cursor-x'),
    cursorLine('reports-cursor-y'),
  );
  return { unit: spec.unit };
}

function clearRulerSlots(page) {
  page.style.setProperty('--reports-ruler-size', '0px');
  for (const band of page.querySelectorAll('.reports-band')) {
    band.classList.add('reports-band-plain');
    band.classList.remove('reports-band-headed');
    band.querySelector('.reports-band-ruler-slot')?.remove();
    band.querySelector('.reports-band-ruler-top')?.remove();
    band.querySelector('.reports-band-ruler-pad')?.remove();
  }
}

/**
 * Put the top ruler above the first measured band's descriptor
 * and a left ruler on every measured band. Each left ruler
 * starts at 0 at that band's body. A bad unit removes the
 * slots and sets `error`.
 * @param {HTMLElement} page
 * @param {object} docPage
 * @param {number} contentWidthPx
 * @param {string} [chosen]
 * @returns {{ error?: string, unit?: string }}
 */
export function attachBandRulers(page, docPage, contentWidthPx, chosen) {
  const across = rulerAxis(docPage, contentWidthPx, chosen);
  if (across.error) {
    clearRulerSlots(page);
    return across;
  }
  let topped = false;
  let lefts = 0;
  for (const band of page.querySelectorAll('.reports-band')) {
    const slot = band.querySelector('.reports-band-ruler-slot');
    if (!slot) continue;
    const body = band.querySelector('.reports-band-body');
    const height = body
      ? Number.parseFloat(body.style.getPropertyValue('--reports-band-height'))
      : NaN;
    const down = rulerAxis(docPage, height, chosen);
    if (down.error) {
      slot.remove();
      continue;
    }
    if (!topped) {
      const top = document.createElement('div');
      top.className = 'reports-band-ruler-top';
      const strip = band.querySelector('.reports-band-strip');
      const measure = band.querySelector('.reports-band-measure');
      const pad = document.createElement('div');
      pad.className = 'reports-band-ruler-pad';
      pad.setAttribute('aria-hidden', 'true');
      band.insertBefore(top, strip || measure);
      band.insertBefore(pad, strip || measure);
      band.classList.add('reports-band-headed');
      top.append(
        cornerButton(across.unit),
        rulerSvg('reports-ruler-top', across.ticks, 'x', contentWidthPx, 'x', across.unit),
      );
      topped = true;
    }
    const key = lefts === 0 ? 'y' : `y-${lefts}`;
    lefts += 1;
    slot.replaceWith(rulerSvg('reports-ruler-left', down.ticks, 'y', height, key, down.unit));
  }
  return { unit: across.unit };
}

function sheetOf(node) {
  if (!node) return null;
  if (node.classList && node.classList.contains('reports-sheet')) return node;
  return node.closest ? node.closest('.reports-sheet') : null;
}

function contentWidthPx(sheet) {
  const page = sheet.querySelector ? sheet.querySelector('#reports-page') : null;
  const raw = (page || sheet).style.getPropertyValue('--reports-content-width');
  return Number.parseFloat(raw);
}

/**
 * Move the crosshair. `x` is CSS pixels from the content's left
 * edge. `y` is CSS pixels from the top of `band`'s body. The
 * vertical line crosses the whole stack. The horizontal line
 * stays on that band.
 * @param {HTMLElement} page
 * @param {number} x
 * @param {number|null} y
 * @param {HTMLElement|null} [band]
 */
export function placeReportCursor(page, x, y, band) {
  const sheet = sheetOf(page);
  if (!sheet) return;
  const vertical = sheet.querySelector('.reports-cursor-x');
  if (vertical) {
    const width = contentWidthPx(sheet);
    const inside = Number.isFinite(x) && x >= 0 && (!(width > 0) || x <= width);
    vertical.hidden = !inside;
    if (inside) vertical.style.left = `${RULER_SIZE + x}px`;
  }
  for (const line of sheet.querySelectorAll('.reports-cursor-y')) {
    const owner = line.closest('.reports-band');
    const show = Boolean(band) && owner === band && Number.isFinite(y);
    line.hidden = !show;
    if (show) line.style.top = `${y}px`;
  }
}

/**
 * Item box in page pixels. `itemRect` and `pageRect` are viewport boxes.
 * The result is unscaled CSS pixels from the page origin.
 * @param {{ left: number, top: number, width: number, height: number }} itemRect
 * @param {{ left: number, top: number }} pageRect
 * @param {number} scale
 * @returns {{ x0: number, x1: number, y0: number, y1: number }}
 */
export function pageSpan(itemRect, pageRect, scale) {
  const factor = scale > 0 ? scale : 1;
  const left = (itemRect.left - pageRect.left) / factor;
  const top = (itemRect.top - pageRect.top) / factor;
  return {
    x0: left,
    x1: left + itemRect.width / factor,
    y0: top,
    y1: top + itemRect.height / factor,
  };
}

function bandBodyFrom(node) {
  if (!node || !node.closest) return null;
  if (node.classList && node.classList.contains('reports-band-body')) return node;
  const ruler = node.closest('.reports-ruler-left');
  if (ruler) {
    const band = ruler.closest('.reports-band');
    return band ? band.querySelector('.reports-band-body') : null;
  }
  return node.closest('.reports-band-body');
}

/**
 * Move the crosshair to the pointer. `x` is measured from the
 * content edge. `y` is measured from the band body under the
 * pointer, or from `origin` when a drag owns the pointer.
 * A missing coordinate does nothing.
 * @param {HTMLElement} page
 * @param {{ clientX?: number, clientY?: number, target?: EventTarget }} event
 * @param {HTMLElement} [origin]
 */
export function trackReportCursor(page, event, origin) {
  const sheet = sheetOf(page);
  if (!sheet || !event || typeof sheet.getBoundingClientRect !== 'function') return;
  const rect = sheet.getBoundingClientRect();
  const scale = readViewScale(page);
  const factor = scale > 0 ? scale : 1;
  const x = (event.clientX - rect.left) / factor - RULER_SIZE;
  if (!Number.isFinite(x)) return;
  const body = bandBodyFrom(origin) || bandBodyFrom(event.target);
  let band = null;
  let y = null;
  if (body && typeof body.getBoundingClientRect === 'function') {
    const bodyRect = body.getBoundingClientRect();
    const next = (event.clientY - bodyRect.top) / factor;
    const owner = body.closest('.reports-band');
    const stored = Number.parseFloat(body.style.getPropertyValue('--reports-band-height'));
    const max = bodyRect.height > 0 ? bodyRect.height / factor : stored;
    if (Number.isFinite(next) && next >= 0 && (!(max > 0) || next <= max)) {
      y = next;
      band = owner;
    }
  }
  placeReportCursor(page, x, y, band);
}

function spanTargets(node, axis) {
  const sheet = sheetOf(node);
  if (!sheet) return [];
  if (axis === 'x') return [...sheet.querySelectorAll('.reports-ruler-top')];
  const band = node && node.closest ? node.closest('.reports-band') : null;
  const root = band || sheet;
  return [...root.querySelectorAll('.reports-ruler-left')];
}

/**
 * Invert one slice of a ruler. `fromPx` and `toPx` are CSS pixels
 * from that ruler's own origin. The horizontal slice is the top
 * ruler. The vertical slice is the left ruler of the band that
 * holds `node`, or every left ruler when `node` is the page.
 * A zero span draws nothing.
 * @param {HTMLElement} node
 * @param {'x'|'y'} axis
 * @param {number} fromPx
 * @param {number} toPx
 */
export function showRulerSpan(node, axis, fromPx, toPx) {
  if (!node || (axis !== 'x' && axis !== 'y')) return;
  if (!Number.isFinite(fromPx) || !Number.isFinite(toPx)) return;
  const start = Math.min(fromPx, toPx);
  const size = Math.abs(toPx - fromPx);
  for (const svg of spanTargets(node, axis)) {
    const clip = svg.querySelector('.reports-ruler-span-clip');
    const bar = svg.querySelector('.reports-ruler-span');
    if (!clip || !bar) continue;
    setSpanBox(clip, axis, start, size);
    setSpanBox(bar, axis, start, size);
  }
}

/** Clear a ruler span. An omitted axis clears both edges. */
export function hideRulerSpan(node, axis) {
  const axes = axis ? [axis] : ['x', 'y'];
  for (const name of axes) showRulerSpan(node, name, 0, 0);
}

/** Hide the crosshair. */
export function hideReportCursor(page) {
  const sheet = sheetOf(page);
  if (!sheet) return;
  const vertical = sheet.querySelector('.reports-cursor-x');
  if (vertical) vertical.hidden = true;
  for (const line of sheet.querySelectorAll('.reports-cursor-y')) line.hidden = true;
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
    const sheet = sheetOf(page) || page;
    const rect = sheet.getBoundingClientRect();
    const scale = readViewScale(page);
    const factor = scale > 0 ? scale : 1;
    const x = (event.clientX - rect.left) / factor - RULER_SIZE;
    const ySheet = (event.clientY - rect.top) / factor;
    const width = contentWidthPx(sheet);
    const height = rect.height / factor;
    const pastRight = width > 0 ? x > width : x > rect.width / factor;
    if (x < -RULER_SIZE || ySheet < 0 || pastRight || (height > 0 && ySheet > height)) {
      hideReportCursor(page);
      return;
    }
    trackReportCursor(page, event);
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
