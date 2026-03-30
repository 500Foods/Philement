// JSONEditor Initialization Module
// Handles vanilla-jsoneditor initialization and configuration
//
// vanilla-jsoneditor API reference:
//   import { JSONEditor } from 'vanilla-jsoneditor';
//   const editor = new JSONEditor({ target, props: { content, mode, ... } });
//   editor.set({ json }); editor.get(); editor.destroy(); editor.updateProps({});

class JSONEditorInit {
  constructor() {
    this.editors = new Map(); // Store editor instances
    this.defaultProps = {
      mode: 'tree',
      mainMenuBar: true,
      navigationBar: true,
      statusBar: true,
      onChange: () => {},
    };
  }

  /**
   * Initialize a vanilla-jsoneditor instance
   * @param {string} elementId - The DOM element ID to attach the editor to
   * @param {Object} jsonData - Initial JSON data
   * @param {Object} props - Custom props to override defaults
   * @returns {Promise<Object>} vanilla-jsoneditor instance
   */
  async initEditor(elementId, jsonData = {}, props = {}) {
    try {
      const element = document.getElementById(elementId);
      if (!element) {
        throw new Error(`Element with ID ${elementId} not found`);
      }

      // Dynamic import vanilla-jsoneditor
      const { JSONEditor } = await import('vanilla-jsoneditor');

      // Merge default props with custom props
      const mergedProps = {
        ...this.defaultProps,
        ...props,
        content: { json: jsonData },
      };

      // Initialize vanilla-jsoneditor
      const editor = new JSONEditor({
        target: element,
        props: mergedProps,
      });

      // Store reference
      this.editors.set(elementId, editor);

      // console.log(`vanilla-jsoneditor initialized on ${elementId}`);
      return editor;
    } catch (error) {
      console.error(`Failed to initialize vanilla-jsoneditor on ${elementId}:`, error);
      throw error;
    }
  }

  /**
   * Get an editor instance by element ID
   * @param {string} elementId - The DOM element ID
   * @returns {Object|null} vanilla-jsoneditor instance or null if not found
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
    this.editors.forEach((editor) => {
      editor.destroy();
    });
    this.editors.clear();
  }

  /**
   * Set default props for all future editors
   * @param {Object} props - Props to set as defaults
   */
  setDefaultProps(props) {
    this.defaultProps = { ...this.defaultProps, ...props };
  }

  /**
   * Get current default props
   * @returns {Object} Current default props
   */
  getDefaultProps() {
    return { ...this.defaultProps };
  }

  /**
   * Create a vanilla-jsoneditor with navigation controls
   * @param {string} elementId - The DOM element ID for the editor
   * @param {string} navContainerId - Container for navigation controls
   * @param {Object} jsonData - Initial JSON data
   * @param {Object} props - Custom props
   * @returns {Promise<Object>} Editor and navigation controls
   */
  async initEditorWithNavigation(elementId, navContainerId, jsonData = {}, props = {}) {
    // Initialize the editor
    const editor = await this.initEditor(elementId, jsonData, props);

    // Create navigation controls
    const navContainer = document.getElementById(navContainerId);
    if (navContainer) {
      navContainer.innerHTML = `
        <div class="jsoneditor-nav d-flex justify-content-between align-items-center mb-2">
          <div class="nav-left">
            <button class="btn btn-sm btn-outline-primary mr-2" id="${elementId}-refresh">
              <fa fa-sync-alt></fa> Refresh
            </button>
            <button class="btn btn-sm btn-outline-secondary mr-2" id="${elementId}-export">
              <fa fa-file-export></fa> Export
            </button>
            <button class="btn btn-sm btn-outline-secondary mr-2" id="${elementId}-import">
              <fa fa-file-import></fa> Import
            </button>
          </div>
          <div class="nav-right">
            <div class="btn-group btn-group-sm" role="group">
              <button class="btn btn-outline-secondary" id="${elementId}-mode-tree" title="Tree Mode">
                <fa fa-sitemap></fa>
              </button>
              <button class="btn btn-outline-secondary" id="${elementId}-mode-text" title="Text Mode">
                <fa fa-brackets-curly></fa>
              </button>
              <button class="btn btn-outline-secondary" id="${elementId}-mode-table" title="Table Mode">
                <fa fa-table></fa>
              </button>
            </div>
          </div>
        </div>
      `;

      // Set up event handlers
      document.getElementById(`${elementId}-refresh`).addEventListener('click', () => {
        editor.set({ json: jsonData });
      });

      document.getElementById(`${elementId}-export`).addEventListener('click', () => {
        const content = editor.get();
        const data = content.json ?? JSON.parse(content.text);
        const dataStr = JSON.stringify(data, null, 2);
        const dataUri = 'data:application/json;charset=utf-8,' + encodeURIComponent(dataStr);
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
                const importedData = JSON.parse(event.target.result);
                editor.set({ json: importedData });
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

      // Mode switch buttons (vanilla-jsoneditor modes: tree, text, table)
      document.getElementById(`${elementId}-mode-tree`).addEventListener('click', () => {
        editor.updateProps({ mode: 'tree' });
      });

      document.getElementById(`${elementId}-mode-text`).addEventListener('click', () => {
        editor.updateProps({ mode: 'text' });
      });

      document.getElementById(`${elementId}-mode-table`).addEventListener('click', () => {
        editor.updateProps({ mode: 'table' });
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
    if (!editor) return null;

    const content = editor.get();
    // vanilla-jsoneditor returns { json } or { text }
    if (content.json !== undefined) return content.json;
    if (content.text !== undefined) {
      try {
        return JSON.parse(content.text);
      } catch (e) {
        return null;
      }
    }
    return null;
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
        editor.set({ json: jsonData });
        return true;
      } catch (error) {
        console.error(`Failed to set JSON data to editor ${elementId}:`, error);
        return false;
      }
    }
    return false;
  }

  /**
   * Configure editor themes via CSS class
   * @param {string} elementId - The DOM element ID
   * @param {string} theme - Theme name ('dark' applies .jse-theme-dark)
   */
  applyTheme(elementId, theme) {
    const element = document.getElementById(elementId);
    if (element) {
      // Remove existing theme classes
      element.classList.remove('jse-theme-dark');
      if (theme === 'dark') {
        element.classList.add('jse-theme-dark');
      }
    }
  }

  /**
   * Create a simple JSON viewer (read-only)
   * @param {string} elementId - The DOM element ID
   * @param {Object} jsonData - JSON data to display
   * @param {Object} props - Custom props
   * @returns {Promise<Object>} vanilla-jsoneditor instance
   */
  async initViewer(elementId, jsonData = {}, props = {}) {
    const viewerProps = {
      ...props,
      readOnly: true,
      mainMenuBar: false,
      navigationBar: false,
      statusBar: false,
    };

    return this.initEditor(elementId, jsonData, viewerProps);
  }

  /**
   * Create a compact JSON editor
   * @param {string} elementId - The DOM element ID
   * @param {Object} jsonData - JSON data to edit
   * @param {Object} props - Custom props
   * @returns {Promise<Object>} vanilla-jsoneditor instance
   */
  async initCompactEditor(elementId, jsonData = {}, props = {}) {
    const compactProps = {
      ...props,
      mainMenuBar: false,
      navigationBar: false,
      statusBar: false,
    };

    return this.initEditor(elementId, jsonData, compactProps);
  }
}

// Export singleton instance
export const jsonEditorInit = new JSONEditorInit();
