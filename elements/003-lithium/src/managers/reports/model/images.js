/**
 * Static image registry and display geometry.
 * Bytes live once in doc.images, keyed by a FNV-1a hash of the
 * data URI. The item stores that hash. Preview builds one raster
 * per hash, pixel size, fit, anchor, grayscale, and invert.
 * The Reports.Render.Images module reads the same key in params.rasters.
 */

import { convertLength, parseLength } from './units.js';

export const IMAGE_FITS = Object.freeze(['stretch', 'contain', 'cover', 'none', 'scale-down']);
export const FLIPS = Object.freeze(['none', 'horizontal', 'vertical', 'both']);
export const FIT_ALIGNS = Object.freeze(['Left', 'Center', 'Right']);
export const FIT_VALIGNS = Object.freeze(['Top', 'Middle', 'Bottom']);
export const DEFAULT_DPI = 72;

const SVG_TAGS = new Set(['svg', 'g', 'path', 'rect', 'circle', 'ellipse', 'line', 'polyline', 'polygon', 'defs']);
const SVG_ATTRS = new Set([
  'fill', 'stroke', 'stroke-width', 'opacity', 'fill-opacity', 'stroke-opacity',
  'fill-rule', 'clip-rule', 'transform', 'd', 'x', 'y',
  'x1', 'y1', 'x2', 'y2', 'cx', 'cy', 'r', 'rx', 'ry', 'width', 'height',
  'points', 'viewbox', 'xmlns', 'id',
]);
const PATH_D = /^[MmLlHhVvCcSsQqTtAaZz0-9eE\s,.+\-]+$/;
const OPACITY_VALUE = /^(?:0(?:\.0+)?|1(?:\.0+)?|0?\.\d+)$/;
const OPACITY_DECL = /(?:^|[{;\s])opacity\s*:\s*(0(?:\.0+)?|1(?:\.0+)?|0?\.\d+)\s*(?:[;}\s]|$)/i;

/**
 * Half away from zero.
 * @param {number} value
 * @returns {number}
 */
export function halfAway(value) {
  if (!Number.isFinite(value)) return 0;
  return Math.sign(value) * Math.round(Math.abs(value));
}

/**
 * FNV-1a 32-bit, low byte xor then multiply, matching the script.
 * @param {string} text
 * @returns {string} Eight hex digits.
 */
export function imageHash(text) {
  let hash = 2166136261;
  const source = String(text);
  for (let index = 0; index < source.length; index += 1) {
    hash ^= source.charCodeAt(index) & 0xff;
    hash = Math.imul(hash, 16777619);
  }
  return (hash >>> 0).toString(16).padStart(8, '0');
}

/**
 * @param {string} dataUri
 * @returns {boolean}
 */
export function isSvgData(dataUri) {
  return typeof dataUri === 'string' && /^data:image\/svg\+xml/i.test(dataUri.trim());
}

/** A field image cell: base64 PNG or base64 SVG data URI. Anything else is blank. */
const FIELD_IMAGE_URI = /^data:image\/(?:png|svg\+xml);base64,[A-Za-z0-9+/]+=*$/i;

/**
 * @param {unknown} value
 * @returns {string} The URI, or `''` when it is not a PNG or SVG data URI.
 */
export function imageDataUriValue(value) {
  if (typeof value !== 'string') return '';
  const text = value.trim();
  return FIELD_IMAGE_URI.test(text) ? text : '';
}

/**
 * Display pixels: inches times dpi, at least 1 on a side.
 * @param {string} length
 * @param {number} [dpi]
 * @returns {number}
 */
export function displayPixels(length, dpi) {
  const parsed = parseLength(convertLength(length, 'in'));
  const density = typeof dpi === 'number' && Number.isFinite(dpi) && dpi > 0 ? dpi : DEFAULT_DPI;
  return Math.max(1, halfAway(parsed.value * density));
}

/**
 * @param {string} fit
 * @returns {string}
 */
