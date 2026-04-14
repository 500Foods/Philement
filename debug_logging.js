/**
 * Debug logging to understand selection/persistence issues
 */

// Add debug logging to key methods to trace the flow

// 1. In loadData, add debug log for previouslySelectedId
console.log('[DEBUG loadData] previouslySelectedId:', previouslySelectedId);

// 2. In autoSelectRow, add debug log
console.log('[DEBUG autoSelectRow] targetId:', targetId, 'rows:', rows.length, 'pkFields:', pkFields);

// 3. In rowSelectionChanged handler, add debug log
console.log('[DEBUG rowSelectionChanged] _inSelectionTransition:', this._inSelectionTransition, 'selectedRows:', this.table.getSelectedRows().length);

// 4. In selectDataRow, add debug log  
console.log('[DEBUG selectDataRow] transitioning:', this._inSelectionTransition);

// 5. Add a method to verify localStorage state
getLocalStorageState() {
  const parentSel = this._loadParentSelection();
  const childSel = this._loadChildSelection(this.selectedLookupId);
  return { parentSel, childSel, selectedLookupId: this.selectedLookupId };
}