// CodeMirror Initialization Module
// Handles CodeMirror code editor initialization and configuration

class CodeMirrorInit {
  constructor() {
    this.editors = new Map(); // Store editor instances
    this.defaultOptions = {
      lineNumbers: true,
      lineWrapping: true,
      indentWithTabs: false,
      tabSize: 2,
      indentUnit: 2,
      styleActiveLine: true,
      matchBrackets: true,
      autoCloseBrackets: true,
      foldGutter: true,
      gutters: ['CodeMirror-linenumbers', 'CodeMirror-foldgutter'],
      highlightSelectionMatches: { showToken: true, annotateScrollbar: true },
      mode: 'javascript',
      theme: 'dracula',
      fontFamily: '"Vanadium Mono", "Courier New", monospace',
      extraKeys: {
        'Ctrl-Space': 'autocomplete',
        'Alt-F': 'findPersistent',
        'Ctrl-F': 'find',
        'Ctrl-G': 'findNext',
        'Shift-Ctrl-G': 'findPrev',
        'Ctrl-H': 'replace',
        'Shift-Ctrl-H': 'replaceAll',
        'F11': (cm) => cm.setOption('fullScreen', !cm.getOption('fullScreen')),
        'Esc': (cm) => cm.getOption('fullScreen') && cm.setOption('fullScreen', false)
      },
      autocomplete: true,
      hintOptions: {
        completeSingle: false,
        alignWithWord: true
      }
    };
  }

  /**
   * Initialize a CodeMirror editor
   * @param {string} elementId - The DOM element ID to attach the editor to
   * @param {string} content - Initial content
   * @param {Object} options - Custom options to override defaults
   * @returns {CodeMirror.Editor} CodeMirror editor instance
   */
  initEditor(elementId, content = '', options = {}) {
    try {
      const element = document.getElementById(elementId);
      if (!element) {
        throw new Error(`Element with ID ${elementId} not found`);
      }

      // Merge default options with custom options
      const mergedOptions = { ...this.defaultOptions, ...options };

      // Initialize CodeMirror editor
      const editor = CodeMirror(element, mergedOptions);

      // Set initial content
      if (content) {
        editor.setValue(content);
      }

      // Store reference
      this.editors.set(elementId, editor);

      console.log(`CodeMirror editor initialized on ${elementId}`);
      return editor;
    } catch (error) {
      console.error(`Failed to initialize CodeMirror editor on ${elementId}:`, error);
      throw error;
    }
  }

