/**
 * LithiumProperties: a two column Tabulator property grid.
 * Rows are grouped by section (see sections.js). Editors:
 * Text, Number (text with unit), List, Boolean (Yes/No select),
 * PageSize, List, Boolean, FontFamily / FontStyle / FontSize via
 * customDropdownEditor (never native <select> in the PP). Pick-only
 * lists of 2…CUSTOM_DROPDOWN_CYCLE_MAX options double-click cycle
 * without opening the menu. Color (Coloris). Read-only
 * rows do not edit. Omitted values show the default in italics with
 * the row's note as a tooltip. No header; the name column resizes
 * from its cell edge.
 *
 * Chrome (one framed object): a top bar with toggles for Search and
 * the Outline plus Expand all / Collapse all; the search box, then
 * the caller's outline (vertically resizable, scrolls), then the
 * grid; a bottom bar with Favourites and Hidden filters and Previous /
 * Next object on the right. Right-click a row to check Favourite or
 * Hidden. Hovering a filter button shows its icon right-justified in the
 * name cell of the rows it applies to; no other styling marks them.
 * Drag a name to reorder rows inside a section.
 * Favourites, hidden ids, row order, open sections, and sizes come in
 * through `state` and go out through `onStateChange` (see getState). Section headers carry a
 * caret; the whole header toggles. Tab and Shift+Tab in a value editor
 * walk the editable values up and down, skipping read-only rows.
 */

import { TabulatorFull as Tabulator } from 'tabulator-tables';
import { FlatpickrPicker } from '../shared/flatpickr-picker.js';
import { hexColor, namedColourOptionButton, namedColourMenuEntries, setupColorPicker, closeOpenPicker } from '../shared/coloris-picker.js';
import { resolveNamedColour } from '../managers/reports/reports-named-colours.js';
import { NONE_COLOR, normalizeColorValue } from '../managers/reports/reports-colours.js';
import { groupBySection } from './sections.js';
import { processIcons } from '../core/icons.js';

function setupColoris() {
  setupColorPicker('.lithium-properties-color');
}

/**
 * Search match on label, section caption, or stable id.
 * @param {object} data
 * @param {string} term
 */
export function matchesSearch(data, term) {
  const needle = String(term || '').trim().toLowerCase();
  if (!needle) return true;
  return [data.label, data.sectionCaption, data.id].some((value) => String(value ?? '').toLowerCase().includes(needle));
}

/** Section header html: icon (from 075 `icon`), caption, count. */
export const VALUE_MIN_WIDTH = 60;

/**
 * Value column width: what is left of the inner width after the name.
 * @param {number} inner Table holder clientWidth (borders and scrollbar excluded)
 * @param {number} label Name column width
 * @returns {number}
 */
/**
 * Label for a list value. Object entries show `label`. A plain entry
 * shows the stored value.
 * @param {unknown} value
 * @param {unknown} list
 * @returns {string|null}
 */
export function choiceLabel(value, list) {
  if (!Array.isArray(list)) return null;
  const hit = list.find((entry) => entry && typeof entry === 'object' && String(entry.value) === String(value));
  return hit && hit.label != null ? String(hit.label) : null;
}

/** Normalize list entries to { value, label }. */
export function listChoices(list) {
  return (list || []).map((entry) => (
    entry && typeof entry === 'object'
      ? { value: entry.value, label: entry.label == null ? String(entry.value) : String(entry.label) }
      : { value: entry, label: entry }
  ));
}

/** CSS for a Font Style list caption (independent of Family/Size). */
export function fontStyleCss(style) {
  const text = String(style || '');
  const bold = /bold/i.test(text);
  const italic = /italic/i.test(text);
  return {
    fontWeight: bold ? '700' : '400',
    fontStyle: italic ? 'italic' : 'normal',
  };
}

/** Pick-only lists with this many options or fewer cycle on double-click. */
export const CUSTOM_DROPDOWN_CYCLE_MAX = 7;

/**
 * True when a pick-only list should cycle on double-click (2…CYCLE_MAX).
 * Booleans are the two-option case. Editable lists never cycle.
 */
export function isCycleableChoices(choices) {
  if (!Array.isArray(choices)) return false;
  const n = choices.length;
  return n >= 2 && n <= CUSTOM_DROPDOWN_CYCLE_MAX;
}

/** @deprecated Prefer isCycleableChoices; kept as the 2-option boolean case. */
export function isBooleanPairChoices(choices) {
  return Array.isArray(choices) && choices.length === 2 && isCycleableChoices(choices);
}

const STAY_KEYS = new Set([
  'ArrowUp', 'ArrowDown', 'ArrowLeft', 'ArrowRight',
  'Home', 'End', 'PageUp', 'PageDown',
]);

/**
 * Custom Properties Panel dropdown (never native `<select>`).
 * Menu mounts on document.body (fixed) so Tabulator overflow cannot
 * clip it. `editable: true` allows free entry (Font Size); false is
 * pick-only (Font Family / Font Style). `styleOption` paints each
 * option independently. Keyboard: arrows move highlight, Enter picks,
 * Esc closes (then cancels if already closed), Home/End stay in the
 * field when editable (Phase 72). Pick-only lists with 2…
 * CUSTOM_DROPDOWN_CYCLE_MAX options cycle on double-click (wrap;
 * unknown current → first); longer or editable lists do not.
 *
 * @param {object} cell Tabulator cell
 * @param {Function} onRendered
 * @param {Function} success
 * @param {Function} cancel
 * @param {{ value: unknown, label: string }[]} choices
 * @param {{ editable?: boolean, inputMode?: string, title?: string,
 *   ariaLabel?: string, className?: string,
 *   styleOption?: (choice: { value: unknown, label: string }, el: HTMLElement) => void,
 *   commitValue?: (raw: string) => unknown }} [opts]
 */
