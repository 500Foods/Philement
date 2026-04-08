/**
 * Navigator Builder Module
 *
 * Builds the navigator control bar HTML and wires up event handlers.
 *
 * @module tables/navigator/navigator-builder
 */

import { processIcons } from '../../core/icons.js';

/**
 * Get the HTML for the navigator control bar
 * @param {Object} options - Configuration options
 * @param {string} options.cssPrefix - CSS class prefix
 * @param {boolean} options.filtersVisible - Whether filters are visible
 * @param {boolean} options.readonly - Whether table is readonly
 * @param {boolean} options.alwaysEditable - Whether table is always editable
 * @returns {string} HTML string
 */
export function getNavigatorHTML({ cssPrefix, filtersVisible, readonly, alwaysEditable }) {
  return `
    <div class="lithium-nav-wrapper">
      <div class="lithium-nav-block lithium-nav-block-control">
        <button type="button" class="lithium-nav-btn" id="${cssPrefix}-nav-refresh" title="Refresh">
          <fa fa-arrows-rotate></fa>
        </button>
        <button type="button" class="lithium-nav-btn ${filtersVisible ? 'lithium-nav-btn-active' : ''}" id="${cssPrefix}-nav-filter" title="Toggle Filters">
          <fa fa-filter></fa>
        </button>
        <button type="button" class="lithium-nav-btn lithium-nav-btn-has-popup" id="${cssPrefix}-nav-menu" title="Table Options">
          <fa fa-layer-group></fa>
        </button>
        <button type="button" class="lithium-nav-btn lithium-nav-btn-has-popup" id="${cssPrefix}-nav-width" title="Table Width">
          <fa fa-left-right></fa>
        </button>
        <button type="button" class="lithium-nav-btn lithium-nav-btn-has-popup" id="${cssPrefix}-nav-layout" title="Table Layout">
          <fa fa-table-columns></fa>
        </button>
        <button type="button" class="lithium-nav-btn lithium-nav-btn-has-popup" id="${cssPrefix}-nav-template" title="Templates">
          <fa fa-star></fa>
        </button>
      </div>
      <div class="lithium-nav-block lithium-nav-block-move">
        <button type="button" class="lithium-nav-btn" id="${cssPrefix}-nav-first" title="First Record">
          <fa fa-triple-chevrons-left></fa>
        </button>
        <button type="button" class="lithium-nav-btn" id="${cssPrefix}-nav-pgup" title="Previous Page">
          <fa fa-chevrons-left></fa>
        </button>
        <button type="button" class="lithium-nav-btn" id="${cssPrefix}-nav-prev" title="Previous Record">
          <fa fa-chevron-left></fa>
        </button>
        <button type="button" class="lithium-nav-btn" id="${cssPrefix}-nav-next" title="Next Record">
          <fa fa-chevron-right></fa>
        </button>
        <button type="button" class="lithium-nav-btn" id="${cssPrefix}-nav-pgdn" title="Next Page">
          <fa fa-chevrons-right></fa>
        </button>
        <button type="button" class="lithium-nav-btn" id="${cssPrefix}-nav-last" title="Last Record">
          <fa fa-triple-chevrons-right></fa>
        </button>
      </div>
      ${!readonly && !alwaysEditable ? `
      <div class="lithium-nav-block lithium-nav-block-manage">
        <button type="button" class="lithium-nav-btn" id="${cssPrefix}-nav-add" title="Add">
          <fa fa-plus></fa>
        </button>
        <button type="button" class="lithium-nav-btn" id="${cssPrefix}-nav-duplicate" title="Duplicate">
          <fa fa-copy></fa>
        </button>
        <button type="button" class="lithium-nav-btn" id="${cssPrefix}-nav-edit" title="Edit">
          <fa fa-pen-to-square></fa>
        </button>
        <button type="button" class="lithium-nav-btn" id="${cssPrefix}-nav-flag" title="Flag">
          <fa fa-flag></fa>
        </button>
        <button type="button" class="lithium-nav-btn" id="${cssPrefix}-nav-annotate" title="Annotate">
          <fa fa-note-sticky></fa>
        </button>
        <button type="button" class="lithium-nav-btn lithium-nav-btn-danger" id="${cssPrefix}-nav-delete" title="Delete">
          <fa fa-trash-can></fa>
        </button>
      </div>
      ` : ''}
      <div class="lithium-nav-block lithium-nav-block-search">
        <button type="button" class="lithium-nav-btn" id="${cssPrefix}-search-btn" title="Search">
          <fa fa-magnifying-glass></fa>
        </button>
        <input type="text" class="lithium-nav-search-input" id="${cssPrefix}-search-input" placeholder="Search...">
        <button type="button" class="lithium-nav-btn" id="${cssPrefix}-search-clear-btn" title="Clear Search">
          <fa fa-xmark></fa>
        </button>
      </div>
    </div>
  `;
}

