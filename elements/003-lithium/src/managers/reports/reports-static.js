/**
 * Static number, datetime, memo, and image.
 * Layout rows (wrap, fitHeight, valign, lineHeight) are client-only.
 * Image bytes live in doc.images. The item stores the hash.
 */

import { log, Subsystems, Status } from '../../core/log.js';
import { toast } from '../../shared/toast.js';
import { addItem } from './model/document.js';
import {
  DEFAULT_DPI, FIT_ALIGNS, FIT_VALIGNS, FLIPS, IMAGE_FITS, imageHash, isSvgData,
  releaseImage, resolveFit, resolveFlip, storeImage,
} from './model/images.js';
import { toCssPx } from './model/units.js';
import {
  FIT_HEIGHTS, VALIGNS, WRAP_MODES, defaultFitHeight, defaultWrap, lengthPt, wrapText,
} from './model/wrap.js';
import { formatStaticDate, formatStaticNumber } from './reports-format.js';
import { canonicalImage } from './reports-rasters.js';

export const STATIC_NUMBER = 1;
export const STATIC_IMAGE = 2;
export const STATIC_MEMO = 3;
export const STATIC_DATE = 4;
export const LAYOUT_TYPES = new Set([0, 1, 3, 4, 20, 23]);
export const NUMBER_MASKS = Object.freeze(['#,##0.00', '$#,##0.00', '(#,##0.00)', '0%']);
export const STATIC_BOX = Object.freeze({
  2: { width: '1 in', height: '0.75 in' },
  3: { width: '1.5 in', height: '0.6 in' },
  22: { width: '1 in', height: '0.75 in' },
  23: { width: '1.5 in', height: '0.6 in' },
  43: { width: '1.75 in', height: '0.7 in' },
  45: { width: '1.75 in', height: '0.7 in' },
});
export const STATIC_GHOST = Object.freeze({
  1: 'Number', 2: 'Image', 3: 'Memo', 4: 'Datetime', 43: 'Barcode', 45: 'Barcode',
});

const ROW = Object.freeze({
  value: 'static-value',
  wrap: 'static-wrap',
  fitHeight: 'static-fit-height',
  valign: 'static-valign',
  lineHeight: 'static-line-height',
  dpi: 'static-dpi',
  flip: 'static-flip',
  opacity: 'static-opacity',
  grayscale: 'static-grayscale',
  invert: 'static-invert',
  fitAlign: 'static-fit-align',
  fitValign: 'static-fit-valign',
  numberMask: 'static-number-mask',
  embedFonts: 'embed-fonts',
  file: 'static-file',
});

const SECTION = 7;
const CSS_PX_PER_PT = 96 / 72;

function clone(value) {
  return JSON.parse(JSON.stringify(value));
}

function own(item, key) {
  return Boolean(item) && Object.hasOwn(item, key) && item[key] !== '' && item[key] !== null;
}

/**
 * @param {number} typeKey
 * @returns {{ typeKey: number }}
 */
export function staticItemSpec(typeKey) {
  return { typeKey: Number(typeKey) };
}

function selectedItem(doc, sel) {
  const bands = Array.isArray(doc && doc.bands) ? doc.bands : [];
  const band = bands.find((entry) => entry.id === (sel && sel.bandId));
  const items = band && Array.isArray(band.items) ? band.items : [];
  return items.find((entry) => entry.id === (sel && sel.itemId)) || null;
}

function row(id, label, value, omitted, editor, list, section, group) {
  return {
    id, key: null, label, value, omitted, note: '', editor, list,
    section, group, readOnly: false,
  };
}

/**
 * Value row for static number and datetime. Section Object.
 * @param {object} item
 * @returns {object[]}
 */
export function staticValueRows(item) {
  const empty = !own(item, 'value');
  return [row(ROW.value, 'Value', empty ? '' : String(item.value), empty, 'Text', null, 4, 'Object')];
}

/**
 * Wrap, fit height, vertical align, and line height. Section Formatting.
 * An omitted key shows the type default.
 * @param {object} item
 * @returns {object[]}
 */
