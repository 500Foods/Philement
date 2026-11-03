import { describe, expect, it, vi, beforeEach } from 'vitest';
import { LithiumProperties } from '../../src/properties/lithium-properties.js';
import { PROPS_PREFS_KEY, loadPanelPrefs, savePanelPrefs } from '../../src/managers/reports/reports-panel.js';

function mount(options = {}) {
  const element = document.createElement('aside');
  document.body.append(element);
  return new LithiumProperties({ element, outline: document.createElement('nav'), onEdit: vi.fn(), ...options });
}

const click = (props, action) => props.element.querySelector(`[data-action="${action}"]`).click();

describe('property panel layout prefs', () => {
  beforeEach(() => localStorage.clear());

  it('restores a saved state', () => {
    const props = mount({ state: {
      showSearch: true, showOutline: false, nameWidth: 90, outlineHeight: 140, closedSections: ['Fonts'],
    } });
    expect(props.getState()).toMatchObject({
      showSearch: true, showOutline: false, nameWidth: 90, outlineHeight: 140, closedSections: ['Fonts'],
    });
    expect(props.search.hidden).toBe(false);
    expect(props.outlineWrap.hidden).toBe(true);
    props.destroy();
  });

  it('defaults with no state: search off, outline on, nothing closed', () => {
    const props = mount();
    expect(props.getState()).toEqual({
      showSearch: false, showOutline: true, nameWidth: null, outlineHeight: null, closedSections: [],
      favourites: [], hidden: [], order: {}, showHidden: false, favouritesOnly: false, panelWidth: null,
    });
    props.destroy();
  });

  it('reports each toggle change, not the initial setup', () => {
    const onStateChange = vi.fn();
    const props = mount({ onStateChange });
    expect(onStateChange).not.toHaveBeenCalled();
    click(props, 'search');
    click(props, 'outline');
    expect(onStateChange.mock.calls.at(-1)[0]).toMatchObject({ showSearch: true, showOutline: false });
    props.destroy();
  });

  it('round-trips through localStorage with no profile service', () => {
    const state = { showSearch: true, showOutline: true, nameWidth: 110, outlineHeight: null, closedSections: ['Fonts', 'Border'] };
    savePanelPrefs(state, 'designer', null);
    expect(JSON.parse(localStorage.getItem(PROPS_PREFS_KEY))).toEqual(state);
    expect(loadPanelPrefs('designer', null)).toEqual(state);
  });

  it('bad stored JSON loads as empty prefs', () => {
    localStorage.setItem(PROPS_PREFS_KEY, '{nope');
    expect(loadPanelPrefs('designer', null)).toEqual({});
  });
});
