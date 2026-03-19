/**
 * Queries Manager - Navigation Module
 *
 * Handles data loading, navigation, and CRUD operations.
 */

import { authQuery } from '../../shared/conduit.js';
import { toast } from '../../shared/toast.js';
import { log, Subsystems, Status } from '../../core/log.js';
import { getJsonTreeData } from '../../components/json-tree-component.js';

// Constants
const SELECTED_ROW_KEY = 'lithium_queries_selected_id';

/**
 * NavigationManager - Manages data loading and navigation
 */
export class NavigationManager {
  constructor(manager) {
    this.manager = manager;
    this._tableLoaderElement = null;
  }

  /**
   * Load queries from the server
   */
  async loadQueries(searchTerm = '') {
    const mgr = this.manager;
    const previouslySelectedId = this._getSelectedQueryId();
    const listRef = mgr.queryRefs?.queryRef ?? 25;
    const searchRef = mgr.queryRefs?.searchQueryRef ?? 32;
    const queryRef = searchTerm ? searchRef : listRef;

    log(Subsystems.CONDUIT, Status.INFO, `[QueriesManager] Loading queries (queryRef: ${queryRef}${searchTerm ? `, search: "${searchTerm}"` : ''})`);

    this._showTableLoading();

    try {
      const rows = searchTerm
        ? await authQuery(mgr.app.api, searchRef, { STRING: { SEARCH: searchTerm.toUpperCase() } })
        : await authQuery(mgr.app.api, listRef);

      if (!mgr.table) return;

      mgr.table.blockRedraw?.();
      try {
        mgr.table.setData(rows);
        this._discoverColumns(rows);
      } finally {
        mgr.table.restoreRedraw?.();
      }

      requestAnimationFrame(() => {
        this._autoSelectRow(previouslySelectedId);
        this._updateMoveButtonState();
      });
    } catch (error) {
      toast.error('Query Load Failed', { serverError: error.serverError, subsystem: 'Conduit', duration: 8000 });
    } finally {
      this._hideTableLoading();
    }
  }

  /**
   * Load query details for a selected row
   */
  loadQueryDetails(queryData) {
    const queryId = queryData?.query_id;
    if (queryId == null || String(this.manager._loadedDetailRowId) === String(queryId)) return;

    this.manager.currentQuery = queryData;
    this.fetchQueryDetails(queryId);
  }

  /**
   * Fetch detailed query information from the server
   */
  async fetchQueryDetails(queryId) {
    const mgr = this.manager;
    const detailRef = mgr.queryRefs?.detailQueryRef ?? 27;
    try {
      const queryDetails = await authQuery(mgr.app.api, detailRef, { INTEGER: { QUERYID: queryId } });
      if (queryDetails?.length) {
        const fullQuery = queryDetails[0];
        mgr.currentQuery = fullQuery;
        mgr._pendingSqlContent = fullQuery.code || fullQuery.query_text || fullQuery.sql || '';
        mgr._pendingSummaryContent = fullQuery.summary || fullQuery.markdown || '';
        mgr._pendingCollectionContent = fullQuery.collection || fullQuery.json || {};

        if (!mgr.editModeManager.isEditing()) {
          mgr.dirtyTracker.captureOriginalData(fullQuery);
        }

        // Update SQL editor if it exists
        if (mgr.sqlEditor) {
          mgr.sqlEditor.dispatch({ changes: { from: 0, to: mgr.sqlEditor.state.doc.length, insert: mgr._pendingSqlContent } });
        }
        
        // Update summary editor if it exists
        if (mgr.summaryEditor) {
          mgr.summaryEditor.dispatch({ changes: { from: 0, to: mgr.summaryEditor.state.doc.length, insert: mgr._pendingSummaryContent } });
        }
        
        // Update collection editor if it exists (recreate to ensure clean state)
        if (mgr.collectionEditor) {
          const { setJsonTreeData } = await import('../../components/json-tree-component.js');
          setJsonTreeData(mgr.elements.collectionEditorContainer, mgr._pendingCollectionContent);
        }

        mgr._loadedDetailRowId = queryId;
      }
    } catch (error) {
      toast.error('Query Details Failed', { serverError: error.serverError, subsystem: 'Conduit', duration: 8000 });
    }
  }

