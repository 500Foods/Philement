/**
 * Report definition: import, export, and edits.
 * Import keeps omitted keys omitted. Create writes the Defaults table.
 * Lengths written by an edit are stored in the document unit.
 * A generated band id is b1, b2, ... and a generated item id is i1, i2, ...
 * createBlank uses ph, detail, and pf.
 */

import {
  contentBox,
  convertLength,
  formatLength,
  parseLength,
} from './units.js';

const BAND_HEIGHT = {
  12: '0.4 in',
  13: '0.4 in',
  14: '0.25 in',
  15: '0.25 in',
  16: '0.25 in',
  17: '0.4 in',
  18: '0.4 in',
};

const ITEM_TYPES = new Set([
  0, 5, 6, 20, 21, 24, 28, 29, 32, 34, 35, 38, 42, 43, 44, 45, 61, 62, 63,
]);

const MASK_TYPES = new Set([5, 21, 24, 29, 32, 35, 38, 61, 62, 63]);

const MIN_BAND_INCHES = 0.15;
const ITEM_WIDTH = '1.5 in';
const ITEM_HEIGHT = '0.2 in';

function clone(value) {
  return JSON.parse(JSON.stringify(value));
}

function formatShown(format) {
  if (format === undefined) return 'no format';
  return JSON.stringify(format);
}

function assertId(id, label) {
  if (typeof id !== 'string' || id === '') throw new Error(`${label} needs an id.`);
}

function assertDefinition(value) {
  if (!value || typeof value !== 'object' || Array.isArray(value)) {
    throw new Error('Report file must be a JSON object.');
  }
  if (value.format !== 1) {
    throw new Error(`Report format must be 1. This file has ${formatShown(value.format)}.`);
  }
  if (value.datasources !== undefined && !Array.isArray(value.datasources)) {
    throw new Error('Datasources must be an array.');
  }
  if (value.functions !== undefined && !Array.isArray(value.functions)) {
    throw new Error('Functions must be an array.');
  }
  if (value.bands === undefined) return;
  if (!Array.isArray(value.bands)) throw new Error('Bands must be an array.');
  const seenBands = new Set();
  const seenItems = new Set();
  for (const band of value.bands) {
    if (!band || typeof band !== 'object' || Array.isArray(band)) {
      throw new Error('Each band must be an object.');
    }
    assertId(band.id, 'Band');
    if (seenBands.has(band.id)) throw new Error(`Band id "${band.id}" is already used.`);
    seenBands.add(band.id);
    if (band.items === undefined) continue;
    if (!Array.isArray(band.items)) {
      throw new Error(`Items on band "${band.id}" must be an array.`);
    }
    for (const item of band.items) {
      if (!item || typeof item !== 'object' || Array.isArray(item)) {
        throw new Error(`Each item on band "${band.id}" must be an object.`);
      }
      assertId(item.id, 'Item');
      if (seenItems.has(item.id)) throw new Error(`Item id "${item.id}" is already used.`);
      seenItems.add(item.id);
    }
  }
}

function parseInput(input) {
  if (typeof input !== 'string') return input;
  try {
    return JSON.parse(input);
  } catch {
    throw new Error('Report file is not JSON.');
  }
}

function copyDefinition(input) {
  const value = parseInput(input);
  assertDefinition(value);
  return clone(value);
}

/**
 * @param {object|string} input Parsed definition or JSON text.
 * @returns {object}
 */
export function importDefinition(input) {
  return copyDefinition(input);
}

/**
 * Detached copy. Key order is not significant.
 * @param {object} doc
 * @returns {object}
 */
export function exportDefinition(doc) {
  return copyDefinition(doc);
}

function blankBand(id, typeKey) {
  return {
    id,
    typeKey,
    height: BAND_HEIGHT[typeKey],
    items: [],
  };
}

