/**
 * View math for Preview PDF.
 * A page's /Rotate is which side is up. The remembered quarter-turn
 * is added to it. Flips are a separate view transform. Fit page and
 * fit width are modes. The numeric scale is the last explicit scale.
 * Nothing here reads a file or writes the DOM.
 */

export const PDF_FIXTURE_URL = '/assets/documents/test.pdf';
export const PDF_FIXTURE_NAME = 'test.pdf';
/** pdf.js worker. Deploy copies this file. It is not a Vite asset. */
export const PDF_WORKER_URL = '/assets/pdf.worker.min.js';
export const PDF_FIXTURE_PAGE_KEY = 'fixture';
export const PDF_SETTINGS_KEY = 'lithium_reports_pdf';

export const PDF_SCALE_MIN = 0.1;
export const PDF_SCALE_MAX = 8;
export const PDF_ZOOM_FACTOR = 1.1;
export const PDF_PAGE_GAP = 16;
/** Numeric 100% is one CSS inch. PDF user space is 72 points per inch. */
export const PDF_CSS_PER_POINT = 96 / 72;
/** Thumbnail zoom. 100% is one row as tall as the framed body. */
export const THUMB_PERCENT_MIN = 25;
export const THUMB_PERCENT_MAX = 400;
export const THUMB_PERCENT_STEP = 5;
/** Page-number row. reports-pdf.css uses this same 16px. */
export const THUMB_NUMBER_PX = 16;

/** Saved before zoom was a percent. Those values reset to 100%. */
const OLD_THUMB_PIXELS = new Set([64, 96, 128, 176]);

const MODES = new Set(['fit-page', 'fit-width', 'scale']);
const POINTERS = new Set(['text', 'marquee', 'hand']);

/**
 * @param {number} value
 * @returns {number} 0, 90, 180, or 270
 */
export function quarter(value) {
  const n = Number(value);
  if (!Number.isFinite(n)) return 0;
  const turns = Math.round(n / 90);
  return ((turns % 4) + 4) % 4 * 90;
}

/**
 * Page /Rotate plus the viewer's quarter-turn.
 * @param {number} pageRotate
 * @param {number} userRotate
 * @returns {number}
 */
export function composedRotation(pageRotate, userRotate) {
  return quarter(quarter(pageRotate) + quarter(userRotate));
}

/**
 * @param {number} userRotate
 * @param {number} delta 90 or -90
 * @returns {number}
 */
export function addQuarter(userRotate, delta) {
  return quarter(quarter(userRotate) + delta);
}

/**
 * Flip both turns both mirrors on. A second click clears both.
 * @param {boolean} flipH
 * @param {boolean} flipV
 * @returns {{ flipH: boolean, flipV: boolean }}
 */
export function flipBoth(flipH, flipV) {
  if (flipH && flipV) return { flipH: false, flipV: false };
  return { flipH: true, flipV: true };
}

/**
 * @param {number} value
 * @returns {number}
 */
export function clampScale(value) {
  const n = Number(value);
  if (!(n > 0)) return 1;
  return Math.min(PDF_SCALE_MAX, Math.max(PDF_SCALE_MIN, n));
}

/**
 * One wheel or button step. Both directions multiply.
 * @param {number} scale
 * @param {number} direction 1 zooms in. -1 zooms out.
 * @returns {number}
 */
export function multiplyScale(scale, direction) {
  const current = scale > 0 ? scale : 1;
  const next = direction < 0 ? current / PDF_ZOOM_FACTOR : current * PDF_ZOOM_FACTOR;
  return clampScale(next);
}

/**
 * Zooming leaves fit mode. The base is the scale on screen.
 * @param {string} mode
 * @param {number} displayedScale
 * @param {number} explicitScale
 * @param {number} direction
 * @returns {{ mode: 'scale', scale: number }}
 */
export function scaleAfterZoom(mode, displayedScale, explicitScale, direction) {
  const base = mode === 'scale' ? explicitScale : displayedScale;
  return { mode: 'scale', scale: multiplyScale(base, direction) };
}

/**
 * Double-click toggles fit page and the last explicit scale.
 * @param {string} mode
 * @param {number} explicitScale
 * @returns {{ mode: string, scale: number }}
 */
