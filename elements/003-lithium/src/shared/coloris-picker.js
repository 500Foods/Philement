/**
 * Lithium's one Coloris picker, after Pavillion's ui/src/common/coloris.js.
 *
 * Every colour field uses this module, so the picker shows the same
 * retained swatches everywhere. Swatches are the theme's list
 * (`--lithium-picker-swatches`, hex colours separated by spaces), then the
 * colours the user added. Added colours live on the user profile
 * (`window.lithiumSettings`, path `colorPicker.swatches`). A row under the
 * swatches holds + / − / none / ✓ (apply) / ✕ (cancel = revert). Format tabs
 * include Name for the searchable named-colour list.
 *
 * Usage:
 *   import { setupColorPicker } from '../shared/coloris-picker.js';
 *   setupColorPicker('.my-color-input');
 */

import Coloris from '@melloware/coloris';
import '@melloware/coloris/dist/coloris.css';
import '../styles/vendor-coloris.css';
import { processIcons } from '../core/icons.js';
import { namedSwatchEntries, filterNamedColours } from '../managers/reports/reports-named-colours.js';

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

/**
 * Theme, then named (lookup swatch flag), then user. Two-arg form
 * mergeSwatches(theme, user) still works. Duplicates drop; cap SWATCH_CAP.
 * @param {string[]|string} theme
 * @param {string[]|string} namedOrUser
 * @param {string[]|string} [user]
 */