  /**
   * Get the ID of the currently selected query
   */
  _getSelectedQueryId() {
    if (!this.manager.table) return null;
    const selected = this.manager.table.getSelectedRows();
    return selected.length > 0 ? selected[0].getData()[this.manager.primaryKeyField] ?? null : null;
  }

  /**
   * Auto-select a row by ID
   */
  _autoSelectRow(previousId) {
    if (!this.manager.table) return;
    const rows = this.manager.table.getRows('active');
    if (rows.length === 0) return;

    let targetId = previousId;
    if (targetId == null) {
      try { targetId = localStorage.getItem(SELECTED_ROW_KEY); } catch {}
    }

    if (targetId != null) {
      for (const row of rows) {
        if (String(row.getData()[this.manager.primaryKeyField]) === String(targetId)) {
          row.select();
          row.scrollTo();
          return;
        }
      }
    }
    rows[0].select();
    rows[0].scrollTo();
  }

  /**
   * Discover columns from data and add any missing ones
   */
  _discoverColumns(rows) {
    if (!this.manager.table || !rows || rows.length === 0) return;

    const allKeys = new Set();
    rows.forEach(row => Object.keys(row).forEach(key => allKeys.add(key)));

    const existingFields = new Set(
      this.manager.table.getColumns().map(col => col.getField()).filter(Boolean)
    );

    for (const key of allKeys) {
      if (existingFields.has(key) || key === '_selector') continue;

      const title = key.replace(/_/g, ' ').replace(/\b\w/g, c => c.toUpperCase());

      this.manager.table.addColumn({
        title,
        field: key,
        resizable: true,
        headerSort: true,
        headerFilter: this.manager._createFilterEditor.bind(this.manager),
        headerFilterFunc: "like",
        visible: false,
      });
    }
  }

  // ============ NAVIGATOR HANDLERS ============

  /**
   * Handle refresh button click
   */
  async handleNavRefresh() {
    const mgr = this.manager;
    if (mgr.editModeManager.isEditing() && mgr.dirtyTracker.isAnyDirty()) {
      await mgr.handleNavSave();
    } else if (mgr.editModeManager.isEditing()) {
      await mgr.editModeManager.exitEditMode('cancel');
    }
    this.loadQueries();
  }

  /**
   * Handle add button click
   */
  async handleNavAdd() {
    if (!this.manager.table) return;
    const newRow = await this.manager.table.addRow({}, true);
    if (newRow) {
      this.manager.table.deselectRow();
      newRow.select();
      newRow.scrollTo();
      await this.manager.editModeManager.enterEditMode(newRow);
    }
  }

  /**
   * Handle duplicate button click
   */
  async handleNavDuplicate() {
    if (!this.manager.table) return;
    const selected = this.manager.table.getSelectedRows();
    if (selected.length === 0) {
      toast.info('No Row Selected', { description: 'Please select a row to duplicate', duration: 3000 });
      return;
    }

    const originalData = selected[0].getData();
    const duplicateData = { ...originalData };
    delete duplicateData[this.manager.primaryKeyField];
    if (duplicateData.name) duplicateData.name = `${duplicateData.name} (Copy)`;

    const newRow = await this.manager.table.addRow(duplicateData, true);
    if (newRow) {
      this.manager.table.deselectRow();
      newRow.select();
      newRow.scrollTo();
      await this.manager.editModeManager.enterEditMode(newRow);
    }
  }

  /**
   * Handle edit button click
   */
  async handleNavEdit() {
    await this.manager.editModeManager.toggleEditMode();
  }

