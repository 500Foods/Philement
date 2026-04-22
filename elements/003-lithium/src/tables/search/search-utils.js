/**
 * Search Utilities Module
 *
 * Provides search functionality for LithiumTable.
 *
 * @module tables/search/search-utils
 */

/**
 * Perform a client-side local search on the table's current data.
 * Filters rows by searching across specified fields (or all visible fields if none specified).
 *
 * @param {Object} table - LithiumTable instance
 * @param {string} searchTerm - The search term
 */
export function performLocalSearch(table, searchTerm) {
  if (!table.table) return;

  table._localSearchTerm = searchTerm;

  const term = searchTerm.trim().toLowerCase();

  if (!term) {
    // Clear filter — show all rows
    table.table.clearFilter(true);
    table.updateMoveButtonState?.();
    return;
  }

  // Determine which fields to search
  let fieldsToSearch = table.localSearchFields;
  if (!fieldsToSearch || !Array.isArray(fieldsToSearch) || fieldsToSearch.length === 0) {
    // Default: search all visible columns that have a field defined
    fieldsToSearch = table.table.getColumns()
      .map(col => col.getDefinition().field)
      .filter(field => field && field !== '_selector');
  }

  // Apply a custom filter function
  table.table.setFilter((rowData) => {
    for (const field of fieldsToSearch) {
      const value = rowData[field];
      if (value != null && String(value).toLowerCase().includes(term)) {
        return true;
      }
    }
    return false;
  });

  table.updateMoveButtonState?.();
}