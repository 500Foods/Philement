// Tabulator.js Initialization Module
// Handles Tabulator table initialization and configuration

class TabulatorInit {
  constructor() {
    this.tables = new Map(); // Store table instances
    this.defaultOptions = {
      layout: "fitColumns",
      responsiveLayout: "hide",
      tooltips: true,
      movableColumns: true,
      resizableRows: true,
      pagination: "local",
      paginationSize: 10,
      paginationSizeSelector: [5, 10, 20, 50, 100],
      paginationButtonCount: 3,
      height: "311px",
      placeholder: "No Data Available",
      clipboard: true,
      clipboardCopySelector: "active",
      clipboardPasteParser: "table",
      clipboardCopyStyled: true,
      columnHeaderVertAlign: "bottom",
      columnCalcs: "both",
      autoColumns: false,
      progressiveLoad: "scroll",
      progressiveLoadScrollMargin: 400,
      ajaxLoaderLoading: "<div class='loader'><i class='fas fa-spinner fa-spin'></i> Loading...</div>",
      ajaxError: (xhr) => {
        console.error("Tabulator AJAX Error:", xhr);
        return "Error loading data";
      }
    };
  }

  /**
   * Initialize a basic Tabulator table
   * @param {string} elementId - The DOM element ID to attach the table to
   * @param {Array} columns - Column definitions
   * @param {Array} data - Initial data
   * @param {Object} options - Custom options to override defaults
   * @returns {Tabulator} Tabulator instance
   */
  initTable(elementId, columns, data = [], options = {}) {
    try {
      const element = document.getElementById(elementId);
      if (!element) {
        throw new Error(`Element with ID ${elementId} not found`);
      }

      // Merge default options with custom options
      const mergedOptions = { ...this.defaultOptions, ...options, columns, data };

      // Initialize Tabulator
      const table = new Tabulator(element, mergedOptions);

      // Store reference
      this.tables.set(elementId, table);

      console.log(`Tabulator table initialized on ${elementId}`);
      return table;
    } catch (error) {
      console.error(`Failed to initialize Tabulator table on ${elementId}:`, error);
      throw error;
    }
  }

  /**
   * Initialize a table with navigation controls
   * @param {string} elementId - The DOM element ID to attach the table to
   * @param {string} navContainerId - Container for navigation controls
   * @param {Array} columns - Column definitions
   * @param {Array} data - Initial data
   * @param {Object} options - Custom options
   * @returns {Object} Table and navigation controls
   */
  initTableWithNavigation(elementId, navContainerId, columns, data = [], options = {}) {
    // Initialize the table
    const table = this.initTable(elementId, columns, data, options);

    // Create navigation controls
    const navContainer = document.getElementById(navContainerId);
    if (navContainer) {
      navContainer.innerHTML = `
        <div class="tabulator-nav d-flex justify-content-between align-items-center mb-2">
          <div class="nav-left">
            <button class="btn btn-sm btn-outline-primary mr-2" id="${elementId}-refresh">
              <i class="fas fa-sync-alt"></i> Refresh
            </button>
            <button class="btn btn-sm btn-outline-secondary mr-2" id="${elementId}-export-csv">
              <i class="fas fa-file-csv"></i> Export CSV
            </button>
            <button class="btn btn-sm btn-outline-secondary" id="${elementId}-export-json">
              <i class="fas fa-file-code"></i> Export JSON
            </button>
          </div>
          <div class="nav-right">
            <div class="input-group input-group-sm" style="width: 200px;">
              <input type="text" class="form-control" placeholder="Search..." id="${elementId}-search">
              <button class="btn btn-outline-secondary" type="button" id="${elementId}-search-btn">
                <i class="fas fa-search"></i>
              </button>
            </div>
          </div>
        </div>
      `;

      // Set up event handlers
      document.getElementById(`${elementId}-refresh`).addEventListener('click', () => {
        table.setData(data);
      });

      document.getElementById(`${elementId}-export-csv`).addEventListener('click', () => {
        table.download('csv', `${elementId}-data.csv`);
      });

      document.getElementById(`${elementId}-export-json`).addEventListener('click', () => {
        table.download('json', `${elementId}-data.json`);
      });

      document.getElementById(`${elementId}-search-btn`).addEventListener('click', () => {
        const searchTerm = document.getElementById(`${elementId}-search`).value;
        table.setFilter('name', 'like', searchTerm);
      });

      // Add keyboard support for search
      document.getElementById(`${elementId}-search`).addEventListener('keyup', (e) => {
        if (e.key === 'Enter') {
          const searchTerm = e.target.value;
          table.setFilter('name', 'like', searchTerm);
        }
      });
    }

    return { table, navContainer };
  }

  /**
   * Get a table instance by element ID
   * @param {string} elementId - The DOM element ID
   * @returns {Tabulator|null} Tabulator instance or null if not found
   */
  getTable(elementId) {
    return this.tables.get(elementId) || null;
  }

  /**
   * Destroy a table instance
   * @param {string} elementId - The DOM element ID
   */
  destroyTable(elementId) {
    const table = this.getTable(elementId);
    if (table) {
      table.destroy();
      this.tables.delete(elementId);
    }
  }

  /**
   * Destroy all table instances
   */
  destroyAllTables() {
    this.tables.forEach((table, elementId) => {
      table.destroy();
    });
    this.tables.clear();
  }

  /**
   * Set default options for all future tables
   * @param {Object} options - Options to set as defaults
   */
  setDefaultOptions(options) {
    this.defaultOptions = { ...this.defaultOptions, ...options };
  }

  /**
   * Get current default options
   * @returns {Object} Current default options
   */
  getDefaultOptions() {
    return { ...this.defaultOptions };
  }

  /**
   * Configure column formatting and validation
   * @param {Array} columns - Column definitions to enhance
   * @returns {Array} Enhanced column definitions
   */
  enhanceColumns(columns) {
    return columns.map(column => {
      const enhancedColumn = { ...column };

      // Add default formatter if not specified
      if (!enhancedColumn.formatter && !enhancedColumn.formatterParams) {
        switch (typeof column.field) {
          case 'number':
            enhancedColumn.formatter = "money";
            enhancedColumn.formatterParams = { decimal: ".", thousand: ",", symbol: "$" };
            break;
          case 'boolean':
            enhancedColumn.formatter = "tickCross";
            break;
          case 'date':
            enhancedColumn.formatter = "datetime";
            enhancedColumn.formatterParams = { inputFormat: "YYYY-MM-DD", outputFormat: "MMM D, YYYY" };
            break;
        }
      }

      // Add default validation if not specified
      if (!enhancedColumn.validator && column.field) {
        if (column.field.includes('email')) {
          enhancedColumn.validator = "email";
        } else if (column.field.includes('url') || column.field.includes('link')) {
          enhancedColumn.validator = "url";
        }
      }

      return enhancedColumn;
    });
  }

  /**
   * Apply themes to tables
   * @param {string} elementId - The DOM element ID
   * @param {string} theme - Theme name (e.g., 'tabulator_simple', 'tabulator_modern')
   */
  applyTheme(elementId, theme) {
    const table = this.getTable(elementId);
    if (table) {
      table.setTheme(theme);
    }
  }

  /**
   * Configure localization
   * @param {Object} locale - Localization settings
   */
  setLocale(locale) {
    Tabulator.prototype.setLocale(locale);
  }
}

// Export singleton instance
export const tabulatorInit = new TabulatorInit();