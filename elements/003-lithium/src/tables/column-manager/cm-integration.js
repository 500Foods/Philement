/**
 * Column Manager Integration
 *
 * Handles Column Manager popup lifecycle and coordination.
 *
 * @module tables/column-manager/cm-integration
 */

import { log, Subsystems, Status } from '../../core/log.js';
import { LithiumColumnManager } from '../lithium-column-manager.js';

/**
 * Toggle the Column Manager popup.
 * @param {Object} table - LithiumTable instance
 * @param {Event} e - Click event
 * @param {Object} column - Tabulator column component
 */
export async function toggleColumnManager(table, e, column) {
  e.stopPropagation();

  if (table.columnManager) {
    closeColumnManager(table);
    return;
  }

  const parentColumnManager = table.container.closest('.col-manager-popup');
  closeAllColumnManagers(parentColumnManager);

  const managerId = getManagerId(table);

  if (!table.columnManager) {
    table.columnManager = new LithiumColumnManager({
      parentContainer: table.container,
      anchorElement: column.getElement(),
      parentTable: table,
      app: table.app,
      managerId: managerId,
      cssPrefix: 'col-manager',
      onColumnChange: (field, property, value) => {
        onColumnManagerChange(table, field, property, value);
      },
      onClose: () => {
        table.columnManager.hide();
      },
    });
  }

  await table.columnManager.init();
  table.columnManager.show();
}

/**
 * Close all Column Manager popups except the specified parent.
 * @param {HTMLElement} [parentColumnManager=null] - Popup to exclude from closing
 */
export function closeAllColumnManagers(parentColumnManager = null) {
  const popups = document.querySelectorAll('.col-manager-popup');

  popups.forEach((popup) => {
    if (popup === parentColumnManager) {
      return;
    }

    if (popup._columnManagerInstance) {
      popup._columnManagerInstance.cleanup();
    } else {
      popup.remove();
    }
  });
}

/**
 * Get a unique manager ID for this table.
 * @param {Object} table - LithiumTable instance
 * @returns {string} Manager ID
 */
export function getManagerId(table) {
  if (table.storageKey) {
    return table.storageKey;
  }

  if (table.tablePath) {
    return table.tablePath.replace(/\//g, '_');
  }

  if (table.container?.id) {
    return table.container.id;
  }

  return `table_${Date.now()}`;
}

/**
 * Close the Column Manager for this table.
 * @param {Object} table - LithiumTable instance
 */
export function closeColumnManager(table) {
  if (table.columnManager) {
    table.columnManager.cleanup();
    table.columnManager = null;
  }
}

/**
 * Handle column changes from the Column Manager.
 * @param {Object} table - LithiumTable instance
 * @param {string} field - Column field name
 * @param {string} property - Changed property
 * @param {*} value - New value
 */
export function onColumnManagerChange(table, field, property, value) {
  log(Subsystems.TABLE, Status.DEBUG,
    `[LithiumTable] Column manager change: ${field}.${property} = ${value}`);
}