  /**
   * Handle save button click
   */
  async handleNavSave() {
    const mgr = this.manager;
    if (!mgr.table || !mgr.dirtyTracker.isAnyDirty()) {
      toast.info('No Changes', { description: 'Nothing to save', duration: 3000 });
      return;
    }

    const selected = mgr.table.getSelectedRows();
    if (selected.length === 0) {
      toast.info('No Row Selected', { description: 'Please select a row to save', duration: 3000 });
      return;
    }

    const row = selected[0];
    const rowData = row.getData();
    const pkValue = rowData[mgr.primaryKeyField];
    const isInsert = pkValue == null || pkValue === '' || pkValue === 0;
    const queryRef = isInsert ? (mgr.queryRefs?.insertQueryRef ?? null) : (mgr.queryRefs?.updateQueryRef ?? 28);

    if (!queryRef) {
      await mgr.editModeManager.exitEditMode('save');
      return;
    }

    try {
      const sqlContent = mgr.sqlEditor?.state.doc.toString() || '';
      const summaryContent = mgr.summaryEditor?.state.doc.toString() || '';
      const collectionContent = getJsonTreeData(mgr.elements.collectionEditorContainer);
      const collectionString = collectionContent ? JSON.stringify(collectionContent) : '{}';

      const params = {
        INTEGER: {
          QUERYID: pkValue,
          QUERYTYPE: rowData.query_type_a28 ?? 1,
          QUERYDIALECT: rowData.query_dialect_a30 ?? 1,
          QUERYSTATUS: rowData.query_status_a27 ?? 1,
          USERID: mgr.app?.user?.id ?? 0,
          QUERYREF: parseInt(rowData.query_ref, 10) || 0,
        },
        STRING: {
          QUERYCODE: sqlContent || rowData.code || '',
          QUERYNAME: rowData.name || '',
          QUERYSUMMARY: summaryContent || rowData.summary || '',
          QUERYJSON: collectionString,
        },
      };

      const result = await authQuery(mgr.app.api, queryRef, params);
      if (isInsert && result?.[0]?.[mgr.primaryKeyField] != null) {
        row.update({ [mgr.primaryKeyField]: result[0][mgr.primaryKeyField] });
      }

      mgr.dirtyTracker.markAllClean();
      await mgr.editModeManager.exitEditMode('save');
      toast.success('Changes Saved', { description: `Query ${isInsert ? 'inserted' : 'updated'} successfully`, duration: 3000 });
    } catch (error) {
      toast.error('Save Failed', { serverError: error.serverError, subsystem: 'Conduit', duration: 8000 });
    }
  }

  /**
   * Handle cancel button click
   */
  async handleNavCancel() {
    if (!this.manager.dirtyTracker.isAnyDirty()) {
      await this.manager.editModeManager.exitEditMode('cancel');
      return;
    }

    await this.manager.dirtyTracker.revertAllChanges();
    await this.manager.editModeManager.exitEditMode('cancel');
    toast.info('Changes Cancelled', { description: 'All changes have been reverted', duration: 3000 });
  }

  /**
   * Handle delete button click
   */
  async handleNavDelete() {
    if (!this.manager.table) return;
    const selected = this.manager.table.getSelectedRows();
    if (selected.length === 0) {
      toast.info('No Row Selected', { description: 'Please select a row to delete', duration: 3000 });
      return;
    }

    const row = selected[0];
    const pkValue = row.getData()[this.manager.primaryKeyField];
    const rowName = row.getData()?.name || '';

    if (!window.confirm(rowName ? `Delete "${rowName}"?\n\nThis action cannot be undone.` : 'Delete this row?\n\nThis action cannot be undone.')) return;

    const deleteRef = this.manager.queryRefs?.deleteQueryRef;
    if (deleteRef && pkValue != null) {
      try {
        await authQuery(this.manager.app.api, deleteRef, { INTEGER: { [this.manager.primaryKeyField.toUpperCase()]: pkValue } });
      } catch (error) {
        toast.error('Delete Failed', { serverError: error.serverError, subsystem: 'Conduit', duration: 8000 });
        return;
      }
    }

    const allRows = this.manager.table.getRows('active');
    const currentIndex = allRows.findIndex(r => r === row);
    const nextRow = allRows[currentIndex + 1] || allRows[currentIndex - 1] || null;

    row.delete();
    if (nextRow) { nextRow.select(); nextRow.scrollTo(); }
    await this.manager.editModeManager.exitEditMode('cancel');
    toast.success('Row Deleted', { description: rowName ? `"${rowName}" deleted` : 'Row deleted', duration: 3000 });
  }

