/**
 * Queries Manager - Dirty State Tracking Module
 *
 * Handles change detection and dirty state management for all editors.
 */

import { log, Subsystems, Status } from '../../core/log.js';
import { getJsonTreeData } from '../../components/json-tree-component.js';

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
   * Set dirty state for a specific type
   */
  setDirty(type, isDirty) {
    const wasDirty = this.isAnyDirty();
    this._isDirty[type] = isDirty;
    const nowDirty = this.isAnyDirty();
    if (wasDirty !== nowDirty) {
      this.manager.updateFooterSaveCancelState();
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
   * Capture original data for change tracking
   */
  captureOriginalData(queryData) {
    if (!queryData) return;
    this._originalRowData = { ...queryData };
    this._originalSqlContent = queryData.code || queryData.query_text || queryData.sql || '';
    this._originalSummaryContent = queryData.summary || queryData.markdown || '';
    const collection = queryData.collection || queryData.json || {};
    this._originalCollectionContent = typeof collection === 'string' ? collection : JSON.stringify(collection);
    this.markAllClean();
  }

  /**
   * Revert all changes to original values
   */
  async revertAllChanges() {
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

    if (this._isDirty.collection && this._originalCollectionContent != null) {
      try {
        const { setJsonTreeData } = await import('../../components/json-tree-component.js');
        const jsonData = typeof this._originalCollectionContent === 'string'
          ? JSON.parse(this._originalCollectionContent)
          : this._originalCollectionContent;
        setJsonTreeData(mgr.elements.collectionEditorContainer, jsonData);
      } catch (e) {
        const { setJsonTreeData } = await import('../../components/json-tree-component.js');
        setJsonTreeData(mgr.elements.collectionEditorContainer, {});
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
    const data = getJsonTreeData(this.manager.elements.collectionEditorContainer);
    return data ? JSON.stringify(data) : '{}';
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
   * Refresh all dirty states
   */
  refreshDirtyState() {
    this._isDirty.sql = this.checkSqlDirty();
    this._isDirty.summary = this.checkSummaryDirty();
    this._isDirty.collection = this.checkCollectionDirty();
    this.manager.updateFooterSaveCancelState();
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
