/**
 * Phase 80 Overflow (replaces Fit Height).
 * Values match lookup 054 List: Clip, Ellipsis, Grow, Shrink, Fit.
 * Legacy fitHeight maps on load: clip→Clip, grow→Grow, shrink→Shrink, both→Grow.
 */

export const OVERFLOW_MODES = Object.freeze(['Clip', 'Ellipsis', 'Grow', 'Shrink', 'Fit']);

const FIT_HEIGHT_MAP = Object.freeze({
  clip: 'Clip',
  grow: 'Grow',
  shrink: 'Shrink',
  both: 'Grow',
});

/**
 * @param {number} typeKey
 * @returns {string}
 */
export function defaultOverflow(typeKey) {
  // Memo kinds defaulted to fitHeight both → Grow.
  if (typeKey === 3 || typeKey === 23) return 'Grow';
  return 'Clip';
}

/**
 * Map a legacy fitHeight value, or return null when absent/unknown.
 * @param {unknown} fitHeight
 * @returns {string|null}
 */
export function mapFitHeight(fitHeight) {
  if (typeof fitHeight !== 'string' || fitHeight === '') return null;
  return FIT_HEIGHT_MAP[fitHeight] || null;
}

/**
 * Resolve Overflow for an item. Prefers item.overflow; else maps fitHeight;
 * else type default. Warns once on unknown overflow / fitHeight.
 * @param {object} item
 * @param {object} seen
 * @param {(m: string) => void} [warn]
 * @returns {string}
 */
export function resolveOverflow(item, seen, warn) {
  const typeKey = Number(item && item.typeKey) || 0;
  const fallback = defaultOverflow(typeKey);
  const raw = item && item.overflow;
  if (typeof raw === 'string' && raw !== '') {
    if (OVERFLOW_MODES.includes(raw)) return raw;
    if (warn) {
      const key = `overflow:${item.id}`;
      if (!seen[key]) {
        seen[key] = true;
        warn(`item ${item.id} overflow ${raw} is not Clip, Ellipsis, Grow, Shrink, or Fit; used ${fallback}`);
      }
    }
    return fallback;
  }
  const mapped = mapFitHeight(item && item.fitHeight);
  if (mapped) return mapped;
  if (item && typeof item.fitHeight === 'string' && item.fitHeight !== '' && warn) {
    const key = `fit:${item.id}`;
    if (!seen[key]) {
      seen[key] = true;
      warn(`item ${item.id} fitHeight ${item.fitHeight} is not clip, grow, shrink, or both; used ${fallback}`);
    }
  }
  return fallback;
}

/**
 * Load-time: fitHeight → overflow, then drop fitHeight.
 * @param {object} item
 */
export function migrateFitHeight(item) {
  if (!item || typeof item !== 'object') return;
  if (Object.hasOwn(item, 'overflow') && item.overflow !== '' && item.overflow != null) {
    if (Object.hasOwn(item, 'fitHeight')) delete item.fitHeight;
    return;
  }
  const mapped = mapFitHeight(item.fitHeight);
  if (mapped) item.overflow = mapped;
  else if (!Object.hasOwn(item, 'overflow')) item.overflow = defaultOverflow(Number(item.typeKey) || 0);
  if (Object.hasOwn(item, 'fitHeight')) delete item.fitHeight;
}

/**
 * Wrap modes that fit width only (single-line target).
 * @param {string} wrap
 * @returns {boolean}
 */
export function isWidthOnlyWrap(wrap) {
  return wrap === 'nowrap';
}
