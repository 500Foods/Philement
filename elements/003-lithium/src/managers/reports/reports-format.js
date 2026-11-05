/**
 * Canvas masks for static number and static datetime.
 * The preview uses the same rules in Reports.Render.
 * A civil string is not shifted. A unix number is UTC.
 */

import { DateTime } from 'luxon';

function decimalString(value) {
  let text = value.toPrecision(15);
  if (Number(text) !== value) text = value.toPrecision(17);
  const match = /^([0-9.]+)e([+-]\d+)$/i.exec(text);
  if (!match) return text;
  const mantissa = match[1];
  const exp = Number(match[2]);
  const parts = mantissa.split('.');
  const digits = `${parts[0]}${parts[1] || ''}`;
  const point = parts[0].length + exp;
  if (point <= 0) return `0.${'0'.repeat(-point)}${digits}`;
  if (point >= digits.length) return `${digits}${'0'.repeat(point - digits.length)}`;
  return `${digits.slice(0, point)}.${digits.slice(point)}`;
}

function roundDigits(intPart, frac, places) {
  if (frac.length <= places) return [intPart, frac + '0'.repeat(places - frac.length)];
  const up = frac.charCodeAt(places) >= 53;
  let all = intPart + frac.slice(0, places);
  if (up) {
    const digits = [...all].map((ch) => ch.charCodeAt(0) - 48);
    let index = digits.length - 1;
    while (index >= 0) {
      if (digits[index] === 9) {
        digits[index] = 0;
        index -= 1;
      } else {
        digits[index] += 1;
        break;
      }
    }
    all = `${index < 0 ? '1' : ''}${digits.map((digit) => String.fromCharCode(digit + 48)).join('')}`;
  }
  const cut = all.length - places;
  return [all.slice(0, cut), all.slice(cut)];
}

/**
 * @param {string} mask
 * @returns {object|null}
 */
export function numberMask(mask) {
  let body = String(mask);
  let paren = false;
  const wrapped = /^\((.*)\)$/.exec(body);
  if (wrapped) {
    paren = true;
    body = wrapped[1];
  }
  let prefix = '';
  if (body.startsWith('$')) {
    prefix = '$';
    body = body.slice(1);
  }
  let pct = false;
  if (body.endsWith('%')) {
    pct = true;
    body = body.slice(0, -1);
  }
  const dotted = /^([#0,]*)\.([#0]*)$/.exec(body);
  const plain = /^([#0,]*)$/.exec(body);
  const intPat = dotted ? dotted[1] : (plain ? plain[1] : null);
  const fracPat = dotted ? dotted[2] : '';
  if (intPat === null || `${intPat}${fracPat}` === '' || !/[#0]/.test(`${intPat}${fracPat}`)) return null;
  if (/#0/.test(fracPat)) return null;
  return {
    paren,
    prefix,
    pct,
    comma: intPat.includes(','),
    minint: (intPat.match(/0/g) || []).length,
    minfrac: (fracPat.match(/0/g) || []).length,
    maxfrac: fracPat.length,
  };
}

/**
 * @param {number} value
 * @param {object} spec
 * @returns {string}
 */
export function applyNumber(value, spec) {
  let next = spec.pct ? value * 100 : value;
  let neg = next < 0;
  const text = decimalString(Math.abs(next));
  const match = /^(\d*)\.?(\d*)$/.exec(text);
  let intPart = match ? match[1] : '0';
  let frac = match ? match[2] : '';
  [intPart, frac] = roundDigits(intPart, frac, spec.maxfrac);
  intPart = intPart.replace(/^0+/, '');
  if (intPart.length < spec.minint) intPart = `${'0'.repeat(spec.minint - intPart.length)}${intPart}`;
  while (frac.length > spec.minfrac && frac.endsWith('0')) frac = frac.slice(0, -1);
  if (spec.comma && intPart.length > 3) {
    intPart = intPart.split('').reverse().join('').replace(/(\d{3})/g, '$1,').split('').reverse().join('').replace(/^,/, '');
  }
  const shown = `${spec.prefix}${intPart}${frac !== '' ? `.${frac}` : ''}${spec.pct ? '%' : ''}`;
  if (!/[1-9]/.test(`${intPart}${frac}`)) neg = false;
  if (neg) return spec.paren ? `(${shown})` : `-${shown}`;
  return shown;
}

/**
 * Static number text. A non-number is blank.
 * @param {unknown} value
 * @param {string} [mask]
 * @returns {string}
 */
export function formatStaticNumber(value, mask) {
  if (value === undefined || value === null || value === '') return '';
  if (typeof value !== 'number' || !Number.isFinite(value)) return '';
  if (mask === undefined || mask === '') return String(value);
  const spec = numberMask(mask);
  if (!spec) return String(value);
  return applyNumber(value, spec);
}

function civil(value) {
  if (typeof value === 'number' && Number.isFinite(value)) {
    const dt = DateTime.fromSeconds(value, { zone: 'utc' });
    return dt.isValid ? dt : null;
  }
  if (typeof value !== 'string') return null;
  if (!/^\d{4}-\d{2}-\d{2}(T\d{2}:\d{2}:\d{2})?$/.test(value)) return null;
  const dt = DateTime.fromISO(value, { zone: 'utc' });
  return dt.isValid ? dt : null;
}

function hasTime(value) {
  return typeof value === 'number' || (typeof value === 'string' && value.includes('T'));
}

/**
 * Static datetime text. A value the mask cannot read returns the raw text.
 * @param {unknown} value
 * @param {string} [mask]
 * @returns {string}
 */
export function formatStaticDate(value, mask) {
  if (value === undefined || value === null || value === '') return '';
  const dt = civil(value);
  if (!dt) return String(value);
  const pattern = mask && mask !== '' ? mask : (hasTime(value) ? 'yyyy-MM-dd HH:mm' : 'yyyy-MM-dd');
  try {
    const shown = dt.toFormat(pattern);
    return shown || String(value);
  } catch {
    return String(value);
  }
}