export function toggleFit(mode, explicitScale) {
  const scale = clampScale(explicitScale);
  if (mode === 'fit-page') return { mode: 'scale', scale };
  return { mode: 'fit-page', scale };
}

/**
 * Fit one page into the pane. The result may sit outside 10%–800%
 * so a large page still fits.
 * @param {number} pageWidth
 * @param {number} pageHeight
 * @param {number} paneWidth
 * @param {number} paneHeight
 * @param {number} pad
 * @returns {number}
 */
export function pdfFitPageScale(pageWidth, pageHeight, paneWidth, paneHeight, pad) {
  const inset = pad > 0 ? pad * 2 : 0;
  const width = paneWidth - inset;
  const height = paneHeight - inset;
  if (!(width > 0) || !(height > 0) || !(pageWidth > 0) || !(pageHeight > 0)) return 1;
  return Math.min(width / pageWidth, height / pageHeight);
}

/**
 * @param {number} pageWidth
 * @param {number} paneWidth
 * @param {number} pad
 * @returns {number}
 */
export function pdfFitWidthScale(pageWidth, paneWidth, pad) {
  const inset = pad > 0 ? pad * 2 : 0;
  const width = paneWidth - inset;
  if (!(width > 0) || !(pageWidth > 0)) return 1;
  return width / pageWidth;
}

/**
 * @param {number} scale
 * @returns {string}
 */
export function scaleLabel(scale) {
  const n = scale > 0 ? scale : 1;
  return `${Math.round(n * 100)}%`;
}

/**
 * Column count is 1 through 5. Hiding the strip is a separate flag.
 * @param {number} value
 * @returns {number}
 */
export function clampColumns(value) {
  const n = Math.round(Number(value));
  if (!Number.isFinite(n) || n <= 0) return 1;
  return Math.min(5, n);
}

/**
 * @param {number} value
 * @returns {number}
 */
export function clampThumbPercent(value) {
  const n = Number(value);
  if (!Number.isFinite(n) || OLD_THUMB_PIXELS.has(n)) return 100;
  const stepped = Math.round(n / THUMB_PERCENT_STEP) * THUMB_PERCENT_STEP;
  return Math.min(THUMB_PERCENT_MAX, Math.max(THUMB_PERCENT_MIN, stepped));
}

/**
 * @param {number} value
 * @param {number} direction 1 grows. -1 shrinks.
 * @returns {number}
 */
export function stepThumbPercent(value, direction) {
  const current = clampThumbPercent(value);
  const delta = direction < 0 ? -THUMB_PERCENT_STEP : THUMB_PERCENT_STEP;
  return clampThumbPercent(current + delta);
}

/**
 * At 100% the cell is as tall as the body and one column wide.
 * @param {number} bodyWidth
 * @param {number} bodyHeight
 * @param {number} columns
 * @param {number} percent
 * @returns {{ width: number, height: number }}
 */
export function thumbCellSize(bodyWidth, bodyHeight, columns, percent) {
  if (!(bodyWidth > 0) || !(bodyHeight > 0)) return { width: 0, height: 0 };
  const scale = clampThumbPercent(percent) / 100;
  return {
    width: (bodyWidth / clampColumns(columns)) * scale,
    height: bodyHeight * scale,
  };
}

/**
 * The page sits in the cell, centered, above the number row.
 * @param {number} pageWidth
 * @param {number} pageHeight
 * @param {number} cellWidth
 * @param {number} cellHeight
 * @param {number} numberPx
 * @returns {{ width: number, height: number, scale: number }}
 */
export function thumbContain(pageWidth, pageHeight, cellWidth, cellHeight, numberPx) {
  const wellW = cellWidth > 0 ? cellWidth : 0;
  const wellH = cellHeight - (numberPx > 0 ? numberPx : 0);
  if (!(pageWidth > 0) || !(pageHeight > 0) || !(wellW > 0) || !(wellH > 0)) {
    return { width: 0, height: 0, scale: 1 };
  }
  const scale = Math.min(wellW / pageWidth, wellH / pageHeight);
  return { width: pageWidth * scale, height: pageHeight * scale, scale };
}

/**
 * @param {number} reportId
 * @returns {string}
 */
export function pdfPageKey(reportId) {
  const id = Number(reportId);
  return id > 0 ? String(Math.trunc(id)) : PDF_FIXTURE_PAGE_KEY;
}

/**
 * @returns {object}
 */
