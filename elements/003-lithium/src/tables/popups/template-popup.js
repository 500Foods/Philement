/**
 * Template Popup Module
 *
 * Template menu popup with save/load/manage functionality.
 *
 * @module tables/popups/template-popup
 */

import { processIcons } from '../../core/icons.js';
import { log, Subsystems, Status } from '../../core/log.js';
import { createPopupSeparator } from './popup-manager.js';

/**
 * Build template popup
 * @param {Object} table - LithiumTable instance
 * @returns {HTMLElement} Popup element
 */
export function buildTemplatePopup(table) {
  const templates = getSavedTemplates(table);
  const selectedName = getSelectedTemplateName(table, templates);
  const selectedIndex = templates.findIndex(
    (template) => getTemplateName(table, template) === selectedName,
  );

  const popup = document.createElement('div');
  popup.className = 'lithium-nav-popup lithium-template-menu';

  popup.appendChild(createTemplateMenuAction(table, 'Save Template', async () => {
    logTemplateMenuSelection(table, 'save');
    const savedTemplate = saveTemplate(table);
    if (!savedTemplate) return;
    table.templateMenuSelectedName = getTemplateName(table, savedTemplate);
    refreshTemplatePopup(table);
  }, { icon: 'floppy-disk' }));

  popup.appendChild(createTemplateMenuAction(table, 'Clear Template', () => {
    logTemplateMenuSelection(table, 'clear');
    closeNavPopup(table);
    table.clearTemplate?.();
  }, { icon: 'database' }));

  popup.appendChild(createPopupSeparator());

  const list = document.createElement('div');
  list.className = 'lithium-template-list';

  if (templates.length === 0) {
    const empty = document.createElement('div');
    empty.className = 'lithium-template-empty';
    empty.textContent = 'No saved templates';
    list.appendChild(empty);
  } else {
    templates.forEach((template) => {
      const templateName = getTemplateName(table, template);
      const row = document.createElement('button');
      row.type = 'button';
      row.className = 'lithium-nav-popup-item lithium-template-item';
      row.classList.toggle('is-selected', templateName === selectedName);

      const check = document.createElement('span');
      check.className = 'lithium-nav-popup-check';
      check.innerHTML = templateName === selectedName ? '&#10003;' : '';

      const nameSpan = document.createElement('span');
      nameSpan.className = 'lithium-template-name';
      nameSpan.textContent = templateName;

      row.appendChild(check);
      row.appendChild(nameSpan);

      row.addEventListener('click', async () => {
        logTemplateMenuSelection(table, 'select', templateName);
        await table.loadTemplate?.(template);
        refreshTemplatePopup(table);
      });

      list.appendChild(row);
    });
  }

  popup.appendChild(list);
  popup.appendChild(createPopupSeparator());

  popup.appendChild(createTemplateMenuAction(table, 'Copy to Clipboard', async () => {
    const selectedTemplate = selectedIndex >= 0 ? templates[selectedIndex] : null;
    logTemplateMenuSelection(table, 'copy', selectedTemplate ? getTemplateName(table, selectedTemplate) : 'current view');
    await table.copyTemplateToClipboard?.(selectedTemplate);
    closeNavPopup(table);
  }, { icon: 'copy' }));

  popup.appendChild(createTemplateMenuAction(table, 'Delete Template', () => {
    if (selectedIndex < 0) return;
    logTemplateMenuSelection(table, 'delete', selectedName);
    const deleted = deleteTemplate(table, selectedIndex);
    if (!deleted) return;
    table.templateMenuSelectedName = null;
    refreshTemplatePopup(table);
  }, { disabled: selectedIndex < 0, icon: 'trash-can' }));

  processIcons(popup);

  return popup;
}

/**
 * Create template menu action button
 * @param {Object} table - LithiumTable instance
 * @param {string} label - Button label
 * @param {Function} action - Click handler
 * @param {Object} options - Button options
 * @returns {HTMLElement} Button element
 */
function createTemplateMenuAction(table, label, action, options = {}) {
  const row = document.createElement('button');
  row.type = 'button';
  row.className = 'lithium-nav-popup-item';
  row.innerHTML = `
    <span class="lithium-nav-popup-icon"><fa fa-${options.icon || 'circle'}></fa></span>
    <span class="lithium-nav-popup-label">${label}</span>
  `;
  row.disabled = options.disabled === true;
  row.addEventListener('click', async () => {
    if (row.disabled) return;
    await action();
  });
  return row;
}

/**
 * Refresh template popup
 * @param {Object} table - LithiumTable instance
 */
function refreshTemplatePopup(table) {
  if (table.activeNavPopupId !== 'template') return;
  const btn = table.activeNavPopupButton
    || table.navigatorContainer?.querySelector(`#${table.cssPrefix}-nav-template`);
  if (!btn) return;

  const popup = buildTemplatePopup(table);
  if (!popup || !table.activeNavPopup) return;

  popup.style.position = 'fixed';
  popup.style.top = '0px';
  popup.style.left = '0px';
  popup.style.bottom = 'auto';
  table.activeNavPopup.replaceWith(popup);
  table.activeNavPopup = popup;
  
  // Re-position using popup-manager's logic
  const viewportPadding = 8;
  const gap = 4;
  const btnRect = btn.getBoundingClientRect();
  const popupRect = popup.getBoundingClientRect();
  const popupHeight = popupRect.height || popup.offsetHeight || 0;
  const availableAbove = btnRect.top - viewportPadding;
  const availableBelow = window.innerHeight - btnRect.bottom - viewportPadding;

  let top;
  if (popupHeight <= availableAbove || availableAbove >= availableBelow) {
    top = btnRect.top - popupHeight - gap;
  } else {
    top = btnRect.bottom + gap;
  }

  let left = btnRect.left;
  if (left + popupRect.width > window.innerWidth - viewportPadding) {
    left = window.innerWidth - popupRect.width - viewportPadding;
  }
  if (left < viewportPadding) {
    left = viewportPadding;
  }

  const maxTop = Math.max(viewportPadding, window.innerHeight - popupHeight - viewportPadding);
  popup.style.top = `${Math.min(Math.max(top, viewportPadding), maxTop)}px`;
  popup.style.left = `${left}px`;
  popup.style.right = 'auto';
  
  popup.classList.add('visible');
  requestAnimationFrame(() => refreshTemplatePopup(table));
}

