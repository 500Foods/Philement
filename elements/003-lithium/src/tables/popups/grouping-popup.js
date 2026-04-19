/**
 * Grouping Popup Module
 *
 * Handles the grouping popup menu with tri-state sort direction
 * and drag-to-reorder functionality.
 *
 * @module tables/popups/grouping-popup
 */

import { log, Subsystems, Status } from '../../core/log.js';
import { processIcons } from '../../core/icons.js';
import { resolveTableOptions, getLookup } from '../lithium-table.js';
import { closeNavPopupImmediate } from './popup-manager.js';

/**
 * Toggle the grouping popup
 * @param {Object} table - LithiumTable instance
 * @param {Event} e - Click event
 */
export function toggleGroupingPopup(table, e) {
  e.stopPropagation();

  // If already open, close it and return (toggle behavior)
  if (table.activeNavPopup && table.activeNavPopupId === 'grouping') {
    closeGroupingPopup(table);
    return;
  }

  // Close any other open nav popup first (immediately, no animation)
  if (table.activeNavPopup) {
    // Use immediate close to avoid animation conflicts when switching popups
    closeNavPopupImmediate(table);
  }

  // Dispatch event to close all manager popups
  document.dispatchEvent(new CustomEvent('close-all-popups'));

  const btn = e.currentTarget;
  const popup = buildGroupingPopup(table);

  if (!popup) return;

  showGroupingPopup(table, btn, popup);
}

/**
 * Build the grouping popup with column list
 * @param {Object} table - LithiumTable instance
 * @returns {HTMLElement} Popup element
 */
function buildGroupingPopup(table) {
  const popup = document.createElement('div');
  popup.className = 'lithium-nav-popup lithium-grouping-popup';

  // Collapse All (at top)
  const collapseBtn = createPopupItem({
    icon: 'angles-up',
    label: 'Collapse All',
    action: () => {
      closeGroupingPopup(table);
      table.collapseAll?.();
    },
  });
  popup.appendChild(collapseBtn);

  // Expand All (below collapse)
  const expandBtn = createPopupItem({
    icon: 'angles-down',
    label: 'Expand All',
    action: () => {
      closeGroupingPopup(table);
      table.expandAll?.();
    },
  });
  popup.appendChild(expandBtn);

  // Separator
  popup.appendChild(createSeparator());

  // Get columns that can be grouped (only those with groupable: true)
  const groupableColumns = getGroupableColumns(table);

  if (groupableColumns.length === 0) {
    const noCols = document.createElement('div');
    noCols.className = 'lithium-grouping-empty';
    noCols.textContent = 'No groupable columns';
    popup.appendChild(noCols);
  } else {
    // Sort by current group priority
    const sortedCols = sortColumnsByGroupPriority(groupableColumns);

    // Create list container
    const list = document.createElement('div');
    list.className = 'lithium-grouping-list';

    sortedCols.forEach((col, index) => {
      const item = createGroupingItem(table, col, index, sortedCols.length);
      list.appendChild(item);
    });

    popup.appendChild(list);

    // Setup drag and drop
    setupDragAndDrop(list, table);
  }

  // Separator
  popup.appendChild(createSeparator());

  // Remove Grouping
  const removeBtn = createPopupItem({
    icon: 'broom',
    label: 'Remove Grouping',
    action: () => {
      closeGroupingPopup(table);
      removeAllGrouping(table);
    },
  });
  popup.appendChild(removeBtn);

  return popup;
}

/**
 * Create a standard popup item with icon
 * @param {Object} options - Item options
 * @returns {HTMLElement} Item element
 */
function createPopupItem({ icon, label, action }) {
  const btn = document.createElement('button');
  btn.type = 'button';
  btn.className = 'lithium-nav-popup-item lithium-grouping-action';
  btn.innerHTML = `
    <fa class="lithium-grouping-action-icon" fa-${icon}></fa>
    <span class="lithium-grouping-action-label">${label}</span>
  `;
  btn.addEventListener('click', action);
  return btn;
}

/**
 * Create a separator element
 * @returns {HTMLElement} Separator element
 */
function createSeparator() {
  const sep = document.createElement('div');
  sep.className = 'lithium-nav-popup-separator';
  return sep;
}

/**
 * Check if a title contains an icon (fa- tag)
 * @param {string} title - Column title
 * @returns {boolean} True if title contains an icon
 */