export function defaultPdfSettings() {
  return {
    columns: 1,
    thumbPercent: 100,
    thumbsOpen: true,
    mode: 'fit-page',
    scale: 1,
    rotate: 0,
    flipH: false,
    flipV: false,
    outlineOpen: false,
    searchOpen: false,
    pointer: 'text',
    pages: {},
  };
}

/**
 * Drop anything that is not a view toggle. Search words are not stored.
 * @param {object|null} raw
 * @returns {object}
 */
export function normalizePdfSettings(raw) {
  const base = defaultPdfSettings();
  const src = raw && typeof raw === 'object' ? raw : {};
  const mode = MODES.has(src.mode) ? src.mode : base.mode;
  const pointer = POINTERS.has(src.pointer) ? src.pointer : base.pointer;
  const pages = {};
  if (src.pages && typeof src.pages === 'object') {
    for (const [key, value] of Object.entries(src.pages)) {
      const n = Math.round(Number(value));
      if (key && n >= 1 && n <= 100000) pages[key] = n;
    }
  }
  const rawColumns = Math.round(Number(src.columns));
  const columnsKnown = Number.isFinite(rawColumns);
  const thumbRaw = src.thumbPercent != null ? src.thumbPercent : src.thumbSize;
  return {
    columns: columnsKnown ? clampColumns(rawColumns) : base.columns,
    thumbPercent: thumbRaw == null ? base.thumbPercent : clampThumbPercent(thumbRaw),
    thumbsOpen: typeof src.thumbsOpen === 'boolean'
      ? src.thumbsOpen
      : (columnsKnown ? rawColumns !== 0 : base.thumbsOpen),
    mode,
    scale: clampScale(src.scale ?? base.scale),
    rotate: quarter(src.rotate),
    flipH: src.flipH === true,
    flipV: src.flipV === true,
    outlineOpen: src.outlineOpen === true,
    searchOpen: src.searchOpen === true,
    pointer,
    pages,
  };
}

/**
 * @param {{ get?: Function }|null} service
 * @returns {object}
 */
export function loadPdfSettings(service) {
  const raw = service && typeof service.get === 'function'
    ? service.get(PDF_SETTINGS_KEY, null)
    : null;
  return normalizePdfSettings(raw);
}

/**
 * @param {{ set?: Function }|null} service
 * @param {object} settings
 */
export function savePdfSettings(service, settings) {
  if (!service || typeof service.set !== 'function') return;
  service.set(PDF_SETTINGS_KEY, normalizePdfSettings(settings));
}

/**
 * @param {object} settings
 * @param {string} key
 * @param {number} pageCount
 * @returns {number}
 */
export function rememberedPage(settings, key, pageCount) {
  const stored = settings && settings.pages ? settings.pages[key] : 0;
  const n = Math.round(Number(stored));
  if (!(n >= 1)) return 1;
  if (pageCount >= 1) return Math.min(pageCount, n);
  return n;
}

/**
 * The page that occupies the most of the pane.
 * @param {number} scrollTop
 * @param {number} paneHeight
 * @param {number[]} tops
 * @param {number[]} heights
 * @returns {number} 1-based
 */
export function pageOccupyingPane(scrollTop, paneHeight, tops, heights) {
  if (!tops.length) return 1;
  const viewTop = scrollTop > 0 ? scrollTop : 0;
  const viewBottom = viewTop + (paneHeight > 0 ? paneHeight : 0);
  let best = 0;
  let bestArea = -1;
  for (let i = 0; i < tops.length; i += 1) {
    const top = tops[i];
    const bottom = top + heights[i];
    const overlap = Math.min(viewBottom, bottom) - Math.max(viewTop, top);
    if (overlap > bestArea) {
      bestArea = overlap;
      best = i;
    }
  }
  return best + 1;
}

/**
 * @param {number} scrollTop
 * @param {number} paneHeight
 * @param {number[]} tops
 * @param {number[]} heights
 * @param {number} [overscan]
 * @returns {{ start: number, end: number }}
 */
