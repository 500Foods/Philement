/**
 * Report page surface.
 * Unzoomed scale is 96 CSS pixels per inch. Margin guides are the
 * inset. Bands stack inside that inset. Each band body carries the
 * section grid. Rulers sit on the top and left of the sheet.
 * `--reports-scale` on the canvas scales that sheet together.
 */

import { paintReportBands } from './bands.js';
import { gridSpec } from './grid.js';
import { pageBox } from './model/units.js';
import { RULER_SIZE, paintReportRulers } from './rulers.js';
import { formatViewScale } from './scale.js';

function pageObject(doc) {
  const page = doc && doc.page;
  if (!page || typeof page !== 'object' || Array.isArray(page)) return null;
  return page;
}

/**
 * Pixel frame for the open page. An omitted page has no box.
 * A length the model cannot measure sets `error`.
 * @param {object} doc
 * @returns {{ omitted: boolean, error?: string, widthPx?: number, heightPx?: number, leftPx?: number, rightPx?: number, topPx?: number, bottomPx?: number }}
 */
export function pageFrame(doc) {
  const page = pageObject(doc);
  if (!page) return { omitted: true };
  if (typeof page.width !== 'string' || page.width === '') return { omitted: true };
  if (typeof page.height !== 'string' || page.height === '') return { omitted: true };
  try {
    return { omitted: false, ...pageBox(page) };
  } catch (error) {
    const message = error instanceof Error && error.message
      ? error.message
      : 'Page could not be measured.';
    return { omitted: false, error: message };
  }
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

/**
 * Paint one page into the host. Size and insets are CSS variables.
 * The host carries `--reports-scale`. 1 is 96 CSS pixels per inch.
 * @param {HTMLElement} host
 * @param {object} doc
 * @param {string} [rulerUnit]
 * @param {number} [scale]
 * @returns {ReturnType<typeof pageFrame>}
 */
export function paintReportPage(host, doc, rulerUnit, scale) {
  const frame = pageFrame(doc);
  host.replaceChildren();
  host.classList.remove('reports-canvas-fitted');
  host.style.removeProperty('--reports-page-width');
  host.style.removeProperty('--reports-page-height');
  host.style.removeProperty('--reports-ruler-size');
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
  page.style.setProperty('--reports-page-width', px(frame.widthPx));
  page.style.setProperty('--reports-page-height', px(frame.heightPx));
  page.style.setProperty('--reports-margin-left', px(frame.leftPx));
  page.style.setProperty('--reports-margin-right', px(frame.rightPx));
  page.style.setProperty('--reports-margin-top', px(frame.topPx));
  page.style.setProperty('--reports-margin-bottom', px(frame.bottomPx));
  const grid = gridSpec(doc);
  paintReportBands(page, doc, grid);
  if (grid.error) frame.gridError = grid.error;
  const guide = document.createElement('div');
  guide.className = 'reports-margin-guide';
  page.append(guide);

  const sheet = document.createElement('div');
  sheet.className = 'reports-sheet';
  sheet.id = 'reports-sheet';
  sheet.style.setProperty('--reports-page-width', px(frame.widthPx));
  sheet.style.setProperty('--reports-page-height', px(frame.heightPx));
  const rulers = paintReportRulers(sheet, pageObject(doc), frame, rulerUnit);
  if (rulers.error) frame.rulerError = rulers.error;
  if (grid.error) host.append(note(grid.error));
  if (rulers.error) host.append(note(rulers.error));
  if (rulers.error) host.append(page);
  else {
    sheet.append(page);
    host.style.setProperty('--reports-page-width', px(frame.widthPx));
    host.style.setProperty('--reports-page-height', px(frame.heightPx));
    host.style.setProperty('--reports-ruler-size', `${RULER_SIZE}px`);
    host.style.setProperty('--reports-scale', formatViewScale(scale));
    host.classList.add('reports-canvas-fitted');
    host.append(sheet);
  }
  return frame;
}
