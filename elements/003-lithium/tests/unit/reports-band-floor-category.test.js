import { describe, expect, it } from 'vitest';
import {
  addItem,
  bandContentBottom,
  createBlank,
  moveItem,
  resizeBand,
  resizeItem,
} from '../../src/managers/reports/model/document.js';
import {
  CATEGORY_PROFILE_PATH,
  loadCategory,
  saveCategory,
} from '../../src/managers/reports/reports-fields.js';

const band = (doc, id) => doc.bands.find((entry) => entry.id === id);

function fakeSettings(initial = {}) {
  const store = { ...initial };
  return { store, get: (k, d) => (k in store ? store[k] : d), set: (k, v) => { store[k] = v; } };
}

describe('band resize floor', () => {
  it('an empty band clamps only to the minimum', () => {
    const doc = resizeBand(createBlank(), 'detail', '0.05 in');
    expect(band(doc, 'detail').height).toBe('0.15 in');
    expect(bandContentBottom(band(doc, 'detail'))).toBe(0);
  });

  it('a band cannot be shorter than its lowest item', () => {
    let doc = resizeBand(createBlank(), 'detail', '2 in');
    doc = addItem(doc, 'detail', { typeKey: 0 });
    const id = band(doc, 'detail').items[0].id;
    doc = moveItem(doc, 'detail', id, { left: '0 in', top: '0.5 in' });
    doc = resizeItem(doc, 'detail', id, { width: '1 in', height: '0.25 in' });
    expect(bandContentBottom(band(doc, 'detail'))).toBeCloseTo(0.75, 6);
    expect(band(resizeBand(doc, 'detail', '0.3 in'), 'detail').height).toBe('0.75 in');
    expect(band(resizeBand(doc, 'detail', '1 in'), 'detail').height).toBe('1 in');
  });
});

describe('element group preference', () => {
  it('defaults to static and ignores unknown values', () => {
    expect(loadCategory(null)).toBe('static');
    expect(loadCategory(fakeSettings({ [CATEGORY_PROFILE_PATH]: 'bogus' }))).toBe('static');
  });

  it('saves and restores a valid group', () => {
    const s = fakeSettings();
    saveCategory('field', s);
    expect(s.store[CATEGORY_PROFILE_PATH]).toBe('field');
    expect(loadCategory(s)).toBe('field');
    saveCategory('nope', s);
    expect(loadCategory(s)).toBe('field');
  });
});