  // ============ NAVIGATION ============

  /**
   * Navigate to first record
   */
  async navigateFirst() {
    if (await this._checkSaveBeforeNav()) this._selectRowByIndex(0);
  }

  /**
   * Navigate to last record
   */
  async navigateLast() {
    const rows = this.manager.table?.getRows('active') || [];
    if (rows.length && await this._checkSaveBeforeNav()) {
      this._selectRowByIndex(rows.length - 1);
    }
  }

  /**
   * Navigate to previous record
   */
  async navigatePrevRec() {
    if (await this._checkSaveBeforeNav()) {
      const { selectedIndex } = this._getVisibleRowsAndIndex();
      if (selectedIndex > 0) this._selectRowByIndex(selectedIndex - 1);
    }
  }

  /**
   * Navigate to next record
   */
  async navigateNextRec() {
    if (await this._checkSaveBeforeNav()) {
      const { rows, selectedIndex } = this._getVisibleRowsAndIndex();
      if (selectedIndex < rows.length - 1) this._selectRowByIndex(selectedIndex + 1);
    }
  }

  /**
   * Navigate to previous page
   */
  async navigatePrevPage() {
    if (await this._checkSaveBeforeNav()) {
      const { selectedIndex } = this._getVisibleRowsAndIndex();
      this._selectRowByIndex(Math.max(0, selectedIndex - 10));
    }
  }

  /**
   * Navigate to next page
   */
  async navigateNextPage() {
    if (await this._checkSaveBeforeNav()) {
      const { rows, selectedIndex } = this._getVisibleRowsAndIndex();
      this._selectRowByIndex(Math.min(rows.length - 1, selectedIndex + 10));
    }
  }

  /**
   * Check if save is needed before navigation
   */
  async _checkSaveBeforeNav() {
    const mgr = this.manager;
    if (mgr.editModeManager.isEditing() && mgr.dirtyTracker.isAnyDirty()) {
      const saveSucceeded = await this._autoSaveBeforeRowChange();
      if (!saveSucceeded) return false;
      await mgr.editModeManager.exitEditMode('save');
    } else if (mgr.editModeManager.isEditing()) {
      await mgr.editModeManager.exitEditMode('cancel');
    }
    return true;
  }

  /**
   * Auto-save before row change
   */
  async _autoSaveBeforeRowChange() {
    const mgr = this.manager;
    try {
      log(Subsystems.MANAGER, Status.INFO, `[QueriesManager] Auto-saving changes before switching rows (row ${mgr.editModeManager.getEditingRowId()})`);

      const selected = mgr.table?.getSelectedRows();
      const row = selected?.[0];
      if (!row) return true;

      const rowData = row.getData();
      const pkField = mgr.primaryKeyField || 'query_id';
      const pkValue = rowData[pkField];
      const isInsert = pkValue == null || pkValue === '' || pkValue === 0;
      const queryRef = isInsert ? (mgr.queryRefs?.insertQueryRef ?? null) : (mgr.queryRefs?.updateQueryRef ?? 28);

      if (!queryRef) return true;

      const sqlContent = mgr.dirtyTracker.getCurrentSqlContent();
      const summaryContent = mgr.dirtyTracker.getCurrentSummaryContent();
      const collectionContent = getJsonTreeData(mgr.elements.collectionEditorContainer);
      const collectionString = collectionContent ? JSON.stringify(collectionContent) : '{}';

      const params = {
        INTEGER: {
          QUERYID: pkValue,
          QUERYTYPE: rowData.query_type_a28 ?? 1,
          QUERYDIALECT: rowData.query_dialect_a30 ?? 1,
          QUERYSTATUS: rowData.query_status_a27 ?? 1,
          USERID: mgr.app?.user?.id ?? 0,
          QUERYREF: parseInt(rowData.query_ref, 10) || 0,
        },
        STRING: {
          QUERYCODE: sqlContent || rowData.code || '',
          QUERYNAME: rowData.name || '',
          QUERYSUMMARY: summaryContent || rowData.summary || '',
          QUERYJSON: collectionString,
        },
      };

      await authQuery(mgr.app.api, queryRef, params);
      mgr.dirtyTracker.markAllClean();

      log(Subsystems.MANAGER, Status.INFO, `[QueriesManager] Auto-saved successfully (row ${pkValue ?? 'new'})`);
      toast.success('Changes Saved', { description: `Query updated before switching rows`, duration: 2000 });

      return true;
    } catch (error) {
      log(Subsystems.MANAGER, Status.ERROR, `[QueriesManager] Auto-save failed: ${error.message}`);
      toast.error('Save Failed', { serverError: error.serverError, subsystem: 'Conduit', duration: 6000 });
      return false;
    }
  }