export function layoutRows(item) {
  const typeKey = Number(item.typeKey);
  const wrap = own(item, 'wrap') ? item.wrap : '';
  const fit = own(item, 'fitHeight') ? item.fitHeight : '';
  const valign = own(item, 'valign') ? item.valign : '';
  const leading = own(item, 'lineHeight') ? item.lineHeight : '';
  return [
    row(ROW.wrap, 'Wrap', wrap || defaultWrap(typeKey), wrap === '', 'List', [...WRAP_MODES], SECTION, 'Formatting'),
    row(ROW.fitHeight, 'Fit height', fit || defaultFitHeight(typeKey), fit === '', 'List', [...FIT_HEIGHTS], SECTION, 'Formatting'),
    row(ROW.valign, 'Vertical align', valign || 'top', valign === '', 'List', [...VALIGNS], SECTION, 'Formatting'),
    row(ROW.lineHeight, 'Line height', leading === '' ? '1.2' : String(leading), leading === '', 'Number', null, SECTION, 'Formatting'),
  ];
}

/**
 * Fit list for an image when lookup 053 has not listed key 5 yet.
 * @param {object} item
 * @returns {object}
 */
export function imageFitRow(item) {
  const fit = own(item, 'fit') ? item.fit : '';
  return {
    id: '5', key: 5, label: 'Fit', value: fit || 'contain', omitted: fit === '', note: '',
    editor: 'List', list: [...IMAGE_FITS], section: SECTION, group: 'Formatting', readOnly: false,
  };
}

/**
 * Image placement rows. Fit itself is lookup 054 key 5.
 * @param {object} item
 * @returns {object[]}
 */
export function imageRows(item) {
  const dpi = own(item, 'dpi') ? item.dpi : '';
  const flip = own(item, 'flip') ? item.flip : '';
  const opacity = own(item, 'opacity') ? item.opacity : '';
  const gray = item.grayscale === true;
  const invert = item.invert === true;
  const align = own(item, 'fitAlign') ? item.fitAlign : '';
  const valign = own(item, 'fitValign') ? item.fitValign : '';
  const name = typeof item.fileName === 'string' ? item.fileName.trim() : '';
  return [
    row(ROW.file, 'File', name || 'Choose a file', name === '', 'File', null, 4, 'Object'),
    row(ROW.dpi, 'Dpi', dpi === '' ? String(DEFAULT_DPI) : String(dpi), dpi === '', 'Number', null, SECTION, 'Formatting'),
    row(ROW.flip, 'Flip', flip || 'none', flip === '', 'List', [...FLIPS], SECTION, 'Formatting'),
    row(ROW.opacity, 'Opacity', opacity === '' ? '1' : String(opacity), opacity === '', 'Number', null, SECTION, 'Formatting'),
    row(ROW.grayscale, 'Grayscale', gray ? 'True' : 'False', !own(item, 'grayscale'), 'Boolean', null, SECTION, 'Formatting'),
    row(ROW.invert, 'Invert', invert ? 'True' : 'False', !own(item, 'invert'), 'Boolean', null, SECTION, 'Formatting'),
    row(ROW.fitAlign, 'Fit align', align || 'Center', align === '', 'List', [...FIT_ALIGNS], SECTION, 'Formatting'),
    row(ROW.fitValign, 'Fit vertical', valign || 'Middle', valign === '', 'List', [...FIT_VALIGNS], SECTION, 'Formatting'),
  ];
}

/** Copies one number mask into the item mask. */
export function numberMaskRow() {
  return row(ROW.numberMask, 'Mask preset', '', true, 'List', [...NUMBER_MASKS], SECTION, 'Formatting');
}

/**
 * Report flag. Bytes stay in params.fonts, not in the file.
 * @param {object} doc
 * @returns {object}
 */
export function embedFontsRow(doc) {
  const on = doc && doc.embedFonts === true;
  return row(ROW.embedFonts, 'Embed fonts', on ? 'True' : 'False', !(doc && Object.hasOwn(doc, 'embedFonts')), 'Boolean', null, 1, 'Report');
}

function edit(doc, sel, mutate) {
  const next = clone(doc);
  const item = selectedItem(next, sel);
  if (!item) throw new Error('That property needs a selected item.');
  mutate(item);
  return next;
}

function finiteNumber(value) {
  if (typeof value === 'number' && Number.isFinite(value)) return value;
  const text = String(value ?? '').trim();
  if (text === '' || !/^-?(?:\d+(?:\.\d+)?|\.\d+)$/.test(text)) return null;
  const number = Number(text);
  return Number.isFinite(number) ? number : null;
}

function flag(value) {
  return value === true || value === 'True';
}

/**
 * Panel edits for the static rows. Null when the row belongs elsewhere.
 * @param {object} doc
 * @param {object} sel
 * @param {string} rowId
 * @param {*} value
 * @returns {object|null}
 */
