import { readFileSync } from 'node:fs';
import { resolve } from 'node:path';
import { describe, expect, it } from 'vitest';
import { createBlank } from '../../src/managers/reports/model/document.js';
import { pageBox } from '../../src/managers/reports/model/units.js';
import { RULER_SIZE } from '../../src/managers/reports/rulers.js';
import {
  VIEW_SCALE_100,
  VIEW_SCALE_MAX,
  VIEW_SCALE_MIN,
  fitWidthScale,
  formatViewScale,
  stepViewScale,
  viewScaleLabel,
} from '../../src/managers/reports/scale.js';

const css = readFileSync(
  resolve(process.cwd(), 'src/managers/reports/reports.css'),
  'utf8',
);

describe('fit-width scale', () => {
  it('returns the hundredths that fit a known pane around a Letter sheet', () => {
    const box = pageBox(createBlank().page);
    expect(box.widthPx).toBe(816);
    expect(RULER_SIZE).toBe(28);
    const sheet = box.widthPx + RULER_SIZE;
    expect(sheet).toBe(844);
    expect(fitWidthScale(1000, sheet)).toBe(1.18);
    expect(844 * 1.18).toBeLessThanOrEqual(1000);
    expect(fitWidthScale(600, sheet)).toBe(0.71);
    expect(844 * 0.71).toBeLessThanOrEqual(600);
    expect(fitWidthScale(sheet, sheet)).toBe(1);
  });

  it('stays at 100% when the pane or the sheet cannot be measured', () => {
    expect(VIEW_SCALE_100).toBe(1);
    expect(fitWidthScale(0, 844)).toBe(1);
    expect(fitWidthScale(1000, 0)).toBe(1);
    expect(fitWidthScale(-20, 844)).toBe(1);
  });

  it('steps by a tenth and stops at the limits', () => {
    expect(stepViewScale(1, 1)).toBe(1.1);
    expect(stepViewScale(1, -1)).toBe(0.9);
    expect(stepViewScale(1.18, 1)).toBe(1.28);
    expect(stepViewScale(1.18, -1)).toBe(1.08);
    expect(stepViewScale(VIEW_SCALE_MAX, 1)).toBe(VIEW_SCALE_MAX);
    expect(stepViewScale(VIEW_SCALE_MIN, -1)).toBe(VIEW_SCALE_MIN);
    expect(stepViewScale(5, -1)).toBe(4.9);
    expect(stepViewScale(5, 1)).toBe(5);
  });

  it('labels 100% as 96 CSS pixels per inch and formats the variable', () => {
    expect(formatViewScale(1)).toBe('1');
    expect(formatViewScale(1.184)).toBe('1.18');
    expect(formatViewScale(0)).toBe('1');
    expect(viewScaleLabel(1)).toBe('100%');
    expect(viewScaleLabel(1.18)).toBe('118%');
    expect(viewScaleLabel(0.5)).toBe('50%');
  });

  it('scales the page, the grid, and the rulers with one variable', () => {
    expect(css).toMatch(/transform:\s*scale\(var\(--reports-scale,\s*1\)\)/);
    expect(css).toMatch(/width:\s*calc\(\(var\(--reports-ruler-size\) \+ var\(--reports-page-width\)\) \* var\(--reports-scale,\s*1\)\)/);
  });
});