  /**
   * Get visible rows and current index
   */
  _getVisibleRowsAndIndex() {
    if (!this.manager.table) return { rows: [], selectedIndex: -1 };
    const rows = this.manager.table.getRows('active');
    const selected = this.manager.table.getSelectedRows();
    let selectedIndex = -1;
    if (selected.length > 0) {
      const selPos = selected[0].getPosition(true);
      selectedIndex = selPos - 1;
    }
    return { rows, selectedIndex };
  }

  /**
   * Select row by index
   */
  _selectRowByIndex(index) {
    if (!this.manager.table) return;
    const rows = this.manager.table.getRows('active');
    if (index < 0 || index >= rows.length) return;
    this.manager.table.deselectRow();
    rows[index].select();
    rows[index].scrollTo();
  }

  /**
   * Select a data row
   */
  _selectDataRow(row) {
    if (!this.manager.table || !row || this._isCalcRow(row)) return false;
    const currentRow = this.manager.table.getSelectedRows()[0];
    if (currentRow === row) return false;
    this.manager.table.deselectRow();
    row.select();
    return true;
  }

  /**
   * Check if row is a calculation row
   */
  _isCalcRow(row) {
    if (!row) return false;
    const rowEl = row.getElement?.();
    return !!rowEl?.closest?.('.tabulator-calcs, .tabulator-calcs-holder, .tabulator-calcs-bottom');
  }

  /**
   * Update move button state based on selection
   */
  _updateMoveButtonState() {
    if (!this.manager.table) return;
    const nav = this.manager.elements.navigatorContainer;
    if (!nav || typeof this.manager.table.getRows !== 'function') return;

    const { rows, selectedIndex } = this._getVisibleRowsAndIndex();
    const rowCount = rows.length;

    const atFirst = selectedIndex <= 0;
    const atLast = selectedIndex < 0 || selectedIndex >= rowCount - 1;

    const firstBtn = nav.querySelector('#queries-nav-first');
    const prevRecBtn = nav.querySelector('#queries-nav-prev');
    const nextRecBtn = nav.querySelector('#queries-nav-next');
    const lastBtn = nav.querySelector('#queries-nav-last');

    if (firstBtn) firstBtn.disabled = atFirst;
    if (prevRecBtn) prevRecBtn.disabled = atFirst;
    if (nextRecBtn) nextRecBtn.disabled = atLast;
    if (lastBtn) lastBtn.disabled = atLast;
  }

  /**
   * Show table loading indicator
   */
  _showTableLoading() {
    if (!this.manager.elements.tableContainer) return;
    this._hideTableLoading();
    const loader = document.createElement('div');
    loader.className = 'queries-table-loader';
    loader.innerHTML = '<div class="spinner-fancy spinner-fancy-md"></div>';
    this.manager.elements.tableContainer.appendChild(loader);
    this._tableLoaderElement = loader;
  }

  /**
   * Hide table loading indicator
   */
  _hideTableLoading() {
    if (this._tableLoaderElement) {
      this._tableLoaderElement.remove();
      this._tableLoaderElement = null;
    }
  }
}

/**
 * Create a navigation manager for the manager
 */
export function createNavigationManager(manager) {
  return new NavigationManager(manager);
}
