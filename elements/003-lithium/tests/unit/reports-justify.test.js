import { describe, expect, it } from 'vitest';
import {
  justifyWordSpacings, layoutBand, measurePt, wrapText,
} from '../../src/managers/reports/model/wrap.js';

describe('justifyWordSpacings', () => {
  it('stretches full lines and leaves the last line at 0', () => {
    const text = 'the quick brown fox jumps over the lazy dog again';
    const width = 120;
    const lines = wrapText(text, width, 12, 'Normal', 'pre-line', 'Helvetica');
    expect(lines.length).toBeGreaterThan(1);
    const gaps = justifyWordSpacings(lines, width, 12, 'Normal', 'Helvetica', 'pre-line', text);
    expect(gaps[gaps.length - 1]).toBe(0);
    for (let i = 0; i < lines.length - 1; i += 1) {
      const parts = lines[i].split(' ').filter(Boolean);
      if (parts.length < 2) {
        expect(gaps[i]).toBe(0);
        continue;
      }
      expect(gaps[i]).toBeGreaterThan(0);
      const spaceW = measurePt(' ', 12, 'Normal', 'Helvetica');
      let natural = 0;
      parts.forEach((w, wi) => {
        natural += measurePt(w, 12, 'Normal', 'Helvetica');
        if (wi < parts.length - 1) natural += spaceW;
      });
      expect(natural + gaps[i] * (parts.length - 1)).toBeCloseTo(width, 4);
    }
  });

  it('does not stretch nowrap or single-word lines', () => {
    expect(justifyWordSpacings(['hello world'], 200, 12, 'Normal', 'Helvetica', 'nowrap', 'hello world'))
      .toEqual([0]);
    const gaps = justifyWordSpacings(['hello', 'world'], 200, 12, 'Normal', 'Helvetica', 'pre-line', 'hello\nworld');
    expect(gaps).toEqual([0, 0]);
  });

  it('treats hard-newline ends as last (not stretched)', () => {
    const text = 'one two three four\nfive six';
    const width = 80;
    const lines = wrapText(text, width, 12, 'Normal', 'pre-line', 'Helvetica');
    const gaps = justifyWordSpacings(lines, width, 12, 'Normal', 'Helvetica', 'pre-line', text);
    // 'four' alone (end of first paragraph) and last line stay 0
    lines.forEach((line, i) => {
      if (line === 'four' || i === lines.length - 1) expect(gaps[i]).toBe(0);
    });
  });

  it('layoutBand returns wordSpacings for Justified Grow', () => {
    const doc = { font: { family: 'Helvetica', size: '12 pt', style: 'Normal' } };
    const band = { height: '1 in' };
    const item = {
      id: 'j1', typeKey: 0, overflow: 'Grow', wrap: 'pre-line', align: 'Justified',
      text: 'the quick brown fox jumps over the lazy dog again',
      box: { left: '0 in', top: '0 in', width: '120 pt', height: '0.4 in' },
      font: { family: 'Helvetica', size: '12 pt', style: 'Normal' },
    };
    const laid = layoutBand(doc, band, [{ item, text: item.text }]);
    const row = laid.items[0];
    expect(row.align).toBe('Justified');
    expect(Array.isArray(row.wordSpacings)).toBe(true);
    expect(row.wordSpacings.length).toBe(row.lines.length);
    expect(row.wordSpacings[row.wordSpacings.length - 1]).toBe(0);
  });

  it('Ellipsis last visible line is not justified', () => {
    const doc = { font: { family: 'Helvetica', size: '12 pt', style: 'Normal' } };
    const band = { height: '1 in' };
    const text = 'alpha bravo charlie delta echo foxtrot golf hotel india juliet kilo lima mike november oscar';
    const item = {
      id: 'je', typeKey: 0, overflow: 'Ellipsis', wrap: 'pre-line', align: 'Justified',
      text,
      box: { left: '0 in', top: '0 in', width: '1.2 in', height: '0.5 in' },
      font: { family: 'Helvetica', size: '12 pt', style: 'Normal' },
    };
    const laid = layoutBand(doc, band, [{ item, text }]);
    const row = laid.items[0];
    expect(row.lines.length).toBeGreaterThan(1);
    // Truncation may leave a full-width last line without a glyph ellipsis;
    // forceLast still zeroes its word spacing.
    expect(row.wordSpacings[row.wordSpacings.length - 1]).toBe(0);
    expect(row.wordSpacings.slice(0, -1).some((g) => g > 0)).toBe(true);
  });
});