function titleContainsIcon(title) {
  return typeof title === 'string' && /<fa\s+/.test(title);
}

/**
 * Get the display title for a column - uses title from tableDef
 * @param {Object} col - Column info from tableDef
 * @returns {string} Display title
 */
function getColumnDisplayTitle(col) {
  // Use the title property from the column definition
  if (col.title) {
    return col.title;
  }
  // Fallback to field name if no title
  return col.field || '';
}

/**
 * Create a lookup preview element for the grouping popup
 * For iconLabel: shows title (icon) + groupTitle (text label)
 * @param {Object} col - Column info from tableDef
 * @returns {string} HTML string for the preview, or empty string if not applicable
 */
function createLookupPreview(col) {
  if (!col.lookupRef) {
    return '';
  }

  // groupStyle controls display in grouping UI, fallback to lookupStyle
  const groupStyle = col.groupStyle || col.lookupStyle || 'label';

  switch (groupStyle) {
    case 'icon':
      // Title contains the icon
      return '';
    case 'iconLabel':
      // Title (icon) + groupTitle (text label)
      // groupTitle defaults to field name if not set
      {
        const groupTitle = col.groupTitle || col.field || '';
        if (groupTitle) {
          return `<span class="lithium-grouping-lookup-preview lithium-grouping-lookup-iconlabel"><span class="lithium-grouping-grouptitle">${groupTitle}</span></span>`;
        }
      }
      break;
    case 'label':
    default:
      return '';
  }

  return '';
}

/**
 * Create a grouping item for a column
 * @param {Object} table - LithiumTable instance
 * @param {Object} col - Column info
 * @param {number} index - Current index in list
 * @param {number} total - Total items in list
 * @returns {HTMLElement} Item element
 */
function createGroupingItem(table, col, index, total) {
  const item = document.createElement('div');
  item.className = 'lithium-grouping-item';
  item.dataset.field = col.field;
  item.draggable = true;

  // Determine current state
  const isGrouped = col.groupPri != null && col.groupable === true;
  const sortDir = col.groupDir || 'asc';

  // Get tooltip text for current state
  const tooltipText = getGroupingTooltip(isGrouped, sortDir);

  // Tri-state arrow (left side) - using Font Awesome icons
  const arrowBtn = document.createElement('button');
  arrowBtn.type = 'button';
  arrowBtn.className = 'lithium-grouping-arrow';
  arrowBtn.title = tooltipText;
  arrowBtn.innerHTML = getArrowIcon(isGrouped, sortDir);

  // Column title - handle icon titles, use the title value from column definition
  const title = document.createElement('span');
  title.className = 'lithium-grouping-title';
  title.title = tooltipText;

  const displayTitle = getColumnDisplayTitle(col);

  // Build title content with optional lookup preview
  const lookupPreview = createLookupPreview(col);
  if (lookupPreview) {
    // Show title with lookup preview
    title.innerHTML = `<span class="lithium-grouping-title-text">${displayTitle}</span><span class="lithium-grouping-title-separator">—</span>${lookupPreview}`;
  } else if (titleContainsIcon(displayTitle)) {
    // Title contains an icon - render as HTML
    title.innerHTML = displayTitle;
  } else {
    // Regular text title
    title.textContent = displayTitle;
  }

  if (isGrouped) {
    title.classList.add('lithium-grouping-active');
  }

  // Make the title clickable to cycle through states (same as clicking the arrow)
  title.style.cursor = 'pointer';
  title.addEventListener('click', (e) => {
    e.stopPropagation();
    cycleArrowState(table, col.field, isGrouped, sortDir);
  });

  // Also handle arrow button click
  arrowBtn.addEventListener('click', (e) => {
    e.stopPropagation();
    cycleArrowState(table, col.field, isGrouped, sortDir);
  });

  // Drag handle (right side) - using grip-dots-vertical
  const handle = document.createElement('span');
  handle.className = 'lithium-grouping-handle';
  handle.innerHTML = '<fa fa-grip-dots-vertical></fa>';
  handle.title = 'Drag to reorder grouping priority';

  item.appendChild(arrowBtn);
  item.appendChild(title);
  item.appendChild(handle);

  return item;
}

