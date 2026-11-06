/**
 * Line breaks and fitHeight for a band instance.
 * Widths are Helvetica AFM advances. The HTML and PDF emitters
 * paint the lines this function returns. acuranzo_1468.lua
 * keeps the same advance table and the same break rules.
 * A grown item is drawn in Helvetica so the measured line fits.
 */

import { AFM_BOLD, AFM_MISSING, AFM_ROMAN } from './afm-helvetica.js';
import { parseLength } from './units.js';

export const WRAP_MODES = Object.freeze(['normal', 'pre', 'pre-line', 'pre-wrap', 'nowrap']);
export const FIT_HEIGHTS = Object.freeze(['clip', 'grow', 'shrink', 'both']);
export const VALIGNS = Object.freeze(['top', 'middle', 'bottom']);
export const MIN_BAND_PT = 0.15 * 72;
const PT_PER_INCH = 72;
const EPS = 1e-6;

const UNIT_PT = { in: 72, mm: 72 / 25.4, cm: 72 / 2.54, pt: 1, px: 0.75 };

/**
 * Bold faces use the bold table. Oblique uses the roman table.
 * @param {string} [style]
 * @returns {'bold'|'roman'}
 */
export function faceForStyle(style) {
  if (style === 'Bold' || style === 'Bold + Italic') return 'bold';
  return 'roman';
}

/**
 * Advance of one character, in AFM units.
 * @param {string} ch
 * @param {'bold'|'roman'} face
 * @returns {number}
 */
export function advanceUnits(ch, face) {
  const code = ch.codePointAt(0);
  const table = face === 'bold' ? AFM_BOLD : AFM_ROMAN;
  if (code === undefined || code < 32 || code > 126) return AFM_MISSING;
  return table[code - 32];
}

/**
 * Advance in points at a font size.
 * @param {string} ch
 * @param {number} fontSizePt
 * @param {string} [style]
 * @returns {number}
 */
export function advancePt(ch, fontSizePt, style) {
  return advanceUnits(ch, faceForStyle(style)) * fontSizePt / 1000;
}

/**
 * @param {string} text
 * @param {number} fontSizePt
 * @param {string} [style]
 * @returns {number}
 */
export function measurePt(text, fontSizePt, style) {
  let width = 0;
  for (const ch of String(text)) width += advancePt(ch, fontSizePt, style);
  return width;
}

/**
 * Used line height. The multiple and the font size share one unit.
 * @param {number} fontSizePt
 * @param {number} [multiple]
 * @returns {number}
 */
export function lineHeightPt(fontSizePt, multiple) {
  const factor = typeof multiple === 'number' && Number.isFinite(multiple) && multiple > 0 ? multiple : 1.2;
  return factor * fontSizePt;
}

function isSpace(ch) {
  return ch === ' ' || ch === '\t' || ch === '\n' || ch === '\r' || ch === '\f';
}

function collapse(text) {
  return String(text).replace(/[ \t\n\r\f]+/g, ' ').trim();
}

function collapseLine(text) {
  return String(text).replace(/[ \t\f]+/g, ' ').trim();
}

/**
 * Break one word that is wider than the box. Each piece fits,
 * except a box that is narrower than one character.
 * @param {string} word
 * @param {number} widthPt
 * @param {number} fontSizePt
 * @param {string} style
 * @returns {string[]}
 */
function breakWord(word, widthPt, fontSizePt, style) {
  if (!(widthPt > EPS)) return [...word];
  const parts = [];
  let line = '';
  let used = 0;
  for (const ch of word) {
    const width = advancePt(ch, fontSizePt, style);
    if (line !== '' && used + width > widthPt + EPS) {
      parts.push(line);
      line = ch;
      used = width;
    } else {
      line += ch;
      used += width;
    }
  }
  if (line !== '') parts.push(line);
  return parts.length ? parts : [''];
}

/**
 * Pack words separated by single spaces. A word wider than the
 * box breaks by character when `charBreak` is set.
 * @param {string[]} words
 * @param {number} widthPt
 * @param {number} fontSizePt
 * @param {string} style
 * @param {boolean} charBreak
 * @returns {string[]}
 */