/** New report. Letter portrait, dots grid, Vanadium Sans, three bands. */
export function createBlank() {
  return {
    format: 1,
    name: 'Report',
    title: 'Report',
    page: {
      sizeKey: 0,
      orientation: 'portrait',
      units: 'in',
      width: '8.5 in',
      height: '11 in',
      margin: {
        left: '0.25 in',
        right: '0.25 in',
        top: '0.25 in',
        bottom: '0.25 in',
      },
    },
    grid: {
      visible: true,
      style: 'dots',
      spacing: '0.125 in',
      weight: '0.5',
      color: 'silver',
      background: 'white',
    },
    font: { family: 'Vanadium Sans', size: '10 pt', style: 'Normal' },
    datasources: [],
    bands: [
      blankBand('ph', 13),
      blankBand('detail', 15),
      blankBand('pf', 17),
    ],
    functions: [],
  };
}

function documentUnit(doc) {
  const units = doc.page && doc.page.units;
  if (typeof units !== 'string' || units === '') return 'in';
  return units;
}

function storedLength(length, unit) {
  return convertLength(length, unit);
}

function clampBandHeight(length, unit) {
  const inches = parseLength(convertLength(length, 'in')).value;
  if (inches < MIN_BAND_INCHES - 1e-9) return storedLength('0.15 in', unit);
  return storedLength(length, unit);
}

function bandIds(doc) {
  if (!Array.isArray(doc.bands)) return new Set();
  return new Set(doc.bands.map((band) => band.id));
}

function itemIds(doc) {
  const ids = new Set();
  if (!Array.isArray(doc.bands)) return ids;
  for (const band of doc.bands) {
    if (!Array.isArray(band.items)) continue;
    for (const item of band.items) ids.add(item.id);
  }
  return ids;
}

function nextId(used, prefix) {
  let n = 1;
  while (used.has(`${prefix}${n}`)) n += 1;
  return `${prefix}${n}`;
}

function requireBandType(typeKey) {
  if (typeof typeKey !== 'number' || !Object.hasOwn(BAND_HEIGHT, typeKey)) {
    const shown = typeof typeKey === 'number' ? String(typeKey) : JSON.stringify(typeKey);
    throw new Error(`Band type ${shown} is not a v1 band.`);
  }
  return typeKey;
}

function requireItemType(typeKey) {
  if (typeof typeKey !== 'number' || !ITEM_TYPES.has(typeKey)) {
    const shown = typeof typeKey === 'number' ? String(typeKey) : JSON.stringify(typeKey);
    throw new Error(`Item type ${shown} is not a v1 item.`);
  }
  return typeKey;
}

function requireIndex(index, max, label) {
  if (!Number.isInteger(index) || index < 0 || index > max) {
    throw new Error(`${label} index ${index} is outside the report.`);
  }
  return index;
}

function ensureBands(doc) {
  if (doc.bands === undefined) doc.bands = [];
  if (!Array.isArray(doc.bands)) throw new Error('Bands must be an array.');
  return doc.bands;
}

function findBand(doc, id) {
  const bands = ensureBands(doc);
  const band = bands.find((entry) => entry.id === id);
  if (!band) throw new Error(`Band "${id}" was not found.`);
  return band;
}

function findItem(band, itemId) {
  if (!Array.isArray(band.items)) throw new Error(`Item "${itemId}" was not found on band "${band.id}".`);
  const item = band.items.find((entry) => entry.id === itemId);
  if (!item) throw new Error(`Item "${itemId}" was not found on band "${band.id}".`);
  return item;
}

function effectiveBandHeight(band) {
  if (typeof band.height === 'string' && band.height !== '') return band.height;
  if (Object.hasOwn(BAND_HEIGHT, band.typeKey)) return BAND_HEIGHT[band.typeKey];
  throw new Error(`Band "${band.id}" needs a height.`);
}