export function applyStaticProperty(doc, sel, rowId, value) {
  if (rowId === ROW.embedFonts) {
    const next = clone(doc);
    next.embedFonts = flag(value);
    return next;
  }
  const item = selectedItem(doc, sel);
  if (!item) return null;
  const typeKey = Number(item.typeKey);
  if (rowId === '5' && (typeKey === STATIC_IMAGE || typeKey === 22)) {
    const fit = String(value ?? '').trim();
    if (!IMAGE_FITS.includes(fit)) throw new Error(`Fit must be one of ${IMAGE_FITS.join(', ')}.`);
    return edit(doc, sel, (target) => { target.fit = fit; });
  }
  if (rowId === ROW.value && (typeKey === STATIC_NUMBER || typeKey === STATIC_DATE)) {
    const text = String(value ?? '').trim();
    if (text === '') return edit(doc, sel, (target) => { delete target.value; });
    if (typeKey === STATIC_NUMBER) {
      const number = finiteNumber(value);
      if (number === null) throw new Error('Value is not a number.');
      return edit(doc, sel, (target) => { target.value = number; });
    }
    if (/^\d{4}-\d{2}-\d{2}(T\d{2}:\d{2}:\d{2})?$/.test(text)) {
      return edit(doc, sel, (target) => { target.value = text; });
    }
    const number = finiteNumber(text);
    if (number === null || String(value).trim() !== text) throw new Error('Value is not a date.');
    return edit(doc, sel, (target) => { target.value = number; });
  }
  if (rowId === ROW.wrap && LAYOUT_TYPES.has(typeKey)) {
    const mode = String(value ?? '').trim();
    if (mode === '') return edit(doc, sel, (target) => { delete target.wrap; });
    if (!WRAP_MODES.includes(mode)) throw new Error(`Wrap must be one of ${WRAP_MODES.join(', ')}.`);
    return edit(doc, sel, (target) => { target.wrap = mode; });
  }
  if (rowId === ROW.fitHeight && LAYOUT_TYPES.has(typeKey)) {
    const fit = String(value ?? '').trim();
    if (fit === '') return edit(doc, sel, (target) => { delete target.fitHeight; });
    if (!FIT_HEIGHTS.includes(fit)) throw new Error('Fit height must be clip, grow, shrink, or both.');
    return edit(doc, sel, (target) => { target.fitHeight = fit; });
  }
  if (rowId === ROW.valign && LAYOUT_TYPES.has(typeKey)) {
    const valign = String(value ?? '').trim();
    if (valign === '') return edit(doc, sel, (target) => { delete target.valign; });
    if (!VALIGNS.includes(valign)) throw new Error('Vertical align must be top, middle, or bottom.');
    return edit(doc, sel, (target) => { target.valign = valign; });
  }
  if (rowId === ROW.lineHeight && LAYOUT_TYPES.has(typeKey)) {
    const text = String(value ?? '').trim();
    if (text === '') return edit(doc, sel, (target) => { delete target.lineHeight; });
    const number = finiteNumber(text);
    if (number === null || number <= 0) throw new Error('Line height must be a positive multiple.');
    return edit(doc, sel, (target) => { target.lineHeight = number; });
  }
  if (rowId === ROW.numberMask && typeKey === STATIC_NUMBER) {
    const mask = String(value ?? '').trim();
    if (!NUMBER_MASKS.includes(mask)) throw new Error('Unknown number mask.');
    return edit(doc, sel, (target) => { target.mask = mask; });
  }
  if (typeKey !== STATIC_IMAGE && typeKey !== 22) return null;
  if (rowId === ROW.dpi) {
    const number = finiteNumber(value);
    if (number === null || number <= 0) throw new Error('Dpi must be a positive number.');
    return edit(doc, sel, (target) => { target.dpi = number; });
  }
  if (rowId === ROW.flip) {
    const flip = String(value ?? '').trim();
    if (!FLIPS.includes(flip)) throw new Error(`Flip must be one of ${FLIPS.join(', ')}.`);
    return edit(doc, sel, (target) => { target.flip = flip; });
  }
  if (rowId === ROW.opacity) {
    const number = finiteNumber(value);
    if (number === null || number < 0 || number > 1) throw new Error('Opacity must be from 0 to 1.');
    return edit(doc, sel, (target) => { target.opacity = number; });
  }
  if (rowId === ROW.grayscale) return edit(doc, sel, (target) => { target.grayscale = flag(value); });
  if (rowId === ROW.invert) return edit(doc, sel, (target) => { target.invert = flag(value); });
  if (rowId === ROW.fitAlign) {
    const align = String(value ?? '').trim();
    if (!FIT_ALIGNS.includes(align)) throw new Error('Fit align must be Left, Center, or Right.');
    return edit(doc, sel, (target) => { target.fitAlign = align; });
  }
  if (rowId === ROW.fitValign) {
    const valign = String(value ?? '').trim();
    if (!FIT_VALIGNS.includes(valign)) throw new Error('Fit vertical must be Top, Middle, or Bottom.');
    return edit(doc, sel, (target) => { target.fitValign = valign; });
  }
  return null;
}