export function customDropdownEditor(cell, onRendered, success, cancel, choices, opts = {}) {
  const editable = opts.editable !== false;
  const canCycle = !editable && isCycleableChoices(choices);
  const wrap = document.createElement('div');
  wrap.className = 'lithium-properties-combo';
  const input = document.createElement('input');
  input.type = 'text';
  if (opts.inputMode) input.inputMode = opts.inputMode;
  input.className = `lithium-properties-combo-input${opts.className ? ` ${opts.className}` : ''}`;
  if (opts.title) input.title = opts.title;
  input.readOnly = !editable;
  {
    const raw = cell.getValue() ?? '';
    const match = choices.find((choice) => String(choice.value) === String(raw));
    input.value = !editable && match ? match.label : (raw ?? '');
  }
  if (!editable) input.classList.add('lithium-properties-combo-pick');
  const toggle = document.createElement('button');
  toggle.type = 'button';
  toggle.className = 'lithium-properties-combo-toggle';
  toggle.setAttribute('aria-label', opts.ariaLabel || 'Open list');
  toggle.setAttribute('aria-expanded', 'false');
  toggle.tabIndex = -1;
  toggle.textContent = '▾';
  const menu = document.createElement('div');
  menu.className = 'lithium-properties-combo-menu';
  menu.hidden = true;
  menu.setAttribute('role', 'listbox');
  const buttons = [];
  let highlight = -1;
  for (const choice of choices) {
    const btn = document.createElement('button');
    btn.type = 'button';
    btn.className = 'lithium-properties-combo-option';
    btn.setAttribute('role', 'option');
    btn.dataset.value = String(choice.value);
    btn.textContent = choice.label;
    if (typeof opts.styleOption === 'function') opts.styleOption(choice, btn);
    btn.addEventListener('mousedown', (event) => {
      event.preventDefault();
      pick(choice);
    });
    menu.append(btn);
    buttons.push(btn);
  }
  wrap.append(input, toggle);

  let done = false;
  let openTimer = null;
  const currentIndex = () => {
    const raw = String(input.value ?? '');
    const byValue = buttons.findIndex((btn) => btn.dataset.value === raw);
    if (byValue >= 0) return byValue;
    return buttons.findIndex((btn) => btn.textContent === raw);
  };
  const setHighlight = (index) => {
    if (!buttons.length) { highlight = -1; return; }
    highlight = ((index % buttons.length) + buttons.length) % buttons.length;
    buttons.forEach((btn, i) => {
      btn.classList.toggle('lithium-properties-combo-option-active', i === highlight);
      if (i === highlight) btn.setAttribute('aria-selected', 'true');
      else btn.removeAttribute('aria-selected');
    });
    buttons[highlight].scrollIntoView({ block: 'nearest' });
  };
  const placeMenu = () => {
    const rect = wrap.getBoundingClientRect();
    menu.style.position = 'fixed';
    menu.style.left = `${rect.left}px`;
    menu.style.top = `${rect.bottom}px`;
    menu.style.width = `${Math.max(rect.width, 4)}px`;
    menu.style.right = 'auto';
  };
  const closeMenu = () => {
    menu.hidden = true;
    toggle.setAttribute('aria-expanded', 'false');
    if (menu.parentNode) menu.remove();
  };
  const openMenu = () => {
    placeMenu();
    if (!menu.parentNode) document.body.append(menu);
    menu.hidden = false;
    toggle.setAttribute('aria-expanded', 'true');
    const at = currentIndex();
    setHighlight(at >= 0 ? at : 0);
  };
  const finish = (ok, value) => {
    if (done) return;
    done = true;
    closeMenu();
    if (ok) {
      const raw = value === undefined ? input.value : value;
      success(typeof opts.commitValue === 'function' ? opts.commitValue(raw) : raw);
    } else cancel();
  };
  const pick = (choice) => {
    input.value = String(choice.value);
    finish(true, String(choice.value));
  };

  toggle.addEventListener('mousedown', (event) => {
    event.preventDefault();
    event.stopPropagation();
    if (openTimer) { clearTimeout(openTimer); openTimer = null; }
    if (menu.hidden) openMenu();
    else closeMenu();
    input.focus();
  });
  if (!editable) {
    input.addEventListener('mousedown', (event) => {
      event.preventDefault();
      if (canCycle) {
        // Delay open so a double-click can cycle without flashing the menu.
        if (openTimer) clearTimeout(openTimer);
        openTimer = setTimeout(() => {
          openTimer = null;
          if (done) return;
          if (menu.hidden) openMenu();
          else closeMenu();
        }, 280);
      } else if (menu.hidden) openMenu();
      else closeMenu();
      input.focus();
    });
  }
  if (canCycle) {
    wrap.addEventListener('dblclick', (event) => {
      event.preventDefault();
      event.stopPropagation();
      if (openTimer) { clearTimeout(openTimer); openTimer = null; }
      closeMenu();
      const raw = String(cell.getValue() ?? '');
      let idx = choices.findIndex((choice) => String(choice.value) === raw);
      if (idx < 0) idx = choices.findIndex((choice) => String(choice.label) === raw);
      // Unknown current → first; else advance with wrap.
      const next = idx < 0 ? 0 : (idx + 1) % choices.length;
      pick(choices[next]);
    });
  }
  input.addEventListener('blur', () => {
    setTimeout(() => {
      if (done) return;
      if (!menu.hidden && (menu.contains(document.activeElement) || document.activeElement === input)) {
        return;
      }
      if (editable) finish(true);
      else finish(true, String(cell.getValue() ?? ''));
    }, 0);
  });
  input.addEventListener('keydown', (event) => {
    if (event.key === 'ArrowDown') {
      event.preventDefault();
      event.stopPropagation();
      if (menu.hidden) openMenu();
      else setHighlight(highlight + 1);
      return;
    }
    if (event.key === 'ArrowUp') {
      event.preventDefault();
      event.stopPropagation();
      if (menu.hidden) openMenu();
      else setHighlight(highlight - 1);
      return;
    }
    if (event.key === 'Enter') {
      event.stopPropagation();
      if (!menu.hidden && highlight >= 0 && buttons[highlight]) {
        event.preventDefault();
        pick({ value: buttons[highlight].dataset.value, label: buttons[highlight].textContent });
        return;
      }
      if (editable) finish(true);
      else if (!menu.hidden) closeMenu();
      else finish(true, String(cell.getValue() ?? ''));
      return;
    }
    if (event.key === 'Escape') {
      event.stopPropagation();
      if (!menu.hidden) { closeMenu(); return; }
      finish(false);
      return;
    }
    if (STAY_KEYS.has(event.key)) {
      event.stopPropagation();
    }
  });
  onRendered(() => {
    input.focus();
    if (editable) input.select();
  });
  return wrap;
}

/** Font Size: editable custom dropdown; each option at that size. */
function fontSizeEditor(cell, onRendered, success, cancel, choices) {
  return customDropdownEditor(cell, onRendered, success, cancel, choices, {
    editable: true,
    inputMode: 'decimal',
    className: 'lithium-properties-number',
    title: 'A size number (bare uses Font Units) or a size with a unit.',
    ariaLabel: 'Font size list',
    styleOption: (choice, el) => {
      const pt = Number(choice.value);
      if (Number.isFinite(pt) && pt > 0) el.style.fontSize = `${pt}pt`;
    },
  });
}

