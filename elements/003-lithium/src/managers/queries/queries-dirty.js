/**
 * Queries Manager - Dirty State Tracking Module
 *
 * Handles change detection and dirty state management for all editors.
 */

import { log, Subsystems, Status } from '../../core/log.js';

/**
 * DirtyStateTracker - Manages dirty state for all data sources
 */
export class DirtyStateTracker {
  constructor(manager) {
    this.manager = manager;
    this._isDirty = {
      table: false,
      sql: false,
      summary: false,
      collection: false,
    };
    this._originalRowData = null;
    this._originalSqlContent = null;
    this._originalSummaryContent = null;
    this._originalCollectionContent = null;
  }

  /**
   * Check if any data source is dirty
   */
  isAnyDirty() {
    return this._isDirty.table || this._isDirty.sql || this._isDirty.summary || this._isDirty.collection;
  }

  /**
   * Set dirty state for a specific type.
   * 
   * This is called when editors fire change events (which are just triggers
   * for us to check - we compare against our snapshot, not trust their state).
   * 
   * We also sync the table's isDirty flag so that handleSave() works correctly
   * for CodeMirror/SunEditor changes that don't set the table's isDirty flag.
   */
  setDirty(type, isDirty) {
    const wasDirty = this.isAnyDirty();
    this._isDirty[type] = isDirty;
    const nowDirty = this.isAnyDirty();
    
    // Update footer buttons when dirty state changes
    if (wasDirty !== nowDirty) {
      this.manager.updateFooterSaveCancelState(true, nowDirty);
      
      // Sync the table's isDirty so handleSave() can proceed
      if (this.manager.queryTable) {
        this.manager.queryTable.isDirty = nowDirty;
      }
      
      log(Subsystems.MANAGER, Status.DEBUG, 
        `[DirtyTracker] ${type} dirty=${isDirty}, anyDirty=${nowDirty}`);
    }
  }

  /**
   * Mark all data sources as clean
   */
  markAllClean() {
    this._isDirty.table = false;
    this._isDirty.sql = false;
    this._isDirty.summary = false;
    this._isDirty.collection = false;
    this.manager.updateFooterSaveCancelState();
  }

  /**
   * Capture original data for change tracking.
   * 
   * IMPORTANT: We capture content directly from the editors, not from the row data.
   * The row data might have truncated/different content than what's in the editors.
   * 
   * Priority order:
   * 1. Editor content (if editor exists and has content)
   * 2. Pending content (full DB content loaded for lazy editors)
   * 3. Row data (may be truncated)
   * 
   * This ensures our snapshot is in "database format" - exactly what we'd save.
   * Change events just trigger us to regenerate and compare - we don't trust them.
   */
  captureOriginalData(queryData, editors = {}) {
    if (!queryData) return;
    
    this._originalRowData = { ...queryData };
    
    // Capture SQL: editor > pending content > row data
    this._originalSqlContent = editors.sql?.state?.doc?.toString()
      ?? editors.sqlContent
      ?? queryData.code ?? queryData.query_text ?? queryData.sql ?? '';
    
    // Capture Summary: editor > pending content > row data
    this._originalSummaryContent = editors.summary?.state?.doc?.toString()
      ?? editors.summaryContent
      ?? queryData.summary ?? queryData.markdown ?? '';
    
    // Capture Collection: editor > pending content > row data
    const collectionFromEditor = editors.collection?.state?.doc?.toString();
    const collection = collectionFromEditor
      ?? editors.collectionContent
      ?? queryData.collection ?? queryData.json ?? {};
    this._originalCollectionContent = typeof collection === 'string' ? collection : JSON.stringify(collection);
    
    this.markAllClean();
    
    log(Subsystems.MANAGER, Status.DEBUG, 
      `[DirtyTracker] Snapshot captured - SQL: ${this._originalSqlContent?.length} chars`);
  }

