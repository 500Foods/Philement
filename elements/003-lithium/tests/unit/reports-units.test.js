import { readFileSync } from 'node:fs';
import { resolve } from 'node:path';
import { describe, expect, it } from 'vitest';
import {
  CSS_PX_PER_INCH,
  contentBox,
  convertLength,
  formatLength,
  parseLength,
  toCssPx,
} from '../../src/managers/reports/model/units.js';

const DEFINITIONS = [
  'static.definition.json',
  'detail.definition.json',
  'grouped.definition.json',
];

function loadPage(name) {
  const path = resolve(process.cwd(), 'tests/fixtures/reporting', name);
  return JSON.parse(readFileSync(path, 'utf8')).page;
}

describe('report lengths', () => {
  it('converts 1 inch to 96 CSS pixels', () => {
    expect(CSS_PX_PER_INCH).toBe(96);
    expect(toCssPx('1 in')).toBe(96);
  });

  it('converts the other units from the same inch', () => {
    expect(toCssPx('1 px')).toBe(1);
    expect(toCssPx('72 pt')).toBe(96);
    expect(toCssPx('25.4 mm')).toBe(96);
    expect(toCssPx('2.54 cm')).toBe(96);
  });

  it('formats a parsed length back to a unit string', () => {
    expect(formatLength(8.5, 'in')).toBe('8.5 in');
    expect(formatLength(parseLength('0.125 in').value, 'in')).toBe('0.125 in');
    expect(formatLength(10, 'mm')).toBe('10 mm');
    expect(parseLength('11 in')).toEqual({ value: 11, unit: 'in' });
  });

  it('converts a length into another unit', () => {
    expect(convertLength('96 px', 'in')).toBe('1 in');
    expect(convertLength('1 in', 'in')).toBe('1 in');
    expect(convertLength('1 in', 'mm')).toBe('25.4 mm');
    expect(convertLength('10 mm', 'cm')).toBe('1 cm');
    expect(convertLength('72 pt', 'px')).toBe('96 px');
  });

  it('rejects a bare number and an unknown unit', () => {
    expect(() => parseLength('8.5')).toThrow(/needs a unit/);
    expect(() => parseLength(8.5)).toThrow(/string with a unit/);
    expect(() => parseLength('8.5 furlong')).toThrow(/Unknown unit "furlong"/);
    expect(() => toCssPx('')).toThrow(/empty/);
  });

  it('measures the Phase 1 pages at 96 CSS pixels per inch', () => {
    for (const name of DEFINITIONS) {
      const page = loadPage(name);
      expect(page.width).toBe('8.5 in');
      expect(page.height).toBe('11 in');
      expect(page.margin).toEqual({
        left: '0.25 in',
        right: '0.25 in',
        top: '0.25 in',
        bottom: '0.25 in',
      });
      expect(contentBox(page)).toEqual({
        units: 'in',
        width: '8 in',
        height: '10.5 in',
        widthPx: 8 * 96,
        heightPx: 10.5 * 96,
      });
    }
  });

  it('keeps the full page when every margin is zero', () => {
    expect(contentBox({
      units: 'in',
      width: '8.5 in',
      height: '11 in',
      margin: { left: '0 in', right: '0 in', top: '0 in', bottom: '0 in' },
    })).toEqual({
      units: 'in',
      width: '8.5 in',
      height: '11 in',
      widthPx: 8.5 * 96,
      heightPx: 11 * 96,
    });
  });

  it('uses 0.25 in when a margin side is omitted', () => {
    expect(contentBox({
      units: 'in',
      width: '8.5 in',
      height: '11 in',
    }).width).toBe('8 in');
  });
});