const SOURCE_LINE = /^Source:\s+\S.*$/;
const FAKE_PATH = /^[A-Za-z]:\\fakepath\\/i;

/**
 * Notes may take `Source: <path>` when they are empty or that line alone.
 * Any other text stays.
 * @param {unknown} notes
 * @returns {boolean}
 */
export function sourceNoteOpen(notes) {
  const text = typeof notes === 'string' ? notes.trim() : '';
  return text === '' || SOURCE_LINE.test(text);
}

/**
 * The source line to store, or `''` when Notes must stay as they are.
 * @param {unknown} notes
 * @param {string} path
 * @returns {string}
 */
export function sourceNoteFor(notes, path) {
  const source = typeof path === 'string' ? path.trim() : '';
  if (!source || !sourceNoteOpen(notes)) return '';
  return `Source: ${source}`;
}

/**
 * Path for the source note. `file.path` is the full path when the
 * host provides one. A dialog value of `C:\fakepath\` is the name
 * the browser is allowed to share.
 * @param {File|null|undefined} file
 * @param {string} [chosen] The file input value before it is cleared.
 * @returns {string}
 */
export function imageSourcePath(file, chosen) {
  const host = file && typeof file.path === 'string' ? file.path.trim() : '';
  if (host && !FAKE_PATH.test(host)) return host;
  const raw = typeof chosen === 'string' ? chosen.trim() : '';
  if (raw && !FAKE_PATH.test(raw)) return raw;
  const relative = file && typeof file.webkitRelativePath === 'string' ? file.webkitRelativePath.trim() : '';
  if (relative) return relative;
  const name = file && typeof file.name === 'string' ? file.name.trim() : '';
  return name;
}

/**
 * Store a data URI on one image item and drop the previous bytes
 * when nothing else names them. The caller commits.
 * @param {object} doc
 * @param {string} bandId
 * @param {string} itemId
 * @param {string} dataUri
 * @param {string} [fileName] Original file name. A blank name leaves the stored name.
 * @param {string} [sourcePath] Full path for a `Source:` note. A blank path leaves Notes.
 * @returns {object}
 */
export function assignItemImage(doc, bandId, itemId, dataUri, fileName, sourcePath) {
  const next = clone(doc);
  const item = selectedItem(next, { bandId, itemId });
  if (!item || Number(item.typeKey) !== STATIC_IMAGE) throw new Error('Image needs a selected image item.');
  const previous = item.image;
  const hash = storeImage(next, dataUri);
  item.image = hash;
  const name = typeof fileName === 'string' ? fileName.trim() : '';
  if (name) item.fileName = name;
  const note = sourceNoteFor(item.notes, sourcePath);
  if (note) item.notes = note;
  if (previous && previous !== hash) releaseImage(next, previous);
  return next;
}

/**
 * Toast text for a failed image import. The image endpoint puts the
 * reason in `details`, and a 503 body has no `message`.
 * @param {unknown} error
 * @returns {string}
 */
export function importFailureMessage(error) {
  const data = error && typeof error === 'object' ? error.data : null;
  if (data && typeof data.details === 'string' && data.details.trim() !== '') return data.details.trim();
  if (data && typeof data.error === 'string' && data.error.trim() !== '') return data.error.trim();
  if (error instanceof Error && error.message) return error.message;
  return 'Image could not be imported.';
}

function sourceText(item) {
  const typeKey = Number(item.typeKey);
  if (typeKey === STATIC_NUMBER) return formatStaticNumber(item.value, item.mask);
  if (typeKey === STATIC_DATE) return formatStaticDate(item.value, item.mask);
  if (typeof item.text === 'string') return item.text;
  if (typeKey === STATIC_MEMO) return 'Memo';
  return 'Text';
}

