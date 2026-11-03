import { describe, expect, it, vi, beforeEach } from 'vitest';
import {
  LithiumProperties, applyOrder, dateValue, normalizePrefs, sectionOrder, tableData,
} from '../../src/properties/lithium-properties.js';
import {
  PROPS_PREFS_KEY, PROPS_WIDTH_KEY, loadPanelPrefs, propsProfilePath, savePanelPrefs,
} from '../../src/managers/reports/reports-panel.js';
import { KNOWN_EDITORS } from '../../src/managers/reports/report-properties.js';

const sections = [{ id: 1, label: 'Object', sortSeq: 1 }, { id: 2, label: 'Fonts', sortSeq: 2 }];
const rows = [
  { id: 'a', label: 'A', section: 1 },
  { id: 'b', label: 'B', section: 1 },
  { id: 'c', label: 'C', section: 1 },
  { id: 'f', label: 'F', section: 2 },
];

function fakeSettings(seed = {}) {
  const data = { ...seed };
  return { data, get: (path, fallback) => (path in data ? data[path] : fallback), set: vi.fn((path, value) => { data[path] = value; }) };
}

describe('preference shaping', () => {
  it('favourites stay in their own section; no extra section', () => {
    const out = tableData(rows, sections, { favourites: ['f'] });
    expect(out.map((row) => row.id)).toEqual(['a', 'b', 'c', 'f']);
    expect([...new Set(out.map((row) => row.sectionName))]).toEqual(['00 Object', '01 Fonts']);
    expect(out.find((row) => row.id === 'f')).toMatchObject({ sectionCaption: 'Fonts', isFavourite: true, isHidden: false });
  });

  it('hidden rows drop out; Hidden on includes them in the regular list', () => {
    expect(tableData(rows, sections, { hidden: ['b'] }).map((row) => row.id)).toEqual(['a', 'c', 'f']);
    const shown = tableData(rows, sections, { hidden: ['b'], showHidden: true });
    expect(shown.map((row) => row.id)).toEqual(['a', 'b', 'c', 'f']);
    expect(shown.find((row) => row.id === 'b').isHidden).toBe(true);
    expect(shown.find((row) => row.id === 'a').isHidden).toBe(false);
  });

  it('the Favourite filter keeps only favourites, in their sections', () => {
    const out = tableData(rows, sections, { favourites: ['c', 'f'], hidden: ['f'], favouritesOnly: true });
    expect(out.map((row) => [row.id, row.sectionCaption])).toEqual([['c', 'Object']]);
  });

  it('a saved order sorts a section; unlisted rows follow in lookup order', () => {
    const out = tableData(rows, sections, { order: { Object: ['c', 'a'] } });
    expect(out.map((row) => row.id)).toEqual(['c', 'a', 'b', 'f']);
    expect(applyOrder(rows, undefined)).toBe(rows);
  });

  it('sectionOrder reads one section in display order', () => {
    expect(sectionOrder([{ id: 'b', sectionCaption: 'Object' }, { id: 'f', sectionCaption: 'Fonts' }, { id: 'a', sectionCaption: 'Object' }], 'Object'))
      .toEqual(['b', 'a']);
  });

  it('normalizePrefs keeps string ids without repeats', () => {
    expect(normalizePrefs({ favourites: [1, '1', 2], hidden: 'x', order: { Object: [3, 3], Empty: [] } }))
      .toEqual({ favourites: ['1', '2'], hidden: [], order: { Object: ['3'] } });
  });

  it('Date is a known editor and values trim to the date', () => {
    expect(KNOWN_EDITORS.has('Date')).toBe(true);
    expect(dateValue('2026-11-03T10:20:30')).toBe('2026-11-03');
    expect(dateValue(null)).toBe('');
  });
});

