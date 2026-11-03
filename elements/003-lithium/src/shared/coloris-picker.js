/**
 * Lithium's one Coloris picker, after Pavillion's ui/src/common/coloris.js.
 *
 * Every colour field uses this module, so the picker shows the same
 * retained swatches everywhere. Swatches are the theme's list
 * (`--lithium-picker-swatches`, hex colours separated by spaces), then the
 * colours the user added. Added colours live on the user profile
 * (`window.lithiumSettings`, path `colorPicker.swatches`). A row under the
 * swatches holds + (add the current colour), − (remove it, only for a
 * colour the user added), and ✕ (Coloris's close button, moved there).
 *
 * Usage:
 *   import { setupColorPicker } from '../shared/coloris-picker.js';
 *   setupColorPicker('.my-color-input');
 */

import Coloris from '@melloware/coloris';
import '@melloware/coloris/dist/coloris.css';
import '../styles/vendor-coloris.css';
import { processIcons } from '../core/icons.js';

export const SWATCH_CAP = 24;
export const SWATCH_PATH = 'colorPicker.swatches';

let ready = false;
let extrasBound = false;
const selectors = new Set();
let settingsOverride;

function settings() {
  if (settingsOverride !== undefined) return settingsOverride;
  return typeof window !== 'undefined' ? window.lithiumSettings || null : null;
}

/** Tests can pass a fake settings service; `undefined` goes back to the window one. */
export function useSwatchSettings(service) {
  settingsOverride = service;
}

/** #rgb or #rrggbb (any case) as lowercase #rrggbb; '' when not a hex colour. */
export function hexColor(value) {
  const text = String(value ?? '').trim();
  const short = /^#([0-9a-f]{3})$/i.exec(text);
  if (short) {
    const [r, g, b] = short[1];
    return `#${r}${r}${g}${g}${b}${b}`.toLowerCase();
  }
  const long = /^#([0-9a-f]{6})([0-9a-f]{2})?$/i.exec(text);
  return long ? `#${long[1].toLowerCase()}` : '';
}

/** Hex colours from a space-separated string or an array. Duplicates drop; at most 24. */
export function swatchList(raw) {
  const parts = Array.isArray(raw) ? raw : String(raw || '').split(/\s+/);
  const out = [];
  for (const part of parts) {
    const hex = typeof part === 'string' ? hexColor(part) : '';
    if (!hex || out.includes(hex)) continue;
    out.push(hex);
    if (out.length >= SWATCH_CAP) break;
  }
  return out;
}

/** Theme colours first, then user colours not already shown, up to the cap. */
export function mergeSwatches(theme, user) {
  const out = swatchList(theme);
  for (const hex of swatchList(user)) {
    if (out.length >= SWATCH_CAP) break;
    if (!out.includes(hex)) out.push(hex);
  }
  return out;
}

export function userSwatches() {
  const service = settings();
  return swatchList(service ? service.get(SWATCH_PATH, []) : []);
}

function saveUserSwatches(list) {
  const service = settings();
  if (service) service.set(SWATCH_PATH, swatchList(list));
}

function themeSwatches() {
  if (typeof document === 'undefined' || !document.documentElement) return [];
  return swatchList(getComputedStyle(document.documentElement).getPropertyValue('--lithium-picker-swatches'));
}

export function shownSwatches() {
  return mergeSwatches(themeSwatches(), userSwatches());
}

function currentHex() {
  return hexColor(document.getElementById('clr-color-value')?.value);
}

/** Add the picker's current colour to the user's swatches. Returns true when it was added. */
export function addCurrentSwatch() {
  const hex = currentHex();
  const shown = shownSwatches();
  if (!hex || shown.includes(hex) || shown.length >= SWATCH_CAP) return false;
  saveUserSwatches([...userSwatches(), hex]);
  applySwatches();
  return true;
}

/** Remove the picker's current colour, if the user added it. Returns true when removed. */
export function removeCurrentSwatch() {
  const hex = currentHex();
  const list = userSwatches();
  if (!hex || !list.includes(hex)) return false;
  saveUserSwatches(list.filter((item) => item !== hex));
  applySwatches();
  return true;
}

function actionButton(icon, label, action) {
  const button = document.createElement('button');
  button.type = 'button';
  button.className = 'lithium-color-action';
  button.dataset.colorAction = action;
  button.setAttribute('aria-label', label);
  button.title = label;
  button.innerHTML = `<fa ${icon}></fa>`;
  return button;
}

/* Add, Remove, and Close share one row under the swatches. */
function ensureActions(picker) {
  let row = picker.querySelector('.lithium-color-actions');
  if (!row) {
    row = document.createElement('div');
    row.className = 'lithium-color-actions';
    const add = actionButton('fa-plus', 'Add this color', 'add');
    add.addEventListener('click', addCurrentSwatch);
    const remove = actionButton('fa-minus', 'Remove this color', 'remove');
    remove.addEventListener('click', removeCurrentSwatch);
    row.append(add, remove);
    picker.append(row);
  }
  const close = picker.querySelector('#clr-close');
  if (close && close.parentElement !== row) {
    close.classList.add('lithium-color-action');
    close.dataset.colorAction = 'close';
    close.innerHTML = '<fa fa-xmark></fa>';
    close.setAttribute('aria-label', 'Close');
    close.title = 'Close';
    row.append(close);
  }
  processIcons(row);
  return row;
}

/** Enable + and − for the current colour and mark user swatches. */
export function paintActions() {
  const picker = document.getElementById('clr-picker');
  if (!picker) return;
  const row = ensureActions(picker);
  const shown = shownSwatches();
  const owned = new Set(userSwatches());
  const hex = currentHex();
  for (const button of picker.querySelectorAll('.clr-swatches button')) {
    if (owned.has(hexColor(button.textContent))) button.dataset.userSwatch = '1';
    else delete button.dataset.userSwatch;
  }
  row.querySelector('[data-color-action="add"]').disabled = !hex || shown.includes(hex) || shown.length >= SWATCH_CAP;
  row.querySelector('[data-color-action="remove"]').disabled = !hex || !owned.has(hex);
}

function applySwatches() {
  if (!ready) return;
  Coloris({ swatches: shownSwatches() });
  paintActions();
}

function bindExtras() {
  const picker = document.getElementById('clr-picker');
  if (!picker || extrasBound) return;
  extrasBound = true;
  picker.addEventListener('input', paintActions);
  document.addEventListener('coloris:pick', paintActions);
}

/**
 * Bind the shared picker to inputs matching `selector`. Safe to call once
 * per field type; each call refreshes the swatches from the profile.
 * @param {string} selector
 */
export function setupColorPicker(selector) {
  if (!ready) {
    Coloris.init();
    ready = true;
  }
  if (!selectors.has(selector)) {
    selectors.add(selector);
    Coloris({
      el: selector,
      alpha: false,
      format: 'hex',
      formatToggle: true,
      closeButton: true,
      closeLabel: 'Close',
      themeMode: 'dark',
      swatches: shownSwatches(),
    });
    // Coloris re-reads swatches from the profile each time a field opens.
    document.addEventListener('click', (event) => {
      if (event.target instanceof Element && event.target.matches(selector)) applySwatches();
    }, true);
  }
  bindExtras();
  applySwatches();
}
