import { afterEach, describe, expect, it, vi } from 'vitest';

// happy-dom has no canvas, and Coloris grabs a 2D context when it loads.
vi.hoisted(() => {
  window.HTMLCanvasElement.prototype.getContext = () => ({ fillStyle: '', fillRect() {}, getImageData: () => ({ data: [255, 0, 0, 255] }) });
});

import {
  SWATCH_CAP, SWATCH_PATH, addCurrentSwatch, applyAndClosePicker, cancelAndClosePicker, clearNameMenu,
  hexColor, lastOpenedColour, mergeSwatches, namedColourMenuEntries, removeCurrentSwatch,
  setupColorPicker, shownSwatches, swatchList, useSwatchSettings, userSwatches,
} from '../../src/shared/coloris-picker.js';
import { setNamedColoursFromLookup } from '../../src/managers/reports/reports-named-colours.js';

function fakeSettings(initial = {}) {
  const data = { ...initial };
  return { data, get: (path, fallback) => (path in data ? data[path] : fallback), set: vi.fn((path, value) => { data[path] = value; }) };
}

function openOn(value) {
  const input = document.createElement('input');
  input.className = 'swatch-test-color';
  input.value = value;
  document.body.append(input);
  setupColorPicker('.swatch-test-color');
  input.click();
  return input;
}

const action = (name) => document.querySelector(`#clr-picker [data-color-action="${name}"]`);

afterEach(() => {
  useSwatchSettings(undefined);
  clearNameMenu();
  document.getElementById('lithium-clr-name-menu')?.remove();
  document.querySelectorAll('.swatch-test-color').forEach((node) => node.remove());
});

describe('coloris swatch lists', () => {
  it('normalises hex and drops junk and duplicates', () => {
    expect(hexColor('#ABC')).toBe('#aabbcc');
    expect(hexColor('#AABBCCDD')).toBe('#aabbcc');
    expect(hexColor('red')).toBe('');
    expect(swatchList('#fff #FFFFFF nope #000')).toEqual(['#ffffff', '#000000']);
    expect(swatchList(Array.from({ length: 30 }, (_, i) => `#0000${String(i).padStart(2, '0')}`))).toHaveLength(SWATCH_CAP);
  });

  it('theme colours come first, then user colours not already shown', () => {
    expect(mergeSwatches('#111 #222', ['#222', '#333'])).toEqual(['#111111', '#222222', '#333333']);
  });

  it('reads user swatches from the profile', () => {
    useSwatchSettings(fakeSettings({ [SWATCH_PATH]: ['#ff0000', 'bad'] }));
    expect(userSwatches()).toEqual(['#ff0000']);
    expect(shownSwatches()).toContain('#ff0000');
  });
});

describe('coloris swatch buttons', () => {
  it('+ adds the current colour to the profile, − removes it, ✕ sits in the row', () => {
    const service = fakeSettings();
    useSwatchSettings(service);
    openOn('#123456');
    expect(document.getElementById('clr-picker').classList.contains('clr-open')).toBe(true);
    document.getElementById('clr-color-value').value = '#123456';
    expect(action('apply')).not.toBeNull();
    expect(action('cancel')).not.toBeNull();
    expect(document.getElementById('clr-close')?.hidden).toBe(true);
    expect(addCurrentSwatch()).toBe(true);
    expect(service.data[SWATCH_PATH]).toEqual(['#123456']);
    expect(addCurrentSwatch()).toBe(false);
    expect(action('add').disabled).toBe(true);
    expect(action('remove').disabled).toBe(false);
    action('remove').click();
    expect(service.data[SWATCH_PATH]).toEqual([]);
    expect(removeCurrentSwatch()).toBe(false);
    expect(action('remove').disabled).toBe(true);
  });

  it('swatches added in one field show on every picker', () => {
    const service = fakeSettings({ [SWATCH_PATH]: ['#abcdef'] });
    useSwatchSettings(service);
    openOn('#000000');
    const shown = [...document.querySelectorAll('#clr-picker .clr-swatches button')].map((b) => hexColor(b.textContent));
    expect(shown).toContain('#abcdef');
  });
});