export function visibleRange(scrollTop, paneHeight, tops, heights, overscan = 1) {
  const last = tops.length - 1;
  if (last < 0) return { start: 0, end: -1 };
  const viewTop = scrollTop > 0 ? scrollTop : 0;
  const viewBottom = viewTop + (paneHeight > 0 ? paneHeight : 0);
  let start = -1;
  let end = -1;
  for (let i = 0; i < tops.length; i += 1) {
    const top = tops[i];
    const bottom = top + heights[i];
    if (bottom >= viewTop && top <= viewBottom) {
      if (start < 0) start = i;
      end = i;
    }
  }
  if (start < 0) {
    const page = pageOccupyingPane(scrollTop, paneHeight, tops, heights) - 1;
    start = page;
    end = page;
  }
  const extra = overscan > 0 ? overscan : 0;
  return {
    start: Math.max(0, start - extra),
    end: Math.min(last, end + extra),
  };
}

/**
 * @param {number} index 0-based
 * @param {string} key
 * @param {number} columns
 * @param {number} count
 * @returns {number}
 */
export function moveThumb(index, key, columns, count) {
  if (count <= 0) return 0;
  const current = Math.min(Math.max(0, index), count - 1);
  if (key === 'Home') return 0;
  if (key === 'End') return count - 1;
  const cols = columns > 0 ? columns : 1;
  let delta;
  if (key === 'ArrowLeft') delta = -1;
  else if (key === 'ArrowRight') delta = 1;
  else if (key === 'ArrowUp') delta = -cols;
  else if (key === 'ArrowDown') delta = cols;
  else return current;
  return Math.min(count - 1, Math.max(0, current + delta));
}

/**
 * @param {number} scrollTop
 * @param {number} paneHeight
 * @param {number} direction -1 or 1
 * @param {number} maxScroll
 * @returns {number}
 */
export function stackScroll(scrollTop, paneHeight, direction, maxScroll) {
  const step = paneHeight > 0 ? paneHeight : 0;
  const next = (scrollTop > 0 ? scrollTop : 0) + (direction < 0 ? -step : step);
  const cap = maxScroll > 0 ? maxScroll : 0;
  return Math.min(cap, Math.max(0, next));
}

/**
 * @param {string} key
 * @param {{ inField?: boolean, inText?: boolean, searchOpen?: boolean, pointer?: string }} where
 * @returns {string|null}
 */
export function viewerKeyAction(key, where) {
  const place = where || {};
  if (key === 'Escape' && place.searchOpen) return 'close-search';
  if (place.inField) return null;
  if (key === 'PageUp') return 'page-up';
  if (key === 'PageDown') return 'page-down';
  if (key === 'Home') return 'first';
  if (key === 'End') return 'last';
  if (place.inText && place.pointer === 'text') return null;
  if (key === 'ArrowLeft' || key === 'ArrowRight' || key === 'ArrowUp' || key === 'ArrowDown') {
    return 'thumb';
  }
  return null;
}

/**
 * @param {Array<object>|null} nodes
 * @param {number} [depth]
 * @param {Array<object>} [out]
 * @returns {Array<{ title: string, dest: *, url: string, depth: number }>}
 */
export function flattenOutline(nodes, depth = 0, out = []) {
  for (const node of nodes || []) {
    if (!node || typeof node !== 'object') continue;
    const title = typeof node.title === 'string' && node.title.trim() ? node.title.trim() : 'Untitled';
    const url = typeof node.url === 'string' ? node.url : '';
    out.push({ title, dest: node.dest ?? null, url, depth });
    const children = Array.isArray(node.items) ? node.items : node.kids;
    if (Array.isArray(children) && children.length) flattenOutline(children, depth + 1, out);
  }
  return out;
}

/**
 * Left and top in PDF user space, when the destination names them.
 * /Fit names a page and no point.
 * @param {Array|null} explicit
 * @returns {{ left: number|null, top: number|null }}
 */
export function destTarget(explicit) {
  if (!Array.isArray(explicit)) return { left: null, top: null };
  const kind = explicit[1] && (explicit[1].name || explicit[1]);
  const num = (value) => (typeof value === 'number' && Number.isFinite(value) ? value : null);
  if (kind === 'XYZ') return { left: num(explicit[2]), top: num(explicit[3]) };
  if (kind === 'FitH' || kind === 'FitBH') return { left: null, top: num(explicit[2]) };
  if (kind === 'FitV' || kind === 'FitBV') return { left: num(explicit[2]), top: null };
  if (kind === 'FitR') return { left: num(explicit[2]), top: num(explicit[5]) };
  return { left: null, top: null };
}

