/**
 * Report page surface.
 * Unzoomed scale is 96 CSS pixels per inch. Margin guides are the
 * inset. Bands, the grid, and rulers are later phases.
 */

import { pageBox } from './model/units.js';

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
 * @param {HTMLElement} host
 * @param {object} doc
 * @returns {ReturnType<typeof pageFrame>}
 */
export function paintReportPage(host, doc) {
  const frame = pageFrame(doc);
  host.replaceChildren();
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
  const guide = document.createElement('div');
  guide.className = 'reports-margin-guide';
  page.append(guide);
  host.append(page);
  return frame;
}
