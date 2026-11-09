import { describe, expect, it, beforeEach } from 'vitest';
import {
  filterNamedColours,
  mergeNamedColourSources,
  parseNamedColourRows,
  resolveNamedColour,
  setNamedColoursFromLookup,
  namedSwatchColours,
} from '../../src/managers/reports/reports-named-colours.js';

describe('reports-named-colours', () => {
  beforeEach(() => {
    setNamedColoursFromLookup([
      { key_idx: 1, value_txt: 'Blood Red', code: '#980002', value_int: 0 },
      { key_idx: 2, value_txt: 'Cobalt Blue', code: '#030aa7', value_int: 1 },
      { key_idx: 3, value_txt: 'Alice Blue', code: '#f0f8ff', value_int: 0 },
      { key_idx: 0, value_txt: 'Named Colours', code: '', value_int: 0 },
    ]);
  });

  it('parses lookup rows and skips the header', () => {
    const rows = parseNamedColourRows([
      { key_idx: 0, value_txt: 'Named Colours', code: '', value_int: 0 },
      { key_idx: 1, value_txt: 'Hunter Green', code: '#0b4008', value_int: 0 },
    ]);
    expect(rows).toEqual([
      { name: 'Hunter Green', hex: '#0b4008', swatch: false, key: 1 },
    ]);
  });

  it('resolves names to hex and passes hex through', () => {
    expect(resolveNamedColour('Blood Red')).toBe('#980002');
    expect(resolveNamedColour('blood red')).toBe('#980002');
    expect(resolveNamedColour('#Abc')).toBe('#aabbcc');
    expect(resolveNamedColour('nope')).toBe('');
  });

  it('filters by name substring', () => {
    const hits = filterNamedColours('blue');
    expect(hits.map((h) => h.name).sort()).toEqual(['Alice Blue', 'Cobalt Blue']);
  });

  it('lists swatch-flagged hexes', () => {
    expect(namedSwatchColours()).toEqual(['#030aa7']);
  });

  it('merges CSS over xkcd on duplicate names', () => {
    const merged = mergeNamedColourSources(
      [{ name: 'Green', hex: '#008000' }],
      [{ name: 'Green', hex: '#15b01a' }, { name: 'Blood Red', hex: '#980002' }],
    );
    expect(merged).toEqual([
      { name: 'Blood Red', hex: '#980002', source: 'xkcd' },
      { name: 'Green', hex: '#008000', source: 'css' },
    ]);
  });
});
