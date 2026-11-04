import { describe, expect, it, vi } from 'vitest';

// happy-dom has no canvas, and Coloris grabs a 2D context when it loads.
vi.hoisted(() => {
  window.HTMLCanvasElement.prototype.getContext = () => ({ fillStyle: '', fillRect() {}, getImageData: () => ({ data: [255, 0, 0, 255] }) });
});

import { LithiumProperties, choiceLabel, groupHeaderHtml, valueWidth } from '../../src/properties/lithium-properties.js';

function mount(options = {}) {
  const element = document.createElement('aside');
  const outline = document.createElement('nav');
  outline.id = 'outline';
  outline.textContent = 'Report';
  document.body.append(element);
  const onStub = vi.fn();
  const props = new LithiumProperties({ element, outline, onEdit: vi.fn(), onStub, ...options });
  return { element, outline, props, onStub };
}

const click = (props, action) => props.element.querySelector(`[data-action="${action}"]`).click();

describe('LithiumProperties chrome', () => {
  it('frames the grid with a top and bottom bar', () => {
    const { element, props } = mount();
    expect(element.querySelector('.lithium-properties-bar-top')).not.toBeNull();
    expect(element.querySelector('.lithium-properties-bar-bottom')).not.toBeNull();
    const body = element.querySelector('.lithium-properties-body');
    expect([...body.children].map((node) => node.className)).toEqual([
      'lithium-properties-search-wrap', 'lithium-properties-outline', 'lithium-properties-outline-sizer', 'lithium-properties-grid',
    ]);
    props.destroy();
  });

  it('search starts off, outline on; toggles flip them', () => {
    const { element, outline, props } = mount();
    expect(props.search.hidden).toBe(true);
    expect(outline.parentElement.hidden).toBe(false);
    click(props, 'search');
    expect(props.search.hidden).toBe(false);
    expect(element.querySelector('[data-action="search"]').getAttribute('aria-pressed')).toBe('true');
    expect(element.querySelector('[data-action="search"]').classList.contains('lithium-nav-btn-active')).toBe(true);
    expect(element.querySelector('[data-action="outline"]').classList.contains('lithium-nav-btn-active')).toBe(true);
    click(props, 'outline');
    expect(element.querySelector('[data-action="outline"]').classList.contains('lithium-nav-btn-active')).toBe(false);
    expect(outline.parentElement.hidden).toBe(true);
    expect(props.outlineSizer.hidden).toBe(true);
    props.destroy();
  });

  it('shows a list label in the closed cell and keeps the stored code', () => {
    expect(choiceLabel('roundrect', [{ value: 'roundrect', label: 'Rounded Rectangle' }])).toBe('Rounded Rectangle');
    expect(choiceLabel('solid', ['solid', 'dashed'])).toBeNull();
    const { props } = mount();
    const wrap = props._format({
      getRow: () => ({
        getData: () => ({
          editor: 'List',
          value: 'roundrect',
          list: [{ value: 'roundrect', label: 'Rounded Rectangle' }],
        }),
      }),
    });
    expect(wrap.textContent).toBe('Rounded Rectangle');
    props.destroy();
  });

  it('centers the stack icons between the side controls', () => {
    const { element, props, onStub } = mount();
    const bar = element.querySelector('.lithium-properties-bar-top');
    expect([...bar.children].map((node) => node.className)).toEqual([
      'lithium-properties-bar-side',
      'lithium-properties-order',
      'lithium-properties-bar-side lithium-properties-bar-side-end',
    ]);
    const actions = (selector) => [...bar.querySelector(selector).children].map((node) => node.dataset.action);
    expect(actions('.lithium-properties-bar-side')).toEqual(['search', 'outline']);
    expect(actions('.lithium-properties-order')).toEqual(['backward', 'back', 'forward', 'front']);
    expect(actions('.lithium-properties-bar-side-end')).toEqual(['expand', 'collapse']);
    const icon = (id) => [...props.buttons[id].querySelector('i, fa').classList];
    expect(icon('backward')).toContain('fa-send-backward');
    expect(icon('back')).toEqual(expect.arrayContaining(['fa-send-back']));
    expect(icon('back')).not.toContain('fa-send-backward');
    expect(icon('forward')).toContain('fa-bring-forward');
    expect(icon('front')).toEqual(expect.arrayContaining(['fa-bring-front']));
    expect(icon('front')).not.toContain('fa-bring-forward');
    for (const id of ['backward', 'back', 'forward', 'front']) {
      expect(props.buttons[id].disabled).toBe(true);
      expect(props.buttons[id].classList.contains('lithium-properties-btn-tight')).toBe(true);
    }
    props.setOrderEnabled({ backward: true, back: true, forward: false, front: true });
    expect(props.buttons.backward.disabled).toBe(false);
    expect(props.buttons.back.disabled).toBe(false);
    expect(props.buttons.forward.disabled).toBe(true);
    expect(props.buttons.front.disabled).toBe(false);
    const onOrder = vi.fn();
    props.onOrder = onOrder;
    click(props, 'backward');
    expect(onOrder).toHaveBeenCalledWith('backward');
    expect(onStub).not.toHaveBeenCalled();
    props.destroy();
  });

  it('edits a multiline note in a textarea and keeps Enter as a newline', () => {
    const { props } = mount();
    let saved;
    const area = props._editor(
      { getRow: () => ({ getData: () => ({ multiline: true, editor: 'Text' }) }), getValue: () => 'hi' },
      (fn) => fn(),
      (value) => { saved = value; },
      () => {},
    );
    expect(area.tagName).toBe('TEXTAREA');
    expect(Number(area.rows)).toBe(1);
    const host = document.createElement('div');
    host.append(area);
    const bubbled = [];
    host.addEventListener('keydown', (event) => bubbled.push(event.key));
    area.value = 'line one';
    const enter = new globalThis.KeyboardEvent('keydown', { key: 'Enter', bubbles: true, cancelable: true });
    area.dispatchEvent(enter);
    expect(enter.defaultPrevented).toBe(false);
    expect(saved).toBeUndefined();
    area.dispatchEvent(new globalThis.KeyboardEvent('keydown', { key: 'ArrowUp', bubbles: true }));
    area.dispatchEvent(new globalThis.KeyboardEvent('keydown', { key: 'ArrowDown', bubbles: true }));
    expect(bubbled).toEqual([]);
    host.remove();
    area.value = 'a note';
    area.dispatchEvent(new globalThis.Event('blur'));
    expect(saved).toBe('a note');
    props.destroy();
  });

  it('favourites and hidden are toggles, not stubs', () => {
    const { props, onStub } = mount();
    click(props, 'favourites');
    click(props, 'hidden');
    expect(onStub).not.toHaveBeenCalled();
    expect(props.buttons.favourites.getAttribute('aria-pressed')).toBe('true');
    expect(props.buttons.hidden.getAttribute('aria-pressed')).toBe('true');
    props.destroy();
  });

  it('section header has a caret and a count', () => {
    const html = groupHeaderHtml('Fonts', '', 3);
    expect(html.indexOf('lithium-properties-caret')).toBeLessThan(html.indexOf('Fonts'));
    expect(html).toContain('lithium-properties-count">3<');
  });

  it('expand/collapse all reaches every group, looking each up fresh', () => {
    const { props } = mount();
    const state = { '01 A': true, '02 B': true, '03 C': true };
    // Each toggle replaces the group objects, as Tabulator does.
    let generation = 0;
    const groups = () => {
      const mine = generation;
      return Object.keys(state).map((key) => ({
        getKey: () => key,
        isVisible: () => state[key],
        show: () => { if (mine === generation) { state[key] = true; generation += 1; } },
        hide: () => { if (mine === generation) { state[key] = false; generation += 1; } },
      }));
    };
    props._built = true;
    props.table = { getGroups: groups, destroy() {} };
    props.setAllGroups(false);
    expect(Object.values(state)).toEqual([false, false, false]);
    expect([...props._closedGroups].sort()).toEqual(['A', 'B', 'C']);
    props.setAllGroups(true);
    expect(Object.values(state)).toEqual([true, true, true]);
    props.destroy();
  });

  it('search has a manual clear X that empties the field', () => {
    const { element, props } = mount();
    click(props, 'search');
    const clear = element.querySelector('.lithium-properties-search-clear');
    expect(clear.classList.contains('lithium-header-filter-clear')).toBe(true);
    props.search.value = 'font';
    props.search.dispatchEvent(new globalThis.Event('input'));
    expect(clear.style.display).toBe('flex');
    clear.click();
    expect(props.search.value).toBe('');
    expect(clear.style.display).toBe('none');
    props.destroy();
  });

  it('refits after a column resize unless a cell is being edited', () => {
    const { props } = mount();
    const redraw = vi.fn();
    props._built = true;
    props.table = { redraw, destroy() {} };
    props._refit();
    expect(redraw).toHaveBeenCalledWith(true);
    const editing = document.createElement('div');
    editing.className = 'tabulator-editing';
    props.gridHost.append(editing);
    props._refit();
    expect(redraw).toHaveBeenCalledTimes(1);
    props.destroy();
  });

  it('value width is the inner width minus the name width, floored at the minimum', () => {
    expect(valueWidth(400, 120)).toBe(280);
    expect(valueWidth(400.6, 120)).toBe(280);
    expect(valueWidth(150, 120)).toBe(60);
  });

  it('fits the value column to the holder width', () => {
    const { props } = mount();
    const holder = document.createElement('div');
    holder.className = 'tabulator-tableholder';
    Object.defineProperty(holder, 'clientWidth', { value: 500 });
    props.gridHost.append(holder);
    const setWidth = vi.fn();
    props._built = true;
    props.table = {
      getColumns: () => [{ getWidth: () => 150 }, { getWidth: () => 200, setWidth }],
      redraw() {}, destroy() {},
    };
    props._fitValue();
    expect(setWidth).toHaveBeenCalledWith(350);
    props.destroy();
  });

  it('a dragged name width survives the refit; value takes the rest', () => {
    const { props } = mount();
    const holder = document.createElement('div');
    holder.className = 'tabulator-tableholder';
    Object.defineProperty(holder, 'clientWidth', { value: 400 });
    props.gridHost.append(holder);
    const widths = { label: 90, value: 200 };
    const column = (field) => ({
      getField: () => field,
      getWidth: () => widths[field],
      setWidth: (width) => { widths[field] = width; },
    });
    const handlers = {};
    props._built = true;
    props.table = {
      getColumns: () => [column('label'), column('value')],
      // A forced redraw in fitData resets columns to definition widths.
      redraw: () => { widths.label = 120; widths.value = 160; },
      getRows: () => [],
      on: (name, fn) => { handlers[name] = fn; },
      destroy() {},
    };
    props._nameWidth = 90;
    props._refit();
    expect(widths).toEqual({ label: 90, value: 310 });
    props.destroy();
  });

  it('Color editor opens Coloris on render and saves on close', () => {
    const { props } = mount();
    const cell = { getValue: () => '#ff0000', getRow: () => ({ getData: () => ({ editor: 'Color' }) }) };
    let rendered;
    const success = vi.fn();
    const input = props._editor(cell, (fn) => { rendered = fn; }, success, vi.fn());
    document.body.append(input);
    rendered();
    expect(input.className).toBe('lithium-properties-color');
    expect(document.getElementById('clr-picker').classList.contains('clr-open')).toBe(true);
    input.value = '#00ff00';
    input.dispatchEvent(new window.Event('close'));
    input.dispatchEvent(new window.Event('change'));
    expect(success).toHaveBeenCalledTimes(1);
    expect(success).toHaveBeenCalledWith('#00ff00');
    input.remove();
    props.destroy();
  });
});
