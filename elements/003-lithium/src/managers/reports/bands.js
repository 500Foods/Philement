/**
 * Report bands on the page.
 * Each band is a label strip plus a body of the stored height, in
 * array order. The strip is chrome. The body is the stored height.
 * Drag preview uses element.style.height. The resting height is a
 * CSS variable.
 */

import { paintBandGrid } from './grid.js';
import { bandHeight } from './model/document.js';
import { toCssPx } from './model/units.js';
import { bandRoleName } from './reports-file.js';

/**
 * Pixel height while a band edge is dragged. The floor is 0.15 in.
 * @param {number} startPx
 * @param {number} deltaPx
 * @returns {number}
 */
export function dragBandHeight(startPx, deltaPx) {
  const minPx = toCssPx('0.15 in');
  const next = startPx + deltaPx;
  if (!Number.isFinite(next) || next < minPx) return minPx;
  return next;
}

function frameFor(band) {
  const entry = band && typeof band === 'object' && !Array.isArray(band) ? band : null;
  const role = bandRoleName(entry ? entry.typeKey : undefined);
  const id = entry && typeof entry.id === 'string' ? entry.id : '';
  if (!entry) return { id, role, error: 'Each band must be an object.' };
  try {
    const height = bandHeight(entry);
    return {
      id, role, height, heightPx: toCssPx(height),
    };
  } catch (error) {
    const message = error instanceof Error && error.message
      ? error.message
      : 'Band could not be measured.';
    return { id, role, error: message };
  }
}

/**
 * One frame per band, in array order. An omitted height uses the
 * Defaults table and is not written. A length that cannot be measured
 * sets `error` and does not invent a body.
 * @param {object} doc
 * @returns {Array<object>}
 */
export function bandFrames(doc) {
  const bands = Array.isArray(doc && doc.bands) ? doc.bands : [];
  return bands.map(frameFor);
}

function roleLabel(frame) {
  const strip = document.createElement('div');
  strip.className = 'reports-band-strip';
  const role = document.createElement('span');
  role.className = 'reports-band-role';
  role.textContent = frame.role;
  strip.append(role);
  if (frame.id) {
    const id = document.createElement('span');
    id.className = 'reports-band-id';
    id.textContent = frame.id;
    strip.append(id);
  }
  return strip;
}

/**
 * Paint the bands into the page. The caller draws the margin guide
 * after this so the guide stays on top.
 * @param {HTMLElement} page
 * @param {object} doc
 * @param {object} [grid]
 * @returns {ReturnType<typeof bandFrames>}
 */
export function paintReportBands(page, doc, grid) {
  const frames = bandFrames(doc);
  const stack = document.createElement('div');
  stack.className = 'reports-bands';
  frames.forEach((frame, index) => {
    const band = document.createElement('section');
    band.className = 'reports-band';
    if (frame.id) band.dataset.bandId = frame.id;
    band.append(roleLabel(frame));
    if (frame.error) {
      const note = document.createElement('p');
      note.className = 'reports-band-note';
      note.textContent = frame.error;
      band.append(note);
    } else {
      const body = document.createElement('div');
      body.className = 'reports-band-body';
      body.style.setProperty('--reports-band-height', `${frame.heightPx}px`);
      if (grid) paintBandGrid(body, grid, `reports-grid-${index}`);
      if (frame.id) {
        const edge = document.createElement('div');
        edge.className = 'reports-band-edge';
        edge.title = 'Resize band';
        body.append(edge);
      }
      band.append(body);
    }
    stack.append(band);
  });
  page.append(stack);
  return frames;
}
