// Highlight.js Initialization Module
// Handles Highlight.js syntax highlighting initialization and configuration

class HighlightInit {
  constructor() {
    this.highlightedElements = new Map(); // Track highlighted elements
    this.supportedLanguages = [
      'javascript', 'js', 'html', 'css', 'sql', 'python', 'java',
      'c', 'cpp', 'csharp', 'php', 'ruby', 'go', 'rust', 'swift',
      'kotlin', 'typescript', 'ts', 'json', 'xml', 'yaml', 'markdown'
    ];
    this.defaultOptions = {
      showLineNumbers: false,
      showLanguageLabel: true,
      copyButton: true,
      downloadButton: false,
      theme: 'default',
      plugins: []
    };
  }

  /**
   * Highlight code in a specific element
   * @param {string} elementId - The DOM element ID containing code
   * @param {string} language - Programming language
   * @param {Object} options - Custom options
   * @returns {HTMLElement|null} Highlighted element or null if failed
   */
  highlightElement(elementId, language = 'javascript', options = {}) {
    try {
      const element = document.getElementById(elementId);
      if (!element) {
        throw new Error(`Element with ID ${elementId} not found`);
      }

      // Validate language
      if (!this.supportedLanguages.includes(language.toLowerCase())) {
        console.warn(`Language ${language} not supported, using default`);
        language = 'javascript';
      }

      // Merge options
      const mergedOptions = { ...this.defaultOptions, ...options };

      // Add language class
      element.classList.add(`language-${language}`);

      // Highlight the code
      if (typeof hljs !== 'undefined') {
        hljs.highlightElement(element);
      } else {
        console.error('Highlight.js not loaded');
        return null;
      }

      // Add additional features based on options
      if (mergedOptions.showLineNumbers) {
        this.addLineNumbers(element);
      }

      if (mergedOptions.copyButton) {
        this.addCopyButton(element, elementId);
      }

      if (mergedOptions.downloadButton) {
        this.addDownloadButton(element, elementId, language);
      }

      // Store reference
      this.highlightedElements.set(elementId, element);

      console.log(`Highlight.js highlighting applied to ${elementId} (${language})`);
      return element;
    } catch (error) {
      console.error(`Failed to highlight element ${elementId}:`, error);
      return null;
    }
  }

  /**
   * Highlight all code blocks on the page
   * @param {Object} options - Custom options
   */
  highlightAll(options = {}) {
    const codeBlocks = document.querySelectorAll('pre code');
    codeBlocks.forEach((block) => {
      const language = block.className.replace('language-', '') || 'javascript';
      const elementId = block.id || `highlight-block-${Math.random().toString(36).substr(2, 9)}`;
      block.id = elementId;
      this.highlightElement(elementId, language, options);
    });
  }

  /**
   * Add line numbers to a code block
   * @param {HTMLElement} element - The code element
   */
  addLineNumbers(element) {
    if (!element.classList.contains('hljs-line-numbers')) {
      element.classList.add('hljs-line-numbers');
      
      // Create line numbers container
      const lines = element.textContent.split('\n').length;
      const lineNumbers = document.createElement('div');
      lineNumbers.className = 'hljs-line-numbers';
      
      for (let i = 1; i <= lines; i++) {
        const lineNumber = document.createElement('span');
        lineNumber.textContent = i;
        lineNumbers.appendChild(lineNumber);
      }
      
      // Insert line numbers before the code element
      element.parentNode.insertBefore(lineNumbers, element);
    }
  }

  /**
   * Add copy button to a code block
   * @param {HTMLElement} element - The code element
   * @param {string} elementId - The element ID
   */
  addCopyButton(element, elementId) {
    const copyButton = document.createElement('button');
    copyButton.className = 'highlight-copy-btn btn btn-sm btn-outline-secondary';
    copyButton.title = 'Copy to clipboard';
    copyButton.innerHTML = '<i class="fas fa-copy"></i>';
    copyButton.style.position = 'absolute';
    copyButton.style.right = '10px';
    copyButton.style.top = '10px';
    copyButton.style.zIndex = '10';

    copyButton.addEventListener('click', () => {
      const code = element.textContent;
      navigator.clipboard.writeText(code).then(() => {
        // Show success feedback
        copyButton.innerHTML = '<i class="fas fa-check"></i>';
        copyButton.title = 'Copied!';
        setTimeout(() => {
          copyButton.innerHTML = '<i class="fas fa-copy"></i>';
          copyButton.title = 'Copy to clipboard';
        }, 2000);
      }).catch((error) => {
        console.error('Failed to copy code:', error);
      });
    });

    // Add button to the code block container
    const container = element.parentElement;
    if (container) {
      container.style.position = 'relative';
      container.appendChild(copyButton);
    }
  }

