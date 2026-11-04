/**
 * Static text items on a band body.
 * The box is CSS pixels at 96 per inch, before view scale.
 * A move slides the box. A corner resize keeps the top left
 * and changes width and height. Both stay inside the band.
 * A resize is at least 1 CSS pixel when the band has room.
 * Import does not use this clamp. The canvas clips the text
 * to the box. The band does not grow.
 */

import { bandHeight } from './model/document.js';
import { contentBox, toCssPx } from './model/units.js';
import { aggregateItemLabel, isAggregateType } from './reports-aggregate.js';
import { fieldItemLabel, isFieldType, PARAM_TYPE, paramItemLabel, SYSTEM_TYPES, systemItemLabel } from './reports-sample.js';

/** Smallest resize, in CSS pixels. */
export const ITEM_MIN_PX = 1;

const FONT_STYLES = new Set(['Normal', 'Bold', 'Italic', 'Bold + Italic']);
const FAMILY_PATTERN = /^[A-Za-z0-9][A-Za-z0-9 -]*$/;

function finite(value, fallback) {
  const number = Number(value);
  return Number.isFinite(number) ? number : fallback;
}

function clampSpan(origin, size, limit, resize) {
  const roomLimit = limit > 0 ? limit : 0;
  if (resize) {
    let nextOrigin = finite(origin, 0);
    if (nextOrigin < 0) nextOrigin = 0;
    if (nextOrigin > roomLimit) nextOrigin = roomLimit;
    const room = roomLimit - nextOrigin;
    const floor = room >= ITEM_MIN_PX ? ITEM_MIN_PX : room;
    let nextSize = finite(size, floor);
    if (nextSize < floor) nextSize = floor;
    if (nextSize > room) nextSize = room;
    return { origin: nextOrigin, size: nextSize };
  }
  let nextSize = finite(size, 0);
  if (nextSize < 0) nextSize = 0;
  if (nextSize > roomLimit) nextSize = roomLimit;
  let nextOrigin = finite(origin, 0);
  const maxOrigin = Math.max(0, roomLimit - nextSize);
  if (nextOrigin < 0) nextOrigin = 0;
  if (nextOrigin > maxOrigin) nextOrigin = maxOrigin;
  return { origin: nextOrigin, size: nextSize };
}

/**
 * Pixel box after a pointer move. `resize` keeps the top left.
 * @param {{ left: number, top: number, width: number, height: number }} start
 * @param {number} dx
 * @param {number} dy
 * @param {{ width: number, height: number }} bounds Band body, in CSS pixels.
 * @param {boolean} resize
 * @returns {{ left: number, top: number, width: number, height: number }}
 */
export function dragItemBox(start, dx, dy, bounds, resize) {
  const left = resize ? start.left : start.left + finite(dx, 0);
  const top = resize ? start.top : start.top + finite(dy, 0);
  const width = resize ? start.width + finite(dx, 0) : start.width;
  const height = resize ? start.height + finite(dy, 0) : start.height;
  const limitW = bounds && bounds.width > 0 ? bounds.width : 0;
  const limitH = bounds && bounds.height > 0 ? bounds.height : 0;
  const horizontal = clampSpan(left, width, limitW, resize);
  const vertical = clampSpan(top, height, limitH, resize);
  return {
    left: horizontal.origin,
    top: vertical.origin,
    width: horizontal.size,
    height: vertical.size,
  };
}

/**
 * Band body in CSS pixels. Width is the content box. Height is the band.
 * @param {object} doc
 * @param {object} band
 * @returns {{ width: number, height: number }}
 */
export function bandItemBounds(doc, band) {
  const page = doc && doc.page;
  if (!page || typeof page.width !== 'string' || typeof page.height !== 'string') {
    throw new Error('Page needs a width and a height.');
  }
  const units = typeof page.units === 'string' && page.units !== '' ? page.units : 'in';
  const content = contentBox({
    units,
    width: page.width,
    height: page.height,
    margin: page.margin,
  });
  return {
    width: content.widthPx,
    height: toCssPx(bandHeight(band)),
  };
}

/**
 * Band the palette uses when the open report has no selection yet.
 * Detail wins. Otherwise the first band id.
 * @param {object} doc
 * @returns {string}
 */
export function defaultBandId(doc) {
  const bands = Array.isArray(doc && doc.bands) ? doc.bands : [];
  const detail = bands.find((band) => band && band.typeKey === 15 && typeof band.id === 'string' && band.id !== '');
  if (detail) return detail.id;
  const first = bands.find((band) => typeof band.id === 'string' && band.id !== '');
  return first ? first.id : '';
}

/**
 * Stored box, or null when the item is gone.
 * @param {object} doc
 * @param {string} bandId
 * @param {string} itemId
 * @returns {object|null}
 */
export function itemBox(doc, bandId, itemId) {
  const bands = Array.isArray(doc && doc.bands) ? doc.bands : [];
  const band = bands.find((entry) => entry.id === bandId);
  const items = band && Array.isArray(band.items) ? band.items : [];
  const item = items.find((entry) => entry.id === itemId);
  return item && item.box ? item.box : null;
}

/**
 * Text painted for a static item. A missing key reads "Text".
 * A stored string is shown as written, including a blank.
 * @param {object} item
 * @returns {string}
 */