/**
 * Get template storage key
 * @param {Object} table - LithiumTable instance
 * @returns {string} Storage key
 */
function getTemplateStorageKey(table) {
  return `${table.storageKey}_templates`;
}

/**
 * Get saved templates from localStorage
 * @param {Object} table - LithiumTable instance
 * @returns {Array} Saved templates
 */
export function getSavedTemplates(table) {
  const storageKey = getTemplateStorageKey(table);
  try {
    const stored = localStorage.getItem(storageKey);
    const templates = stored ? JSON.parse(stored) : [];
    return Array.isArray(templates) ? templates : [];
  } catch (_e) {
    return [];
  }
}

/**
 * Save templates to localStorage
 * @param {Object} table - LithiumTable instance
 * @param {Array} templates - Templates to save
 */
export function saveStoredTemplates(table, templates) {
  const storageKey = getTemplateStorageKey(table);
  localStorage.setItem(storageKey, JSON.stringify(templates));
}

/**
 * Get template name from template object
 * @param {Object} table - LithiumTable instance
 * @param {Object} template - Template object
 * @returns {string} Template name
 */
export function getTemplateName(table, template) {
  return template?._templateMeta?.name || template?.name || 'Unnamed Template';
}

/**
 * Get selected template name
 * @param {Object} table - LithiumTable instance
 * @param {Array} templates - Available templates
 * @returns {string|null} Selected template name
 */
function getSelectedTemplateName(table, templates = []) {
  if (table.templateMenuSelectedName && templates.some(
    (template) => getTemplateName(table, template) === table.templateMenuSelectedName,
  )) {
    return table.templateMenuSelectedName;
  }

  if (table.activeTemplateName && templates.some(
    (template) => getTemplateName(table, template) === table.activeTemplateName,
  )) {
    return table.activeTemplateName;
  }

  return null;
}

/**
 * Log template menu selection
 * @param {Object} table - LithiumTable instance
 * @param {string} action - Action performed
 * @param {string} detail - Additional detail
 */
function logTemplateMenuSelection(table, action, detail = '') {
  const suffix = detail ? `: ${detail}` : '';
  log(Subsystems.TABLE, Status.INFO, `[LithiumTable] Template menu ${action}${suffix}`);
}

/**
 * Save current table configuration as template
 * @param {Object} table - LithiumTable instance
 * @returns {Object|null} Saved template or null
 */
function saveTemplate(table) {
  const template = table.generateTemplateJSON?.();
  if (!template) return null;

  const defaultName = `${template._templateMeta?.name || 'Template'} - ${new Date().toLocaleDateString()}`;
  const name = window.prompt('Enter template name:', defaultName);
  if (!name) return null;

  template._templateMeta = template._templateMeta || {};
  template._templateMeta.name = name;

  const templates = getSavedTemplates(table);
  const existingIndex = templates.findIndex((item) => getTemplateName(table, item) === name);

  if (existingIndex >= 0) {
    templates[existingIndex] = template;
  } else {
    templates.push(template);
  }

  try {
    saveStoredTemplates(table, templates);
    table.templateMenuSelectedName = name;
    log(Subsystems.TABLE, Status.INFO, `[LithiumTable] Template saved: ${name}`);
    return template;
  } catch (_e) {
    log(Subsystems.TABLE, Status.ERROR, '[LithiumTable] Failed to save template');
    return null;
  }
}

/**
 * Delete template by index
 * @param {Object} table - LithiumTable instance
 * @param {number} index - Template index
 * @returns {boolean} Success
 */
function deleteTemplate(table, index) {
  const templates = getSavedTemplates(table);

  if (index >= 0 && index < templates.length) {
    const name = getTemplateName(table, templates[index]);
    templates.splice(index, 1);
    try {
      saveStoredTemplates(table, templates);
      if (table.templateMenuSelectedName === name) {
        table.templateMenuSelectedName = null;
      }
      if (table.activeTemplateName === name) {
        table.activeTemplateName = null;
      }
      log(Subsystems.TABLE, Status.INFO, `[LithiumTable] Template deleted: ${name}`);
      return true;
    } catch (_e) {
      log(Subsystems.TABLE, Status.ERROR, `[LithiumTable] Failed to delete template: ${_e?.message}`);
    }
  }
  return false;
}

// Stub for closeNavPopup - will be imported from popup-manager in main mixin
function closeNavPopup(table) {
  if (table.activeNavPopup) {
    table.activeNavPopup.remove();
    table.activeNavPopup = null;
    table.activeNavPopupId = null;
    table.activeNavPopupButton = null;
  }
  if (table.navPopupCloseHandler) {
    document.removeEventListener('click', table.navPopupCloseHandler);
    table.navPopupCloseHandler = null;
  }
  if (table.navPopupRepositionHandler) {
    window.removeEventListener('resize', table.navPopupRepositionHandler);
    document.removeEventListener('scroll', table.navPopupRepositionHandler, true);
    table.navPopupRepositionHandler = null;
  }
}