export function resolveFit(fit) {
  return IMAGE_FITS.includes(fit) ? fit : 'contain';
}

/**
 * @param {string} flip
 * @returns {string}
 */
export function resolveFlip(flip) {
  return FLIPS.includes(flip) ? flip : 'none';
}

/**
 * Shared raster key. The script looks up params.rasters with it.
 * @param {string} hash
 * @param {number} pxW
 * @param {number} pxH
 * @param {string} fit
 * @param {string} align
 * @param {string} valign
 * @param {boolean} grayscale
 * @param {boolean} invert
 * @returns {string}
 */
export function rasterKey(hash, pxW, pxH, fit, align, valign, grayscale, invert) {
  const anchorX = FIT_ALIGNS.includes(align) ? align : 'Center';
  const anchorY = FIT_VALIGNS.includes(valign) ? valign : 'Middle';
  return [
    hash, pxW, pxH, resolveFit(fit), anchorX, anchorY,
    grayscale ? 'true' : 'false',
    invert ? 'true' : 'false',
  ].join('|');
}

function anchorFactor(name, horizontal) {
  if (horizontal) {
    if (name === 'Left') return 0;
    if (name === 'Right') return 1;
    return 0.5;
  }
  if (name === 'Top') return 0;
  if (name === 'Bottom') return 1;
  return 0.5;
}

/**
 * Where the source lands in the display bitmap.
 * Cover fills the bitmap and the overflow is cropped.
 * Contain and scale-down letterbox. Stretch fills.
 * none keeps intrinsic pixels and clips to the bitmap.
 * @param {number} srcW
 * @param {number} srcH
 * @param {number} dstW
 * @param {number} dstH
 * @param {string} fit
 * @param {string} align
 * @param {string} valign
 * @returns {{ dx: number, dy: number, dw: number, dh: number }}
 */
export function fitRect(srcW, srcH, dstW, dstH, fit, align, valign) {
  const mode = resolveFit(fit);
  const ax = anchorFactor(FIT_ALIGNS.includes(align) ? align : 'Center', true);
  const ay = anchorFactor(FIT_VALIGNS.includes(valign) ? valign : 'Middle', false);
  const safeW = srcW > 0 ? srcW : 1;
  const safeH = srcH > 0 ? srcH : 1;
  let scaleX = dstW / safeW;
  let scaleY = dstH / safeH;
  if (mode === 'contain' || mode === 'cover' || mode === 'scale-down' || mode === 'none') {
    let scale = mode === 'cover' ? Math.max(scaleX, scaleY) : Math.min(scaleX, scaleY);
    if (mode === 'none') scale = 1;
    if (mode === 'scale-down') scale = Math.min(1, scale);
    scaleX = scale;
    scaleY = scale;
  }
  const dw = safeW * scaleX;
  const dh = safeH * scaleY;
  return {
    dx: (dstW - dw) * ax,
    dy: (dstH - dh) * ay,
    dw,
    dh,
  };
}

/**
 * @param {object} doc
 * @returns {Set<string>}
 */
export function referencedImages(doc) {
  const used = new Set();
  const bands = Array.isArray(doc && doc.bands) ? doc.bands : [];
  for (const band of bands) {
    const items = Array.isArray(band && band.items) ? band.items : [];
    for (const item of items) {
      if (item && typeof item.image === 'string' && item.image !== '') used.add(item.image);
    }
  }
  return used;
}

/**
 * Drop registry entries no item references. An empty map is omitted.
 * @param {object} doc
 * @returns {object}
 */
export function pruneUnreferencedImages(doc) {
  if (!doc || !doc.images || typeof doc.images !== 'object' || Array.isArray(doc.images)) return doc;
  const used = referencedImages(doc);
  for (const key of Object.keys(doc.images)) {
    if (!used.has(key)) delete doc.images[key];
  }
  if (Object.keys(doc.images).length === 0) delete doc.images;
  return doc;
}

/**
 * Store one data URI. Returns the hash. Does not remove the previous one.
 * @param {object} doc
 * @param {string} dataUri
 * @returns {string}
 */