  /**
   * Add download button to a code block
   * @param {HTMLElement} element - The code element
   * @param {string} elementId - The element ID
   * @param {string} language - The programming language
   */
  addDownloadButton(element, elementId, language) {
    const downloadButton = document.createElement('button');
    downloadButton.className = 'highlight-download-btn btn btn-sm btn-outline-secondary';
    downloadButton.title = 'Download code';
    downloadButton.innerHTML = '<i class="fas fa-download"></i>';
    downloadButton.style.position = 'absolute';
    downloadButton.style.right = '50px';
    downloadButton.style.top = '10px';
    downloadButton.style.zIndex = '10';

    downloadButton.addEventListener('click', () => {
      const code = element.textContent;
      const blob = new Blob([code], { type: 'text/plain' });
      const url = URL.createObjectURL(blob);
      const a = document.createElement('a');
      a.href = url;
      a.download = `${elementId}.${language}`;
      a.click();
      URL.revokeObjectURL(url);
    });

    // Add button to the code block container
    const container = element.parentElement;
    if (container) {
      container.style.position = 'relative';
      container.appendChild(downloadButton);
    }
  }

  /**
   * Set default options for all future highlighting
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
   * Apply a theme to Highlight.js
   * @param {string} theme - Theme name
   */
  applyTheme(theme) {
    // Remove current theme
    document.querySelectorAll('link[href*="highlight"]').forEach((link) => {
      if (link.href.includes('highlight.js') || link.href.includes('highlight-')) {
        link.remove();
      }
    });

    // Add new theme
    const themeLink = document.createElement('link');
    themeLink.rel = 'stylesheet';
    themeLink.href = `https://cdnjs.cloudflare.com/ajax/libs/highlight.js/11.9.0/styles/${theme}.min.css`;
    document.head.appendChild(themeLink);

    this.defaultOptions.theme = theme;
  }

  /**
   * Register a new language with Highlight.js
   * @param {string} language - Language name
   * @param {Function} definition - Language definition function
   */
  registerLanguage(language, definition) {
    if (typeof hljs !== 'undefined') {
      hljs.registerLanguage(language, definition);
      if (!this.supportedLanguages.includes(language)) {
        this.supportedLanguages.push(language);
      }
    }
  }

  /**
   * Check if a language is supported
   * @param {string} language - Language name
   * @returns {boolean} True if supported, false otherwise
   */
  isLanguageSupported(language) {
    return this.supportedLanguages.includes(language.toLowerCase());
  }

  /**
   * Get list of supported languages
   * @returns {Array} Array of supported languages
   */
  getSupportedLanguages() {
    return [...this.supportedLanguages];
  }

  /**
   * Create a code block with syntax highlighting
   * @param {string} containerId - Container element ID
   * @param {string} code - Code content
   * @param {string} language - Programming language
   * @param {Object} options - Custom options
   * @returns {HTMLElement|null} Created code block or null if failed
   */
  createCodeBlock(containerId, code, language = 'javascript', options = {}) {
    try {
      const container = document.getElementById(containerId);
      if (!container) {
        throw new Error(`Container with ID ${containerId} not found`);
      }

      const elementId = `highlight-code-${Math.random().toString(36).substr(2, 9)}`;
      const codeElement = document.createElement('code');
      codeElement.id = elementId;
      codeElement.className = `language-${language}`;
      codeElement.textContent = code;
      codeElement.style.fontFamily = '"Vanadium Mono", "Courier New", monospace';

      const preElement = document.createElement('pre');
      preElement.appendChild(codeElement);

      container.appendChild(preElement);

      // Highlight the code
      return this.highlightElement(elementId, language, options);
    } catch (error) {
      console.error(`Failed to create code block in ${containerId}:`, error);
      return null;
    }
  }