describe('coloris apply / cancel / name list', () => {
  it('✓ apply and ✕ cancel buttons sit in the action row', () => {
    openOn('#abcdef');
    expect(action('apply')).not.toBeNull();
    expect(action('cancel')).not.toBeNull();
    expect(document.getElementById('clr-close')?.hidden).toBe(true);
  });

  it('cancel reverts the linked field to the value at open', () => {
    const input = openOn('#112233');
    expect(lastOpenedColour()).toBe('#112233');
    document.getElementById('clr-color-value').value = '#445566';
    document.getElementById('clr-color-value').dispatchEvent(new globalThis.Event('change', { bubbles: true }));
    // Coloris pickColor on change updates the linked input.
    expect(input.value).toBe('#445566');
    cancelAndClosePicker();
    expect(input.value).toBe('#112233');
    expect(document.getElementById('clr-picker').classList.contains('clr-open')).toBe(false);
  });

  it('name menu lists none first then lookup hits', () => {
    setNamedColoursFromLookup([
      { key_idx: 1, value_txt: 'Blood Red', code: '#980002', value_int: 0 },
    ]);
    const all = namedColourMenuEntries('');
    expect(all[0]).toMatchObject({ name: 'none', hex: 'none' });
    expect(all.some((e) => e.name === 'Blood Red')).toBe(true);
  });

  it('Name format tab is injected into Coloris', () => {
    openOn('#000000');
    expect(document.getElementById('clr-f4')?.value).toBe('name');
    expect(document.querySelector('label[for="clr-f4"]')?.textContent).toBe('Name');
    // Old mid-picker name strip must stay gone.
    expect(document.querySelector('#clr-picker > .lithium-color-named')).toBeNull();
  });
});


describe('coloris none button and name-list close', () => {
  it('none button sets pending none without closing the picker', () => {
    openOn('#112233');
    const noneBtn = action('none');
    expect(noneBtn).not.toBeNull();
    expect(noneBtn.title).toBe('None');
    noneBtn.click();
    expect(document.getElementById('clr-color-value').value).toBe('none');
    expect(document.getElementById('clr-picker').classList.contains('clr-open')).toBe(true);
  });

  it('✓ commits pending none onto the linked field', () => {
    const input = openOn('#112233');
    action('none').click();
    expect(input.value).toBe('#112233'); // pending only in the picker
    applyAndClosePicker();
    expect(input.value).toBe('none');
    expect(document.getElementById('clr-picker').classList.contains('clr-open')).toBe(false);
  });

  it('stock Close button stays hidden / absent', () => {
    openOn('#000000');
    const close = document.getElementById('clr-close');
    expect(!close || close.hidden || close.style.display === 'none').toBe(true);
  });

  it('picking a named colour closes the suggestion list and does not reopen it', () => {
    setNamedColoursFromLookup([
      { key_idx: 1, value_txt: 'Black', code: '#000000', value_int: 0 },
      { key_idx: 2, value_txt: 'Blood Red', code: '#980002', value_int: 0 },
    ]);
    openOn('#abcdef');
    const nameRadio = document.getElementById('clr-f4');
    nameRadio.click();
    const valueInput = document.getElementById('clr-color-value');
    expect(valueInput.dataset.lithiumNameMode).toBe('1');
    valueInput.value = 'Bla';
    valueInput.dispatchEvent(new globalThis.Event('input', { bubbles: true }));
    const menu = document.getElementById('lithium-clr-name-menu');
    expect(menu).not.toBeNull();
    expect(menu.hidden).toBe(false);
    const black = [...menu.querySelectorAll('button')].find((b) => b.textContent.includes('Black'));
    expect(black).toBeTruthy();
    black.dispatchEvent(new globalThis.MouseEvent('mousedown', { bubbles: true, cancelable: true }));
    expect(document.getElementById('clr-color-value').value).toBe('#000000');
    expect(document.getElementById('lithium-clr-name-menu').hidden).toBe(true);
    // A trailing input from setPickerColour must not bring the list back.
    expect(document.getElementById('lithium-clr-name-menu').hidden).toBe(true);
  });

  it('name list closes when switching away from the Name tab', () => {
    setNamedColoursFromLookup([
      { key_idx: 1, value_txt: 'Black', code: '#000000', value_int: 0 },
    ]);
    openOn('#abcdef');
    document.getElementById('clr-f4').click();
    const valueInput = document.getElementById('clr-color-value');
    valueInput.value = '';
    valueInput.dispatchEvent(new globalThis.Event('input', { bubbles: true }));
    expect(document.getElementById('lithium-clr-name-menu').hidden).toBe(false);
    document.querySelector('#clr-format input[value="hex"]').click();
    expect(document.getElementById('lithium-clr-name-menu').hidden).toBe(true);
  });
});