  /**
   * Get an editor instance by element ID
   * @param {string} elementId - The DOM element ID
   * @returns {CodeMirror.Editor|null} CodeMirror editor instance or null if not found
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
      // CodeMirror doesn't have a built-in destroy method, so we remove it from the map
      this.editors.delete(elementId);
      // Clear the element content
      const element = document.getElementById(elementId);
      if (element) {
        element.innerHTML = '';
      }
    }
  }

  /**
   * Destroy all editor instances
   */
  destroyAllEditors() {
    this.editors.forEach((editor, elementId) => {
      const element = document.getElementById(elementId);
      if (element) {
        element.innerHTML = '';
      }
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
   * Create a CodeMirror editor with navigation controls
   * @param {string} elementId - The DOM element ID for the editor
   * @param {string} navContainerId - Container for navigation controls
   * @param {string} content - Initial content
   * @param {Object} options - Custom options
   * @returns {Object} Editor and navigation controls
   */
  initEditorWithNavigation(elementId, navContainerId, content = '', options = {}) {
    // Initialize the editor
    const editor = this.initEditor(elementId, content, options);

    // Create navigation controls
    const navContainer = document.getElementById(navContainerId);
    if (navContainer) {
      navContainer.innerHTML = `
        <div class="codemirror-nav d-flex justify-content-between align-items-center mb-2">
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
              <button class="btn btn-outline-secondary" id="${elementId}-mode-js" title="JavaScript">
                <i class="fab fa-js"></i> JS
              </button>
              <button class="btn btn-outline-secondary" id="${elementId}-mode-html" title="HTML">
                <i class="fab fa-html5"></i> HTML
              </button>
              <button class="btn btn-outline-secondary" id="${elementId}-mode-css" title="CSS">
                <i class="fab fa-css3"></i> CSS
              </button>
              <button class="btn btn-outline-secondary" id="${elementId}-mode-sql" title="SQL">
                <i class="fas fa-database"></i> SQL
              </button>
            </div>
          </div>
        </div>
      `;

      // Set up event handlers
      document.getElementById(`${elementId}-refresh`).addEventListener('click', () => {
        editor.setValue(content);
      });

      document.getElementById(`${elementId}-export`).addEventListener('click', () => {
        const codeContent = editor.getValue();
        const blob = new Blob([codeContent], { type: 'text/plain' });
        const url = URL.createObjectURL(blob);
        const a = document.createElement('a');
        a.href = url;
        a.download = `${elementId}-code.txt`;
        a.click();
        URL.revokeObjectURL(url);
      });

      document.getElementById(`${elementId}-import`).addEventListener('click', () => {
        const input = document.createElement('input');
        input.type = 'file';
        input.accept = '.js,.html,.css,.sql,.txt';
        input.onchange = (e) => {
          const file = e.target.files[0];
          if (file) {
            const reader = new FileReader();
            reader.onload = (event) => {
              editor.setValue(event.target.result);
            };
            reader.readAsText(file);
          }
        };
        input.click();
      });

      // Mode switch buttons
      document.getElementById(`${elementId}-mode-js`).addEventListener('click', () => {
        this.setMode(elementId, 'javascript');
      });

      document.getElementById(`${elementId}-mode-html`).addEventListener('click', () => {
        this.setMode(elementId, 'htmlmixed');
      });

      document.getElementById(`${elementId}-mode-css`).addEventListener('click', () => {
        this.setMode(elementId, 'css');
      });

      document.getElementById(`${elementId}-mode-sql`).addEventListener('click', () => {
        this.setMode(elementId, 'sql');
      });
    }

    return { editor, navContainer };
  }

  /**
   * Set the mode (language) for an editor
   * @param {string} elementId - The DOM element ID
   * @param {string} mode - Mode name (e.g., 'javascript', 'htmlmixed', 'css')
   * @returns {boolean} True if successful, false otherwise
   */
  setMode(elementId, mode) {
    const editor = this.getEditor(elementId);
    if (editor) {
      try {
        editor.setOption('mode', mode);
        return true;
      } catch (error) {
        console.error(`Failed to set mode for editor ${elementId}:`, error);
        return false;
      }
    }
    return false;
  }

  /**
   * Set the theme for an editor
   * @param {string} elementId - The DOM element ID
   * @param {string} theme - Theme name
   * @returns {boolean} True if successful, false otherwise
   */
  setTheme(elementId, theme) {
    const editor = this.getEditor(elementId);
    if (editor) {
      try {
        editor.setOption('theme', theme);
        return true;
      } catch (error) {
        console.error(`Failed to set theme for editor ${elementId}:`, error);
        return false;
      }
    }
    return false;
  }

  /**
   * Get the content from an editor
   * @param {string} elementId - The DOM element ID
   * @returns {string|null} Editor content or null if editor not found
   */
  getContent(elementId) {
    const editor = this.getEditor(elementId);
    return editor ? editor.getValue() : null;
  }

  /**
   * Set the content for an editor
   * @param {string} elementId - The DOM element ID
   * @param {string} content - Content to set
   * @returns {boolean} True if successful, false otherwise
   */
  setContent(elementId, content) {
    const editor = this.getEditor(elementId);
    if (editor) {
      try {
        editor.setValue(content);
        return true;
      } catch (error) {
        console.error(`Failed to set content for editor ${elementId}:`, error);
        return false;
      }
    }
    return false;
  }

  /**
   * Create a minimal CodeMirror editor
   * @param {string} elementId - The DOM element ID
   * @param {string} content - Initial content
   * @param {Object} options - Custom options
   * @returns {CodeMirror.Editor} CodeMirror editor instance
   */
  initMinimalEditor(elementId, content = '', options = {}) {
    const minimalOptions = {
      ...this.defaultOptions,
      ...options,
      lineNumbers: false,
      lineWrapping: false,
      foldGutter: false,
      gutters: ['CodeMirror-linenumbers'],
      extraKeys: {
        'Ctrl-Space': 'autocomplete'
      }
    };

    return this.initEditor(elementId, content, minimalOptions);
  }

  /**
   * Create a full-featured CodeMirror editor
   * @param {string} elementId - The DOM element ID
   * @param {string} content - Initial content
   * @param {Object} options - Custom options
   * @returns {CodeMirror.Editor} CodeMirror editor instance
   */
  initFullEditor(elementId, content = '', options = {}) {
    const fullOptions = {
      ...this.defaultOptions,
      ...options,
      lineNumbers: true,
      lineWrapping: true,
      foldGutter: true,
      gutters: ['CodeMirror-linenumbers', 'CodeMirror-foldgutter', 'CodeMirror-lint'],
      lint: true,
      autoCloseBrackets: true,
      autoCloseTags: true,
      matchTags: { bothTags: true },
      highlightSelectionMatches: true,
      styleActiveLine: true,
      showHint: true
    };

    return this.initEditor(elementId, content, fullOptions);
  }

  /**
   * Configure editor for specific language
   * @param {string} elementId - The DOM element ID
   * @param {string} language - Language name
   * @param {Object} options - Additional options
   * @returns {CodeMirror.Editor|null} Configured editor or null if not found
   */
  configureForLanguage(elementId, language, options = {}) {
    const editor = this.getEditor(elementId);
    if (editor) {
      let mode = 'javascript'; // default
      let theme = 'dracula';
      let extraOptions = {};

      switch (language.toLowerCase()) {
        case 'javascript':
        case 'js':
          mode = 'javascript';
          break;
        case 'html':
          mode = 'htmlmixed';
          break;
        case 'css':
          mode = 'css';
          break;
        case 'sql':
          mode = 'sql';
          break;
        case 'python':
          mode = 'python';
          break;
        case 'java':
          mode = 'clike';
          break;
        case 'json':
          mode = 'application/json';
          break;
      }

      const languageOptions = {
        mode,
        theme,
        ...extraOptions,
        ...options
      };

      // Apply options
      for (const [key, value] of Object.entries(languageOptions)) {
        editor.setOption(key, value);
      }

      return editor;
    }
    return null;
  }

  /**
   * Enable/disable linting for an editor
   * @param {string} elementId - The DOM element ID
   * @param {boolean} enable - Whether to enable linting
   * @returns {boolean} True if successful, false otherwise
   */
  setLinting(elementId, enable) {
    const editor = this.getEditor(elementId);
    if (editor) {
      try {
        editor.setOption('lint', enable);
        return true;
      } catch (error) {
        console.error(`Failed to set linting for editor ${elementId}:`, error);
        return false;
      }
    }
    return false;
  }

  /**
   * Apply custom themes to an editor
   * @param {string} elementId - The DOM element ID
   * @param {string} theme - Theme name or CSS class
   */
  applyTheme(elementId, theme) {
    const editor = this.getEditor(elementId);
    if (editor) {
      const wrapper = editor.getWrapperElement();
      wrapper.classList.remove('codemirror-theme-default');
      wrapper.classList.add(`codemirror-theme-${theme}`);
    }
  }
}

// Export singleton instance
export const codeMirrorInit = new CodeMirrorInit();