  /**
   * Revert all changes to original values
   */
  revertAllChanges() {
    const mgr = this.manager;

    if (this._isDirty.table && mgr.queryTable?.table && this._originalRowData) {
      const selected = mgr.queryTable.table.getSelectedRows();
      if (selected.length > 0) {
        selected[0].update(this._originalRowData);
      }
    }

    if (this._isDirty.sql && mgr.sqlEditor && this._originalSqlContent != null) {
      mgr.sqlEditor.dispatch({
        changes: { from: 0, to: mgr.sqlEditor.state.doc.length, insert: this._originalSqlContent },
      });
    }

    if (this._isDirty.summary && mgr.summaryEditor && this._originalSummaryContent != null) {
      mgr.summaryEditor.dispatch({
        changes: { from: 0, to: mgr.summaryEditor.state.doc.length, insert: this._originalSummaryContent },
      });
    }

    if (this._isDirty.collection && mgr.collectionEditor && this._originalCollectionContent != null) {
      try {
        const content = typeof this._originalCollectionContent === 'string'
          ? this._originalCollectionContent
          : JSON.stringify(this._originalCollectionContent, null, 2);
        mgr.collectionEditor.dispatch({
          changes: { from: 0, to: mgr.collectionEditor.state.doc.length, insert: content },
        });
      } catch (e) {
        mgr.collectionEditor.dispatch({
          changes: { from: 0, to: mgr.collectionEditor.state.doc.length, insert: '{}' },
        });
      }
    }

    this.markAllClean();
  }

  /**
   * Get current SQL content
   */
  getCurrentSqlContent() {
    return this.manager.sqlEditor?.state.doc.toString() || '';
  }

  /**
   * Get current summary content
   */
  getCurrentSummaryContent() {
    return this.manager.summaryEditor?.state.doc.toString() || '';
  }

  /**
   * Get current collection content as JSON string
   */
  getCurrentCollectionContent() {
    return this.manager.collectionEditor?.state.doc.toString() || '{}';
  }

  /**
   * Check if SQL is dirty
   */
  checkSqlDirty() {
    if (this._originalSqlContent == null) return false;
    return this.getCurrentSqlContent() !== this._originalSqlContent;
  }

  /**
   * Check if summary is dirty
   */
  checkSummaryDirty() {
    if (this._originalSummaryContent == null) return false;
    return this.getCurrentSummaryContent() !== this._originalSummaryContent;
  }

  /**
   * Check if collection is dirty
   */
  checkCollectionDirty() {
    if (this._originalCollectionContent == null) return false;
    return this.getCurrentCollectionContent() !== this._originalCollectionContent;
  }

  /**
   * Refresh all dirty states by comparing editors to snapshots.
   * Called when save button is clicked or other triggers - we always
   * regenerate and compare because we don't trust change events.
   */
  refreshDirtyState() {
    this._isDirty.sql = this.checkSqlDirty();
    this._isDirty.summary = this.checkSummaryDirty();
    this._isDirty.collection = this.checkCollectionDirty();
    
    const nowDirty = this.isAnyDirty();
    this.manager.updateFooterSaveCancelState(true, nowDirty);
    
    // Sync table's isDirty so handleSave() can proceed
    if (this.manager.queryTable) {
      this.manager.queryTable.isDirty = nowDirty;
    }
    
    return nowDirty;
  }

  /**
   * Clear all original data
   */
  clearOriginalData() {
    this._originalRowData = null;
    this._originalSqlContent = null;
    this._originalSummaryContent = null;
    this._originalCollectionContent = null;
  }

  /**
   * Get dirty state object
   */
  getState() {
    return { ...this._isDirty };
  }

  /**
   * Get original data
   */
  getOriginalData() {
    return {
      row: this._originalRowData,
      sql: this._originalSqlContent,
      summary: this._originalSummaryContent,
      collection: this._originalCollectionContent,
    };
  }
}

/**
 * Create a dirty state tracker for the manager
 */
export function createDirtyStateTracker(manager) {
  return new DirtyStateTracker(manager);
}