/**
 * Build navigator and attach to container
 * @param {Object} table - LithiumTable instance
 */
export function buildNavigator(table) {
  if (!table.navigatorContainer) return;

  table.navigatorContainer.innerHTML = getNavigatorHTML({
    cssPrefix: table.cssPrefix,
    filtersVisible: table.filtersVisible,
    readonly: table.readonly,
    alwaysEditable: table.alwaysEditable,
  });
  processIcons(table.navigatorContainer);

  wireControlButtons(table);
  wireMoveButtons(table);
  wireManageButtons(table);
  wireSearchButtons(table);
}

/**
 * Wire control button events
 * @param {Object} table - LithiumTable instance
 */
function wireControlButtons(table) {
  const nav = table.navigatorContainer;
  if (!nav) return;

  nav.querySelector(`#${table.cssPrefix}-nav-refresh`)?.addEventListener('click', () => table.handleRefresh?.());
  nav.querySelector(`#${table.cssPrefix}-nav-filter`)?.addEventListener('click', () => table.handleFilter?.());
  nav.querySelector(`#${table.cssPrefix}-nav-menu`)?.addEventListener('click', (e) => table.toggleNavPopup?.(e, 'menu'));
  nav.querySelector(`#${table.cssPrefix}-nav-width`)?.addEventListener('click', (e) => table.toggleNavPopup?.(e, 'width'));
  nav.querySelector(`#${table.cssPrefix}-nav-layout`)?.addEventListener('click', (e) => table.toggleNavPopup?.(e, 'layout'));
  nav.querySelector(`#${table.cssPrefix}-nav-template`)?.addEventListener('click', (e) => table.toggleNavPopup?.(e, 'template'));
}

/**
 * Wire move/navigation button events
 * @param {Object} table - LithiumTable instance
 */
function wireMoveButtons(table) {
  const nav = table.navigatorContainer;
  if (!nav) return;

  nav.querySelector(`#${table.cssPrefix}-nav-first`)?.addEventListener('click', () => table.navigateFirst?.());
  nav.querySelector(`#${table.cssPrefix}-nav-pgup`)?.addEventListener('click', () => table.navigatePrevPage?.());
  nav.querySelector(`#${table.cssPrefix}-nav-prev`)?.addEventListener('click', () => table.navigatePrevRec?.());
  nav.querySelector(`#${table.cssPrefix}-nav-next`)?.addEventListener('click', () => table.navigateNextRec?.());
  nav.querySelector(`#${table.cssPrefix}-nav-pgdn`)?.addEventListener('click', () => table.navigateNextPage?.());
  nav.querySelector(`#${table.cssPrefix}-nav-last`)?.addEventListener('click', () => table.navigateLast?.());
}

/**
 * Wire manage (CRUD) button events
 * @param {Object} table - LithiumTable instance
 */
function wireManageButtons(table) {
  if (table.readonly || table.alwaysEditable) return;

  const nav = table.navigatorContainer;
  if (!nav) return;

  nav.querySelector(`#${table.cssPrefix}-nav-add`)?.addEventListener('click', () => table.handleAdd?.());
  nav.querySelector(`#${table.cssPrefix}-nav-duplicate`)?.addEventListener('click', () => table.handleDuplicate?.());
  nav.querySelector(`#${table.cssPrefix}-nav-edit`)?.addEventListener('click', () => table.handleEdit?.());
  nav.querySelector(`#${table.cssPrefix}-nav-flag`)?.addEventListener('click', () => table.handleFlag?.());
  nav.querySelector(`#${table.cssPrefix}-nav-annotate`)?.addEventListener('click', () => table.handleAnnotate?.());
  nav.querySelector(`#${table.cssPrefix}-nav-delete`)?.addEventListener('click', () => table.handleDelete?.());
}

/**
 * Wire search input events
 * @param {Object} table - LithiumTable instance
 */