describe('panel preference toggles', () => {
  const mount = (options = {}) => {
    const element = document.createElement('aside');
    document.body.append(element);
    return new LithiumProperties({ element, onEdit: vi.fn(), ...options });
  };

  it('restores favourites, hidden, order and the bar toggles from state', () => {
    const props = mount({ state: { favourites: ['a'], hidden: ['b'], order: { Object: ['c'] }, showHidden: true, panelWidth: 300 } });
    expect(props.getState()).toMatchObject({ favourites: ['a'], hidden: ['b'], order: { Object: ['c'] }, showHidden: true, panelWidth: 300 });
    expect(props.buttons.hidden.getAttribute('aria-pressed')).toBe('true');
    expect(props.buttons.favourites.getAttribute('aria-pressed')).toBe('false');
    props.destroy();
  });

  it('favourite, hide, the bar buttons and the width report a state change', () => {
    const onStateChange = vi.fn();
    const onStub = vi.fn();
    const props = mount({ onStateChange, onStub });
    props.toggleFavourite('a');
    props.toggleHidden('b');
    props.element.querySelector('[data-action="favourites"]').click();
    props.element.querySelector('[data-action="hidden"]').click();
    props.setPanelWidth(321.4);
    expect(onStub).not.toHaveBeenCalled();
    expect(onStateChange.mock.calls.at(-1)[0]).toMatchObject({
      favourites: ['a'], hidden: ['b'], favouritesOnly: true, showHidden: true, panelWidth: 321,
    });
    props.toggleFavourite('a');
    expect(props.getState().favourites).toEqual([]);
    props.destroy();
  });

  it('the row menu has checkable Favourite and Hidden items', () => {
    const props = mount({ state: { favourites: ['a'] } });
    const row = { getData: () => ({ id: 'a', sectionCaption: 'Object' }) };
    const items = props._rowMenu(row);
    expect(items.map((item) => item.label.replace(/<[^>]+>/g, '').slice(1).trim())).toEqual(['Favourite', 'Hidden']);
    expect(items[0].label).toContain('aria-checked="true"');
    expect(items[1].label).toContain('aria-checked="false"');
    items[1].action();
    expect(props.getState().hidden).toEqual(['a']);
    props.destroy();
  });

  it('hovering a filter button marks the panel; Hidden on keeps its marks', () => {
    const props = mount();
    props.buttons.favourites.dispatchEvent(new window.Event('pointerenter'));
    expect(props.element.classList.contains('lithium-properties-show-favourite')).toBe(true);
    props.buttons.favourites.dispatchEvent(new window.Event('pointerleave'));
    expect(props.element.classList.contains('lithium-properties-show-favourite')).toBe(false);
    props.buttons.hidden.click();
    expect(props.element.classList.contains('lithium-properties-hidden-on')).toBe(true);
    props.destroy();
  });
});

describe('profile storage by panel id', () => {
  beforeEach(() => localStorage.clear());

  it('saves and loads at 24.props.<panel id>', () => {
    const settings = fakeSettings();
    savePanelPrefs({ favourites: ['x'] }, 'designer', settings);
    expect(settings.set).toHaveBeenCalledWith('24.props.designer', { favourites: ['x'] });
    expect(loadPanelPrefs('designer', settings)).toEqual({ favourites: ['x'] });
    expect(loadPanelPrefs('other', settings)).toEqual({});
    expect(propsProfilePath('other')).toBe('24.props.other');
  });

  it('copies the old localStorage prefs and width onto the profile once', () => {
    localStorage.setItem(PROPS_PREFS_KEY, JSON.stringify({ showSearch: true }));
    localStorage.setItem(PROPS_WIDTH_KEY, '333');
    const settings = fakeSettings();
    expect(loadPanelPrefs('designer', settings)).toEqual({ showSearch: true, panelWidth: 333 });
    expect(settings.data['24.props.designer']).toEqual({ showSearch: true, panelWidth: 333 });
    expect(localStorage.getItem(PROPS_PREFS_KEY)).toBeNull();
    expect(localStorage.getItem(PROPS_WIDTH_KEY)).toBeNull();
  });

  it('the profile copy wins over a leftover legacy blob', () => {
    localStorage.setItem(PROPS_PREFS_KEY, JSON.stringify({ showSearch: true }));
    const settings = fakeSettings({ '24.props.designer': { showSearch: false } });
    expect(loadPanelPrefs('designer', settings)).toEqual({ showSearch: false });
  });
});
