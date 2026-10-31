/**
 * Report length strings.
 * One inch is 96 CSS pixels on the unzoomed canvas. A point is 1/72 inch.
 */

export const CSS_PX_PER_INCH = 96;

const PER_INCH = {
  in: 1,
  mm: 25.4,
  cm: 2.54,
  pt: 72,
  px: CSS_PX_PER_INCH,
};

const UNIT_LIST = 'in, mm, cm, pt, or px';
const DEFAULT_MARGIN = '0.25 in';
const DEFAULT_UNIT = 'in';
const LENGTH_PATTERN = /^\s*([+-]?(?:\d+(?:\.\d+)?|\.\d+))\s*([A-Za-z]+)\s*$/;

function snapNearInteger(value) {
  const nearest = Math.round(value);
  if (Math.abs(value - nearest) < 1e-9) return nearest;
  return value;
}

function assertUnit(unit) {
  if (!Object.hasOwn(PER_INCH, unit)) {
    throw new Error(`Unknown unit "${unit}". Use ${UNIT_LIST}.`);
  }
}

function formatNumber(value) {
  const rounded = Math.round(snapNearInteger(value) * 1e6) / 1e6;
  if (rounded === 0) return '0';
  return rounded.toFixed(6).replace(/\.?0+$/, '');
}

/**
 * @param {string} input Length such as "8.5 in".
 * @returns {{ value: number, unit: string }}
 */
export function parseLength(input) {
  if (typeof input !== 'string') {
    throw new Error(`Length must be a string with a unit (${UNIT_LIST}).`);
  }
  const match = LENGTH_PATTERN.exec(input);
  if (!match) {
    const shown = input.trim();
    if (shown === '') throw new Error('Length is empty.');
    if (/^[+-]?(?:\d+(?:\.\d+)?|\.\d+)$/.test(shown)) {
      throw new Error(`Length "${shown}" needs a unit (${UNIT_LIST}).`);
    }
    throw new Error(`Length "${shown}" is not a number and a unit.`);
  }
  const unit = match[2].toLowerCase();
  assertUnit(unit);
  return { value: Number(match[1]), unit };
}

/**
 * @param {number} value
 * @param {string} unit
 * @returns {string}
 */
export function formatLength(value, unit) {
  if (typeof unit !== 'string') {
    throw new Error(`Unknown unit "${unit}". Use ${UNIT_LIST}.`);
  }
  const normalized = unit.toLowerCase();
  assertUnit(normalized);
  if (typeof value !== 'number' || !Number.isFinite(value)) {
    throw new Error('Length value must be a finite number.');
  }
  return `${formatNumber(value)} ${normalized}`;
}

function toInches(input) {
  const parsed = parseLength(input);
  return parsed.value / PER_INCH[parsed.unit];
}

function toCssPxFromInches(inches) {
  return snapNearInteger(inches * CSS_PX_PER_INCH);
}

/** @param {string} input */
export function toCssPx(input) {
  return toCssPxFromInches(toInches(input));
}

/**
 * Convert a length into another unit. 96 CSS pixels and 72 points are 1 inch.
 * @param {string} input
 * @param {string} unit
 * @returns {string}
 */
export function convertLength(input, unit) {
  if (typeof unit !== 'string') {
    throw new Error(`Unknown unit "${unit}". Use ${UNIT_LIST}.`);
  }
  const normalized = unit.toLowerCase();
  assertUnit(normalized);
  return formatLength(toInches(input) * PER_INCH[normalized], normalized);
}

function marginSide(margin, name) {
  const value = margin[name];
  if (value === undefined || value === null || value === '') return DEFAULT_MARGIN;
  return value;
}

/**
 * Page size minus margins. Width and height are strings in the document
 * unit. An omitted margin side is 0.25 in. An omitted document unit is in.
 * @param {{ units?: string, width: string, height: string, margin?: object }} page
 */
export function contentBox(page) {
  if (!page || typeof page !== 'object') throw new Error('Page is missing.');
  const units = typeof page.units === 'string' && page.units !== ''
    ? page.units.toLowerCase()
    : DEFAULT_UNIT;
  assertUnit(units);
  const margin = page.margin && typeof page.margin === 'object' ? page.margin : {};
  const widthIn = toInches(page.width)
    - toInches(marginSide(margin, 'left'))
    - toInches(marginSide(margin, 'right'));
  const heightIn = toInches(page.height)
    - toInches(marginSide(margin, 'top'))
    - toInches(marginSide(margin, 'bottom'));
  const perInch = PER_INCH[units];
  return {
    units,
    width: formatLength(widthIn * perInch, units),
    height: formatLength(heightIn * perInch, units),
    widthPx: toCssPxFromInches(widthIn),
    heightPx: toCssPxFromInches(heightIn),
  };
}