/**
 * Get tooltip text based on grouping state
 * @param {boolean} isGrouped - Whether column is grouped
 * @param {string} sortDir - Sort direction
 * @returns {string} Tooltip text
 */
function getGroupingTooltip(isGrouped, sortDir) {
  if (!isGrouped) return 'Click to group ascending';
  return sortDir === 'asc' ? 'Click to group descending' : 'Click to remove grouping';
}

/**
 * Get arrow icon based on state - using Font Awesome icons
 * @param {boolean} isGrouped - Whether column is grouped
 * @param {string} sortDir - Sort direction
 * @returns {string} HTML for icon
 */
function getArrowIcon(isGrouped, sortDir) {
  if (!isGrouped) {
    // Disabled state - show arrows up down icon muted
    return '<span class="lithium-grouping-sort-icons" data-sort-dir="none"><fa class="lithium-grouping-sort-none" fa-angles-up-down></fa></span>';
  }
  // Active state - show only the active icon
  return sortDir === 'asc'
    ? '<span class="lithium-grouping-sort-icons" data-sort-dir="asc"><fa class="lithium-grouping-sort-asc" fa-angle-up></fa></span>'
    : '<span class="lithium-grouping-sort-icons" data-sort-dir="desc"><fa class="lithium-grouping-sort-desc" fa-angle-down></fa></span>';
}

/**
 * Cycle through arrow states: disabled -> asc -> desc -> disabled
 * @param {Object} table - LithiumTable instance
 * @param {string} field - Column field name
 * @param {boolean} isGrouped - Current grouped state
 * @param {string} sortDir - Current sort direction
 */
function cycleArrowState(table, field, isGrouped, sortDir) {
  let newGroupPri = null;
  let newGroupOrd = 'asc';

  if (!isGrouped) {
    // Disabled -> Ascending: assign next available priority
    const maxPri = getMaxGroupPriority(table);
    newGroupPri = maxPri + 1;
    newGroupOrd = 'asc';
  } else if (sortDir === 'asc') {
    // Ascending -> Descending: keep priority, change direction
    const col = getColumnFromTableDef(table, field);
    newGroupPri = col?.groupPri || 1;
    newGroupOrd = 'desc';
  } else {
    // Descending -> Disabled: remove from grouping
    newGroupPri = null;
    newGroupOrd = 'asc';
  }

  // Update the column definition
  updateColumnGrouping(table, field, newGroupPri, newGroupOrd);
}

/**
 * Get the maximum group priority currently assigned
 * @param {Object} table - LithiumTable instance
 * @returns {number} Max priority
 */
function getMaxGroupPriority(table) {
  if (!table.tableDef?.columns) return 0;

  let max = 0;
  for (const col of Object.values(table.tableDef.columns)) {
    if (col.groupPri != null && col.groupPri > max) {
      max = col.groupPri;
    }
  }
  return max;
}

/**
 * Get column info from tableDef
 * @param {Object} table - LithiumTable instance
 * @param {string} field - Field name
 * @returns {Object|null} Column definition
 */
function getColumnFromTableDef(table, field) {
  return table.tableDef?.columns?.[field] || null;
}

/**
 * Update column grouping properties
 * @param {Object} table - LithiumTable instance
 * @param {string} field - Column field
 * @param {number|null} groupPri - Group priority (null to remove)
 * @param {string} groupDir - Group direction
 */
function updateColumnGrouping(table, field, groupPri, groupDir) {
  if (!table.tableDef?.columns?.[field]) return;

  const col = table.tableDef.columns[field];

  if (groupPri == null) {
    // Remove grouping - renumber remaining columns
    delete col.groupPri;
    delete col.groupDir;
    renumberGroupPriorities(table);
  } else {
    col.groupable = true;
    col.groupPri = groupPri;
    col.groupDir = groupDir;
  }

  // Apply the new grouping to the table
  applyGroupingChanges(table);

  // Update the popup UI in place instead of closing/reopening
  updatePopupUIInPlace(table);
}

/**
 * Renumber group priorities to be sequential (1, 2, 3...)
 * @param {Object} table - LithiumTable instance
 */
function renumberGroupPriorities(table) {
  if (!table.tableDef?.columns) return;

  // Collect grouped columns and sort by current priority
  const grouped = [];
  for (const [field, col] of Object.entries(table.tableDef.columns)) {
    if (col.groupPri != null) {
      grouped.push({ field, pri: col.groupPri });
    }
  }
  grouped.sort((a, b) => a.pri - b.pri);

  // Reassign sequential priorities
  grouped.forEach((item, index) => {
    table.tableDef.columns[item.field].groupPri = index + 1;
  });
}

