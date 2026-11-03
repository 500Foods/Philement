/**
 * Report design surface.
 * Unzoomed scale is 96 CSS pixels per inch. The surface is a stack
 * of bands at the content width. The paper height stays off it.
 * The first measured band carries the top ruler above its
 * descriptor. Every measured band carries a left ruler that
 * starts at 0. Each band body
 * carries the section grid. `--reports-scale` on the canvas scales
 * that stack together. The End of Report marker closes the stack: a
 * strip with no page height. Each ruler side can be turned off; a
 * bottom ruler sits under the marker and a right ruler beside each band.
 */

import { BAND_STRIP_PX, paintReportBands } from './bands.js';
import { gridSpec } from './grid.js';
import { contentWidthPx, pageBox } from './model/units.js';
import { CLASSIC_SIDES, RULER_SIZE, attachBandRulers } from './rulers.js';
import { formatViewScale } from './scale.js';

function pageObject(doc) {
  const page = doc && doc.page;
  if (!page || typeof page !== 'object' || Array.isArray(page)) return null;
  return page;
}

/**
 * Pixel frame for the open page. An omitted page has no box.
 * `contentWidthPx` is the design width, inside the left and right
 * margins. A length the model cannot measure sets `error`.
 * @param {object} doc
 * @returns {{ omitted: boolean, error?: string, widthPx?: number, heightPx?: number, contentWidthPx?: number, leftPx?: number, rightPx?: number, topPx?: number, bottomPx?: number }}
 */
export function pageFrame(doc) {
  const page = pageObject(doc);
  if (!page) return { omitted: true };
  if (typeof page.width !== 'string' || page.width === '') return { omitted: true };
  if (typeof page.height !== 'string' || page.height === '') return { omitted: true };
  try {
    const box = pageBox(page);
    const widthPx = contentWidthPx(page);
    if (!(widthPx > 0)) return { omitted: false, error: 'The content box has no width.' };
    return { omitted: false, ...box, contentWidthPx: widthPx };
  } catch (error) {
    const message = error instanceof Error && error.message
      ? error.message
      : 'Page could not be measured.';
    return { omitted: false, error: message };
  }
}

/**
 * Unscaled height of the band stack. The top ruler is counted once,
 * on the first measured band.
 * @param {Array<{ error?: string, heightPx?: number }>} frames
 * @param {boolean} ruled
 * @returns {number}
 */
export function designStackHeight(frames, ruled) {
  let height = 0;
  let topped = false;
  for (const frame of frames) {
    height += BAND_STRIP_PX;
    if (!frame || frame.error || !(frame.heightPx > 0)) {
      if (frame && frame.error) height += BAND_STRIP_PX;
      continue;
    }
    if (ruled && !topped) {
      height += RULER_SIZE;
      topped = true;
    }
    height += frame.heightPx;
  }
  return height;
}

function note(text) {
  const el = document.createElement('p');
  el.className = 'reports-page-note';
  el.textContent = text;
  return el;
}

function px(value) {
  return `${value}px`;
}

/** CSS class of the End of Report marker. It is not a `.reports-band`. */
export const END_MARKER_CLASS = 'reports-band-end';

/** Strip-only marker after the last band. No page height. */
export function endMarker() {
  const end = document.createElement('section');
  end.className = END_MARKER_CLASS;
  end.setAttribute('aria-label', 'End of Report');
  const strip = document.createElement('div');
  strip.className = 'reports-band-strip';
  const name = document.createElement('span');
  name.className = 'reports-band-role';
  name.textContent = 'End of Report';
  strip.append(name);
  end.append(strip);
  return end;
}

function applyDesignVars(node, frame, stackPx, ruled, sides = CLASSIC_SIDES) {
  node.style.setProperty('--reports-content-width', px(frame.contentWidthPx));
  node.style.setProperty('--reports-stack-height', px(stackPx));
  node.style.setProperty('--reports-band-strip', px(BAND_STRIP_PX));
  // `--reports-ruler-size` stays the left column (0 when off); the bar
  // thickness of the top and bottom rulers is `--reports-ruler-bar`.
  node.style.setProperty('--reports-ruler-size', ruled && sides.left ? `${RULER_SIZE}px` : '0px');
  node.style.setProperty('--reports-ruler-right', ruled && sides.right ? `${RULER_SIZE}px` : '0px');
  node.style.setProperty('--reports-ruler-bar', `${RULER_SIZE}px`);
}

/**
 * Paint the band stack into the host. The width is the content box.
 * The host carries `--reports-scale`. 1 is 96 CSS pixels per inch.
 * @param {HTMLElement} host
 * @param {object} doc
 * @param {string} [rulerUnit]
 * @param {number} [scale]
 * @param {{ top: boolean, left: boolean, right: boolean, bottom: boolean }} [sides]
 * @returns {ReturnType<typeof pageFrame>}
 */
export function paintReportPage(host, doc, rulerUnit, scale, sides = CLASSIC_SIDES) {
  const frame = pageFrame(doc);
  host.replaceChildren();
  host.classList.remove('reports-canvas-fitted');
  host.style.removeProperty('--reports-page-width');
  host.style.removeProperty('--reports-page-height');
  host.style.removeProperty('--reports-content-width');
  host.style.removeProperty('--reports-stack-height');
  host.style.removeProperty('--reports-ruler-size');
  host.style.removeProperty('--reports-ruler-right');
  host.style.removeProperty('--reports-ruler-bar');
  host.style.removeProperty('--reports-scale');
  if (frame.omitted) {
    host.append(note('The file omits the page.'));
    return frame;
  }
  if (frame.error) {
    host.append(note(frame.error));
    return frame;
  }
  const page = document.createElement('div');
  page.className = 'reports-page';
  page.id = 'reports-page';
  const grid = gridSpec(doc);
  const frames = paintReportBands(page, doc, grid);
  if (grid.error) frame.gridError = grid.error;
  page.append(endMarker());
  const rulers = attachBandRulers(page, pageObject(doc), frame.contentWidthPx, rulerUnit, sides);
  if (rulers.error) frame.rulerError = rulers.error;
  const ruled = !rulers.error;
  const stackPx = designStackHeight(frames, ruled && sides.top)
    + BAND_STRIP_PX
    + (ruled && sides.bottom ? RULER_SIZE : 0);
  applyDesignVars(page, frame, stackPx, ruled, sides);
  page.classList.toggle('reports-page-bottom-ruled', ruled && Boolean(sides.bottom));
  if (grid.error) host.append(note(grid.error));
  if (rulers.error) host.append(note(rulers.error));
  if (rulers.error) {
    host.append(page);
    return frame;
  }
  const sheet = document.createElement('div');
  sheet.className = 'reports-sheet';
  sheet.id = 'reports-sheet';
  applyDesignVars(sheet, frame, stackPx, true, sides);
  const cursor = document.createElement('div');
  cursor.className = 'reports-cursor reports-cursor-x';
  cursor.hidden = true;
  sheet.append(page, cursor);
  applyDesignVars(host, frame, stackPx, true, sides);
  host.style.setProperty('--reports-scale', formatViewScale(scale));
  host.classList.add('reports-canvas-fitted');
  host.append(sheet);
  return frame;
}