function packWords(words, widthPt, fontSizePt, style, charBreak) {
  if (words.length === 0) return [''];
  if (!(widthPt > EPS)) {
    if (!charBreak) return [words.join(' ')];
    return [...words.join(' ')];
  }
  const space = advancePt(' ', fontSizePt, style);
  const lines = [];
  let line = '';
  let used = 0;
  const put = (word) => {
    const pieces = charBreak && measurePt(word, fontSizePt, style) > widthPt + EPS
      ? breakWord(word, widthPt, fontSizePt, style)
      : [word];
    for (const piece of pieces) {
      const width = measurePt(piece, fontSizePt, style);
      if (line === '') {
        line = piece;
        used = width;
      } else if (used + space + width <= widthPt + EPS) {
        line = `${line} ${piece}`;
        used += space + width;
      } else {
        lines.push(line);
        line = piece;
        used = width;
      }
    }
  };
  for (const word of words) put(word);
  if (line !== '' || lines.length === 0) lines.push(line);
  return lines;
}

/**
 * pre-wrap: keep spaces, break at the last space that fits, and
 * break a word by character when it has no space.
 * @param {string} text
 * @param {number} widthPt
 * @param {number} fontSizePt
 * @param {string} style
 * @returns {string[]}
 */
function wrapPre(text, widthPt, fontSizePt, style) {
  if (text === '') return [''];
  if (!(widthPt > EPS)) return [...text];
  const lines = [];
  let line = '';
  let used = 0;
  let lastSpace = -1;
  const chars = [...text];
  const finish = (end) => {
    lines.push(line.slice(0, end));
    line = line.slice(end);
    used = measurePt(line, fontSizePt, style);
    lastSpace = -1;
    for (let index = 0; index < line.length; index += 1) {
      if (line[index] === ' ' || line[index] === '\t') lastSpace = index + 1;
    }
  };
  for (const ch of chars) {
    const width = advancePt(ch === '\t' ? ' ' : ch, fontSizePt, style);
    if (line !== '' && used + width > widthPt + EPS) {
      if (lastSpace > 0) finish(lastSpace);
      else finish(line.length);
    }
    line += ch;
    used += width;
    if (ch === ' ' || ch === '\t') lastSpace = line.length;
  }
  if (line !== '' || lines.length === 0) lines.push(line);
  return lines;
}

/**
 * Broken lines for one string. Emitters do not break them again.
 * `normal` collapses whitespace and wraps. `pre` keeps it and
 * breaks only at newlines. `pre-line` keeps newlines, collapses
 * spaces, and wraps. `pre-wrap` keeps spaces and wraps.
 * `nowrap` collapses whitespace onto one line.
 * A word longer than the box breaks by character in the wrapping
 * modes. `pre` and `nowrap` do not.
 * @param {string} text
 * @param {number} widthPt
 * @param {number} fontSizePt
 * @param {string} [style]
 * @param {string} [mode]
 * @returns {string[]}
 */
export function wrapText(text, widthPt, fontSizePt, style = 'Normal', mode = 'pre-line') {
  const source = text == null ? '' : String(text);
  const size = Number.isFinite(fontSizePt) && fontSizePt > 0 ? fontSizePt : 10;
  if (mode === 'pre') {
    if (source === '') return [''];
    return source.split('\n').map((line) => line.replace(/\r/g, ''));
  }
  if (mode === 'nowrap') return [collapse(source)];
  if (mode === 'normal') {
    const collapsed = collapse(source);
    if (collapsed === '') return [''];
    return packWords(collapsed.split(' '), widthPt, size, style, true);
  }
  if (mode === 'pre-wrap') {
    if (source === '') return [''];
    const blocks = source.split('\n');
    const lines = [];
    for (const block of blocks) lines.push(...wrapPre(block.replace(/\r/g, ''), widthPt, size, style));
    return lines.length ? lines : [''];
  }
  const blocks = source.split('\n');
  const lines = [];
  for (const block of blocks) {
    const collapsed = collapseLine(block.replace(/\r/g, ''));
    if (collapsed === '') lines.push('');
    else lines.push(...packWords(collapsed.split(' ').filter((word) => word !== ''), widthPt, size, style, true));
  }
  return lines.length ? lines : [''];
}

