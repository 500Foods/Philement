// JSONEditor Initialization Module
// Handles JSONEditor initialization and configuration

class JSONEditorInit {
  constructor() {
    this.editors = new Map(); // Store editor instances
    this.defaultOptions = {
      mode: 'tree',
      modes: ['tree', 'view', 'form', 'code', 'text'],
      search: true,
      history: true,
      navigationBar: true,
      statusBar: true,
      mainMenuBar: true,
      onChange: () => {},
      onError: (error) => {
        console.error('JSONEditor error:', error);
      },
      onModeChange: (newMode, oldMode) => {
        console.log(`JSONEditor mode changed from ${oldMode} to ${newMode}`);
      }
    };
  }

  /**
   * Initialize a JSONEditor instance
   * @param {string} elementId - The DOM element ID to attach the editor to
   * @param {Object} jsonData - Initial JSON data
   * @param {Object} options - Custom options to override defaults
   * @returns {JSONEditor} JSONEditor instance
   */
  initEditor(elementId, jsonData = {}, options = {}) {
    try {
      const element = document.getElementById(elementId);
      if (!element) {
        throw new Error(`Element with ID ${elementId} not found`);
      }

      // Merge default options with custom options
      const mergedOptions = { ...this.defaultOptions, ...options };

      // Initialize JSONEditor
      const editor = new JSONEditor(element, mergedOptions);

      // Set initial data
      editor.set(jsonData);

      // Store reference
      this.editors.set(elementId, editor);

      console.log(`JSONEditor initialized on ${elementId}`);
      return editor;
    } catch (error) {
      console.error(`Failed to initialize JSONEditor on ${elementId}:`, error);
      throw error;
    }
  }

  /**
   * Get an editor instance by element ID
   * @param {string} elementId - The DOM element ID
   * @returns {JSONEditor|null} JSONEditor instance or null if not found
   */
  getEditor(elementId) {
    return this.editors.get(elementId) || null;
  }

  /**
   * Destroy an editor instance
   * @param {string} elementId - The DOM element ID
   */
  destroyEditor(elementId) {
    const editor = this.getEditor(elementId);
    if (editor) {
      editor.destroy();
      this.editors.delete(elementId);
    }
  }

  /**
   * Destroy all editor instances
   */
  destroyAllEditors() {
    this.editors.forEach((editor, elementId) => {
      editor.destroy();
    });
    this.editors.clear();
  }

  /**
   * Set default options for all future editors
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
   * Create a JSONEditor with navigation controls
   * @param {string} elementId - The DOM element ID for the editor
   * @param {string} navContainerId - Container for navigation controls
   * @param {Object} jsonData - Initial JSON data
   * @param {Object} options - Custom options
   * @returns {Object} Editor and navigation controls
   */
  initEditorWithNavigation(elementId, navContainerId, jsonData = {}, options = {}) {
    // Initialize the editor
    const editor = this.initEditor(elementId, jsonData, options);

    // Create navigation controls
    const navContainer = document.getElementById(navContainerId);
    if (navContainer) {
      navContainer.innerHTML = `
        <div class="jsoneditor-nav d-flex justify-content-between align-items-center mb-2">
          <div class="nav-left">
            <button class="btn btn-sm btn-outline-primary mr-2" id="${elementId}-refresh">
              <i class="fas fa-sync-alt"></i> Refresh
            </button>
            <button class="btn btn-sm btn-outline-secondary mr-2" id="${elementId}-export">
              <i class="fas fa-file-export"></i> Export
            </button>
            <button class="btn btn-sm btn-outline-secondary mr-2" id="${elementId}-import">
              <i class="fas fa-file-import"></i> Import
            </button>
          </div>
          <div class="nav-right">
            <div class="btn-group btn-group-sm" role="group">
              <button class="btn btn-outline-secondary" id="${elementId}-mode-tree" title="Tree Mode">
                <i class="fas fa-sitemap"></i>
              </button>
              <button class="btn btn-outline-secondary" id="${elementId}-mode-code" title="Code Mode">
                <i class="fas fa-code"></i>
              </button>
              <button class="btn btn-outline-secondary" id="${elementId}-mode-form" title="Form Mode">
                <i class="fas fa-wpforms"></i>
              </button>
              <button class="btn btn-outline-secondary" id="${elementId}-mode-view" title="View Mode">
                <i class="fas fa-eye"></i>
              </button>
            </div>
          </div>
        </div>
      `;

      // Set up event handlers
      document.getElementById(`${elementId}-refresh`).addEventListener('click', () => {
        editor.set(jsonData);
      });

      document.getElementById(`${elementId}-export`).addEventListener('click', () => {
        const jsonData = editor.get();
        const dataStr = JSON.stringify(jsonData, null, 2);
        const dataUri = 'data:application/json;charset=utf-8,'+ encodeURIComponent(dataStr);
        const exportFileDefaultName = `${elementId}-data.json`;
        const linkElement = document.createElement('a');
        linkElement.setAttribute('href', dataUri);
        linkElement.setAttribute('download', exportFileDefaultName);
        linkElement.click();
      });

      document.getElementById(`${elementId}-import`).addEventListener('click', () => {
        const input = document.createElement('input');
        input.type = 'file';
        input.accept = '.json';
        input.onchange = (e) => {
          const file = e.target.files[0];
          if (file) {
            const reader = new FileReader();
            reader.onload = (event) => {
              try {
                const jsonData = JSON.parse(event.target.result);
                editor.set(jsonData);
              } catch (error) {
                console.error('Error parsing JSON file:', error);
                alert('Invalid JSON file');
              }
            };
            reader.readAsText(file);
          }
        };
        input.click();
      });

      // Mode switch buttons
      document.getElementById(`${elementId}-mode-tree`).addEventListener('click', () => {
        editor.setMode('tree');
      });

      document.getElementById(`${elementId}-mode-code`).addEventListener('click', () => {
        editor.setMode('code');
      });

      document.getElementById(`${elementId}-mode-form`).addEventListener('click', () => {
        editor.setMode('form');
      });

      document.getElementById(`${elementId}-mode-view`).addEventListener('click', () => {
        editor.setMode('view');
      });
    }

    return { editor, navContainer };
  }

