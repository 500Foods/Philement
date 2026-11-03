import { afterEach, describe, expect, it, vi } from 'vitest';

// happy-dom has no canvas, and Coloris grabs a 2D context when it loads.
vi.hoisted(() => {
  window.HTMLCanvasElement.prototype.getContext = () => ({ fillStyle: '', fillRect() {}, getImageData: () => ({ data: [255, 0, 0, 255] }) });
});

import {
  SWATCH_CAP, SWATCH_PATH, addCurrentSwatch, hexColor, mergeSwatches, removeCurrentSwatch,
  setupColorPicker, shownSwatches, swatchList, useSwatchSettings, userSwatches,
} from '../../src/shared/coloris-picker.js';

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
    expect(action('close')).not.toBeNull();
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