export function staticItemText(item) {
  if (item && typeof item.text === 'string') return item.text;
  return 'Text';
}

function fontFamily(value) {
  if (typeof value !== 'string') return 'Vanadium Sans';
  const trimmed = value.trim();
  if (!FAMILY_PATTERN.test(trimmed)) return 'Vanadium Sans';
  return trimmed;
}

function itemFont(doc, item) {
  const own = item.font && typeof item.font === 'object' && !Array.isArray(item.font) ? item.font : null;
  const report = doc.font && typeof doc.font === 'object' && !Array.isArray(doc.font) ? doc.font : null;
  const source = own || report || {};
  let sizePx = toCssPx('10 pt');
  if (typeof source.size === 'string' && source.size !== '') {
    try {
      sizePx = toCssPx(source.size);
    } catch {
      sizePx = toCssPx('10 pt');
    }
  }
  return {
    family: fontFamily(source.family),
    style: FONT_STYLES.has(source.style) ? source.style : 'Normal',
    sizePx,
  };
}

function measuredBox(item) {
  const box = item.box;
  if (!box || typeof box !== 'object' || Array.isArray(box)) {
    throw new Error(`Item "${item.id}" needs a box.`);
  }
  return {
    left: toCssPx(box.left),
    top: toCssPx(box.top),
    width: toCssPx(box.width),
    height: toCssPx(box.height),
  };
}

function itemNote(text) {
  const note = document.createElement('p');
  note.className = 'reports-band-note';
  note.textContent = text;
  return note;
}

/**
 * Paint static text (typeKey 0) and field items (20, 21, 24) into the
 * band body. A field shows its bound name, or its type caption when
 * unbound. Other types stay in the file.
 * @param {HTMLElement} body
 * @param {object} band
 * @param {object} doc
 */
export function paintBandItems(body, band, doc) {
  const items = Array.isArray(band && band.items) ? band.items : [];
  for (const item of items) {
    if (!item || typeof item !== 'object') continue;
    const aggregate = isAggregateType(item.typeKey);
    if (item.typeKey !== 0 && item.typeKey !== PARAM_TYPE && !SYSTEM_TYPES.includes(item.typeKey) && !isFieldType(item.typeKey) && !aggregate) continue;
    if (typeof item.id !== 'string' || item.id === '') {
      body.append(itemNote('Item needs an id.'));
      continue;
    }
    const el = document.createElement('div');
    el.className = 'reports-item';
    el.dataset.itemId = item.id;
    const field = isFieldType(item.typeKey);
    const param = item.typeKey === PARAM_TYPE;
    const system = SYSTEM_TYPES.includes(item.typeKey);
    if (aggregate) {
      el.classList.add('reports-item-field', 'reports-item-aggregate');
      const bound = (typeof item.field === 'string' && item.field.trim() !== '')
        || (typeof item.function === 'string' && item.function.trim() !== '');
      el.dataset.unbound = String(!bound);
    }
    if (system) el.classList.add('reports-item-field', 'reports-item-system');
    if (field) {
      el.classList.add('reports-item-field');
      el.dataset.unbound = String(!(typeof item.field === 'string' && item.field.trim() !== ''));
    }
    if (param) {
      el.classList.add('reports-item-field', 'reports-item-param');
      el.dataset.unbound = String(!(typeof item.param === 'string' && item.param.trim() !== ''));
    }
    const align = item.align === 'Center' || item.align === 'Right' ? item.align : 'Left';
    el.dataset.align = align;
    const font = itemFont(doc, item);
    el.dataset.fontStyle = font.style;
    el.style.setProperty('--reports-item-font-family', `'${font.family}'`);
    el.style.setProperty('--reports-item-font-size', `${font.sizePx}px`);
    try {
      const box = measuredBox(item);
      el.style.left = `${box.left}px`;
      el.style.top = `${box.top}px`;
      el.style.width = `${box.width}px`;
      el.style.height = `${box.height}px`;
    } catch (error) {
      const message = error instanceof Error && error.message
        ? error.message
        : 'Item could not be measured.';
      body.append(itemNote(message));
      continue;
    }
    const label = document.createElement('span');
    label.className = 'reports-item-text';
    label.textContent = aggregate ? aggregateItemLabel(item) : system ? systemItemLabel(item) : param ? paramItemLabel(item) : field ? fieldItemLabel(item) : staticItemText(item);
    const corner = document.createElement('div');
    corner.className = 'reports-item-corner';
    corner.title = 'Resize item';
    el.append(label, corner);
    body.append(el);
  }
}

/**
 * Mark the selected band and item. Ids that are not on the page are cleared
 * by the caller before this runs.
 * @param {ParentNode} root
 * @param {string} bandId
 * @param {string} itemId
 */
export function markReportSelection(root, bandId, itemId) {
  if (!root || !root.querySelectorAll) return;
  for (const band of root.querySelectorAll('.reports-band')) {
    band.classList.toggle('reports-band-selected', band.dataset.bandId === bandId);
  }
  for (const item of root.querySelectorAll('.reports-item')) {
    const band = item.closest('.reports-band');
    const onBand = Boolean(band && band.dataset.bandId === bandId);
    item.classList.toggle('reports-item-selected', onBand && item.dataset.itemId === itemId);
  }
}
