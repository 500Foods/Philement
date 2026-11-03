import { describe, it, expect } from 'vitest';
import { groupBySection, resolveLabel, resolveListCaptions } from '../../src/properties/sections.js';

const sections = [
  { id: 5, label: 'Position', sortSeq: 5 },
  { id: 1, label: 'Report', sortSeq: 1 },
  { id: 6, label: 'Fonts', sortSeq: 6 },
];

describe('groupBySection', () => {
  it('orders sections by sortSeq and rows by sortSeq within', () => {
    const rows = [
      { id: 1, label: 'Left', sortSeq: 201, collection: { Group: 'Position', section: 5 } },
      { id: 0, label: 'Top', sortSeq: 200, collection: { Group: 'Position', section: 5 } },
      { id: 18, label: 'Title', sortSeq: 0, collection: { Group: 'Report', section: 1 } },
    ];
    const g = groupBySection(rows, sections);
    expect(g.map((x) => x.section.id)).toEqual([1, 5]);
    expect(g[1].rows.map((r) => r.id)).toEqual([0, 1]);
  });
  it('falls back to Group only when section is missing', () => {
    const rows = [
      { id: 15, label: 'Font Family', collection: { Group: 'Fonts' } },
      { id: 2, label: 'Width', collection: { Group: 'Fonts', section: 5 } },
    ];
    const g = groupBySection(rows, sections);
    expect(g.find((x) => x.section.id === 6).rows.map((r) => r.id)).toEqual([15]);
    expect(g.find((x) => x.section.id === 5).rows.map((r) => r.id)).toEqual([2]);
  });
  it('keeps unknown rows in a trailing group', () => {
    const g = groupBySection([{ id: 9, label: 'X', collection: {} }], sections);
    expect(g).toEqual([{ section: null, rows: [{ id: 9, label: 'X', collection: {} }] }]);
  });
});

describe('resolveLabel', () => {
  const row = { label: 'Grid Color', collection: { Label: 'Color', lang: { 'fr-CA': 'Couleur CA', fr: { Label: 'Couleur' } } } };
  it('uses exact locale, then language, then plain', () => {
    expect(resolveLabel(row, ['fr-CA'])).toBe('Couleur CA');
    expect(resolveLabel(row, ['fr-FR'])).toBe('Couleur');
    expect(resolveLabel(row, ['de'])).toBe('Color');
    expect(resolveLabel({ label: 'Rotation' }, ['en-US'])).toBe('Rotation');
  });
  it('profile language wins over browser', () => {
    expect(resolveLabel(row, ['fr-FR', 'en-US'])).toBe('Couleur');
  });
});

describe('resolveListCaptions', () => {
  it('matches captions by position and keeps values otherwise', () => {
    const row = { collection: { List: ['Portrait', 'Landscape'], lang: { fr: { List: ['Portrait FR'] } } } };
    expect(resolveListCaptions(row, ['fr'])).toEqual(['Portrait FR', 'Landscape']);
    expect(resolveListCaptions(row, ['en'])).toEqual(['Portrait', 'Landscape']);
  });
});