function wireSearchButtons(table) {
  const nav = table.navigatorContainer;
  if (!nav) return;

  const searchInput = nav.querySelector(`#${table.cssPrefix}-search-input`);
  const searchBtn = nav.querySelector(`#${table.cssPrefix}-search-btn`);
  const clearBtn = nav.querySelector(`#${table.cssPrefix}-search-clear-btn`);

  const performSearch = () => {
    table.loadData?.(searchInput.value);
  };

  searchBtn?.addEventListener('click', performSearch);
  searchInput?.addEventListener('keypress', (e) => {
    if (e.key === 'Enter') performSearch();
  });

  clearBtn?.addEventListener('click', () => {
    searchInput.value = '';
    table.loadData?.();
  });
}

/**
 * Update edit button state
 * @param {Object} table - LithiumTable instance
 */
export function updateEditButtonState(table) {
  if (table.readonly || table.alwaysEditable || !table.navigatorContainer) return;
  const editBtn = table.navigatorContainer.querySelector(`#${table.cssPrefix}-nav-edit`);
  if (editBtn) {
    editBtn.classList.toggle('lithium-nav-btn-active', table.isEditing);
  }
}

/**
 * Update duplicate button state (disabled when no selection)
 * @param {Object} table - LithiumTable instance
 */
export function updateDuplicateButtonState(table) {
  if (table.readonly || table.alwaysEditable || !table.navigatorContainer) return;
  const duplicateBtn = table.navigatorContainer.querySelector(`#${table.cssPrefix}-nav-duplicate`);
  if (duplicateBtn) {
    const hasSelection = table.table?.getSelectedRows().length > 0;
    duplicateBtn.disabled = !hasSelection;
  }
}

/**
 * Update move button states based on current position
 * @param {Object} table - LithiumTable instance
 */
export function updateMoveButtonState(table) {
  if (!table.table) return;
  const nav = table.navigatorContainer;
  if (!nav) return;

  if (typeof table.table.getRows !== 'function') return;

  const { rows, selectedIndex } = getRowsAndIndex(table);
  const rowCount = rows.length;

  const atFirst = selectedIndex <= 0;
  const atLast = selectedIndex < 0 || selectedIndex >= rowCount - 1;

  const pageSize = getPageSize(table);
  const canGoPrevPage = selectedIndex > 0 && selectedIndex >= pageSize;
  const canGoNextPage = selectedIndex >= 0 && selectedIndex < rowCount - 1 - pageSize;

  const firstBtn = nav.querySelector(`#${table.cssPrefix}-nav-first`);
  const prevPageBtn = nav.querySelector(`#${table.cssPrefix}-nav-pgup`);
  const prevRecBtn = nav.querySelector(`#${table.cssPrefix}-nav-prev`);
  const nextRecBtn = nav.querySelector(`#${table.cssPrefix}-nav-next`);
  const nextPageBtn = nav.querySelector(`#${table.cssPrefix}-nav-pgdn`);
  const lastBtn = nav.querySelector(`#${table.cssPrefix}-nav-last`);

  if (firstBtn) firstBtn.disabled = atFirst;
  if (prevPageBtn) prevPageBtn.disabled = !canGoPrevPage;
  if (prevRecBtn) prevRecBtn.disabled = atFirst;
  if (nextRecBtn) nextRecBtn.disabled = atLast;
  if (nextPageBtn) nextPageBtn.disabled = !canGoNextPage;
  if (lastBtn) lastBtn.disabled = atLast;
}

/**
 * Get rows and current selected index
 * @param {Object} table - LithiumTable instance
 * @returns {Object} { rows, selectedIndex }
 */
function getRowsAndIndex(table) {
  try {
    const rows = table.table.getRows();

    const selectedRows = table.table.getSelectedRows();
    const selectedRow = selectedRows.length > 0 ? selectedRows[0] : null;
    const selectedIndex = selectedRow ? rows.findIndex((r) => r.getData() === selectedRow.getData()) : -1;

    return { rows, selectedIndex };
  } catch {
    return { rows: [], selectedIndex: -1 };
  }
}

/**
 * Get page size for pagination calculations
 * @param {Object} table - LithiumTable instance
 * @returns {number} Page size
 */
function getPageSize(table) {
  const options = table.table?.options || {};
  return options.paginationSize || options.paginationSize || 25;
}