/**
 * @param {Array<object>} items
 * @returns {string}
 */
export function textFromItems(items) {
  let out = '';
  for (const item of items || []) {
    if (!item || typeof item.str !== 'string') continue;
    out += item.str;
    if (item.hasEOL) out += '\n';
  }
  return out;
}

/**
 * @param {Array<{ pageNumber: number, text: string }>} pages
 * @param {string} query
 * @returns {Array<{ page: number, start: number, length: number }>}
 */
export function findHits(pages, query) {
  const needle = String(query || '').trim().toLowerCase();
  if (!needle) return [];
  const hits = [];
  for (const page of pages || []) {
    const hay = String(page.text || '').toLowerCase();
    let from = 0;
    while (from < hay.length) {
      const at = hay.indexOf(needle, from);
      if (at < 0) break;
      hits.push({ page: page.pageNumber, start: at, length: needle.length });
      from = at + needle.length;
    }
  }
  return hits;
}

/**
 * @param {Array<object>} items
 * @param {number} start
 * @param {number} length
 * @returns {number[]}
 */
export function itemsOverlapping(items, start, length) {
  const end = start + length;
  const indexes = [];
  let cursor = 0;
  const list = items || [];
  for (let i = 0; i < list.length; i += 1) {
    const item = list[i];
    if (!item || typeof item.str !== 'string') continue;
    const itemStart = cursor;
    const itemEnd = cursor + item.str.length;
    if (item.str.length && itemEnd > start && itemStart < end) indexes.push(i);
    cursor = itemEnd;
    if (item.hasEOL) cursor += 1;
  }
  return indexes;
}

/**
 * @param {number} index 0-based, or -1 when nothing is current
 * @param {number} total
 * @returns {string}
 */
export function searchCountLabel(index, total) {
  const count = total > 0 ? total : 0;
  if (!count) return '0 of 0';
  const shown = index >= 0 ? index + 1 : 0;
  return `${shown} of ${count}`;
}

/**
 * Screen fraction inside a flipped page, back into canvas space.
 * @param {number} u
 * @param {number} v
 * @param {boolean} flipH
 * @param {boolean} flipV
 * @returns {{ u: number, v: number }}
 */
export function visualToLocal(u, v, flipH, flipV) {
  return {
    u: flipH ? 1 - u : u,
    v: flipV ? 1 - v : v,
  };
}

/**
 * @param {number} localX
 * @param {number} localY
 * @param {number} localW
 * @param {number} localH
 * @param {number} bitmapW
 * @param {number} bitmapH
 * @param {number} cssW
 * @param {number} cssH
 * @returns {{ x: number, y: number, w: number, h: number }}
 */
export function marqueeSource(localX, localY, localW, localH, bitmapW, bitmapH, cssW, cssH) {
  const sx = cssW > 0 ? bitmapW / cssW : 1;
  const sy = cssH > 0 ? bitmapH / cssH : 1;
  const x = Math.max(0, Math.round(localX * sx));
  const y = Math.max(0, Math.round(localY * sy));
  const w = Math.max(1, Math.min(bitmapW - x, Math.round(localW * sx)));
  const h = Math.max(1, Math.min(bitmapH - y, Math.round(localH * sy)));
  return { x, y, w, h };
}

/**
 * A screen rectangle on a flipped sheet, back into unflipped CSS pixels.
 * @param {number} x
 * @param {number} y
 * @param {number} w
 * @param {number} h
 * @param {number} cssW
 * @param {number} cssH
 * @param {boolean} flipH
 * @param {boolean} flipV
 * @returns {{ x: number, y: number, w: number, h: number }}
 */
export function marqueeLocalRect(x, y, w, h, cssW, cssH, flipH, flipV) {
  const width = cssW > 0 ? cssW : 1;
  const height = cssH > 0 ? cssH : 1;
  const corners = [
    [x, y],
    [x + w, y],
    [x, y + h],
    [x + w, y + h],
  ].map(([px, py]) => visualToLocal(px / width, py / height, flipH, flipV));
  const us = corners.map((corner) => corner.u);
  const vs = corners.map((corner) => corner.v);
  const u = Math.min(...us);
  const v = Math.min(...vs);
  return {
    x: u * width,
    y: v * height,
    w: (Math.max(...us) - u) * width,
    h: (Math.max(...vs) - v) * height,
  };
}