/**
 * Apply grouping changes to the live table
 * @param {Object} table - LithiumTable instance
 */
function applyGroupingChanges(table) {
  if (!table.table) return;

  // Use the resolveTableOptions logic to rebuild groupBy
  const opts = resolveTableOptions(table.tableDef);

  if (opts.groupBy) {
    table.table.setGroupBy(opts.groupBy);
    table.table.setGroupStartOpen(true);
  } else {
    table.table.setGroupBy(false);
  }

  // Update group icons after change
  setTimeout(() => table.updateGroupIcons?.(), 50);

  log(Subsystems.TABLE, Status.INFO, `[LithiumTable] Grouping updated: ${JSON.stringify(opts.groupBy)}`);
}

/**
 * Remove all grouping from the table
 * @param {Object} table - LithiumTable instance
 */
function removeAllGrouping(table) {
  if (!table.tableDef?.columns) return;

  // Clear all groupPri values
  for (const col of Object.values(table.tableDef.columns)) {
    delete col.groupPri;
    delete col.groupDir;
  }

  // Remove grouping from the table
  table.table?.setGroupBy(false);

  log(Subsystems.TABLE, Status.INFO, '[LithiumTable] All grouping removed');
}

/**
 * Get list of columns that can be grouped (only those with groupable: true)
 * @param {Object} table - LithiumTable instance
 * @returns {Array} Array of column info objects
 */
function getGroupableColumns(table) {
  if (!table.tableDef?.columns) return [];

  const columns = [];
  for (const [field, col] of Object.entries(table.tableDef.columns)) {
    // Only include columns explicitly marked as groupable: true
    if (col.groupable !== true) continue;

    // Skip selector column
    if (field === '_selector') continue;

    columns.push({
      field,
      title: col.title,
      groupable: col.groupable,
      groupPri: col.groupPri,
      groupDir: col.groupDir,
      lookupRef: col.lookupRef,
      lookupStyle: col.lookupStyle,
      groupStyle: col.groupStyle,
      groupTitle: col.groupTitle,
    });
  }
  return columns;
}

/**
 * Sort columns by group priority (grouped first, then by priority, then alphabetically)
 * @param {Array} columns - Column array
 * @returns {Array} Sorted array
 */
function sortColumnsByGroupPriority(columns) {
  return [...columns].sort((a, b) => {
    // Grouped columns come first
    const aGrouped = a.groupPri != null && a.groupable === true;
    const bGrouped = b.groupPri != null && b.groupable === true;

    if (aGrouped && !bGrouped) return -1;
    if (!aGrouped && bGrouped) return 1;

    // Both grouped: sort by priority
    if (aGrouped && bGrouped) {
      return (a.groupPri || 0) - (b.groupPri || 0);
    }

    // Neither grouped: sort alphabetically by title (strip HTML for comparison)
    const aTitle = typeof a.title === 'string' ? a.title.replace(/<[^>]*>/g, '') : (a.title || a.field);
    const bTitle = typeof b.title === 'string' ? b.title.replace(/<[^>]*>/g, '') : (b.title || b.field);
    return aTitle.localeCompare(bTitle);
  });
}

/**
 * Setup drag and drop for the grouping list
 * @param {HTMLElement} list - List container
 * @param {Object} table - LithiumTable instance
 */
function setupDragAndDrop(list, table) {
  let draggedItem = null;

  list.addEventListener('dragstart', (e) => {
    draggedItem = e.target.closest('.lithium-grouping-item');
    if (!draggedItem) return;

    e.dataTransfer.effectAllowed = 'move';
    draggedItem.classList.add('lithium-grouping-dragging');
  });

  list.addEventListener('dragend', (e) => {
    if (draggedItem) {
      draggedItem.classList.remove('lithium-grouping-dragging');
      draggedItem = null;
    }
  });

  list.addEventListener('dragover', (e) => {
    e.preventDefault();
    e.dataTransfer.dropEffect = 'move';

    if (!draggedItem) return;

    const targetItem = e.target.closest('.lithium-grouping-item');
    if (!targetItem || targetItem === draggedItem) return;

    const rect = targetItem.getBoundingClientRect();
    const midpoint = rect.top + rect.height / 2;

    if (e.clientY < midpoint) {
      list.insertBefore(draggedItem, targetItem);
    } else {
      list.insertBefore(draggedItem, targetItem.nextSibling);
    }
  });

  list.addEventListener('drop', (e) => {
    e.preventDefault();
    if (!draggedItem) return;

    // Recalculate priorities based on new order
    updatePrioritiesFromOrder(list, table);
  });
}

