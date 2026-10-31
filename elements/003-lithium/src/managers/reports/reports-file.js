/**
 * Definition file name and text for Report Manager.
 * Import keeps omitted keys. A failed import leaves the previous document.
 * The download name is `<name>.report.json`. An omitted or blank name
 * uses Report. Characters that break a file name are replaced.
 */

import { createBlank, exportDefinition, importDefinition } from './model/document.js';

const FILE_UNSAFE = '\\/:*?"<>|';

/**
 * Name shown on the shell. An omitted or empty name uses the default.
 * @param {object} doc
 * @returns {string}
 */
export function reportDisplayName(doc) {
  if (doc && typeof doc.name === 'string' && doc.name !== '') return doc.name;
  return 'Report';
}

function safeDownloadBase(label) {
  let out = '';
  for (const char of label) {
    const code = char.codePointAt(0);
    if (code < 32 || FILE_UNSAFE.includes(char)) out += '_';
    else out += char;
  }
  const cleaned = out.trim().replace(/^\.+/, '');
  return cleaned === '' ? 'Report' : cleaned;
}

/**
 * Download base name. Leading dots are dropped so the file stays visible.
 * @param {object} doc
 * @returns {string}
 */
export function reportDownloadName(doc) {
  return `${safeDownloadBase(reportDisplayName(doc))}.report.json`;
}

/**
 * Pretty JSON of the exported definition, with a trailing newline.
 * @param {object} doc
 * @returns {string}
 */
export function definitionText(doc) {
  return `${JSON.stringify(exportDefinition(doc), null, 2)}\n`;
}

const BAND_ROLE = {
  12: 'Report header',
  13: 'Page header',
  14: 'Group header',
  15: 'Detail',
  16: 'Group footer',
  17: 'Page footer',
  18: 'Report footer',
};

function written(value) {
  return typeof value === 'string' && value !== '';
}

function objectValue(value) {
  if (!value || typeof value !== 'object' || Array.isArray(value)) return null;
  return value;
}

function pageText(page) {
  if (!page) return 'Omitted';
  const size = written(page.width) && written(page.height) ? `${page.width} by ${page.height}` : '';
  const orientation = written(page.orientation) ? page.orientation : '';
  const sizeKey = page.sizeKey === undefined || page.sizeKey === null ? '' : `size ${page.sizeKey}`;
  const parts = [size, orientation, sizeKey].filter(Boolean);
  return parts.length ? parts.join(', ') : 'Omitted';
}

function marginText(page) {
  const margin = page && objectValue(page.margin);
  if (!margin) return 'Omitted';
  const sides = ['left', 'right', 'top', 'bottom'];
  const values = sides.map((side) => (written(margin[side]) ? margin[side] : ''));
  if (values.every((value) => value === '')) return 'Omitted';
  if (values.every((value) => value === values[0])) return values[0];
  return sides.map((side, index) => `${side} ${values[index] || 'omitted'}`).join(', ');
}

function gridText(grid) {
  if (!grid) return 'Omitted';
  const parts = [grid.style, grid.spacing].filter(written);
  return parts.length ? parts.join(', ') : 'Omitted';
}

function fontText(font) {
  if (!font) return 'Omitted';
  const parts = [font.family, font.size, font.style].filter(written);
  return parts.length ? parts.join(', ') : 'Omitted';
}

/**
 * Canvas and outline label for a band type key.
 * @param {number} typeKey
 * @returns {string}
 */
export function bandRoleName(typeKey) {
  if (Object.hasOwn(BAND_ROLE, typeKey)) return BAND_ROLE[typeKey];
  if (Number.isInteger(typeKey)) return `Type ${typeKey}`;
  return 'Band';
}

function bandLine(band) {
  const role = bandRoleName(band.typeKey);
  const parts = [role];
  if (written(band.id)) parts.push(band.id);
  if (written(band.height)) parts.push(band.height);
  if (written(band.datasource)) parts.push(band.datasource);
  return parts.join(', ');
}

function itemLine(item) {
  if (written(item.id)) return item.id;
  if (Number.isInteger(item.typeKey)) return `Type ${item.typeKey}`;
  return 'Item';
}

/**
 * Lines the shell shows for the open definition. Omitted sections say so.
 * @param {object} doc
 * @returns {object}
 */
export function reportOutline(doc) {
  const page = objectValue(doc && doc.page);
  const grid = objectValue(doc && doc.grid);
  const font = objectValue(doc && doc.font);
  const bands = Array.isArray(doc && doc.bands) ? doc.bands : [];
  return {
    name: reportDisplayName(doc),
    nameOmitted: !(doc && typeof doc.name === 'string' && doc.name !== ''),
    fileName: reportDownloadName(doc),
    pageText: pageText(page),
    marginText: marginText(page),
    gridText: gridText(grid),
    fontText: fontText(font),
    bands: bands.map((band) => ({
      line: bandLine(band),
      items: Array.isArray(band.items) ? band.items.map(itemLine) : [],
    })),
  };
}

/**
 * Import text. On failure the previous document stays and `error` is set.
 * @param {object|null} current
 * @param {string} text
 * @returns {{ doc: object|null, error: string|null }}
 */
export function openReportText(current, text) {
  try {
    return { doc: importDefinition(text), error: null };
  } catch (error) {
    const message = error instanceof Error && error.message
      ? error.message
      : 'Report file could not be read.';
    return { doc: current, error: message };
  }
}

/** New report from the Defaults table. */
export function blankReport() {
  return createBlank();
}

/**
 * Browser download of the definition text.
 * @param {string} filename
 * @param {string} json
 */
export function saveReportFile(filename, json) {
  const blob = new Blob([json], { type: 'application/json' });
  const url = URL.createObjectURL(blob);
  const anchor = document.createElement('a');
  anchor.href = url;
  anchor.download = filename;
  anchor.rel = 'noopener';
  document.body.appendChild(anchor);
  anchor.click();
  anchor.remove();
  URL.revokeObjectURL(url);
}
