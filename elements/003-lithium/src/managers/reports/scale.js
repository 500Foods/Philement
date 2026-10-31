/**
 * View scale for the report canvas.
 * 1 is 100%, which is 96 CSS pixels per inch. Fit-width is the
 * hundredths of pane width over sheet width, rounded down, so the
 * scaled sheet is no wider than the pane. The definition is not
 * involved. Zoom steps by a tenth and stays between the limits
 * unless a fit already sits outside them.
 */

/** 100%. One inch is 96 CSS pixels. */
export const VIEW_SCALE_100 = 1;

/** One zoom step, as a fraction of 100%. */
export const VIEW_SCALE_STEP = 0.1;

/** Lowest scale a zoom button will walk to. */
export const VIEW_SCALE_MIN = 0.1;

/** Highest scale a zoom button will walk to. */
export const VIEW_SCALE_MAX = 4;

/**
 * Scale that fits `sheetWidth` into `paneWidth`.
 * A pane or a sheet that cannot be measured stays at 100%.
 * Below one percent the exact ratio is kept so a narrow pane
 * still holds the sheet.
 * @param {number} paneWidth
 * @param {number} sheetWidth
 * @returns {number}
 */
export function fitWidthScale(paneWidth, sheetWidth) {
  if (!(paneWidth > 0) || !(sheetWidth > 0)) return VIEW_SCALE_100;
  const hundredths = Math.floor((paneWidth * 100) / sheetWidth);
  if (hundredths > 0) return hundredths / 100;
  return paneWidth / sheetWidth;
}

/**
 * CSS value for a scale. An unusable number is 100%.
 * @param {number} scale
 * @returns {string}
 */
export function formatViewScale(scale) {
  const value = Number(scale);
  if (!(value > 0)) return String(VIEW_SCALE_100);
  return String(Math.round(value * 100) / 100);
}

/**
 * Toolbar label for a scale. 1 is "100%".
 * @param {number} scale
 * @returns {string}
 */
export function viewScaleLabel(scale) {
  return `${Math.round(Number(formatViewScale(scale)) * 100)}%`;
}

/**
 * One zoom step. A scale already outside the limits moves back
 * toward them and does not walk further away.
 * @param {number} scale
 * @param {number} direction 1 zooms in. -1 zooms out.
 * @returns {number}
 */
export function stepViewScale(scale, direction) {
  const current = scale > 0 ? Math.round(scale * 100) : VIEW_SCALE_100 * 100;
  const step = Math.round(VIEW_SCALE_STEP * 100);
  const min = Math.round(VIEW_SCALE_MIN * 100);
  const max = Math.round(VIEW_SCALE_MAX * 100);
  let next = current + (direction < 0 ? -step : step);
  if (direction < 0) next = Math.max(next, Math.min(current, min));
  else next = Math.min(next, Math.max(current, max));
  return next / 100;
}

/**
 * Scale painted on a canvas. A missing variable is 100%.
 * @param {HTMLElement} node
 * @returns {number}
 */
export function readViewScale(node) {
  const canvas = node && node.closest ? node.closest('.reports-canvas') : null;
  if (!canvas) return VIEW_SCALE_100;
  const value = Number(canvas.style.getPropertyValue('--reports-scale'));
  return value > 0 ? value : VIEW_SCALE_100;
}