/**
 * Update group priorities based on the visual order in the list
 * @param {HTMLElement} list - List container
 * @param {Object} table - LithiumTable instance
 */
function updatePrioritiesFromOrder(list, table) {
  const items = list.querySelectorAll('.lithium-grouping-item');
  let priority = 1;

  items.forEach((item) => {
    const field = item.dataset.field;
    const col = getColumnFromTableDef(table, field);

    if (col && col.groupPri != null) {
      col.groupPri = priority++;
    }
  });

  // Apply changes
  applyGroupingChanges(table);
}

/**
 * Show the grouping popup using footer-riseup positioning
 * Bottom-right corner of popup is 1px above top-right corner of button
 * @param {Object} table - LithiumTable instance
 * @param {HTMLElement} btn - Button element
 * @param {HTMLElement} popup - Popup element
 */
function showGroupingPopup(table, btn, popup) {
  popup.style.position = 'fixed';
  // Footer-riseup positioning: bottom-right of popup 1px above top-right of button
  popup.style.bottom = 'auto';
  popup.style.top = 'auto';
  popup.style.left = 'auto';
  popup.style.right = 'auto';

  // Add popup-active class to button for toggle styling
  btn.classList.add('popup-active');

  document.body.appendChild(popup);

  // Process icons after popup is attached to DOM
  processIcons(popup);

  // Footer-riseup positioning: bottom-right of popup 1px above top-right of button
  const btnRect = btn.getBoundingClientRect();
  popup.style.bottom = `${window.innerHeight - btnRect.top + 1}px`;
  popup.style.right = `${window.innerWidth - btnRect.right}px`;

  // Show with animation
  requestAnimationFrame(() => {
    popup.classList.add('visible');
  });

  // Store references
  table.activeNavPopup = popup;
  table.activeNavPopupId = 'grouping';
  table.activeNavPopupButton = btn;

  // Setup close handler
  table.navPopupCloseHandler = (evt) => {
    if (!popup.contains(evt.target) && !btn.contains(evt.target)) {
      closeGroupingPopup(table);
    }
  };
  document.addEventListener('click', table.navPopupCloseHandler);

  // ESC key handler
  table.navPopupKeyHandler = (evt) => {
    if (evt.key === 'Escape') {
      closeGroupingPopup(table);
    }
  };
  document.addEventListener('keydown', table.navPopupKeyHandler);

  // Listen for close-all-popups from manager menus
  table.navPopupGlobalCloseHandler = () => {
    closeGroupingPopup(table);
  };
  document.addEventListener('close-all-popups', table.navPopupGlobalCloseHandler);
}

/**
 * Close the grouping popup immediately (no animation)
 * Used when opening a new popup to avoid animation conflicts
 * @param {Object} table - LithiumTable instance
 */
export function closeGroupingPopupImmediate(table) {
  // Remove popup-active class from button
  if (table.activeNavPopupButton) {
    table.activeNavPopupButton.classList.remove('popup-active');
  }
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
  if (table.navPopupKeyHandler) {
    document.removeEventListener('keydown', table.navPopupKeyHandler);
    table.navPopupKeyHandler = null;
  }
  if (table.navPopupGlobalCloseHandler) {
    document.removeEventListener('close-all-popups', table.navPopupGlobalCloseHandler);
    table.navPopupGlobalCloseHandler = null;
  }
}

/**
 * Close the grouping popup with animation
 * @param {Object} table - LithiumTable instance
 */
