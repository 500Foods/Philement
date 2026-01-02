// SunEditor Initialization Module
// Handles SunEditor (WYSIWYG editor) initialization and configuration

class SunEditorInit {
  constructor() {
    this.editors = new Map(); // Store editor instances
    this.defaultOptions = {
      buttonList: [
        ['undo', 'redo'],
        ['font', 'fontSize', 'formatBlock'],
        ['bold', 'underline', 'italic', 'strike', 'subscript', 'superscript'],
        ['fontColor', 'hiliteColor', 'textStyle'],
        ['removeFormat'],
        ['outdent', 'indent'],
        ['align', 'horizontalRule', 'list', 'lineHeight'],
        ['table', 'link', 'image', 'video', 'audio'],
        ['fullScreen', 'showBlocks', 'codeView'],
        ['preview', 'print'],
        ['save', 'template']
      ],
      height: 'auto',
      minHeight: '300px',
      maxHeight: '800px',
      width: '100%',
      placeholder: 'Type here...',
      charCounter: true,
      maxCharCount: 10000,
      resizeEnable: true,
      showPathLabel: true,
      font: ['Vanadium Sans', 'Arial', 'Times New Roman', 'Courier New', 'Georgia', 'Verdana', 'FontAwesome'],
      fontAwesome: true,
      imageUploadUrl: '/api/upload/image',
      imageUploadHeader: {
        'Authorization': 'Bearer ' + localStorage.getItem('lithium_jwt') || ''
      },
      videoUploadUrl: '/api/upload/video',
      videoUploadHeader: {
        'Authorization': 'Bearer ' + localStorage.getItem('lithium_jwt') || ''
      },
      callbacks: {
        onChange: (contents, core) => {
          console.log('SunEditor content changed:', contents);
        },
        onBlur: (contents, core) => {
          console.log('SunEditor blurred:', contents);
        },
        onFocus: (contents, core) => {
          console.log('SunEditor focused');
        },
        onImageUpload: (targetImgElement, index, state, imageInfo, remainingFilesCount) => {
          console.log('Image upload:', imageInfo);
        },
        onImageUploadError: (errorMessage, result, core) => {
          console.error('Image upload error:', errorMessage);
        }
      }
    };
  }

  /**
   * Initialize a SunEditor instance
   * @param {string} elementId - The DOM element ID to attach the editor to
   * @param {string} content - Initial content
   * @param {Object} options - Custom options to override defaults
   * @returns {SunEditor} SunEditor instance
   */
  initEditor(elementId, content = '', options = {}) {
    try {
      const element = document.getElementById(elementId);
      if (!element) {
        throw new Error(`Element with ID ${elementId} not found`);
      }

      // Merge default options with custom options
      const mergedOptions = { ...this.defaultOptions, ...options };

      // Initialize SunEditor
      const editor = SUNEDITOR.create(element, mergedOptions);

      // Set initial content
      if (content) {
        editor.setContents(content);
      }

      // Store reference
      this.editors.set(elementId, editor);

      console.log(`SunEditor initialized on ${elementId}`);
      return editor;
    } catch (error) {
      console.error(`Failed to initialize SunEditor on ${elementId}:`, error);
      throw error;
    }
  }