/**
 * @param {number} typeKey
 * @returns {'clip'|'both'}
 */
export function defaultFitHeight(typeKey) {
  return typeKey === 3 || typeKey === 23 ? 'both' : 'clip';
}

/**
 * @param {number} typeKey
 * @returns {string}
 */
export function defaultWrap(typeKey) {
  if (typeKey === 3 || typeKey === 23) return 'pre-wrap';
  if (typeKey === 1 || typeKey === 4) return 'nowrap';
  return 'pre-line';
}

/**
 * @param {string} length
 * @returns {number|null}
 */
export function lengthPt(length) {
  if (typeof length !== 'string') return null;
  try {
    const parsed = parseLength(length);
    const factor = UNIT_PT[parsed.unit];
    if (!factor) return null;
    return parsed.value * factor;
  } catch {
    return null;
  }
}

function fontOf(doc, item) {
  const own = item.font && typeof item.font === 'object' ? item.font : null;
  const report = doc && doc.font && typeof doc.font === 'object' ? doc.font : null;
  const source = own || report || {};
  return {
    family: typeof source.family === 'string' && source.family !== '' ? source.family : 'Vanadium Sans',
    size: typeof source.size === 'string' && source.size !== '' ? source.size : '10 pt',
    style: typeof source.style === 'string' && source.style !== '' ? source.style : 'Normal',
  };
}

function warnOnce(seen, key, warn, message) {
  if (!warn || seen[key]) return;
  seen[key] = true;
  warn(message);
}

/**
 * Static text and field text keep one clipped run until fitHeight
 * changes. Number, datetime, and memo always break lines.
 * @param {object} item
 * @param {string} fit
 * @returns {boolean}
 */
export function paintsLines(item, fit) {
  const typeKey = Number(item && item.typeKey);
  if (typeKey === 1 || typeKey === 3 || typeKey === 4 || typeKey === 23) return true;
  if ((typeKey === 0 || typeKey === 20) && fit !== 'clip') return true;
  return false;
}

function resolveMode(item, seen, warn) {
  const typeKey = Number(item.typeKey);
  const mode = item.wrap;
  if (mode === undefined || mode === '') return defaultWrap(typeKey);
  if (WRAP_MODES.includes(mode)) return mode;
  warnOnce(seen, `wrap:${item.id}`, warn, `item ${item.id} wrap ${mode} is not a wrap mode; used ${defaultWrap(typeKey)}`);
  return defaultWrap(typeKey);
}

function resolveFit(item, seen, warn) {
  const fallback = defaultFitHeight(Number(item.typeKey));
  const fit = item.fitHeight;
  if (fit === undefined || fit === '') return fallback;
  if (FIT_HEIGHTS.includes(fit)) return fit;
  warnOnce(seen, `fit:${item.id}`, warn, `item ${item.id} fitHeight ${fit} is not clip, grow, shrink, or both; used ${fallback}`);
  return fallback;
}

function resolveValign(item, seen, warn) {
  const valign = item.valign;
  if (valign === undefined || valign === '') return 'top';
  if (VALIGNS.includes(valign)) return valign;
  warnOnce(seen, `valign:${item.id}`, warn, `item ${item.id} valign ${valign} is not top, middle, or bottom; used top`);
  return 'top';
}

function resolveMultiple(item, seen, warn) {
  if (item.lineHeight === undefined || item.lineHeight === '') return 1.2;
  const value = Number(item.lineHeight);
  if (Number.isFinite(value) && value > 0) return value;
  warnOnce(seen, `lh:${item.id}`, warn, `item ${item.id} lineHeight ${item.lineHeight} is not a positive multiple; used 1.2`);
  return 1.2;
}

/**
 * Vertical offset of the line block inside the used box.
 * @param {string} valign
 * @param {number} boxPt
 * @param {number} blockPt
 * @returns {number}
 */
export function valignOffset(valign, boxPt, blockPt) {
  const slack = boxPt - blockPt;
  if (slack <= EPS) return 0;
  if (valign === 'bottom') return slack;
  if (valign === 'middle') return slack / 2;
  return 0;
}

