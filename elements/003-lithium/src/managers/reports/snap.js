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
 * A move snaps the left and the top. A resize snaps the moved
 * edges (default south-east: right and bottom, top left fixed).
 * @param {{ left: number, top: number, width: number, height: number }} box
 * @param {number} spacingPx
 * @param {{ width: number, height: number }} bounds
 * @param {boolean} resize
 * @param {{ n?: boolean, s?: boolean, e?: boolean, w?: boolean }|null} [edges]
 * @returns {{ left: number, top: number, width: number, height: number }}
 */
export function snapItemBox(box, spacingPx, bounds, resize, edges) {
  if (!(spacingPx > 0) || !box) return box;
  if (!resize) {
    const next = {
      left: snapToGrid(box.left, spacingPx),
      top: snapToGrid(box.top, spacingPx),
      width: box.width,
      height: box.height,
    };
    return dragItemBox(next, 0, 0, bounds, false);
  }
  const active = edges && (edges.n || edges.s || edges.e || edges.w)
    ? edges
    : { n: false, s: true, e: true, w: false };
  let left = box.left;
  let top = box.top;
  let width = box.width;
  let height = box.height;
  const right = left + width;
  const bottom = top + height;
  if (active.e && !active.w) width = snapToGrid(right, spacingPx) - left;
  else if (active.w && !active.e) {
    left = snapToGrid(left, spacingPx);
    width = right - left;
  }
  if (active.s && !active.n) height = snapToGrid(bottom, spacingPx) - top;
  else if (active.n && !active.s) {
    top = snapToGrid(top, spacingPx);
    height = bottom - top;
  }
  return dragItemBox({ left, top, width, height }, 0, 0, bounds, true, active);
}

export function snapSpacingPx(doc) {
  const spec = gridSpec(doc);
  if (!spec || spec.error || !(spec.spacingPx > 0)) return 0;
  return spec.spacingPx;
}
