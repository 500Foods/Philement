/* globals MouseEvent, KeyboardEvent */
import { describe, expect, it, afterEach } from 'vitest';
import {
  CUSTOM_DROPDOWN_CYCLE_MAX,
  customDropdownEditor,
  fontStyleCss,
  isBooleanPairChoices,
  isCycleableChoices,
  listChoices,
} from '../../src/properties/lithium-properties.js';

function fakeCell(value) {
  return { getValue: () => value };
}

function labels(n) {
  return Array.from({ length: n }, (_, i) => ({ value: `v${i}`, label: `L${i}` }));
}

afterEach(() => {
  document.body.querySelectorAll('.lithium-properties-combo-menu').forEach((node) => node.remove());
});

describe('customDropdownEditor', () => {
  it('listChoices normalizes plain and object entries', () => {
    expect(listChoices(['A', { value: 'b', label: 'B' }])).toEqual([
      { value: 'A', label: 'A' },
      { value: 'b', label: 'B' },
    ]);
  });

  it('fontStyleCss maps Bold + Italic independently of family', () => {
    expect(fontStyleCss('Bold + Italic')).toEqual({ fontWeight: '700', fontStyle: 'italic' });
    expect(fontStyleCss('Normal')).toEqual({ fontWeight: '400', fontStyle: 'normal' });
  });

  it('isCycleableChoices is 2…CUSTOM_DROPDOWN_CYCLE_MAX inclusive', () => {
    expect(CUSTOM_DROPDOWN_CYCLE_MAX).toBe(7);
    expect(isCycleableChoices([])).toBe(false);
    expect(isCycleableChoices([{ value: 'a', label: 'A' }])).toBe(false);
    expect(isCycleableChoices(labels(2))).toBe(true);
    expect(isCycleableChoices(labels(7))).toBe(true);
    expect(isCycleableChoices(labels(8))).toBe(false);
    expect(isBooleanPairChoices(labels(2))).toBe(true);
    expect(isBooleanPairChoices(labels(3))).toBe(false);
  });

  it('pick-only mounts a body menu and styles each option', () => {
    const choices = [
      { value: 'Vanadium Sans', label: 'Vanadium Sans' },
      { value: 'Helvetica', label: 'Helvetica' },
    ];
    let committed;
    const wrap = customDropdownEditor(
      fakeCell('Helvetica'),
      (fn) => fn(),
      (value) => { committed = value; },
      () => {},
      choices,
      {
        editable: false,
        styleOption: (choice, el) => { el.style.fontFamily = `"${choice.label}", sans-serif`; },
      },
    );
    expect(wrap.querySelector('input').readOnly).toBe(true);
    wrap.querySelector('.lithium-properties-combo-toggle')
      .dispatchEvent(new MouseEvent('mousedown', { bubbles: true, cancelable: true }));
    const menu = document.body.querySelector('.lithium-properties-combo-menu');
    expect(menu).toBeTruthy();
    expect(menu.hidden).toBe(false);
    const options = [...menu.querySelectorAll('.lithium-properties-combo-option')];
    expect(options).toHaveLength(2);
    expect(options[0].style.fontFamily).toContain('Vanadium Sans');
    options[0].dispatchEvent(new MouseEvent('mousedown', { bubbles: true, cancelable: true }));
    expect(committed).toBe('Vanadium Sans');
    expect(document.body.querySelector('.lithium-properties-combo-menu')).toBeNull();
  });

  it('double-click toggles a 2-option list (boolean case)', () => {
    let committed;
    const wrap = customDropdownEditor(
      fakeCell('True'),
      (fn) => fn(),
      (value) => { committed = value; },
      () => {},
      [{ value: 'True', label: 'Yes' }, { value: 'False', label: 'No' }],
      { editable: false },
    );
    wrap.dispatchEvent(new MouseEvent('dblclick', { bubbles: true, cancelable: true }));
    expect(committed).toBe('False');
    expect(document.body.querySelector('.lithium-properties-combo-menu')).toBeNull();
  });

  it('double-click cycles a 3-option list and wraps last to first', () => {
    const choices = labels(3);
    let value = 'v1';
    const cell = { getValue: () => value };
    const open = () => customDropdownEditor(
      cell, (fn) => fn(), (next) => { value = next; }, () => {}, choices, { editable: false },
    );
    open().dispatchEvent(new MouseEvent('dblclick', { bubbles: true, cancelable: true }));
    expect(value).toBe('v2');
    open().dispatchEvent(new MouseEvent('dblclick', { bubbles: true, cancelable: true }));
    expect(value).toBe('v0');
  });

  it('double-click cycles all 7 options then wraps', () => {
    const choices = labels(7);
    let value = 'v0';
    const cell = { getValue: () => value };
    for (let i = 1; i < 7; i += 1) {
      customDropdownEditor(
        cell, (fn) => fn(), (next) => { value = next; }, () => {}, choices, { editable: false },
      ).dispatchEvent(new MouseEvent('dblclick', { bubbles: true, cancelable: true }));
      expect(value).toBe(`v${i}`);
    }
    customDropdownEditor(
      cell, (fn) => fn(), (next) => { value = next; }, () => {}, choices, { editable: false },
    ).dispatchEvent(new MouseEvent('dblclick', { bubbles: true, cancelable: true }));
    expect(value).toBe('v0');
  });

  it('double-click does not cycle lists longer than CUSTOM_DROPDOWN_CYCLE_MAX', () => {
    let committed = null;
    const wrap = customDropdownEditor(
      fakeCell('v0'),
      (fn) => fn(),
      (value) => { committed = value; },
      () => {},
      labels(8),
      { editable: false },
    );
    wrap.dispatchEvent(new MouseEvent('dblclick', { bubbles: true, cancelable: true }));
    expect(committed).toBeNull();
  });

  it('double-click does not cycle an editable list', () => {
    let committed = null;
    const wrap = customDropdownEditor(
      fakeCell('8'),
      (fn) => fn(),
      (value) => { committed = value; },
      () => {},
      labels(3),
      { editable: true },
    );
    wrap.dispatchEvent(new MouseEvent('dblclick', { bubbles: true, cancelable: true }));
    expect(committed).toBeNull();
  });

  it('double-click with unknown current value picks the first option', () => {
    let committed;
    const wrap = customDropdownEditor(
      fakeCell('not-in-list'),
      (fn) => fn(),
      (value) => { committed = value; },
      () => {},
      labels(4),
      { editable: false },
    );
    wrap.dispatchEvent(new MouseEvent('dblclick', { bubbles: true, cancelable: true }));
    expect(committed).toBe('v0');
  });

  it('Boolean Yes/No commits True/False values', () => {
    let committed;
    const wrap = customDropdownEditor(
      { getValue: () => 'False' },
      (fn) => fn(),
      (value) => { committed = value; },
      () => {},
      [{ value: 'True', label: 'Yes' }, { value: 'False', label: 'No' }],
      { editable: false },
    );
    expect(wrap.querySelector('input').value).toBe('No');
    wrap.querySelector('.lithium-properties-combo-toggle')
      .dispatchEvent(new MouseEvent('mousedown', { bubbles: true, cancelable: true }));
    const yes = [...document.body.querySelectorAll('.lithium-properties-combo-option')]
      .find((btn) => btn.textContent === 'Yes');
    yes.dispatchEvent(new MouseEvent('mousedown', { bubbles: true, cancelable: true }));
    expect(committed).toBe('True');
  });

  it('editable ArrowDown opens the menu; Enter picks the highlight', () => {
    const choices = [
      { value: '8', label: '8' },
      { value: '12', label: '12' },
    ];
    let committed;
    const wrap = customDropdownEditor(
      fakeCell('10 pt'),
      (fn) => fn(),
      (value) => { committed = value; },
      () => {},
      choices,
      {
        editable: true,
        styleOption: (choice, el) => { el.style.fontSize = `${choice.value}pt`; },
      },
    );
    const input = wrap.querySelector('input');
    input.dispatchEvent(new KeyboardEvent('keydown', { key: 'ArrowDown', bubbles: true, cancelable: true }));
    const menu = document.body.querySelector('.lithium-properties-combo-menu');
    expect(menu).toBeTruthy();
    expect(menu.querySelector('.lithium-properties-combo-option-active')?.dataset.value).toBe('8');
    input.dispatchEvent(new KeyboardEvent('keydown', { key: 'ArrowDown', bubbles: true, cancelable: true }));
    expect(menu.querySelector('.lithium-properties-combo-option-active')?.dataset.value).toBe('12');
    input.dispatchEvent(new KeyboardEvent('keydown', { key: 'Enter', bubbles: true, cancelable: true }));
    expect(committed).toBe('12');
  });
});
