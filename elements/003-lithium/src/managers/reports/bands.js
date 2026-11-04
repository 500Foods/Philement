/**
 * Report bands.
 * Each band is a label strip plus a body of the stored height, in
 * array order. The strip is chrome. On the first measured band
 * the top ruler is inserted above it. The body is the stored
 * height. A left-ruler slot sits beside the body and starts at 0.
 * Drag preview uses element.style.height. The resting height is
 * a CSS variable on the body and on the measure, so the left
 * ruler inherits it. The band section does not carry it.
 */

/** Height of the role strip, in CSS pixels. The left ruler starts below it. */
export const BAND_STRIP_PX = 22;

import { paintBandGrid } from './grid.js';
import { paintBandItems } from './items.js';
import { GROUP_HEADER_TYPE, GROUP_FOOTER_TYPE, bandHeight, canRemoveBand, groupPartner } from './model/document.js';
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

function frameFor(band, doc) {
  const entry = band && typeof band === 'object' && !Array.isArray(band) ? band : null;
  const role = bandRoleName(entry ? entry.typeKey : undefined);
  const id = entry && typeof entry.id === 'string' ? entry.id : '';
  if (!entry) return { id, role, error: 'Each band must be an object.' };
  let field = '';
  if (entry.typeKey === GROUP_HEADER_TYPE) field = typeof entry.field === 'string' ? entry.field : '';
  else if (entry.typeKey === GROUP_FOOTER_TYPE) {
    const header = groupPartner(doc, id);
    field = header && typeof header.field === 'string' ? header.field : '';
  }
  try {
    const height = bandHeight(entry);
    return {
      id, role, field, deletable: canRemoveBand(doc, id), height, heightPx: toCssPx(height),
    };
  } catch (error) {
    const message = error instanceof Error && error.message
      ? error.message
      : 'Band could not be measured.';
    return { id, role, field, deletable: canRemoveBand(doc, id), error: message };
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
  return bands.map((band) => frameFor(band, doc));
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
  if (frame.field) {
    const field = document.createElement('span');
    field.className = 'reports-band-field';
    field.textContent = frame.field;
    strip.append(field);
  }
  if (frame.id && frame.deletable !== false) {
    const del = document.createElement('button');
    del.type = 'button';
    del.className = 'reports-band-delete';
    del.title = 'Delete band';
    del.setAttribute('aria-label', `Delete band ${frame.id}`);
    del.innerHTML = '<fa fa-trash-can></fa>';
    strip.append(del);
  }
  return strip;
}

function measureRow(frame, index, doc, grid) {
  const band = document.createElement('section');
  band.className = 'reports-band';
  if (frame.id) band.dataset.bandId = frame.id;
  band.append(roleLabel(frame));
  if (frame.error) {
    const note = document.createElement('p');
    note.className = 'reports-band-note';
    note.textContent = frame.error;
    band.append(note);
    return band;
  }
  const measure = document.createElement('div');
  measure.className = 'reports-band-measure';
  measure.style.setProperty('--reports-band-height', `${frame.heightPx}px`);
  const slot = document.createElement('div');
  slot.className = 'reports-band-ruler-slot';
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
  const source = Array.isArray(doc.bands) ? doc.bands[index] : null;
  if (source) paintBandItems(body, source, doc);
  const cursor = document.createElement('div');
  cursor.className = 'reports-cursor reports-cursor-y';
  cursor.hidden = true;
  measure.append(slot, body, cursor);
  band.append(measure);
  return band;
}

/**
 * Paint the bands into the page, in array order. Each body is the
 * stored height. The ruler slots are filled by the caller.
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
    stack.append(measureRow(frame, index, doc, grid));
  });
  page.append(stack);
  return frames;
}