  /**
   * Validate JSON data
   * @param {Object} jsonData - JSON data to validate
   * @returns {boolean} True if valid, false otherwise
   */
  validateJSON(jsonData) {
    try {
      JSON.stringify(jsonData);
      return true;
    } catch (error) {
      console.error('Invalid JSON:', error);
      return false;
    }
  }

  /**
   * Get JSON data from an editor
   * @param {string} elementId - The DOM element ID
   * @returns {Object|null} JSON data or null if editor not found
   */
  getJSON(elementId) {
    const editor = this.getEditor(elementId);
    return editor ? editor.get() : null;
  }

  /**
   * Set JSON data to an editor
   * @param {string} elementId - The DOM element ID
   * @param {Object} jsonData - JSON data to set
   * @returns {boolean} True if successful, false otherwise
   */
  setJSON(elementId, jsonData) {
    const editor = this.getEditor(elementId);
    if (editor) {
      try {
        editor.set(jsonData);
        return true;
      } catch (error) {
        console.error(`Failed to set JSON data to editor ${elementId}:`, error);
        return false;
      }
    }
    return false;
  }

  /**
   * Configure editor themes
   * @param {string} elementId - The DOM element ID
   * @param {string} theme - Theme name
   */
  applyTheme(elementId, theme) {
    const editor = this.getEditor(elementId);
    if (editor) {
      // JSONEditor doesn't have built-in themes, but we can apply custom CSS classes
      const container = editor.container;
      container.classList.remove('jsoneditor-theme-default');
      container.classList.add(`jsoneditor-theme-${theme}`);
    }
  }

  /**
   * Create a simple JSON viewer (read-only)
   * @param {string} elementId - The DOM element ID
   * @param {Object} jsonData - JSON data to display
   * @param {Object} options - Custom options
   * @returns {JSONEditor} JSONEditor instance
   */
  initViewer(elementId, jsonData = {}, options = {}) {
    const viewerOptions = {
      ...this.defaultOptions,
      ...options,
      mode: 'view',
      navigationBar: false,
      statusBar: false,
      mainMenuBar: false,
      readOnly: true
    };

    return this.initEditor(elementId, jsonData, viewerOptions);
  }

  /**
   * Create a compact JSON editor
   * @param {string} elementId - The DOM element ID
   * @param {Object} jsonData - JSON data to edit
   * @param {Object} options - Custom options
   * @returns {JSONEditor} JSONEditor instance
   */
  initCompactEditor(elementId, jsonData = {}, options = {}) {
    const compactOptions = {
      ...this.defaultOptions,
      ...options,
      modes: ['code', 'tree'],
      search: false,
      history: false,
      navigationBar: false,
      statusBar: false,
      mainMenuBar: false
    };

    return this.initEditor(elementId, jsonData, compactOptions);
  }
}

// Export singleton instance
export const jsonEditorInit = new JSONEditorInit();