function fontOf(doc, item, helvetica) {
  const ownFont = item.font && typeof item.font === 'object' ? item.font : null;
  const report = doc && doc.font && typeof doc.font === 'object' ? doc.font : null;
  const source = ownFont || report || {};
  const family = helvetica ? 'Helvetica' : (typeof source.family === 'string' && source.family !== '' ? source.family : 'Vanadium Sans');
  const style = typeof source.style === 'string' && source.style !== '' ? source.style : 'Normal';
  let size = '10 pt';
  if (typeof source.size === 'string' && source.size !== '') size = source.size;
  let sizePx = toCssPx('10 pt');
  try {
    sizePx = toCssPx(size);
  } catch {
    sizePx = toCssPx('10 pt');
  }
  const sizePt = lengthPt(size) || 10;
  return { family, style, sizePx, sizePt };
}

/**
 * True when the canvas paints pre-broken lines or an image.
 * Field text keeps the column name on the canvas.
 * @param {object} item
 * @returns {boolean}
 */
export function paintsMeasured(item) {
  const typeKey = Number(item && item.typeKey);
  if (typeKey === STATIC_NUMBER || typeKey === STATIC_IMAGE || typeKey === STATIC_MEMO || typeKey === STATIC_DATE
    || typeKey === 22 || typeKey === 23) {
    return true;
  }
  if (typeKey === 0) {
    const fit = item.fitHeight;
    return typeof fit === 'string' && fit !== '' && fit !== 'clip';
  }
  return false;
}

function objectPosition(item) {
  const x = item.fitAlign === 'Left' ? 'left' : item.fitAlign === 'Right' ? 'right' : 'center';
  const y = item.fitValign === 'Top' ? 'top' : item.fitValign === 'Bottom' ? 'bottom' : 'center';
  return `${x} ${y}`;
}

const OBJECT_FIT = {
  stretch: 'fill', contain: 'contain', cover: 'cover', none: 'none', 'scale-down': 'scale-down',
};

function paintImage(el, item, doc, srcOverride) {
  el.classList.add('reports-item-image-item');
  const images = doc && doc.images && typeof doc.images === 'object' ? doc.images : {};
  const src = srcOverride != null
    ? srcOverride
    : (typeof item.image === 'string' ? images[item.image] : '');
  if (!src) {
    const label = document.createElement('span');
    label.className = 'reports-item-text';
    label.textContent = 'Image';
    el.append(label);
    return;
  }
  const img = document.createElement('img');
  img.className = 'reports-item-image';
  img.alt = '';
  img.src = src;
  const fit = resolveFit(typeof item.fit === 'string' ? item.fit : 'contain');
  el.style.setProperty('--reports-image-fit', OBJECT_FIT[fit] || 'contain');
  el.style.setProperty('--reports-image-position', objectPosition(item));
  const flip = resolveFlip(item.flip);
  const fx = flip === 'horizontal' || flip === 'both' ? -1 : 1;
  const fy = flip === 'vertical' || flip === 'both' ? -1 : 1;
  const turn = Number(item.rotation);
  const angle = Number.isFinite(turn) ? turn : 0;
  if (angle !== 0 || fx !== 1 || fy !== 1) {
    el.style.setProperty('--reports-image-transform', `rotate(${angle}deg) scale(${fx}, ${fy})`);
  }
  const opacity = Number(item.opacity);
  if (Number.isFinite(opacity) && opacity !== 1) {
    el.style.setProperty('--reports-image-opacity', String(Math.min(1, Math.max(0, opacity))));
  }
  const filters = [];
  if (item.grayscale === true) filters.push('grayscale(1)');
  if (item.invert === true) filters.push('invert(1)');
  if (filters.length) el.style.setProperty('--reports-image-filter', filters.join(' '));
  el.append(img);
}

/**
 * Paint one static item into the band body. The box is the stored box.
 * @param {HTMLElement} body
 * @param {object} item
 * @param {object} doc
 */