export function mergeSwatches(theme, namedOrUser, user) {
  const out = swatchList(theme);
  const named = user === undefined ? [] : namedOrUser;
  const owned = user === undefined ? namedOrUser : user;
  for (const hex of swatchList(named)) {
    if (out.length >= SWATCH_CAP) break;
    if (!out.includes(hex)) out.push(hex);
  }
  for (const hex of swatchList(owned)) {
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
  const named = namedSwatchEntries().map((entry) => entry.hex);
  return mergeSwatches(themeSwatches(), named, userSwatches());
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

/** Snapshot of the linked field when the picker opened (for cancel). */
let openedValue = '';
/** @type {HTMLInputElement|null} */
let linkedEl = null;

/** When > 0, name-menu paint ignores programmatic value updates (after a pick). */
let suppressNameMenuPaint = 0;

/** Bound Name-tab listeners so setNameMode can unbind cleanly. */
let nameModeState = null;

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

function colorValueInput() {
  return document.getElementById('clr-color-value');
}

/** Hide / clear the Name-tab suggestion list. */
export function clearNameMenu() {
  const menu = document.getElementById('lithium-clr-name-menu');
  if (menu) {
    menu.hidden = true;
    menu.textContent = '';
  }
}

function withSuppressedNameMenu(fn) {
  suppressNameMenuPaint += 1;
  try {
    fn();
  } finally {
    globalThis.queueMicrotask(() => {
      suppressNameMenuPaint = Math.max(0, suppressNameMenuPaint - 1);
    });
  }
}

function isPendingNone(valueInput) {
  if (!valueInput) return false;
  if (valueInput.dataset.lithiumNone === '1') return true;
  const text = String(valueInput.value ?? '').trim().toLowerCase();
  return text === 'none' || text === 'transparent';
}

/**
 * Apply the picker's current value onto the linked field and close.
 * "none" bypasses Coloris's parser (it only accepts colour strings).
 */
export function applyAndClosePicker() {
  clearNameMenu();
  const valueInput = colorValueInput();
  if (isPendingNone(valueInput)) {
    // Put a parseable value in the Coloris field so blur/change is safe,
    // commit "none" onto the linked input, then close (Enter).
    if (valueInput) {
      delete valueInput.dataset.lithiumNone;
      valueInput.value = hexColor(openedValue) || '#000000';
      if (document.activeElement === valueInput) valueInput.blur();
    }
    if (linkedEl) {
      linkedEl.value = 'none';
      linkedEl.dispatchEvent(new globalThis.Event('input', { bubbles: true }));
    }
    document.dispatchEvent(new globalThis.KeyboardEvent('keydown', { key: 'Enter', bubbles: true }));
    return;
  }
  if (valueInput) {
    valueInput.dispatchEvent(new globalThis.Event('change', { bubbles: true }));
  }
  document.dispatchEvent(new globalThis.KeyboardEvent('keydown', { key: 'Enter', bubbles: true }));
}

/** Revert to the value at open and close (Coloris Escape). */
export function cancelAndClosePicker() {
  clearNameMenu();
  const valueInput = colorValueInput();
  if (valueInput) delete valueInput.dataset.lithiumNone;
  document.dispatchEvent(new globalThis.KeyboardEvent('keydown', { key: 'Escape', bubbles: true }));
}

/** Close the open picker without reverting the linked field. */
export function closeOpenPicker() {
  clearNameMenu();
  if (typeof Coloris.close === 'function') Coloris.close();
}

/** Set the picker's active colour from a hex or "none" (pending; ✓ commits). */
export function setPickerColour(raw) {
  withSuppressedNameMenu(() => {
    const valueInput = colorValueInput();
    if (!valueInput) return;
    const text = String(raw ?? '').trim();
    const none = text === '' || text.toLowerCase() === 'none' || text.toLowerCase() === 'transparent';
    if (none) {
      // Coloris cannot parse "none" — show it as pending text only.
      valueInput.value = 'none';
      valueInput.dataset.lithiumNone = '1';
      clearNameMenu();
      paintActions();
      return;
    }
    const hex = hexColor(text);
    if (!hex) return;
    delete valueInput.dataset.lithiumNone;
    valueInput.value = hex;
    valueInput.dispatchEvent(new globalThis.Event('input', { bubbles: true }));
    valueInput.dispatchEvent(new globalThis.Event('change', { bubbles: true }));
    clearNameMenu();
    paintActions();
  });
}

/** Pending "none" in the picker value field (✓ commits, ✕ reverts). */
export function setPickerNone() {
  setPickerColour('none');
}

function hideStockClose(picker) {
  const stockClose = picker.querySelector('#clr-close');
  if (stockClose) {
    stockClose.hidden = true;
    stockClose.setAttribute('aria-hidden', 'true');
    stockClose.style.display = 'none';
  }
}

/* + / − / none / ✓ / ✕ under the swatches. none sets pending; ✓ applies; ✕ cancels. */
function ensureActions(picker) {
  let row = picker.querySelector('.lithium-color-actions');
  if (!row) {
    row = document.createElement('div');
    row.className = 'lithium-color-actions';
    const add = actionButton('fa-plus', 'Add this color', 'add');
    add.addEventListener('click', addCurrentSwatch);
    const remove = actionButton('fa-minus', 'Remove this color', 'remove');
    remove.addEventListener('click', removeCurrentSwatch);
    const none = actionButton('fa-ban', 'None', 'none');
    none.addEventListener('click', (event) => {
      event.preventDefault();
      event.stopPropagation();
      setPickerNone();
    });
    const apply = actionButton('fa-check', 'Apply and close', 'apply');
    apply.addEventListener('click', (event) => {
      event.preventDefault();
      event.stopPropagation();
      applyAndClosePicker();
    });
    const cancel = actionButton('fa-xmark', 'Cancel', 'cancel');
    cancel.addEventListener('click', (event) => {
      event.preventDefault();
      event.stopPropagation();
      cancelAndClosePicker();
    });
    row.append(add, remove, none, apply, cancel);
    picker.append(row);
  } else if (!row.querySelector('[data-color-action="none"]')) {
    const none = actionButton('fa-ban', 'None', 'none');
    none.addEventListener('click', (event) => {
      event.preventDefault();
      event.stopPropagation();
      setPickerNone();
    });
    const apply = row.querySelector('[data-color-action="apply"]');
    if (apply) row.insertBefore(none, apply);
    else row.append(none);
  }
  hideStockClose(picker);
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
  const add = row.querySelector('[data-color-action="add"]');
  const remove = row.querySelector('[data-color-action="remove"]');
  if (add) add.disabled = !hex || shown.includes(hex) || shown.length >= SWATCH_CAP;
  if (remove) remove.disabled = !hex || !owned.has(hex);
}

function titleNamedSwatches(picker) {
  const byHex = new Map(namedSwatchEntries().map((entry) => [entry.hex, entry.name]));
  for (const button of picker.querySelectorAll('.clr-swatches button')) {
    const hex = hexColor(button.textContent);
    const name = byHex.get(hex);
    if (name) button.title = name;
  }
}

/** Build one named-colour option button (swatch + label). */
export function namedColourOptionButton(entry, onPick) {
  const btn = document.createElement('button');
  btn.type = 'button';
  btn.className = 'lithium-color-named-option';
  btn.dataset.hex = entry.hex || 'none';
  const swatch = document.createElement('span');
  swatch.className = 'lithium-color-named-swatch';
  if (entry.hex && entry.hex !== 'none') {
    swatch.style.setProperty('--lithium-named-swatch', entry.hex);
  } else {
    swatch.classList.add('lithium-color-named-swatch-none');
  }
  const label = document.createElement('span');
  label.textContent = entry.name;
  btn.append(swatch, label);
  btn.addEventListener('mousedown', (event) => {
    event.preventDefault();
    onPick(entry);
  });
  return btn;
}

/**
 * Entries for the name list: "none" first, then filtered lookup rows.
 * @param {string} query
 * @param {number} [limit=40]
 */
export function namedColourMenuEntries(query, limit = 40) {
  const q = String(query ?? '').trim().toLowerCase();
  const none = { name: 'none', hex: 'none', swatch: false, key: -1 };
  const hits = filterNamedColours(query, limit);
  if (!q || 'none'.includes(q) || 'transparent'.includes(q)) {
    return [none, ...hits.filter((entry) => entry.name.toLowerCase() !== 'none')];
  }
  return hits;
}

function ensureNameFormat(picker) {
  const fieldset = picker.querySelector('#clr-format .clr-segmented');
  if (!fieldset || fieldset.querySelector('#clr-f4')) return fieldset;
  const radio = document.createElement('input');
  radio.id = 'clr-f4';
  radio.type = 'radio';
  radio.name = 'clr-format';
  radio.value = 'name';
  const label = document.createElement('label');
  label.htmlFor = 'clr-f4';
  label.textContent = 'Name';
  const span = fieldset.querySelector('span');
  if (span) fieldset.insertBefore(radio, span);
  else fieldset.append(radio);
  fieldset.insertBefore(label, span || null);
  return fieldset;
}

function placeNameMenu(anchor, menu) {
  const box = anchor.getBoundingClientRect();
  menu.style.position = 'fixed';
  menu.style.left = `${box.left}px`;
  menu.style.width = `${box.width}px`;
  menu.style.top = `${box.bottom + 2}px`;
  menu.style.zIndex = '10000';
}

function nameMenuElement() {
  let menu = document.getElementById('lithium-clr-name-menu');
  if (!menu) {
    menu = document.createElement('div');
    menu.id = 'lithium-clr-name-menu';
    menu.className = 'lithium-color-named-menu';
    menu.hidden = true;
    document.body.append(menu);
  }
  return menu;
}

function paintNameMenu(valueInput) {
  if (suppressNameMenuPaint) return;
  const menu = nameMenuElement();
  const hits = namedColourMenuEntries(valueInput.value, 40);
  menu.textContent = '';
  for (const entry of hits) {
    menu.append(namedColourOptionButton(entry, (picked) => {
      // Pick closes the list; setPickerColour suppresses reopen from its input event.
      clearNameMenu();
      setPickerColour(picked.hex === 'none' ? 'none' : picked.hex);
    }));
  }
  menu.hidden = hits.length === 0;
  if (!menu.hidden) placeNameMenu(valueInput, menu);
}

function unbindNameMode() {
  if (!nameModeState) return;
  nameModeState.input.removeEventListener('input', nameModeState.onInput);
  nameModeState.input.removeEventListener('focus', nameModeState.onFocus);
  nameModeState = null;
}

function setNameMode(picker, on) {
  picker.classList.toggle('lithium-color-format-name', on);
  const valueInput = colorValueInput();
  unbindNameMode();
  clearNameMenu();
  if (!valueInput) return;
  if (on) {
    valueInput.dataset.lithiumNameMode = '1';
    valueInput.placeholder = 'Colour name';
    const onInput = () => paintNameMenu(valueInput);
    const onFocus = () => paintNameMenu(valueInput);
    valueInput.addEventListener('input', onInput);
    valueInput.addEventListener('focus', onFocus);
    nameModeState = { input: valueInput, onInput, onFocus };
    paintNameMenu(valueInput);
  } else {
    delete valueInput.dataset.lithiumNameMode;
    valueInput.placeholder = '';
  }
}

function bindNameFormat(picker) {
  if (picker.dataset.lithiumNameBound) return;
  picker.dataset.lithiumNameBound = '1';
  ensureNameFormat(picker);
  const format = picker.querySelector('#clr-format');
  format?.addEventListener('click', (event) => {
    const input = event.target instanceof Element
      ? event.target.closest('input[name="clr-format"]')
      : null;
    if (!input) return;
    if (input.value === 'name') {
      // Stop Coloris treating "name" as a colour format string.
      event.stopPropagation();
      input.checked = true;
      setNameMode(picker, true);
      colorValueInput()?.focus();
    } else {
      setNameMode(picker, false);
    }
  }, true);
  document.addEventListener('mousedown', (event) => {
    const menu = document.getElementById('lithium-clr-name-menu');
    const valueInput = colorValueInput();
    if (!menu || menu.hidden) return;
    if (menu.contains(event.target) || event.target === valueInput) return;
    clearNameMenu();
  });
}

function rememberOpenValue(event) {
  if (!(event.target instanceof Element)) return;
  for (const selector of selectors) {
    if (event.target.matches(selector)) {
      openedValue = event.target.value ?? '';
      linkedEl = event.target;
      break;
    }
  }
}

function applySwatches() {
  if (!ready) return;
  Coloris({ swatches: shownSwatches(), closeButton: false });
  const picker = document.getElementById('clr-picker');
  if (picker) {
    // Remove the old mid-picker name field if a prior build left one.
    picker.querySelector('.lithium-color-named')?.remove();
    ensureActions(picker);
    ensureNameFormat(picker);
    bindNameFormat(picker);
    titleNamedSwatches(picker);
    if (!picker.classList.contains('lithium-color-format-name')) {
      setNameMode(picker, false);
    }
  }
  paintActions();
}

function bindExtras() {
  const picker = document.getElementById('clr-picker');
  if (!picker || extrasBound) return;
  extrasBound = true;
  picker.addEventListener('input', paintActions);
  document.addEventListener('coloris:pick', () => {
    clearNameMenu();
    paintActions();
  });
  document.addEventListener('click', rememberOpenValue, true);
  // Picker closed (Escape / Enter / outside) — drop any leftover name list.
  const mo = new MutationObserver(() => {
    if (!picker.classList.contains('clr-open')) clearNameMenu();
  });
  mo.observe(picker, { attributes: true, attributeFilter: ['class'] });
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
      closeButton: false,
      themeMode: 'dark',
      swatches: shownSwatches(),
    });
    document.addEventListener('click', (event) => {
      if (event.target instanceof Element && event.target.matches(selector)) applySwatches();
    }, true);
  }
  bindExtras();
  applySwatches();
}

/** Value captured when the picker last opened (tests / cancel). */
export function lastOpenedColour() {
  return openedValue;
}
