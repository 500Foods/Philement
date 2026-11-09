/**
 * Fore / Back / Background colours (Phase 77).
 * backgroundColor is shared by every Kind. Shape fillPaint colours
 * migrate onto backgroundColor at load; fillStyle and gradient stops stay.
 * Canonical empty colour is the string "none" (transparent / no paint).
 */

const HEX = /^#(?:[0-9A-Fa-f]{3}|[0-9A-Fa-f]{4}|[0-9A-Fa-f]{6}|[0-9A-Fa-f]{8})$/;
const SHAPE_TYPES = new Set([25, 26]);

/** Stored value for no colour / transparent. */
export const NONE_COLOR = 'none';

/** @param {unknown} value */
export function isHexColor(value) {
  return typeof value === 'string' && HEX.test(value.trim());
}

/** @param {unknown} value */
export function isNoneColor(value) {
  if (value == null) return false;
  const text = String(value).trim().toLowerCase();
  return text === NONE_COLOR || text === 'transparent';
}

/**
 * Accept hex, "none", "transparent", or empty → canonical hex or "none".
 * @param {unknown} value
 * @returns {string} lowercase #rrggbb or "none"
 */
export function normalizeColorValue(value) {
  const text = String(value ?? '').trim();
  if (text === '' || text.toLowerCase() === 'transparent' || text.toLowerCase() === NONE_COLOR) {
    return NONE_COLOR;
  }
  if (!HEX.test(text)) throw new Error('Colour must be a hex color or none.');
  return text.toLowerCase();
}

/** @param {unknown} value @param {string} label */
export function requireHexColor(value, label) {
  try {
    const next = normalizeColorValue(value);
    if (next === NONE_COLOR) return NONE_COLOR;
    return next;
  } catch {
    throw new Error(`${label} must be a hex color or none.`);
  }
}

/**
 * Colour to copy from a legacy shape fillPaint onto backgroundColor.
 * @param {object|null|undefined} fillPaint
 * @returns {string|null}
 */
export function backgroundColorFromFillPaint(fillPaint) {
  if (!fillPaint || typeof fillPaint !== 'object' || Array.isArray(fillPaint)) return null;
  if (typeof fillPaint.color === 'string' && HEX.test(fillPaint.color)) {
    return fillPaint.color.toLowerCase();
  }
  const stops = Array.isArray(fillPaint.stops) ? fillPaint.stops : null;
  const first = stops && stops[0];
  if (first && typeof first.color === 'string' && HEX.test(first.color)) {
    return first.color.toLowerCase();
  }
  return null;
}

/**
 * In-place: set backgroundColor from fillPaint when missing (shapes only).
 * @param {object} item
 * @returns {object} item
 */
export function normalizeShapeBackground(item) {
  if (!item || typeof item !== 'object' || Array.isArray(item)) return item;
  const typeKey = Number(item.typeKey);
  if (!SHAPE_TYPES.has(typeKey)) return item;
  if (typeof item.backgroundColor === 'string' && item.backgroundColor !== '') return item;
  const color = backgroundColorFromFillPaint(item.fillPaint);
  if (color) item.backgroundColor = color;
  return item;
}

/**
 * Clone-safe normalize of every item in a definition.
 * @param {object} doc
 * @returns {object}
 */
export function normalizeDefinitionColours(doc) {
  if (!doc || typeof doc !== 'object' || Array.isArray(doc)) return doc;
  const next = JSON.parse(JSON.stringify(doc));
  const bands = Array.isArray(next.bands) ? next.bands : [];
  for (const band of bands) {
    const items = Array.isArray(band.items) ? band.items : [];
    for (const item of items) normalizeShapeBackground(item);
  }
  return next;
}

/**
 * Effective fill colour for a shape: backgroundColor, else fillPaint.
 * Returns null when the colour is none (no fill).
 * @param {object|null|undefined} item
 * @param {string|null} [fallback='#d3d3d3']
 */
export function shapeFillColor(item, fallback = '#d3d3d3') {
  if (item && isNoneColor(item.backgroundColor)) return null;
  if (item && typeof item.backgroundColor === 'string' && HEX.test(item.backgroundColor)) {
    return item.backgroundColor.toLowerCase();
  }
  const fromPaint = backgroundColorFromFillPaint(item && item.fillPaint);
  if (fromPaint) return fromPaint;
  return fallback;
}

/**
 * Apply CSS colour variables on a painted item node.
 * "none" clears the variable so the face stays transparent / default ink
 * is not forced; callers that need transparent ink set transparent explicitly.
 * @param {HTMLElement|null|undefined} el
 * @param {object|null|undefined} item
 */
/**
 * Build the text face: block .reports-item-text wrapping an inline
 * .reports-item-text-ink so Back Color highlights glyphs only
 * (matches Render.Html item_back_span), not the full line box.
 * @param {string} text
 * @returns {HTMLSpanElement}
 */
export function makeItemTextLabel(text) {
  const label = document.createElement('span');
  label.className = 'reports-item-text';
  const ink = document.createElement('span');
  ink.className = 'reports-item-text-ink';
  ink.textContent = text == null ? '' : String(text);
  label.append(ink);
  return label;
}

export function applyItemColourVars(el, item) {
  if (!el || !item) return;
  const setOn = (node, prop, raw, paintNoneAs) => {
    if (isNoneColor(raw)) {
      node.style.setProperty(prop, paintNoneAs);
      return;
    }
    if (isHexColor(raw)) {
      node.style.setProperty(prop, String(raw).trim().toLowerCase());
      return;
    }
    node.style.removeProperty(prop);
  };
  setOn(el, '--reports-item-fore-color', item.foreColor, 'transparent');
  setOn(el, '--reports-item-back-color', item.backColor, 'transparent');
  setOn(el, '--reports-item-background-color', item.backgroundColor, 'transparent');
  // Back Color paints on the inline ink span (Render-matched highlight).
  for (const ink of el.querySelectorAll('.reports-item-text-ink')) {
    setOn(ink, '--reports-item-back-color', item.backColor, 'transparent');
  }
}