/** Font Family: pick-only; each option in its own face. */
function fontFamilyEditor(cell, onRendered, success, cancel, choices) {
  return customDropdownEditor(cell, onRendered, success, cancel, choices, {
    editable: false,
    ariaLabel: 'Font family list',
    styleOption: (choice, el) => {
      el.style.fontFamily = `"${choice.label}", sans-serif`;
    },
  });
}

/** Font Style: pick-only; each option in its own style. */
function fontStyleEditor(cell, onRendered, success, cancel, choices) {
  return customDropdownEditor(cell, onRendered, success, cancel, choices, {
    editable: false,
    ariaLabel: 'Font style list',
    styleOption: (choice, el) => {
      const css = fontStyleCss(choice.label);
      el.style.fontWeight = css.fontWeight;
      el.style.fontStyle = css.fontStyle;
    },
  });
}

export function valueWidth(inner, label, min = VALUE_MIN_WIDTH) {
  return Math.max(min, Math.floor(inner - label));
}

export function groupHeaderHtml(caption, icon, count) {
  const safe = String(caption).replace(/[&<>"]/g, (ch) => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;' }[ch]));
  const iconHtml = /^<fa [a-z0-9 -]+><\/fa>$/.test(icon || '') ? `<span class="lithium-properties-icon">${icon}</span>` : '';
  const caret = '<span class="lithium-properties-caret" aria-hidden="true"><fa fa-caret-right></fa></span>';
  return `${caret}${iconHtml}<span class="lithium-properties-caption">${safe}</span><span class="lithium-properties-count">${count}</span>`;
}

const BAR_BUTTONS = {
  top: [
    { id: 'search', icon: 'fa-magnifying-glass', title: 'Search properties', toggle: true, slot: 'start' },
    { id: 'outline', icon: 'fa-sitemap', title: 'Show the outline', toggle: true, slot: 'start' },
    { id: 'backward', icon: 'fa-send-backward', title: 'Move backward', tight: true, slot: 'center' },
    { id: 'back', icon: 'fa-send-back', title: 'Move to back', tight: true, slot: 'center' },
    { id: 'forward', icon: 'fa-bring-forward', title: 'Move forward', tight: true, slot: 'center' },
    { id: 'front', icon: 'fa-bring-front', title: 'Move to front', tight: true, slot: 'center' },
    { id: 'expand', icon: 'fa-angles-down', title: 'Expand all sections', tight: true, slot: 'end' },
    { id: 'collapse', icon: 'fa-angles-up', title: 'Collapse all sections', tight: true, slot: 'end' },
  ],
  bottom: [
    { id: 'favourites', icon: 'fa-star', title: 'Show only favourites', toggle: true },
    { id: 'hidden', icon: 'fa-eye-slash', title: 'Include hidden properties', toggle: true },
    { gap: true },
    { id: 'prev', icon: 'fa-angle-up', title: 'Previous object', tight: true },
    { id: 'next', icon: 'fa-angle-down', title: 'Next object', tight: true },
  ],
};

function barButton(spec, onClick, buttons) {
  const button = document.createElement('button');
  button.type = 'button';
  button.className = `lithium-toolbar-btn lithium-properties-btn${spec.tight ? ' lithium-properties-btn-tight' : ''}`;
  button.dataset.action = spec.id;
  button.title = spec.title;
  button.setAttribute('aria-label', spec.title);
  if (spec.toggle) button.setAttribute('aria-pressed', 'false');
  button.innerHTML = `<fa ${spec.icon}></fa>`;
  button.addEventListener('click', () => onClick(spec.id, button));
  buttons[spec.id] = button;
  return button;
}

function buildBar(kind, onClick) {
  const bar = document.createElement('div');
  bar.className = `lithium-toolbar lithium-properties-bar lithium-properties-bar-${kind}`;
  const buttons = {};
  if (kind === 'top') {
    const start = document.createElement('div');
    start.className = 'lithium-properties-bar-side';
    const order = document.createElement('div');
    order.className = 'lithium-properties-order';
    const end = document.createElement('div');
    end.className = 'lithium-properties-bar-side lithium-properties-bar-side-end';
    const host = { start, center: order, end };
    for (const spec of BAR_BUTTONS.top) host[spec.slot].append(barButton(spec, onClick, buttons));
    bar.append(start, order, end);
    return { bar, buttons };
  }
  for (const spec of BAR_BUTTONS[kind]) {
    if (spec.gap) {
      const gap = document.createElement('div');
      gap.className = 'lithium-toolbar-placeholder';
      bar.append(gap);
      continue;
    }
    bar.append(barButton(spec, onClick, buttons));
  }
  return { bar, buttons };
}

/**
 * Next editable row id from `ids` (display order) after `current`,
 * stepping `delta` (+1 Tab, -1 Shift+Tab). Read-only rows are skipped.
 * @param {{ id: string, readOnly?: boolean }[]} rows
 * @param {string} current
 * @param {number} delta
 * @returns {string|null}
 */
export function nextEditableId(rows, current, delta) {
  const index = rows.findIndex((row) => String(row.id) === String(current));
  if (index < 0) return null;
  for (let at = index + delta; at >= 0 && at < rows.length; at += delta) {
    if (!rows[at].readOnly) return String(rows[at].id);
  }
  return null;
}

/**
 * Clean preference lists out of a saved state: string ids, no repeats.
 * @param {object} state
 * @returns {{ favourites: string[], hidden: string[], order: Object<string, string[]> }}
 */
export function normalizePrefs(state = {}) {
  const ids = (list) => (Array.isArray(list) ? [...new Set(list.map(String))] : []);
  const order = {};
  if (state.order && typeof state.order === 'object') {
    for (const [caption, list] of Object.entries(state.order)) {
      const clean = ids(list);
      if (clean.length) order[caption] = clean;
    }
  }
  return { favourites: ids(state.favourites), hidden: ids(state.hidden), order };
}

/**
 * Sort rows by a saved id order. Listed ids first in that order, the
 * rest after in their incoming (lookup sort_seq) order.
 * @param {object[]} rows
 * @param {string[]|undefined} order
 */
export function applyOrder(rows, order) {
  if (!order || !order.length) return rows;
  const at = new Map(order.map((id, index) => [id, index]));
  return rows
    .map((row, index) => ({ row, index }))
    .sort((a, b) => (at.get(String(a.row.id)) ?? order.length + a.index) - (at.get(String(b.row.id)) ?? order.length + b.index))
    .map((entry) => entry.row);
}

/**
 * Flatten panel rows into ordered table data with a section caption.
 * Rows stay in their own section. Filters: hidden rows drop out unless
 * `prefs.showHidden`, which includes them in the regular list; `prefs.favouritesOnly`
 * keeps only favourites. Rows carry `isFavourite` / `isHidden` for the
 * hover icons.
 * @param {object[]} rows Panel rows `{ id, label, section, group, ... }`.
 * @param {{ id: number, label: string, sortSeq?: number }[]} sections
 * @param {{ favourites?: string[], hidden?: string[], order?: object,
 *   showHidden?: boolean, favouritesOnly?: boolean }} [prefs]
 * @returns {object[]}
 */
export function tableData(rows, sections, prefs = {}) {
  const favourites = new Set((prefs.favourites || []).map(String));
  const hidden = new Set((prefs.hidden || []).map(String));
  const order = prefs.order || {};
  const shaped = rows.map((row, index) => ({
    ...row,
    sortSeq: index,
    collection: { section: row.section, Group: row.group },
  }));
  const keep = (row) => {
    const id = String(row.id);
    if (hidden.has(id) && !prefs.showHidden) return false;
    return !prefs.favouritesOnly || favourites.has(id);
  };
  const out = [];
  groupBySection(shaped, sections).forEach((group, index) => {
    const caption = group.section ? group.section.label : 'Other';
    const icon = group.section && group.section.icon ? group.section.icon : '';
    for (const row of applyOrder(group.rows, order[caption])) {
      if (!keep(row)) continue;
      const rest = { ...row };
      delete rest.collection;
      delete rest.sortSeq;
      out.push({
        ...rest,
        isFavourite: favourites.has(String(row.id)),
        isHidden: hidden.has(String(row.id)),
        sectionName: `${String(index).padStart(2, '0')} ${caption}`,
        sectionCaption: caption,
        sectionIcon: icon,
      });
    }
  });
  return out;
}

/**
 * The saved order for one section after a drag: ids of that section's
 * rows in their new display order.
 * @param {{ id: string, sectionCaption: string }[]} displayed Rows in display order
 * @param {string} caption
 * @returns {string[]}
 */
export function sectionOrder(displayed, caption) {
  return displayed.filter((row) => row.sectionCaption === caption).map((row) => String(row.id));
}

function colorEditor(cell, onRendered, success, cancel) {
  setupColoris();
  const wrap = document.createElement('div');
  wrap.className = 'lithium-properties-color-wrap';
  const input = document.createElement('input');
  input.type = 'text';
  input.className = 'lithium-properties-color';
  input.value = cell.getValue() ?? '';
  const toggle = document.createElement('button');
  toggle.type = 'button';
  toggle.className = 'lithium-properties-color-toggle';
  toggle.title = 'Named colours';
  toggle.setAttribute('aria-label', 'Named colours');
  toggle.innerHTML = '<fa fa-caret-down></fa>';
  const menu = document.createElement('div');
  menu.className = 'lithium-properties-color-named';
  menu.hidden = true;
  wrap.append(input, toggle, menu);
  let done = false;
  const finish = (raw = input.value) => {
    if (done) return;
    done = true;
    menu.hidden = true;
    let resolved;
    try {
      resolved = normalizeColorValue(raw);
    } catch {
      resolved = resolveNamedColour(raw) || hexColor(raw) || NONE_COLOR;
    }
    success(resolved);
  };
  const paintMenu = () => {
    const hits = namedColourMenuEntries(input.value, 30);
    menu.textContent = '';
    for (const entry of hits) {
      menu.append(namedColourOptionButton(entry, (picked) => {
        input.value = picked.hex === 'none' ? NONE_COLOR : picked.hex;
        menu.hidden = true;
        finish(input.value);
      }));
    }
    menu.hidden = hits.length === 0;
  };
  const openNames = (event) => {
    event.preventDefault();
    event.stopPropagation();
    // Caret opens the name list; do not leave Coloris covering it.
    closeOpenPicker();
    paintMenu();
    menu.hidden = false;
    input.focus();
  };
  toggle.addEventListener('mousedown', openNames);
  toggle.addEventListener('click', openNames);
  // Typing in the field: type-ahead names without opening Coloris until click.
  input.addEventListener('input', () => {
    if (!menu.hidden || document.activeElement === input) paintMenu();
  });
  input.addEventListener('change', () => finish());
  input.addEventListener('close', () => finish());
  input.addEventListener('keydown', (event) => {
    if (event.key === 'Enter') finish();
    if (event.key === 'Escape') { done = true; menu.hidden = true; cancel(); }
  });
  input.addEventListener('blur', () => setTimeout(() => {
    if (menu.contains(document.activeElement) || toggle.contains(document.activeElement)) return;
    if (!document.getElementById('clr-picker')?.classList.contains('clr-open')) finish();
  }, 0));
  // Click the field (not the arrow) opens Coloris.
  input.addEventListener('click', (event) => {
    if (event.target === toggle) return;
    menu.hidden = true;
  });
  onRendered(() => {
    processIcons(wrap);
    input.focus();
    // Opening the editor on the value field starts Coloris; the caret
    // closes it and shows named colours instead (see openNames).
    input.click();
  });
  return wrap;
}

/** A Date value as Flatpickr's date only `YYYY-MM-DD`, or '' when unset. */
export function dateValue(value) {
  const text = String(value ?? '').trim();
  return /^\d{4}-\d{2}-\d{2}/.test(text) ? text.slice(0, 10) : text;
}

function dateEditor(cell, onRendered, success, cancel) {
  const wrap = document.createElement('span');
  wrap.className = 'lithium-properties-date';
  const input = document.createElement('input');
  input.type = 'text';
  input.placeholder = 'YYYY-MM-DD';
  input.value = dateValue(cell.getValue());
  const button = document.createElement('button');
  button.type = 'button';
  button.className = 'lithium-properties-date-btn';
  button.title = 'Pick a date';
  button.setAttribute('aria-label', 'Pick a date');
  button.innerHTML = '<fa fa-calendar-days></fa>';
  wrap.append(input, button);
  let done = false;
  const picker = new FlatpickrPicker({
    enableTime: false,
    enableSeconds: false,
    dateFormat: 'Y-m-d',
    onChange: (_date, formatted) => {
      input.value = dateValue(formatted);
      finish();
    },
  });
  picker.init(button, input);
  function finish() {
    if (done) return;
    done = true;
    const value = dateValue(input.value);
    setTimeout(() => picker.destroy(), 0);
    success(value);
  }
  input.addEventListener('keydown', (event) => {
    if (event.key === 'Enter') finish();
    if (event.key === 'Escape') { done = true; picker.destroy(); cancel(); }
  });
  input.addEventListener('blur', () => setTimeout(() => {
    if (!picker.getIsOpen() && !wrap.contains(document.activeElement)) finish();
  }, 150));
  onRendered(() => { processIcons(wrap); input.focus(); });
  return wrap;
}

export class LithiumProperties {
  /**
   * @param {{ element: HTMLElement, onEdit: (id: string, value: *) => void,
   *   pageSizes?: () => { value: number, label: string }[],
   *   outline?: HTMLElement, onStub?: (action: string) => void,
   *   showSearch?: boolean, showOutline?: boolean,
   *   state?: object, onStateChange?: (state: object) => void,
   *   onNavigate?: (delta: number) => void,
   *   onFile?: (row: object) => void,
   *   onOrder?: (action: string) => void }} options
   *   state: a saved getState() result; onStateChange gets each change.
   *   onNavigate: Previous (-1) / Next (+1) object on the bottom bar.
   *   onOrder: backward, back, forward, or front for the selected item.
   */
  constructor(options) {
    this.element = options.element;
    this.onEdit = options.onEdit;
    this.pageSizes = options.pageSizes || (() => []);
    this.sections = [];
    this.onStub = options.onStub || (() => {});
    this.onNavigate = options.onNavigate || null;
    this.onFile = options.onFile || null;
    this.onOrder = options.onOrder || null;
    const state = options.state || {};
    this.onStateChange = options.onStateChange || null;
    this._closedGroups = new Set(Array.isArray(state.closedSections) ? state.closedSections.map(String) : []);
    this._nameWidth = Number(state.nameWidth) > 0 ? Number(state.nameWidth) : 0;
    const prefs = normalizePrefs(state);
    this._favourites = new Set(prefs.favourites);
    this._hidden = new Set(prefs.hidden);
    this._order = prefs.order;
    this._showHidden = state.showHidden === true;
    this._favouritesOnly = state.favouritesOnly === true;
    this._panelWidth = Number(state.panelWidth) > 0 ? Number(state.panelWidth) : null;
    this._rows = [];
    this.element.classList.add('lithium-properties');
    const top = buildBar('top', (action, button) => this._onBar(action, button));
    const bottom = buildBar('bottom', (action, button) => this._onBar(action, button));
    this.buttons = { ...top.buttons, ...bottom.buttons };
    for (const id of ['backward', 'back', 'forward', 'front']) this.buttons[id].disabled = true;
    for (const [id, cls] of [['favourites', 'lithium-properties-show-favourite'], ['hidden', 'lithium-properties-show-hidden']]) {
      this.buttons[id].addEventListener('pointerenter', () => this.element.classList.add(cls));
      this.buttons[id].addEventListener('pointerleave', () => this.element.classList.remove(cls));
    }
    this.body = document.createElement('div');
    this.body.className = 'lithium-properties-body';
    this.outlineWrap = document.createElement('div');
    this.outlineWrap.className = 'lithium-properties-outline';
    if (options.outline) this.outlineWrap.append(options.outline);
    this.outlineSizer = document.createElement('div');
    this.outlineSizer.className = 'lithium-properties-outline-sizer';
    this.outlineSizer.setAttribute('role', 'separator');
    this.outlineSizer.setAttribute('aria-orientation', 'horizontal');
    this.outlineSizer.title = 'Drag to resize the outline';
    this._onSizerDown = (event) => this._startOutlineResize(event);
    this.outlineSizer.addEventListener('pointerdown', this._onSizerDown);
    this.search = document.createElement('input');
    this.search.type = 'text';
    this.search.className = 'lithium-properties-search';
    this.search.placeholder = 'Search properties';
    this.search.setAttribute('aria-label', 'Search properties');
    this.gridHost = document.createElement('div');
    this.gridHost.className = 'lithium-properties-grid';
    // Manual clear X (browsers draw their own inconsistently); same class
    // as the table header-filter clear.
    this.searchWrap = document.createElement('div');
    this.searchWrap.className = 'lithium-properties-search-wrap';
    this.searchClear = document.createElement('span');
    this.searchClear.className = 'lithium-header-filter-clear lithium-properties-search-clear';
    this.searchClear.innerHTML = '&times;';
    this.searchClear.title = 'Clear search';
    this.searchClear.setAttribute('role', 'button');
    this.searchClear.addEventListener('click', (event) => {
      event.stopPropagation();
      this.search.value = '';
      this._applySearch();
      this.search.focus();
    });
    this.searchWrap.append(this.search, this.searchClear);
    this.body.append(this.searchWrap, this.outlineWrap, this.outlineSizer, this.gridHost);
    this.element.replaceChildren(top.bar, this.body, bottom.bar);
    processIcons(this.element);
    // Capture phase, ahead of Tabulator's own Tab binding (which stops
    // at the first read-only row, e.g. Object / Type).
    this._onKey = (event) => this._tabKey(event);
    this.element.addEventListener('keydown', this._onKey, true);
    this._onSearch = () => this._applySearch();
    this.search.addEventListener('input', this._onSearch);
    if (Number(state.outlineHeight) > 0) this.outlineWrap.style.height = `${Number(state.outlineHeight)}px`;
    this._setSearch(typeof state.showSearch === 'boolean' ? state.showSearch : Boolean(options.showSearch));
    this._setOutline(typeof state.showOutline === 'boolean' ? state.showOutline : options.showOutline !== false);
    this._pressed('favourites', this._favouritesOnly);
    this._pressed('hidden', this._showHidden);
    this.element.classList.toggle('lithium-properties-hidden-on', this._showHidden);
    this._stateReady = true;
    this.table = new Tabulator(this.gridHost, {
      // fitData + our own value width: fitColumns re-split the width by
      // widthGrow on each redraw and fought the drag (see _fitValue).
      layout: 'fitData',
      resizableColumnFit: true,
      height: '100%',
      index: 'id',
      headerVisible: false,
      groupBy: 'sectionName',
      groupStartOpen: (value) => !this._closedGroups.has(String(value).slice(3)),
      movableRows: true,
      rowContextMenu: (event, row) => this._rowMenu(row),
      groupToggleElement: 'header',
      groupHeader: (value, count, data) => groupHeaderHtml(
        data[0] ? data[0].sectionCaption : String(value).slice(3), data[0] ? data[0].sectionIcon : '', count,
      ),
      placeholder: 'No properties',
      columns: [
        {
          title: 'Property', field: 'label', width: this._nameWidth || 120, minWidth: 60, headerSort: false,
          resizable: true, rowHandle: true, formatter: (cell) => this._formatLabel(cell),
        },
        {
          title: 'Value', field: 'value', width: 160, minWidth: VALUE_MIN_WIDTH, headerSort: false,
          editable: (cell) => !cell.getRow().getData().readOnly,
          editor: (cell, onRendered, success, cancel, params) => this._editor(cell, onRendered, success, cancel, params),
          formatter: (cell) => this._format(cell), variableHeight: true,
        },
      ],
    });
    this._built = false;
    this._pending = null;
    this.table.on('tableBuilt', () => {
      this._built = true;
      if (this._pending) this.setRows(this._pending);
      this._fitValue();
    });
    // A name/value split drag: refit the value column into the rest of the
    // width and re-measure row heights for wrapped names.
    // Keep the dragged name width: a forced redraw resets columns to their
    // definition widths, so _refit re-applies it afterwards.
    this.table.on('columnResized', (column) => {
      if (column && column.getField && column.getField() === 'label') this._nameWidth = column.getWidth();
      this._refit();
      this._emitState();
    });
    // Keep the value column filling the frame as the splitter moves.
    this._lastWidth = 0;
    this._resizeWatch = typeof ResizeObserver === 'function'
      ? new ResizeObserver((entries) => {
        const width = Math.round(entries[0]?.contentRect?.width || 0);
        if (!this._built || !width || width === this._lastWidth) return;
        this._lastWidth = width;
        this._refit();
      })
      : null;
    this._resizeWatch?.observe(this.gridHost);
    this.table.on('renderComplete', () => processIcons(this.gridHost));
    this.table.on('groupVisibilityChanged', (group, visible) => {
      const caption = String(group.getKey()).slice(3);
      if (visible) this._closedGroups.delete(caption);
      else this._closedGroups.add(caption);
      this._emitState();
    });
    this.table.on('rowMoved', (row) => this._rowMoved(row));
    this.table.on('cellEdited', (cell) => {
      const data = cell.getRow().getData();
      if (cell.getValue() === cell.getOldValue()) return;
      this.onEdit(data.id, cell.getValue());
    });
  }

  setSections(sections) {
    this.sections = sections || [];
  }

  /** Replace the rows. Skips while a cell is being edited. */
  setRows(rows) {
    if (!this.table) return;
    if (!this._built) {
      this._pending = rows;
      return;
    }
    this._pending = null;
    this._rows = rows || [];
    if (this.element.querySelector('.tabulator-editing')) return;
    this._replacing = true;
    this.table.replaceData(tableData(this._rows, this.sections, this._prefs())).then(() => {
      this._replacing = false;
      this._applySearch();
      this._resumeEdit();
    }).catch(() => { this._replacing = false; });
  }

  _prefs() {
    return {
      favourites: [...this._favourites],
      hidden: [...this._hidden],
      order: this._order,
      showHidden: this._showHidden,
      favouritesOnly: this._favouritesOnly,
    };
  }

  _pressed(id, on) {
    const button = this.buttons[id];
    if (!button) return;
    button.setAttribute('aria-pressed', String(on));
    button.classList.toggle('lithium-nav-btn-active', on);
  }

  /** Repaint from the last rows after a preference change, then save. */
  _repaint() {
    this.setRows(this._rows);
    this._emitState();
  }

  toggleFavourite(id) {
    const key = String(id);
    if (this._favourites.has(key)) this._favourites.delete(key);
    else this._favourites.add(key);
    this._repaint();
  }

  toggleHidden(id) {
    const key = String(id);
    if (this._hidden.has(key)) this._hidden.delete(key);
    else this._hidden.add(key);
    this._repaint();
  }

  _rowMenu(row) {
    const data = row.getData();
    const check = (on, label) => `<span class="lithium-properties-check" aria-checked="${on}">${on ? '\u2611' : '\u2610'}</span> ${label}`;
    const items = [
      { label: check(this._favourites.has(String(data.id)), 'Favourite'), action: () => this.toggleFavourite(data.id) },
      { label: check(this._hidden.has(String(data.id)), 'Hidden'), action: () => this.toggleHidden(data.id) },
    ];
    if (this._order[data.sectionCaption]) {
      items.push({ separator: true }, {
        label: `Reset order in ${data.sectionCaption}`,
        action: () => {
          delete this._order[data.sectionCaption];
          this._repaint();
        },
      });
    }
    return items;
  }

  /**
   * A drag inside one section saves that section's order. A drop in
   * another section is undone by the repaint (rows keep their section).
   */
  _rowMoved(row) {
    const data = row.getData();
    const group = row.getGroup?.();
    const landed = group ? String(group.getKey()).slice(3) : data.sectionCaption;
    if (landed === data.sectionCaption) {
      const displayed = this.table.getRows().map((entry) => entry.getData());
      this._order[data.sectionCaption] = sectionOrder(displayed, data.sectionCaption);
    }
    this._repaint();
  }

  /** Editable rows in display order: open sections, after search. */
  _visibleRows() {
    if (!this.table || !this._built) return [];
    return this.table.getRows('active')
      .filter((row) => {
        const group = row.getGroup?.();
        return !group || group.isVisible();
      })
      .map((row) => row.getData());
  }

  /** Tab / Shift+Tab inside a value editor: next / previous editable value. */
  _tabKey(event) {
    if (event.key !== 'Tab' || event.ctrlKey || event.altKey || event.metaKey) return;
    const editing = this.gridHost.querySelector('.tabulator-cell.tabulator-editing');
    if (!editing) return;
    const rowEl = editing.closest('.tabulator-row');
    const row = this.table.getRows().find((entry) => entry.getElement() === rowEl);
    if (!row) return;
    event.preventDefault();
    event.stopPropagation();
    const target = nextEditableId(this._visibleRows(), row.getData().id, event.shiftKey ? -1 : 1);
    this._editNext = target;
    const field = editing.querySelector('input, select, textarea');
    if (field) field.blur();
    setTimeout(() => this._resumeEdit(), 0);
  }

  /** Open the editor queued by Tab once the commit and repaint settle. */
  _resumeEdit() {
    const id = this._editNext;
    if (!id || !this.table || !this._built || this._replacing) return;
    if (this.gridHost.querySelector('.tabulator-editing')) return;
    this._editNext = null;
    const row = this.table.getRow(id);
    const cell = row && row.getCell('value');
    if (cell && !row.getData().readOnly) cell.edit(true);
  }

  _applySearch() {
    if (this.searchClear) this.searchClear.style.display = this.search.value ? 'flex' : 'none';
    if (!this.table || !this._built) return;
    const term = this.search.hidden ? '' : this.search.value;
    if (!term.trim()) {
      if (this._filtered) this.table.clearFilter();
      this._filtered = false;
      return;
    }
    this._filtered = true;
    this.table.setFilter((data) => matchesSearch(data, term));
  }

  /** Enable the stack-order buttons. A missing flag leaves that button off. */
  setOrderEnabled(flags) {
    for (const id of ['backward', 'back', 'forward', 'front']) {
      const button = this.buttons[id];
      if (button) button.disabled = !flags || !flags[id];
    }
  }

  _onBar(action, button) {
    if (action === 'search') {
      this._setSearch(this.search.hidden);
      if (!this.search.hidden) this.search.focus();
    } else if (action === 'outline') {
      this._setOutline(this.outlineWrap.hidden);
    } else if (action === 'expand' || action === 'collapse') {
      this.setAllGroups(action === 'expand');
    } else if (action === 'prev' || action === 'next') {
      if (this.onNavigate) this.onNavigate(action === 'next' ? 1 : -1);
    } else if (action === 'backward' || action === 'back' || action === 'forward' || action === 'front') {
      if (this.onOrder) this.onOrder(action);
    } else if (action === 'favourites') {
      this._favouritesOnly = !this._favouritesOnly;
      this._pressed('favourites', this._favouritesOnly);
      this._repaint();
    } else if (action === 'hidden') {
      this._showHidden = !this._showHidden;
      this._pressed('hidden', this._showHidden);
      this.element.classList.toggle('lithium-properties-hidden-on', this._showHidden);
      this._repaint();
    } else {
      this.onStub(action, button);
    }
  }

  _setSearch(on) {
    this.search.hidden = !on;
    this.searchWrap.hidden = !on;
    this.buttons.search.setAttribute('aria-pressed', String(on));
    this.buttons.search.classList.toggle('lithium-nav-btn-active', on);
    this._applySearch();
    this._emitState();
  }

  /** Layout prefs for the caller to save; text in Search is not kept. */
  getState() {
    const height = parseFloat(this.outlineWrap.style.height);
    return {
      showSearch: !this.search.hidden,
      showOutline: !this.outlineWrap.hidden,
      nameWidth: this._nameWidth ? Math.round(this._nameWidth) : null,
      outlineHeight: height > 0 ? Math.round(height) : null,
      closedSections: [...this._closedGroups],
      favourites: [...this._favourites],
      hidden: [...this._hidden],
      order: { ...this._order },
      showHidden: this._showHidden,
      favouritesOnly: this._favouritesOnly,
      panelWidth: this._panelWidth,
    };
  }

  /** The caller's splitter width, kept with the other prefs. */
  setPanelWidth(width) {
    this._panelWidth = Number(width) > 0 ? Math.round(Number(width)) : null;
    this._emitState();
  }

  _emitState() {
    if (this._stateReady && this.onStateChange) this.onStateChange(this.getState());
  }

  _setOutline(on) {
    this.outlineWrap.hidden = !on;
    this.outlineSizer.hidden = !on;
    this.buttons.outline.setAttribute('aria-pressed', String(on));
    this.buttons.outline.classList.toggle('lithium-nav-btn-active', on);
    if (this._built) this.table.redraw();
    this._emitState();
  }

  /** Open or close every section; the choice survives a repaint. */
  /** Full redraw (refits fitColumns, re-measures rows) unless a cell is being edited. */
  _refit() {
    if (!this.table || !this._built) return;
    // Order matters: redraw(true) in fitData calls reinitializeColumnWidths,
    // which puts both columns back to their definition widths. Redraw
    // first, then set the widths, then re-measure wrapped rows.
    if (!this.gridHost.querySelector('.tabulator-editing')) this.table.redraw(true);
    this._fitValue();
    this.table.getRows?.().forEach((row) => row.normalizeHeight?.());
  }

  /** Value column = table holder inner width (no scrollbar) - name width. */
  _fitValue() {
    if (!this.table || !this._built || typeof this.table.getColumns !== 'function') return;
    const [label, value] = this.table.getColumns();
    if (!label || !value) return;
    if (this._nameWidth && Math.abs(label.getWidth() - this._nameWidth) > 1) label.setWidth(this._nameWidth);
    const holder = this.gridHost.querySelector('.tabulator-tableholder');
    const inner = (holder || this.gridHost).clientWidth;
    if (!inner) return;
    const want = valueWidth(inner, label.getWidth());
    if (Math.abs(value.getWidth() - want) > 1) value.setWidth(want);
  }

  setAllGroups(open) {
    if (!this.table || !this._built) return;
    // Each group.show()/hide() regroups the table and replaces the group
    // objects, so components from one getGroups() call go stale after the
    // first toggle. Look each group up fresh by key.
    const keys = this.table.getGroups().map((group) => group.getKey());
    for (const key of keys) {
      const caption = String(key).slice(3);
      if (open) this._closedGroups.delete(caption);
      else this._closedGroups.add(caption);
      const group = this.table.getGroups().find((entry) => entry.getKey() === key);
      if (!group || group.isVisible() === open) continue;
      if (open) group.show();
      else group.hide();
    }
    this._emitState();
  }

  _startOutlineResize(event) {
    event.preventDefault();
    const startY = event.clientY;
    const startH = this.outlineWrap.getBoundingClientRect().height;
    const maxH = Math.max(60, this.body.getBoundingClientRect().height - 80);
    const move = (moveEvent) => {
      const height = Math.min(maxH, Math.max(32, startH + moveEvent.clientY - startY));
      this.outlineWrap.style.height = `${height}px`;
    };
    const up = () => {
      document.removeEventListener('pointermove', move);
      document.removeEventListener('pointerup', up);
      this.outlineSizer.classList.remove('lithium-properties-outline-sizing');
      this.table?.redraw();
      this._emitState();
    };
    this.outlineSizer.classList.add('lithium-properties-outline-sizing');
    document.addEventListener('pointermove', move);
    document.addEventListener('pointerup', up);
  }

  _formatLabel(cell) {
    const data = cell.getRow().getData();
    const wrap = document.createElement('span');
    wrap.className = 'lithium-properties-name-wrap';
    const span = document.createElement('span');
    span.className = 'lithium-properties-name';
    span.textContent = cell.getValue() ?? '';
    span.title = `${cell.getValue() ?? ''} (${data.id}). Drag to reorder; right-click for Favourite or Hidden.`;
    if (data.labelType) {
      const type = document.createElement('span');
      type.className = 'lithium-properties-type';
      type.textContent = ` ${data.labelType}`;
      span.append(type);
    }
    wrap.append(span);
    // Right-justified marks, shown only while the matching bar button is
    // hovered (or, for Hidden, while its filter is on). See reports.css.
    if (data.isFavourite) wrap.insertAdjacentHTML('beforeend', '<span class="lithium-properties-mark lithium-properties-mark-favourite" aria-hidden="true"><fa fa-star></fa></span>');
    if (data.isHidden) wrap.insertAdjacentHTML('beforeend', '<span class="lithium-properties-mark lithium-properties-mark-hidden" aria-hidden="true"><fa fa-eye-slash></fa></span>');
    return wrap;
  }

  _format(cell) {
    const data = cell.getRow().getData();
    let text = data.display !== undefined && data.display !== null
      ? String(data.display)
      : (data.value === undefined || data.value === null ? '' : String(data.value));
    if (data.editor === 'PageSize') {
      const hit = this.pageSizes().find((entry) => Number(entry.value) === Number(data.value));
      if (hit) text = hit.label;
    }
    if (data.editor === 'List') {
      const listed = choiceLabel(data.value, data.list);
      if (listed) text = listed;
    }
    const wrap = document.createElement('span');
    wrap.className = 'lithium-properties-value';
    if (data.editor === 'Boolean') text = data.value === true || data.value === 'True' ? 'Yes' : 'No';
    if (data.editor === 'Color' && text) {
      const chip = document.createElement('span');
      chip.className = 'lithium-properties-chip';
      chip.style.background = text;
      wrap.append(chip);
    }
    const span = document.createElement('span');
    span.textContent = text;
    if (data.omitted) span.className = 'lithium-properties-default';
    if (data.multiline) {
      wrap.classList.add('lithium-properties-multiline');
      span.className = `${span.className} lithium-properties-memo`.trim();
    }
    wrap.append(span);
    if (data.note) {
      wrap.title = data.note;
      span.setAttribute('aria-description', data.note);
    }
    return wrap;
  }

  _editor(cell, onRendered, success, cancel) {
    const data = cell.getRow().getData();
    if (data.editor === 'File') {
      const marker = document.createElement('span');
      marker.className = 'lithium-properties-value';
      marker.textContent = cell.getValue() ?? '';
      if (typeof this.onFile === 'function') this.onFile(data);
      // Open the file dialog on this click, then leave the cell.
      // Cancelling inside this call tears the editor down mid-edit.
      onRendered(() => { setTimeout(() => cancel(), 0); });
      return marker;
    }
    if (data.editor === 'Color') return colorEditor(cell, onRendered, success, cancel);
    if (data.editor === 'Date') return dateEditor(cell, onRendered, success, cancel);
    let choices = data.editor === 'PageSize'
      ? this.pageSizes()
      : listChoices(data.list);
    if (data.editor === 'Boolean') {
      choices = [{ value: 'True', label: 'Yes' }, { value: 'False', label: 'No' }];
    }
    if (data.editor === 'FontFamily' && choices.length) {
      return fontFamilyEditor(cell, onRendered, success, cancel, choices);
    }
    if (data.editor === 'FontStyle' && choices.length) {
      return fontStyleEditor(cell, onRendered, success, cancel, choices);
    }
    if (data.editor === 'FontSize') {
      return fontSizeEditor(cell, onRendered, success, cancel, choices);
    }
    if (data.editor === 'PageSize' || data.editor === 'Boolean' || (data.editor === 'List' && choices.length)) {
      // Standing rule: custom dropdown only — never native <select>.
      const seed = {
        getValue: () => (data.editor === 'Boolean'
          ? (cell.getValue() === true || cell.getValue() === 'True' ? 'True' : 'False')
          : cell.getValue()),
      };
      return customDropdownEditor(seed, onRendered, success, cancel, choices, {
        editable: false,
        ariaLabel: data.editor === 'PageSize' ? 'Page size list'
          : data.editor === 'Boolean' ? 'Yes or No' : 'Choose a value',
        commitValue: data.editor === 'PageSize' ? (raw) => Number(raw) : undefined,
      });
    }
    if (data.multiline) {
      const area = document.createElement('textarea');
      area.className = 'lithium-properties-memo';
      area.rows = 1;
      area.value = cell.getValue() ?? '';
      let done = false;
      const commit = () => {
        if (done) return;
        done = true;
        success(area.value);
      };
      // One line tall, then the text. A fixed tall box was centred in
      // the short row, so the first line sat above the visible strip.
      const fit = () => {
        area.style.height = 'auto';
        const next = area.scrollHeight;
        if (next > 0) area.style.height = `${next}px`;
        cell.getRow?.()?.normalizeHeight?.();
      };
      const stay = new Set([
        'ArrowUp', 'ArrowDown', 'ArrowLeft', 'ArrowRight',
        'Home', 'End', 'PageUp', 'PageDown',
      ]);
      area.addEventListener('blur', commit);
      area.addEventListener('input', fit);
      area.addEventListener('keydown', (event) => {
        if (event.key === 'Escape') {
          event.stopPropagation();
          done = true;
          cancel();
        } else if (event.key === 'Enter' && (event.ctrlKey || event.metaKey)) {
          event.preventDefault();
          event.stopPropagation();
          commit();
        } else if (event.key === 'Enter' || stay.has(event.key)) {
          event.stopPropagation();
        }
      });
      onRendered(() => {
        area.closest('.tabulator-cell')?.classList.add('lithium-properties-editing-memo');
        area.focus();
        fit();
      });
      return area;
    }
    const input = document.createElement('input');
    input.type = 'text';
    if (data.editor === 'Number') {
      input.inputMode = 'decimal';
      input.className = 'lithium-properties-number';
      input.title = 'A number, with a unit for lengths (a bare number uses the report unit).';
    }
    input.value = cell.getValue() ?? '';
    let done = false;
    const commit = () => {
      if (done) return;
      done = true;
      success(input.value);
    };
    input.addEventListener('blur', commit);
    const stay = new Set([
      'ArrowUp', 'ArrowDown', 'ArrowLeft', 'ArrowRight',
      'Home', 'End', 'PageUp', 'PageDown',
    ]);
    input.addEventListener('keydown', (event) => {
      if (event.key === 'Enter') {
        event.stopPropagation();
        commit();
      } else if (event.key === 'Escape') {
        event.stopPropagation();
        done = true;
        cancel();
      } else if (stay.has(event.key)) {
        event.stopPropagation();
      }
    });
    onRendered(() => { input.focus(); input.select(); });
    return input;
  }

  destroy() {
    this._resizeWatch?.disconnect();
    this._resizeWatch = null;
    this.search?.removeEventListener('input', this._onSearch);
    this.element?.removeEventListener('keydown', this._onKey, true);
    this.outlineSizer?.removeEventListener('pointerdown', this._onSizerDown);
    this.table?.destroy();
    this.table = null;
  }
}
