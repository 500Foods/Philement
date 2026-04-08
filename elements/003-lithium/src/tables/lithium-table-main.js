/**
 * LithiumTable Main Module
 *
 * Combines all LithiumTable modules into a single reusable class.
 * This is the main export that managers should use.
 *
 * Usage:
 *   import { LithiumTable } from './core/lithium-table-main.js';
 *
 *   const table = new LithiumTable({
 *     container: document.getElementById('table-container'),
 *     navigatorContainer: document.getElementById('nav-container'),
 *     tablePath: 'queries/query-manager',
 *     queryRef: 25,
 *     app: this.app,
 *     cssPrefix: 'queries',
 *     onRowSelected: (rowData) => { ... },
 *   });
 *   await table.init();
 *
 * @module core/lithium-table-main
 */

import { LithiumTableBase } from './lithium-table-base.js';
import { LithiumTableOpsMixin } from './lithium-table-ops.js';
import { LithiumTableUIMixin } from './lithium-table-ui.js';
import { LithiumColumnManager } from './lithium-column-manager.js';
import './lithium-table.css';
import './lithium-column-manager.css';

// ── Combine Mixins into Final Class ─────────────────────────────────────────

/**
 * LithiumTable - Reusable Tabulator + Navigator Component
 *
 * A self-contained table component combining:
 * - Tabulator data grid with JSON-driven column configuration
 * - Navigator control bar (Control, Move, Manage, Search blocks)
 * - Edit mode with dirty state tracking
 * - Column chooser popup
 * - Template system for saving/loading column configurations
 * - Header filters with clear buttons
 */
export class LithiumTable extends LithiumTableBase {
  constructor(options) {
    super(options);
  }
}

// Apply mixins to the class
Object.assign(LithiumTable.prototype, LithiumTableOpsMixin, LithiumTableUIMixin);

// Re-export LithiumColumnManager for advanced usage
export { LithiumColumnManager };

// Default export
export default LithiumTable;