export function paintMeasuredItem(body, item, doc, options = {}) {
  const el = document.createElement('div');
  el.className = 'reports-item';
  el.dataset.itemId = item.id;
  if (options.unbound != null) el.dataset.unbound = String(Boolean(options.unbound));
  const align = item.align === 'Center' || item.align === 'Right' ? item.align : 'Left';
  el.dataset.align = align;
  const typeKey = Number(item.typeKey);
  const fit = typeof item.fitHeight === 'string' && item.fitHeight !== '' ? item.fitHeight : defaultFitHeight(typeKey);
  const helvetica = typeKey !== STATIC_IMAGE && typeKey !== 22 && fit !== 'clip';
  const font = fontOf(doc, item, helvetica);
  el.dataset.fontStyle = font.style;
  el.style.setProperty('--reports-item-font-family', `'${font.family.replace(/'/g, '')}'`);
  el.style.setProperty('--reports-item-font-size', `${font.sizePx}px`);
  const box = item.box || {};
  el.style.left = `${toCssPx(box.left)}px`;
  el.style.top = `${toCssPx(box.top)}px`;
  el.style.width = `${toCssPx(box.width)}px`;
  el.style.height = `${toCssPx(box.height)}px`;
  if (typeKey === STATIC_IMAGE || typeKey === 22) paintImage(el, item, doc, options.src);
  else {
    el.classList.add('reports-item-wrapped');
    const mode = WRAP_MODES.includes(item.wrap) ? item.wrap : defaultWrap(typeKey);
    const widthPt = lengthPt(box.width) || 0;
    const shown = options.text != null ? options.text : sourceText(item);
    const lines = wrapText(shown, widthPt, font.sizePt, font.style, mode);
    const multiple = Number(item.lineHeight);
    const leading = Number.isFinite(multiple) && multiple > 0 ? multiple : 1.2;
    const valign = VALIGNS.includes(item.valign) ? item.valign : 'top';
    el.dataset.valign = valign;
    el.style.setProperty('--reports-item-line-height', `${leading * font.sizePt * CSS_PX_PER_PT}px`);
    const label = document.createElement('span');
    label.className = 'reports-item-text';
    label.textContent = lines.join('\n');
    el.append(label);
  }
  const corner = document.createElement('div');
  corner.className = 'reports-item-corner';
  corner.title = 'Resize item';
  el.append(corner);
  body.append(el);
}

/**
 * Read a local file or a clipboard image and store the canonical bytes.
 * @param {Function} post
 * @param {File} file
 * @returns {Promise<string>}
 */
export async function importImageFile(post, file) {
  return canonicalImage(post, file);
}

/**
 * Put imported bytes on the selected image, or add an image item.
 * @param {object} manager
 * @param {File} file
 * @param {string} [sourcePath] Full path for a `Source:` note.
 */
export async function placeImportedImage(manager, file, sourcePath) {
  if (!manager._recordEditing || !manager.doc || !file) return;
  let dataUri;
  try {
    dataUri = await importImageFile(manager.app && manager.app.api && manager.app.api.post
      ? manager.app.api.post.bind(manager.app.api)
      : null, file);
  } catch (error) {
    const message = importFailureMessage(error);
    log(Subsystems.MANAGER, Status.ERROR, `[Reports] ${message}`);
    toast.error(message, { subsystem: 'Manager' });
    return;
  }
  const bandId = manager._selectedBandId;
  const current = selectedItem(manager.doc, { bandId, itemId: manager._selectedItemId });
  const fileName = file && typeof file.name === 'string' ? file.name : '';
  const path = typeof sourcePath === 'string' && sourcePath.trim() !== ''
    ? sourcePath.trim()
    : imageSourcePath(file);
  try {
    let next;
    let itemId = manager._selectedItemId;
    if (current && Number(current.typeKey) === STATIC_IMAGE) {
      next = assignItemImage(manager.doc, bandId, current.id, dataUri, fileName, path);
    } else {
      if (!bandId) throw new Error('Select a band before adding an image.');
      next = addItem(manager.doc, bandId, staticItemSpec(STATIC_IMAGE));
      const band = next.bands.find((entry) => entry.id === bandId);
      const added = band && band.items[band.items.length - 1];
      if (!added) return;
      itemId = added.id;
      next = assignItemImage(next, bandId, itemId, dataUri, fileName, path);
    }
    manager._selectedItemId = itemId;
    manager._commitDocument(next);
    log(Subsystems.MANAGER, Status.INFO, `[Reports] Stored image ${imageHash(dataUri)} on ${itemId}`);
  } catch (error) {
    manager._failEdit(error, 'Image could not be stored.');
  }
}

export function imageDataUri(doc, item) {
  const images = doc && doc.images;
  if (!images || !item || typeof item.image !== 'string') return '';
  const src = images[item.image];
  return isSvgData(src) ? src : (src || '');
}
