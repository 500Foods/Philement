/**
 * Record lock for Manager 24.
 * The page stays read-only until the list row is in edit mode.
 * Zoom, export, and the ruler corner stay available.
 */

export const reportLockMethods = {
  /**
   * @param {boolean} editable
   */
  setRecordEditing(editable) {
    this._recordEditing = Boolean(editable);
    this._applyRecordLock();
  },

  _applyRecordLock() {
    const editing = Boolean(this._recordEditing);
    const root = this.container && this.container.querySelector
      ? this.container.querySelector('.reports-manager')
      : null;
    if (root) root.dataset.editing = editing ? 'true' : 'false';
    if (typeof this._paintPanel === 'function') this._paintPanel();
    const elements = this.elements || {};
    for (const key of ['metaName', 'metaTitle', 'metaSummary']) {
      if (elements[key]) elements[key].disabled = !editing;
    }
    if (elements.importBtn) elements.importBtn.disabled = !editing;
    if (this._json && typeof this._json.setEditable === 'function') {
      this._json.setEditable(editing);
    }
    if (typeof this._syncItems === 'function') this._syncItems();
    if (typeof this._syncHistory === 'function') this._syncHistory();
    if (typeof this.syncRevisionActions === 'function') this.syncRevisionActions();
  },
};