function centeredBox(doc, band) {
  const unit = documentUnit(doc);
  const page = doc.page;
  if (!page || typeof page.width !== 'string' || typeof page.height !== 'string') {
    throw new Error('Page needs a width and a height.');
  }
  const content = contentBox({
    units: unit,
    width: page.width,
    height: page.height,
    margin: page.margin,
  });
  const contentWidthIn = parseLength(convertLength(content.width, 'in')).value;
  const bandHeightIn = parseLength(convertLength(effectiveBandHeight(band), 'in')).value;
  const itemWidthIn = parseLength(ITEM_WIDTH).value;
  const itemHeightIn = parseLength(ITEM_HEIGHT).value;
  const leftIn = Math.max(0, (contentWidthIn - itemWidthIn) / 2);
  const topIn = Math.max(0, (bandHeightIn - itemHeightIn) / 2);
  return {
    left: storedLength(formatLength(leftIn, 'in'), unit),
    top: storedLength(formatLength(topIn, 'in'), unit),
    width: storedLength(ITEM_WIDTH, unit),
    height: storedLength(ITEM_HEIGHT, unit),
  };
}

function storedBox(box, unit) {
  if (!box || typeof box !== 'object' || Array.isArray(box)) {
    throw new Error('Item box must be an object.');
  }
  for (const side of ['left', 'top', 'width', 'height']) {
    if (typeof box[side] !== 'string') throw new Error(`Item box needs ${side}.`);
  }
  return {
    ...box,
    left: storedLength(box.left, unit),
    top: storedLength(box.top, unit),
    width: storedLength(box.width, unit),
    height: storedLength(box.height, unit),
  };
}

function takeBox(doc, band, item) {
  if (item.box === undefined) return centeredBox(doc, band);
  return { ...item.box };
}

/**
 * @param {object} doc
 * @param {{ id?: string, typeKey: number, height?: string, index?: number, items?: object[] }} spec
 * @returns {object}
 */
export function addBand(doc, spec) {
  if (!spec || typeof spec !== 'object' || Array.isArray(spec)) {
    throw new Error('Band needs a typeKey.');
  }
  const next = clone(doc);
  const bands = ensureBands(next);
  const typeKey = requireBandType(spec.typeKey);
  const id = spec.id === undefined ? nextId(bandIds(next), 'b') : spec.id;
  assertId(id, 'Band');
  if (bandIds(next).has(id)) throw new Error(`Band id "${id}" is already used.`);
  const unit = documentUnit(next);
  const band = { ...clone(spec) };
  delete band.index;
  band.id = id;
  band.typeKey = typeKey;
  band.height = spec.height === undefined
    ? storedLength(BAND_HEIGHT[typeKey], unit)
    : clampBandHeight(spec.height, unit);
  if (!Array.isArray(band.items)) band.items = [];
  const seen = itemIds(next);
  for (const item of band.items) {
    if (!item || typeof item !== 'object' || Array.isArray(item)) {
      throw new Error(`Each item on band "${id}" must be an object.`);
    }
    assertId(item.id, 'Item');
    if (seen.has(item.id)) throw new Error(`Item id "${item.id}" is already used.`);
    seen.add(item.id);
  }
  const index = spec.index === undefined ? bands.length : requireIndex(spec.index, bands.length, 'Band');
  bands.splice(index, 0, band);
  return next;
}

/**
 * @param {object} doc
 * @param {string} id
 * @param {number} index Destination index in the band list.
 * @returns {object}
 */
export function moveBand(doc, id, index) {
  const next = clone(doc);
  const bands = ensureBands(next);
  const from = bands.findIndex((band) => band.id === id);
  if (from < 0) throw new Error(`Band "${id}" was not found.`);
  const to = requireIndex(index, bands.length - 1, 'Band');
  const [band] = bands.splice(from, 1);
  bands.splice(to, 0, band);
  return next;
}

/**
 * @param {object} doc
 * @param {string} id
 * @param {string} height Length string. Clamped to 0.15 in.
 * @returns {object}
 */