  /**
   * Get an editor instance by element ID
   * @param {string} elementId - The DOM element ID
   * @returns {SunEditor|null} SunEditor instance or null if not found
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
      SUNEDITOR.remove(editor);
      this.editors.delete(elementId);
    }
  }

  /**
   * Destroy all editor instances
   */
  destroyAllEditors() {
    this.editors.forEach((editor, elementId) => {
      SUNEDITOR.remove(editor);
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
   * Create a SunEditor with navigation controls
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
        <div class="suneditor-nav d-flex justify-content-between align-items-center mb-2">
          <div class="nav-left">
            <button class="btn btn-sm btn-outline-primary mr-2" id="${elementId}-refresh">
              <i class="fas fa-sync-alt"></i> Refresh
            </button>
            <button class="btn btn-sm btn-outline-secondary mr-2" id="${elementId}-export">
              <i class="fas fa-file-export"></i> Export HTML
            </button>
            <button class="btn btn-sm btn-outline-secondary mr-2" id="${elementId}-export-text">
              <i class="fas fa-file-alt"></i> Export Text
            </button>
          </div>
          <div class="nav-right">
            <button class="btn btn-sm btn-outline-secondary mr-2" id="${elementId}-clear">
              <i class="fas fa-eraser"></i> Clear
            </button>
            <button class="btn btn-sm btn-outline-secondary" id="${elementId}-fullscreen">
              <i class="fas fa-expand"></i> Fullscreen
            </button>
          </div>
        </div>
      `;

      // Set up event handlers
      document.getElementById(`${elementId}-refresh`).addEventListener('click', () => {
        editor.setContents(content);
      });

      document.getElementById(`${elementId}-export`).addEventListener('click', () => {
        const htmlContent = editor.getContents();
        const blob = new Blob([htmlContent], { type: 'text/html' });
        const url = URL.createObjectURL(blob);
        const a = document.createElement('a');
        a.href = url;
        a.download = `${elementId}-content.html`;
        a.click();
        URL.revokeObjectURL(url);
      });

      document.getElementById(`${elementId}-export-text`).addEventListener('click', () => {
        const textContent = editor.getText();
        const blob = new Blob([textContent], { type: 'text/plain' });
        const url = URL.createObjectURL(blob);
        const a = document.createElement('a');
        a.href = url;
        a.download = `${elementId}-content.txt`;
        a.click();
        URL.revokeObjectURL(url);
      });

      document.getElementById(`${elementId}-clear`).addEventListener('click', () => {
        editor.setContents('');
      });

      document.getElementById(`${elementId}-fullscreen`).addEventListener('click', () => {
        editor.core.toolbar._toggleFullScreen();
      });
    }

    return { editor, navContainer };
  }

  /**
   * Get HTML content from an editor
   * @param {string} elementId - The DOM element ID
   * @returns {string|null} HTML content or null if editor not found
   */
  getHTML(elementId) {
    const editor = this.getEditor(elementId);
    return editor ? editor.getContents() : null;
  }

  /**
   * Get text content from an editor
   * @param {string} elementId - The DOM element ID
   * @returns {string|null} Text content or null if editor not found
   */
  getText(elementId) {
    const editor = this.getEditor(elementId);
    return editor ? editor.getText() : null;
  }

  /**
   * Set HTML content to an editor
   * @param {string} elementId - The DOM element ID
   * @param {string} htmlContent - HTML content to set
   * @returns {boolean} True if successful, false otherwise
   */
  setHTML(elementId, htmlContent) {
    const editor = this.getEditor(elementId);
    if (editor) {
      try {
        editor.setContents(htmlContent);
        return true;
      } catch (error) {
        console.error(`Failed to set HTML content to editor ${elementId}:`, error);
        return false;
      }
    }
    return false;
  }

  /**
   * Create a minimal SunEditor (basic toolbar)
   * @param {string} elementId - The DOM element ID
   * @param {string} content - Initial content
   * @param {Object} options - Custom options
   * @returns {SunEditor} SunEditor instance
   */
  initMinimalEditor(elementId, content = '', options = {}) {
    const minimalOptions = {
      ...this.defaultOptions,
      ...options,
      buttonList: [
        ['bold', 'italic', 'underline', 'strike'],
        ['fontColor', 'hiliteColor'],
        ['align', 'list'],
        ['link', 'image'],
        ['fullScreen']
      ],
      height: '200px',
      minHeight: '150px',
      maxHeight: '400px'
    };

    return this.initEditor(elementId, content, minimalOptions);
  }

  /**
   * Create a full-featured SunEditor
   * @param {string} elementId - The DOM element ID
   * @param {string} content - Initial content
   * @param {Object} options - Custom options
   * @returns {SunEditor} SunEditor instance
   */
  initFullEditor(elementId, content = '', options = {}) {
    const fullOptions = {
      ...this.defaultOptions,
      ...options,
      buttonList: [
        ['undo', 'redo'],
        ['font', 'fontSize', 'formatBlock'],
        ['bold', 'underline', 'italic', 'strike', 'subscript', 'superscript'],
        ['fontColor', 'hiliteColor', 'textStyle'],
        ['removeFormat'],
        ['outdent', 'indent'],
        ['align', 'horizontalRule', 'list', 'lineHeight'],
        ['table', 'link', 'image', 'video', 'audio'],
        ['fullScreen', 'showBlocks', 'codeView'],
        ['preview', 'print'],
        ['save', 'template'],
        ['imageGallery']
      ],
      height: '500px',
      minHeight: '400px',
      maxHeight: '1000px'
    };

    return this.initEditor(elementId, content, fullOptions);
  }

  /**
   * Configure FontAwesome support for the editor
   * @param {string} elementId - The DOM element ID
   * @param {boolean} enable - Whether to enable FontAwesome
   * @param {Array} fontAwesomeIcons - Array of FontAwesome icon classes to include
   */
  configureFontAwesome(elementId, enable = true, fontAwesomeIcons = []) {
    const editor = this.getEditor(elementId);
    if (editor) {
      try {
        // Enable FontAwesome support
        editor.setOption('fontAwesome', enable);
        
        // Add FontAwesome to the font list if not already present
        const currentFonts = editor.getOption('font') || [];
        if (enable && !currentFonts.includes('FontAwesome')) {
          const updatedFonts = [...currentFonts, 'FontAwesome'];
          editor.setOption('font', updatedFonts);
        }
        
        // If specific FontAwesome icons are provided, we can add them as custom fonts
        if (fontAwesomeIcons.length > 0) {
          const fontList = editor.getOption('font') || [];
          const fontAwesomeFontList = fontAwesomeIcons.map(icon => `fa-${icon}`);
          editor.setOption('font', [...fontList, ...fontAwesomeFontList]);
        }
        
        console.log(`FontAwesome ${enable ? 'enabled' : 'disabled'} for editor ${elementId}`);
        return true;
      } catch (error) {
        console.error(`Failed to configure FontAwesome for editor ${elementId}:`, error);
        return false;
      }
    }
    return false;
  }

  /**
   * Configure editor plugins
   * @param {string} elementId - The DOM element ID
   * @param {Object} plugins - Plugins to configure
   */
  configurePlugins(elementId, plugins) {
    const editor = this.getEditor(elementId);
    if (editor) {
      // SunEditor plugins are typically configured during initialization
      // This method can be used to update plugin configurations
      console.log(`Configuring plugins for editor ${elementId}:`, plugins);
    }
  }

  /**
   * Apply custom themes to editor
   * @param {string} elementId - The DOM element ID
   * @param {string} theme - Theme name or CSS class
   */
  applyTheme(elementId, theme) {
    const editor = this.getEditor(elementId);
    if (editor) {
      const container = editor.core.layout.container;
      container.classList.remove('suneditor-theme-default');
      container.classList.add(`suneditor-theme-${theme}`);
    }
  }

  /**
   * Update editor toolbar buttons dynamically
   * @param {string} elementId - The DOM element ID
   * @param {Array} buttonList - New button list configuration
   */
  updateToolbar(elementId, buttonList) {
    const editor = this.getEditor(elementId);
    if (editor) {
      // SunEditor doesn't support dynamic toolbar updates directly
      // This would require destroying and recreating the editor
      console.warn('SunEditor toolbar updates require editor recreation');
    }
  }
}

// Export singleton instance
export const sunEditorInit = new SunEditorInit();