  /**
   * Update highlighted code
   * @param {string} elementId - The DOM element ID
   * @param {string} newCode - New code content
   * @returns {boolean} True if successful, false otherwise
   */
  updateCode(elementId, newCode) {
    const element = this.highlightedElements.get(elementId);
    if (element) {
      try {
        element.textContent = newCode;
        if (typeof hljs !== 'undefined') {
          hljs.highlightElement(element);
        }
        return true;
      } catch (error) {
        console.error(`Failed to update code in ${elementId}:`, error);
        return false;
      }
    }
    return false;
  }

  /**
   * Remove highlighting from an element
   * @param {string} elementId - The DOM element ID
   * @returns {boolean} True if successful, false otherwise
   */
  removeHighlighting(elementId) {
    const element = this.highlightedElements.get(elementId);
    if (element) {
      try {
        // Remove Highlight.js classes
        element.classList.remove(...Array.from(element.classList).filter(c => c.startsWith('language-') || c.startsWith('hljs-')));
        
        // Remove line numbers if present
        const lineNumbers = element.parentNode.querySelector('.hljs-line-numbers');
        if (lineNumbers) {
          lineNumbers.remove();
        }
        
        // Remove copy/download buttons
        const container = element.parentElement;
        if (container) {
          container.querySelectorAll('.highlight-copy-btn, .highlight-download-btn').forEach(btn => btn.remove());
        }

        this.highlightedElements.delete(elementId);
        return true;
      } catch (error) {
        console.error(`Failed to remove highlighting from ${elementId}:`, error);
        return false;
      }
    }
    return false;
  }

  /**
   * Configure Highlight.js plugins
   * @param {Array} plugins - Array of plugin names to enable
   */
  configurePlugins(plugins) {
    this.defaultOptions.plugins = plugins;
    console.log('Highlight.js plugins configured:', plugins);
  }

  /**
   * Apply custom CSS to highlighted elements
   * @param {string} elementId - The DOM element ID
   * @param {Object} styles - CSS styles to apply
   */
  applyCustomStyles(elementId, styles) {
    const element = this.highlightedElements.get(elementId);
    if (element) {
      Object.assign(element.style, styles);
    }
  }

  /**
   * Auto-detect language and highlight
   * @param {string} elementId - The DOM element ID
   * @param {Object} options - Custom options
   * @returns {HTMLElement|null} Highlighted element or null if failed
   */
  autoHighlight(elementId, options = {}) {
    const element = document.getElementById(elementId);
    if (element) {
      // Try to auto-detect language based on content or file extension
      let language = 'javascript'; // default
      
      // Check if element has any language hints
      const classList = Array.from(element.classList);
      const languageClass = classList.find(c => c.startsWith('language-'));
      
      if (languageClass) {
        language = languageClass.replace('language-', '');
      }

      return this.highlightElement(elementId, language, options);
    }
    return null;
  }

  /**
   * Highlight inline code
   * @param {string} elementId - The DOM element ID
   * @param {string} language - Programming language
   * @returns {HTMLElement|null} Highlighted element or null if failed
   */
  highlightInline(elementId, language = 'javascript') {
    const element = document.getElementById(elementId);
    if (element) {
      // For inline code, we wrap it in a span with appropriate classes
      const code = element.textContent;
      element.innerHTML = `<span class="hljs-inline language-${language}">${code}</span>`;
      
      if (typeof hljs !== 'undefined') {
        hljs.highlightElement(element.querySelector('span'));
      }
      
      return element;
    }
    return null;
  }

  /**
   * Get highlighted code as HTML
   * @param {string} elementId - The DOM element ID
   * @returns {string|null} HTML content or null if element not found
   */
  getHighlightedHTML(elementId) {
    const element = this.highlightedElements.get(elementId);
    return element ? element.innerHTML : null;
  }

  /**
   * Get highlighted code as plain text
   * @param {string} elementId - The DOM element ID
   * @returns {string|null} Plain text content or null if element not found
   */
  getPlainText(elementId) {
    const element = this.highlightedElements.get(elementId);
    return element ? element.textContent : null;
  }
}

// Export singleton instance
export const highlightInit = new HighlightInit();