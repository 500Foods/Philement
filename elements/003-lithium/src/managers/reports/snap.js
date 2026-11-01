/**
 * Snap a design length to the section grid.
 * The live drag follows the pointer. The commit uses the nearest
 * spacing point, measured from the band body's top-left. A grid
 * that cannot be measured leaves the length alone. The choice
 * stays off the definition.
 */

import { gridSpec } from './grid.js';
import { dragItemBox } from './items.js';

const SAME_PX = 0.5;

/**
 * Nearest grid point. A missing step returns `px`.
 * @param {number} px
 * @param {number} spacingPx
 * @returns {number}
 */
export function snapToGrid(px, spacingPx) {
  if (!(spacingPx > 0) || !Number.isFinite(px)) return px;
  return Math.round(px / spacingPx) * spacingPx;
}

/**
 * Band height on the grid, never under the floor.
 * A grid point below the floor stays at the floor.
 * @param {number} px
 * @param {number} spacingPx
 * @param {number} minPx
 * @returns {number}
 */
export function snapBandHeight(px, spacingPx, minPx) {
  const floor = Number.isFinite(minPx) ? minPx : 0;
  return Math.max(floor, snapToGrid(px, spacingPx));
}

/**
 * @param {{ left: number, top: number, width: number, height: number }} a
 * @param {{ left: number, top: number, width: number, height: number }} b
 * @returns {boolean}
 */
export function boxesNear(a, b) {
  if (!a || !b) return false;
  return Math.abs(a.left - b.left) < SAME_PX
    && Math.abs(a.top - b.top) < SAME_PX
    && Math.abs(a.width - b.width) < SAME_PX
    && Math.abs(a.height - b.height) < SAME_PX;
}

/**
 * Snap a box, then clamp it inside the band.
 * A move snaps the left and the top. A resize snaps the right
 * and the bottom and keeps the top left.
 * @param {{ left: number, top: number, width: number, height: number }} box
 * @param {number} spacingPx
 * @param {{ width: number, height: number }} bounds
 * @param {boolean} resize
 * @returns {{ left: number, top: number, width: number, height: number }}
 */
export function snapItemBox(box, spacingPx, bounds, resize) {
  if (!(spacingPx > 0) || !box) return box;
  const next = resize
    ? {
      left: box.left,
      top: box.top,
      width: snapToGrid(box.left + box.width, spacingPx) - box.left,
      height: snapToGrid(box.top + box.height, spacingPx) - box.top,
    }
    : {
      left: snapToGrid(box.left, spacingPx),
      top: snapToGrid(box.top, spacingPx),
      width: box.width,
      height: box.height,
    };
  return dragItemBox(next, 0, 0, bounds, Boolean(resize));
}

/**
 * Grid step for a snap. An unusable grid returns 0.
 * @param {object} doc
 * @returns {number}
 */
export function snapSpacingPx(doc) {
  const spec = gridSpec(doc);
  if (!spec || spec.error || !(spec.spacingPx > 0)) return 0;
  return spec.spacingPx;
}