export function resizeBand(doc, id, height) {
  const next = clone(doc);
  const band = findBand(next, id);
  band.height = clampBandHeight(height, documentUnit(next));
  return next;
}

/**
 * @param {object} doc
 * @param {string} id
 * @returns {object}
 */
export function deleteBand(doc, id) {
  const next = clone(doc);
  const bands = ensureBands(next);
  const index = bands.findIndex((band) => band.id === id);
  if (index < 0) throw new Error(`Band "${id}" was not found.`);
  bands.splice(index, 1);
  return next;
}

/**
 * @param {object} doc
 * @param {string} bandId
 * @param {{ id?: string, typeKey: number, index?: number, box?: object }} spec
 * @returns {object}
 */
export function addItem(doc, bandId, spec) {
  if (!spec || typeof spec !== 'object' || Array.isArray(spec)) {
    throw new Error('Item needs a typeKey.');
  }
  const next = clone(doc);
  const band = findBand(next, bandId);
  const typeKey = requireItemType(spec.typeKey);
  const id = spec.id === undefined ? nextId(itemIds(next), 'i') : spec.id;
  assertId(id, 'Item');
  if (itemIds(next).has(id)) throw new Error(`Item id "${id}" is already used.`);
  const unit = documentUnit(next);
  const item = { ...clone(spec) };
  delete item.index;
  item.id = id;
  item.typeKey = typeKey;
  if (!Object.hasOwn(item, 'align')) item.align = 'Left';
  item.box = item.box === undefined ? centeredBox(next, band) : storedBox(item.box, unit);
  if (typeKey === 0 && !Object.hasOwn(item, 'text')) item.text = 'Text';
  if (MASK_TYPES.has(typeKey) && !Object.hasOwn(item, 'mask')) item.mask = '';
  if (!Array.isArray(band.items)) band.items = [];
  const index = spec.index === undefined
    ? band.items.length
    : requireIndex(spec.index, band.items.length, 'Item');
  band.items.splice(index, 0, item);
  return next;
}

/**
 * @param {object} doc
 * @param {string} bandId
 * @param {string} itemId
 * @param {{ left?: string, top?: string }} position
 * @returns {object}
 */
export function moveItem(doc, bandId, itemId, position) {
  if (!position || (position.left === undefined && position.top === undefined)) {
    throw new Error('Move needs a left or a top.');
  }
  const next = clone(doc);
  const band = findBand(next, bandId);
  const item = findItem(band, itemId);
  const unit = documentUnit(next);
  const box = takeBox(next, band, item);
  if (position.left !== undefined) box.left = storedLength(position.left, unit);
  if (position.top !== undefined) box.top = storedLength(position.top, unit);
  item.box = box;
  return next;
}

/**
 * @param {object} doc
 * @param {string} bandId
 * @param {string} itemId
 * @param {{ width?: string, height?: string }} size
 * @returns {object}
 */
export function resizeItem(doc, bandId, itemId, size) {
  if (!size || (size.width === undefined && size.height === undefined)) {
    throw new Error('Resize needs a width or a height.');
  }
  const next = clone(doc);
  const band = findBand(next, bandId);
  const item = findItem(band, itemId);
  const unit = documentUnit(next);
  const box = takeBox(next, band, item);
  if (size.width !== undefined) box.width = storedLength(size.width, unit);
  if (size.height !== undefined) box.height = storedLength(size.height, unit);
  item.box = box;
  return next;
}

/**
 * @param {object} doc
 * @param {string} bandId
 * @param {string} itemId
 * @returns {object}
 */
export function deleteItem(doc, bandId, itemId) {
  const next = clone(doc);
  const band = findBand(next, bandId);
  if (!Array.isArray(band.items)) {
    throw new Error(`Item "${itemId}" was not found on band "${bandId}".`);
  }
  const index = band.items.findIndex((item) => item.id === itemId);
  if (index < 0) throw new Error(`Item "${itemId}" was not found on band "${bandId}".`);
  band.items.splice(index, 1);
  return next;
}
