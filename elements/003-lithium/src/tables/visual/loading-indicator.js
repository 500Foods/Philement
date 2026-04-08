/**
 * Loading Indicator Module
 *
 * Table loading state visualization.
 *
 * @module tables/visual/loading-indicator
 */

/**
 * Show loading indicator
 * @param {Object} table - LithiumTable instance
 */
export function showLoading(table) {
  if (!table.container) return;

  hideLoading(table);

  const loader = document.createElement('div');
  loader.className = `lithium-table-loader ${table.cssPrefix}-table-loader`;
  loader.innerHTML = '<div class="spinner-fancy spinner-fancy-md"></div>';
  table.container.appendChild(loader);
  table.loaderElement = loader;
}

/**
 * Hide loading indicator
 * @param {Object} table - LithiumTable instance
 */
export function hideLoading(table) {
  if (table.loaderElement) {
    table.loaderElement.remove();
    table.loaderElement = null;
  }
}