function fittedHeight(fit, storedPt, contentPt) {
  if (fit === 'grow') return Math.max(storedPt, contentPt);
  if (fit === 'shrink') return Math.min(storedPt, contentPt);
  if (fit === 'both') return contentPt;
  return storedPt;
}

/**
 * Lay out one band instance. Clip-only bands keep the stored height.
 * A band with grow, shrink, or both uses the lowest item bottom
 * after the shift, and not below 0.15 in. An item's shift is the
 * sum of the deltas of items whose stored bottom is at or above
 * this item's stored top. Deltas do not depend on order.
 * @param {object} doc
 * @param {object} band
 * @param {Array<{ item: object, text: string, defer?: boolean }>} entries
 * @param {(message: string) => void} [warn]
 * @returns {{ reflow: boolean, usedPt: number, items: object[] }}
 */
export function layoutBand(doc, band, entries, warn) {
  const seen = {};
  const placed = [];
  let reflow = false;
  for (const entry of entries) {
    const item = entry.item || {};
    const fit = entry.defer ? 'clip' : resolveFit(item, seen, warn);
    if (fit !== 'clip') reflow = true;
    const box = item.box && typeof item.box === 'object' ? item.box : {};
    const top = lengthPt(box.top);
    const height = lengthPt(box.height);
    const width = lengthPt(box.width);
    const font = fontOf(doc, item);
    const size = lengthPt(font.size);
    const fontSize = size && size > 0 ? size : 10;
    if (size === null && font.size) {
      warnOnce(seen, `size:${item.id}`, warn, `item ${item.id} font size ${font.size} does not parse; used 10 pt`);
    }
    const multiple = resolveMultiple(item, seen, warn);
    const leading = lineHeightPt(fontSize, multiple);
    const mode = resolveMode(item, seen, warn);
    const valign = resolveValign(item, seen, warn);
    const lines = paintsLines(item, fit) && !entry.defer
      ? wrapText(entry.text || '', width || 0, fontSize, font.style, mode)
      : null;
    const storedH = height && height > 0 ? height : 0;
    const contentH = Math.max(1, lines ? lines.length : 1) * leading;
    const usedH = entry.defer || !lines ? storedH : fittedHeight(fit, storedH, contentH);
    const visible = lines
      ? lines.filter((_, index) => (index + 1) * leading <= usedH + EPS || index === 0 && usedH > EPS)
      : null;
    const painted = visible
      ? visible.filter((_, index) => (index + 1) * leading <= usedH + EPS)
      : null;
    placed.push({
      item,
      text: entry.text || '',
      fit,
      lines: painted,
      font,
      fontSize,
      leading,
      valign,
      mode,
      origTop: top || 0,
      storedH,
      usedH,
      delta: usedH - storedH,
      width: width || 0,
    });
  }
  for (const row of placed) {
    let shift = 0;
    for (const other of placed) {
      if (other === row) continue;
      if (other.origTop + other.storedH <= row.origTop + EPS) shift += other.delta;
    }
    row.shift = shift;
    row.usedTop = row.origTop + shift;
  }
  const storedBand = lengthPt(band && band.height);
  let usedPt = storedBand && storedBand > 0 ? storedBand : 0;
  if (reflow) {
    let bottom = 0;
    for (const row of placed) bottom = Math.max(bottom, row.usedTop + row.usedH);
    usedPt = Math.max(MIN_BAND_PT, bottom);
  }
  return {
    reflow,
    usedPt,
    items: placed.map((row) => {
      const grown = Math.abs(row.usedH - row.storedH) > EPS;
      const block = row.lines ? Math.max(row.lines.length, 0) * row.leading : 0;
      return {
        id: row.item.id,
        typeKey: row.item.typeKey,
        text: row.text,
        lines: row.lines,
        align: row.item.align || 'Left',
        font: grown ? { ...row.font, family: 'Helvetica' } : row.font,
        grown,
        valign: row.valign,
        lineHeightPt: row.leading,
        offsetPt: row.lines ? valignOffset(row.valign, row.usedH, block) : 0,
        topPt: row.usedTop,
        heightPt: row.usedH,
        widthPt: row.width,
      };
    }),
  };
}

export { PT_PER_INCH };