export function closeGroupingPopup(table) {
  // Remove popup-active class from button
  if (table.activeNavPopupButton) {
    table.activeNavPopupButton.classList.remove('popup-active');
  }
  if (table.activeNavPopup) {
    table.activeNavPopup.classList.remove('visible');
    // Remove after animation completes
    const duration = 200; // Match transition duration
    setTimeout(() => {
      if (table.activeNavPopup && table.activeNavPopup.parentNode) {
        table.activeNavPopup.remove();
      }
    }, duration);
    table.activeNavPopup = null;
    table.activeNavPopupId = null;
    table.activeNavPopupButton = null;
  }
  if (table.navPopupCloseHandler) {
    document.removeEventListener('click', table.navPopupCloseHandler);
    table.navPopupCloseHandler = null;
  }
  if (table.navPopupKeyHandler) {
    document.removeEventListener('keydown', table.navPopupKeyHandler);
    table.navPopupKeyHandler = null;
  }
  if (table.navPopupGlobalCloseHandler) {
    document.removeEventListener('close-all-popups', table.navPopupGlobalCloseHandler);
    table.navPopupGlobalCloseHandler = null;
  }
}

/**
 * Update the popup UI in place without closing/reopening
 * @param {Object} table - LithiumTable instance
 */
function updatePopupUIInPlace(table) {
  if (table.activeNavPopupId !== 'grouping' || !table.activeNavPopup) return;

  const popup = table.activeNavPopup;
  const list = popup.querySelector('.lithium-grouping-list');
  if (!list) return;

  // Get current groupable columns
  const groupableColumns = getGroupableColumns(table);
  const sortedCols = sortColumnsByGroupPriority(groupableColumns);

  // Get all current items
  const items = Array.from(list.querySelectorAll('.lithium-grouping-item'));
  const itemMap = new Map(items.map(item => [item.dataset.field, item]));

  // Track which fields we've processed
  const processedFields = new Set();

  // Update existing items and create new ones as needed
  sortedCols.forEach((col, index) => {
    processedFields.add(col.field);
    let item = itemMap.get(col.field);
    const isGrouped = col.groupPri != null && col.groupable === true;
  const sortDir = col.groupDir || 'asc';

    if (!item) {
      // Create new item if it doesn't exist (shouldn't happen normally)
      item = createGroupingItem(table, col, index, sortedCols.length);
      list.appendChild(item);
      processIcons(item);
    } else {
      // Update existing item
      const arrowBtn = item.querySelector('.lithium-grouping-arrow');
      const title = item.querySelector('.lithium-grouping-title');

      // Get current tooltip text for this state
      const tooltipText = getGroupingTooltip(isGrouped, sortDir);

      // Update arrow icon and tooltip
      if (arrowBtn) {
        arrowBtn.title = tooltipText;
        arrowBtn.innerHTML = getArrowIcon(isGrouped, sortDir);
      }

      // Update title styling and tooltip
      if (title) {
        title.classList.toggle('lithium-grouping-active', isGrouped);
        title.title = tooltipText;
      }

      // Update click handlers - look up current state at click time
      const newClickHandler = (e) => {
        e.stopPropagation();
        // Get fresh state from tableDef at click time
        const freshCol = getColumnFromTableDef(table, col.field);
        const freshIsGrouped = freshCol?.groupPri != null && freshCol?.groupable === true;
        const freshSortDir = freshCol?.groupDir || 'asc';
        cycleArrowState(table, col.field, freshIsGrouped, freshSortDir);
      };

      if (arrowBtn) {
        // Clear title to dismiss any visible tooltip before cloning
        const savedTitle = arrowBtn.title;
        arrowBtn.title = '';
        // Replace click handler by cloning
        const newArrowBtn = arrowBtn.cloneNode(true);
        newArrowBtn.title = savedTitle;
        newArrowBtn.addEventListener('click', newClickHandler);
        arrowBtn.parentNode.replaceChild(newArrowBtn, arrowBtn);
      }

      if (title) {
        // Clear title to dismiss any visible tooltip before cloning
        const savedTitle = title.title;
        title.title = '';
        const newTitle = title.cloneNode(true);
        newTitle.title = savedTitle;
        newTitle.style.cursor = 'pointer';
        newTitle.addEventListener('click', newClickHandler);
        title.parentNode.replaceChild(newTitle, title);
      }
    }
  });

  // Re-sort the DOM elements to match the new order
  sortedCols.forEach((col) => {
    const item = itemMap.get(col.field);
    if (item) {
      list.appendChild(item); // Moves to end, effectively re-ordering
    }
  });

  // Process any new icons
  processIcons(list);
}