export function storeImage(doc, dataUri) {
  const hash = imageHash(dataUri);
  if (!doc.images || typeof doc.images !== 'object' || Array.isArray(doc.images)) doc.images = {};
  doc.images[hash] = dataUri;
  return hash;
}

/**
 * Forget a hash when no item still names it.
 * @param {object} doc
 * @param {string} hash
 */
export function releaseImage(doc, hash) {
  if (!hash || !doc.images) return;
  if (referencedImages(doc).has(hash)) return;
  delete doc.images[hash];
  if (Object.keys(doc.images).length === 0) delete doc.images;
}

function transformOk(value) {
  const parts = String(value).trim().split(/\)\s*/).filter((part) => part !== '');
  if (parts.length === 0) return false;
  const one = /^(?:translate\(-?\d+(?:\.\d+)?(?:[ ,]-?\d+(?:\.\d+)?)?|scale\(-?\d+(?:\.\d+)?(?:[ ,]-?\d+(?:\.\d+)?)?|rotate\(-?\d+(?:\.\d+)?(?:[ ,]-?\d+(?:\.\d+)?){0,2})$/;
  return parts.every((part) => one.test(part.trim()));
}

function attrOk(name, value) {
  const attr = String(name).toLowerCase();
  if (!SVG_ATTRS.has(attr)) return false;
  const text = String(value);
  if (/[<>]/.test(text) || /javascript\s*:/i.test(text)) return false;
  if (attr === 'transform') return transformOk(text);
  if (attr === 'xmlns') return text === 'http://www.w3.org/2000/svg';
  if (attr === 'id') return /^[A-Za-z][A-Za-z0-9_-]*$/.test(text);
  if (attr === 'points') return /^-?\d+(?:\.\d+)?(?:[\s,]+-?\d+(?:\.\d+)?)*$/.test(text.trim());
  if (attr === 'd') return PATH_D.test(text);
  if (attr === 'fill' || attr === 'stroke') return /^(?:none|currentColor|#[0-9A-Fa-f]{3}|#[0-9A-Fa-f]{4}|#[0-9A-Fa-f]{6}|#[0-9A-Fa-f]{8})$/.test(text);
  if (attr === 'opacity' || attr === 'fill-opacity' || attr === 'stroke-opacity') return OPACITY_VALUE.test(text.trim());
  if (attr === 'fill-rule' || attr === 'clip-rule') return /^(?:evenodd|nonzero)$/i.test(text.trim());
  return /^-?\d+(?:\.\d+)?%?$/.test(text) || text === '100%';
}

/**
 * A 0–1 opacity token, including the leading-dot form `.4`.
 * @param {string} value
 * @returns {string}
 */
export function unitOpacity(value) {
  const text = String(value ?? '').trim();
  return OPACITY_VALUE.test(text) ? text : '';
}

/**
 * Class opacities written in `<style>` blocks. Duotone uses
 * `.fa-secondary { opacity: .4 }`. Other declarations are ignored.
 * @param {string} markup
 * @returns {Map<string, string>}
 */
export function duotoneOpacityRules(markup) {
  const rules = new Map();
  const blocks = String(markup ?? '').matchAll(/<style[^>]*>([\s\S]*?)<\/style>/gi);
  for (const block of blocks) {
    const rule = /\.([A-Za-z_][\w-]*)\s*\{([^}]*)\}/g;
    for (let match = rule.exec(block[1]); match; match = rule.exec(block[1])) {
      const found = match[2].match(OPACITY_DECL);
      const value = found ? unitOpacity(found[1]) : '';
      if (value) rules.set(match[1], value);
    }
  }
  return rules;
}

/**
 * Copy duotone opacity onto the element. A style block and an inline
 * style are presentation, and the subset does not keep either one.
 * An opacity attribute already on the element stays.
 * @param {Element} svg
 * @param {Map<string, string>} [rules]
 */
export function prepareDuotone(svg, rules) {
  if (!svg || typeof svg.querySelectorAll !== 'function') return;
  const map = rules instanceof Map ? rules : duotoneOpacityRules(svg.outerHTML || '');
  const nodes = [svg, ...svg.querySelectorAll('*')];
  for (const el of nodes) {
    if (typeof el.getAttribute !== 'function' || el.hasAttribute('opacity')) continue;
    const inline = String(el.getAttribute('style') || '').match(OPACITY_DECL);
    const inlineValue = inline ? unitOpacity(inline[1]) : '';
    if (inlineValue) {
      el.setAttribute('opacity', inlineValue);
      continue;
    }
    for (const name of String(el.getAttribute('class') || '').split(/\s+/)) {
      if (map.has(name)) {
        el.setAttribute('opacity', map.get(name));
        break;
      }
    }
  }
}

/**
 * Keep the supported path subset. Script, filters, text, images,
 * and foreignObject are dropped. The file still holds the original.
 * @param {string} markup
 * @returns {{ ok: boolean, markup: string }}
 */
export function svgSubset(markup) {
  if (typeof markup !== 'string' || !/<svg[\s>]/i.test(markup)) return { ok: false, markup: '' };
  if (typeof DOMParser === 'undefined') return { ok: false, markup: '' };
  const doc = new DOMParser().parseFromString(markup, 'image/svg+xml');
  const root = doc.documentElement;
  if (!root || String(root.localName).toLowerCase() !== 'svg') return { ok: false, markup: '' };
  prepareDuotone(root, duotoneOpacityRules(markup));
  let dropped = false;
  const clean = (node) => {
    const copy = doc.createElementNS('http://www.w3.org/2000/svg', node.localName);
    for (const attr of [...node.attributes]) {
      if (attrOk(attr.name, attr.value)) copy.setAttribute(attr.name, attr.value);
      else dropped = true;
    }
    for (const child of [...node.childNodes]) {
      if (child.nodeType !== 1) continue;
      const name = String(child.localName || '').toLowerCase();
      if (!SVG_TAGS.has(name)) {
        dropped = true;
        continue;
      }
      copy.append(clean(child));
    }
    return copy;
  };
  const next = clean(root);
  return { ok: !dropped, markup: next.outerHTML };
}

/**
 * Placements that share one display PNG.
 * @param {object} doc
 * @returns {Array<{ key: string, hash: string, src: string, pxW: number, pxH: number, fit: string, fitAlign: string, fitValign: string, grayscale: boolean, invert: boolean }>}
 */
export function rasterJobs(doc) {
  const images = doc && doc.images && typeof doc.images === 'object' ? doc.images : {};
  const jobs = [];
  const seen = new Set();
  const bands = Array.isArray(doc && doc.bands) ? doc.bands : [];
  for (const band of bands) {
    for (const item of Array.isArray(band && band.items) ? band.items : []) {
      if (!item || Number(item.typeKey) !== 2) continue;
      const hash = typeof item.image === 'string' ? item.image : '';
      const src = images[hash];
      if (!src || isSvgData(src)) continue;
      const box = item.box && typeof item.box === 'object' ? item.box : {};
      if (typeof box.width !== 'string' || typeof box.height !== 'string') continue;
      const pxW = displayPixels(box.width, item.dpi);
      const pxH = displayPixels(box.height, item.dpi);
      const fit = typeof item.fit === 'string' ? item.fit : 'contain';
      const fitAlign = typeof item.fitAlign === 'string' ? item.fitAlign : 'Center';
      const fitValign = typeof item.fitValign === 'string' ? item.fitValign : 'Middle';
      const grayscale = item.grayscale === true;
      const invert = item.invert === true;
      const key = rasterKey(hash, pxW, pxH, fit, fitAlign, fitValign, grayscale, invert);
      if (seen.has(key)) continue;
      seen.add(key);
      jobs.push({ key, hash, src, pxW, pxH, fit, fitAlign, fitValign, grayscale, invert });
    }
  }
  return jobs;
}
