const __vite__mapDeps=(i,m=__vite__mapDeps,d=(m.f||(m.f=["./codemirror-setup-lxLGSWU-.js","./codemirror-Bg1rM8n2.js"])))=>i.map(i=>d[i]);
import { p as processIcons, _ as __vitePreload } from './index-CNmg4i7Y.js';
import { toggleWordWrap, toggleBracketMatch, toggleSelectionHighlight, toggleCommentContinuation, toggleWhitespace, toggleVirtualColumns } from './codemirror-setup-lxLGSWU-.js';

class LithiumEditorFooter {
  /**
   * @param {Object} options
   * @param {HTMLElement} options.container - Footer container element
   * @param {EditorView} options.editorView - The CodeMirror EditorView
   * @param {Compartment} [options.wordWrapCompartment] - Word wrap compartment
   * @param {Compartment} [options.bracketMatchCompartment] - Bracket match compartment
   * @param {boolean} [options.initialWordWrap=false] - Initial word wrap state
   * @param {boolean} [options.initialBracketMatch=true] - Initial bracket matching state
   * @param {Compartment} [options.selectionHighlightCompartment] - Selection highlight compartment
   * @param {boolean} [options.initialSelectionHighlight=true] - Initial selection highlight state
    * @param {Compartment} [options.commentContinuationCompartment] - Comment continuation compartment
    * @param {boolean} [options.initialCommentContinuation=true] - Initial comment continuation state
    * @param {Compartment} [options.whitespaceCompartment] - Whitespace rendering compartment
    * @param {boolean} [options.initialWhitespace=false] - Initial whitespace rendering state
   * @param {Compartment} [options.indentUnitCompartment] - Indent unit compartment
   * @param {string} [options.initialIndentUnit='\t'] - Initial indent unit
   * @param {string} [options.storageKey] - LocalStorage key prefix for saving states
   * @param {Compartment} [options.virtualColumnsCompartment] - Virtual columns compartment
   * @param {boolean} [options.initialVirtualColumns=false] - Initial virtual columns state
   */
  constructor(options = {}) {
    this.container = options.container;
    this.editorView = options.editorView;
    this.wordWrapCompartment = options.wordWrapCompartment;
    this.bracketMatchCompartment = options.bracketMatchCompartment;
    this.selectionHighlightCompartment = options.selectionHighlightCompartment;
    this.commentContinuationCompartment = options.commentContinuationCompartment;
    this.whitespaceCompartment = options.whitespaceCompartment;
    this.indentUnitCompartment = options.indentUnitCompartment;
    this.virtualColumnsCompartment = options.virtualColumnsCompartment;
    this.storageKey = options.storageKey;

    // Load states from storage or use initials
    this.wordWrapEnabled = this._loadState('word_wrap', options.initialWordWrap || false);
    this.bracketMatchEnabled = this._loadState('bracket_match', options.initialBracketMatch !== undefined ? options.initialBracketMatch : true);
    this.selectionHighlightEnabled = this._loadState('selection_highlight', options.initialSelectionHighlight !== undefined ? options.initialSelectionHighlight : true);
    this.commentContinuationEnabled = this._loadState('comment_continuation', options.initialCommentContinuation !== undefined ? options.initialCommentContinuation : true);
    this.whitespaceEnabled = this._loadState('whitespace', options.initialWhitespace || false);
    this.indentUnit = this._loadState('indent_unit', options.initialIndentString || '\t');
    this.virtualColumnsEnabled = this._loadState('virtual_columns', options.initialVirtualColumns !== undefined ? options.initialVirtualColumns : true);

    this.cursorEl = null;
    this.errorsEl = null;
    this.errorMsgEl = null;
    this.indentUnitBtn = null;
    this.wordWrapBtn = null;
    this.bracketsBtn = null;
    this.selectionHighlightBtn = null;
    this.commentContinuationBtn = null;
    this.whitespaceBtn = null;

    // Indent unit options
    this.indentUnitOptions = [
      { value: '\t', icon: 'arrow-right-to-line' },
      { value: '  ', icon: '2' },
      { value: '   ', icon: '3' },
      { value: '    ', icon: '4' },
      { value: '        ', icon: '8' },
    ];
  }

  /**
    * Initialize the footer: create elements, attach listeners, set initial state.
    */
  init() {
    if (!this.container) {
      console.error('[EditorFooter] No container provided');
      return;
    }

    this._createElements();
    this.updateCursorPosition();
    this._updateErrorCount(0);
    this._setInitialStates();
  }

  /**
    * Load state from lithiumSettings or return default.
    */
  _loadState(key, defaultValue) {
    if (!this.storageKey) return defaultValue;
    const path = `${this.storageKey}.${key}`;
    const stored = window.lithiumSettings.get(path);
    return stored !== undefined ? stored : defaultValue;
  }

  /**
    * Save state to lithiumSettings.
    */
  _saveState(key, value) {
    if (!this.storageKey) return;
    const path = `${this.storageKey}.${key}`;
    window.lithiumSettings.set(path, value);
  }

  /**
    * Create footer DOM elements.
    */
  _createElements() {
    this.container.classList.add('lithium-toolbar', 'lithium-editor-footer');
    this.container.innerHTML = '';

    // 1. Cursor position (left)
    const cursorDiv = document.createElement('div');
    cursorDiv.className = 'lithium-editor-cursor';
    cursorDiv.innerHTML = '<fa fa-location-crosshairs></fa><span>Row 1, Col 1</span>';
    this.container.appendChild(cursorDiv);
    this.cursorEl = cursorDiv.querySelector('span');

    // 2. Errors count (left)
    const errorsDiv = document.createElement('div');
    errorsDiv.className = 'lithium-editor-errors';
    errorsDiv.innerHTML = '<fa fa-bug></fa><span>No Errors</span>';
    this.container.appendChild(errorsDiv);
    this.errorsEl = errorsDiv.querySelector('span');

    // 3. Error message (fills space)
    const errorMsg = document.createElement('div');
    errorMsg.className = 'lithium-editor-error-msg lithium-toolbar-placeholder';
    this.container.appendChild(errorMsg);
    this.errorMsgEl = errorMsg;

    // 4. Indent unit toggle (right)
    this.indentUnitBtn = document.createElement('button');
    this.indentUnitBtn.type = 'button';
    this.indentUnitBtn.className = 'lithium-toolbar-btn';
    this._updateIndentUnitIcon();
    this.indentUnitBtn.title = 'Toggle Indent Unit';
    this.indentUnitBtn.addEventListener('click', () => this._toggleIndentUnit());
    this.container.appendChild(this.indentUnitBtn);

    // 5. Word wrap toggle (right)
    this.wordWrapBtn = document.createElement('button');
    this.wordWrapBtn.type = 'button';
    this.wordWrapBtn.className = 'lithium-toolbar-btn';
    this.wordWrapBtn.innerHTML = '<fa fa-swap-arrows fa-rotate-90></fa>';
    this.wordWrapBtn.title = 'Toggle Word Wrap';
    this.wordWrapBtn.addEventListener('click', () => this._toggleWordWrap());
    this.container.appendChild(this.wordWrapBtn);

    // 5. Brackets toggle (right)
    this.bracketsBtn = document.createElement('button');
    this.bracketsBtn.type = 'button';
    this.bracketsBtn.className = 'lithium-toolbar-btn';
    this.bracketsBtn.innerHTML = '<fa fa-brackets-square></fa>';
    this.bracketsBtn.title = 'Toggle Bracket Matching';
    this.bracketsBtn.addEventListener('click', () => this._toggleBracketMatch());
    this.container.appendChild(this.bracketsBtn);

    // 6. Selection highlight toggle (right)
    this.selectionHighlightBtn = document.createElement('button');
    this.selectionHighlightBtn.type = 'button';
    this.selectionHighlightBtn.className = 'lithium-toolbar-btn';
    this.selectionHighlightBtn.innerHTML = '<fa fa-highlighter></fa>';
    this.selectionHighlightBtn.title = 'Toggle Selection Highlight';
    this.selectionHighlightBtn.addEventListener('click', () => this._toggleSelectionHighlight());
    this.container.appendChild(this.selectionHighlightBtn);

    // 7. Comment continuation toggle (right)
    this.commentContinuationBtn = document.createElement('button');
    this.commentContinuationBtn.type = 'button';
    this.commentContinuationBtn.className = 'lithium-toolbar-btn';
    this.commentContinuationBtn.innerHTML = '<fa fa-comment></fa>';
    this.commentContinuationBtn.title = 'Toggle Comment Continuation';
    this.commentContinuationBtn.addEventListener('click', () => this._toggleCommentContinuation());
    this.container.appendChild(this.commentContinuationBtn);

    // 8. Whitespace rendering toggle (right)
    this.whitespaceBtn = document.createElement('button');
    this.whitespaceBtn.type = 'button';
    this.whitespaceBtn.className = 'lithium-toolbar-btn';
    this.whitespaceBtn.innerHTML = '<fa fa-paragraph></fa>';
    this.whitespaceBtn.title = 'Toggle Whitespace';
    this.whitespaceBtn.addEventListener('click', () => this._toggleWhitespace());
    this.container.appendChild(this.whitespaceBtn);

    // 9. Virtual columns toggle (right)
    this.virtualColumnsBtn = document.createElement('button');
    this.virtualColumnsBtn.type = 'button';
    this.virtualColumnsBtn.className = 'lithium-toolbar-btn';
    this.virtualColumnsBtn.innerHTML = '<fa fa-up-down></fa>';
    this.virtualColumnsBtn.title = 'Toggle Virtual Columns';
    this.virtualColumnsBtn.addEventListener('click', () => this._toggleVirtualColumns());
    this.container.appendChild(this.virtualColumnsBtn);

    processIcons(this.container);
  }

  /**
    * Update the cursor position display.
    * Call this from your editor's onUpdate callback when selectionSet is true.
    */
  updateCursorPosition() {
    if (!this.editorView || !this.cursorEl) return;

    const state = this.editorView.state;
    const pos = state.selection.main.head;
    const line = state.doc.lineAt(pos);
    const row = line.number;
    const col = pos - line.from + 1;

    this.cursorEl.textContent = `Row ${row}, Col ${col}`;
  }

  /**
    * Update the error count display.
    * @param {number} count - Number of errors
    */
  _updateErrorCount(count) {
    if (!this.errorsEl) return;
    this.errorsEl.textContent = `Errors: ${count}`;
  }

  /**
   * Set an error message (or clear it).
   * @param {string} [message] - Error message, or empty to clear
   */
  setErrorMessage(message) {
    if (!this.errorMsgEl) return;
    this.errorMsgEl.textContent = message || '';
  }

  /**
    * Toggle indent unit.
    */
  _toggleIndentUnit() {
    const currentIndex = this.indentUnitOptions.findIndex(opt => opt.value === this.indentUnit);
    const nextIndex = (currentIndex + 1) % this.indentUnitOptions.length;
    this.indentUnit = this.indentUnitOptions[nextIndex].value;
    this._updateIndentUnitIcon();
    if (this.indentUnitCompartment && this.editorView) {
      __vitePreload(async () => { const {indentStringFacet} = await import('./codemirror-setup-lxLGSWU-.js');return { indentStringFacet }},true              ?__vite__mapDeps([0,1]):void 0,import.meta.url).then(({ indentStringFacet }) => {
        this.editorView.dispatch({
          effects: this.indentUnitCompartment.reconfigure(indentStringFacet.of(this.indentUnit)),
        });
      });
    }
    this._saveState('indent_unit', this.indentUnit);
  }

  /**
    * Update indent unit button icon.
    */
  _updateIndentUnitIcon() {
    const option = this.indentUnitOptions.find(opt => opt.value === this.indentUnit);
    if (this.indentUnitBtn) {
      if (option) {
        this.indentUnitBtn.innerHTML = `<fa fa-${option.icon}></fa>`;
      } else {
        // Fallback: show a default icon if indentUnit doesn't match any option
        this.indentUnitBtn.innerHTML = '<fa fa-arrow-right-to-line></fa>';
      }
      processIcons(this.indentUnitBtn);
    }
  }

  /**
    * Toggle word wrap state.
    */
  _toggleWordWrap() {
    this.wordWrapEnabled = !this.wordWrapEnabled;
    toggleWordWrap(this.editorView, this.wordWrapCompartment, this.wordWrapEnabled);
    this.wordWrapBtn.classList.toggle('active', this.wordWrapEnabled);
    this._saveState('word_wrap', this.wordWrapEnabled);
  }

  /**
     * Toggle bracket matching state.
     */
  _toggleBracketMatch() {
    this.bracketMatchEnabled = !this.bracketMatchEnabled;
    toggleBracketMatch(this.editorView, this.bracketMatchCompartment, this.bracketMatchEnabled);
    this.bracketsBtn.classList.toggle('active', !this.bracketMatchEnabled); // active when disabled
    this._saveState('bracket_match', this.bracketMatchEnabled);
  }

  /**
     * Toggle selection highlight state.
     */
  _toggleSelectionHighlight() {
    this.selectionHighlightEnabled = !this.selectionHighlightEnabled;
    toggleSelectionHighlight(this.editorView, this.selectionHighlightCompartment, this.selectionHighlightEnabled);
    this.selectionHighlightBtn.classList.toggle('active', !this.selectionHighlightEnabled); // active when disabled
    this._saveState('selection_highlight', this.selectionHighlightEnabled);
  }

  /**
     * Toggle comment continuation state.
     */
  _toggleCommentContinuation() {
    this.commentContinuationEnabled = !this.commentContinuationEnabled;
    toggleCommentContinuation(this.editorView, this.commentContinuationCompartment, this.commentContinuationEnabled);
    this.commentContinuationBtn.classList.toggle('active', !this.commentContinuationEnabled); // active when disabled
    this._saveState('comment_continuation', this.commentContinuationEnabled);
  }

  /**
    * Toggle whitespace rendering state.
    */
  _toggleWhitespace() {
    this.whitespaceEnabled = !this.whitespaceEnabled;
    toggleWhitespace(this.editorView, this.whitespaceCompartment, this.whitespaceEnabled);
    this.whitespaceBtn.classList.toggle('active', this.whitespaceEnabled);
    this._saveState('whitespace', this.whitespaceEnabled);
  }

  /**
     * Toggle virtual columns state.
     */
  _toggleVirtualColumns() {
    this.virtualColumnsEnabled = !this.virtualColumnsEnabled;
    toggleVirtualColumns(this.editorView, this.virtualColumnsCompartment, this.virtualColumnsEnabled);
    this.virtualColumnsBtn.classList.toggle('active', !this.virtualColumnsEnabled); // active when disabled
    this._saveState('virtual_columns', this.virtualColumnsEnabled);
  }

  /**
    * Set initial states for toggles.
    */
  _setInitialStates() {
    // Synchronize indent unit compartment with loaded initial value
    if (this.indentUnitCompartment && this.editorView) {
      __vitePreload(async () => { const {indentStringFacet} = await import('./codemirror-setup-lxLGSWU-.js');return { indentStringFacet }},true              ?__vite__mapDeps([0,1]):void 0,import.meta.url).then(({ indentStringFacet }) => {
        this.editorView.dispatch({
          effects: this.indentUnitCompartment.reconfigure(indentStringFacet.of(this.indentUnit)),
        });
      });
    }

    if (this.indentUnitBtn) {
      this._updateIndentUnitIcon();
    }
    if (this.wordWrapBtn) {
      this.wordWrapBtn.classList.toggle('active', this.wordWrapEnabled); // active when enabled
    }
    if (this.bracketsBtn) {
      this.bracketsBtn.classList.toggle('active', !this.bracketMatchEnabled); // active when disabled
    }
    if (this.selectionHighlightBtn) {
      this.selectionHighlightBtn.classList.toggle('active', !this.selectionHighlightEnabled); // active when disabled
    }
    if (this.commentContinuationBtn) {
      this.commentContinuationBtn.classList.toggle('active', !this.commentContinuationEnabled); // active when disabled
    }
    if (this.whitespaceBtn) {
      this.whitespaceBtn.classList.toggle('active', this.whitespaceEnabled); // active when enabled
    }
    if (this.virtualColumnsBtn) {
      this.virtualColumnsBtn.classList.toggle('active', !this.virtualColumnsEnabled); // active when disabled
    }
  }

  /**
   * Update error count (public API).
   * @param {number} count
   */
  updateErrorCount(count) {
    this._updateErrorCount(count);
  }

  /**
   * Show the footer (make it visible).
   */
  show() {
    if (this.container) {
      this.container.style.display = '';
    }
  }

  /**
   * Hide the footer (make it invisible).
   */
  hide() {
    if (this.container) {
      this.container.style.display = 'none';
    }
  }

 /**
      * Destroy the footer, clean up listeners and clear contents (but don't remove from DOM).
      */
    destroy() {
      // Null out references first to prevent any pending updates from accessing stale elements
      this.editorView = null;
      this.cursorEl = null;
      this.errorsEl = null;
      this.errorMsgEl = null;
      this.indentUnitBtn = null;
      this.wordWrapBtn = null;
      this.bracketsBtn = null;
      this.selectionHighlightBtn = null;
      this.commentContinuationBtn = null;
      this.whitespaceBtn = null;
      this.virtualColumnsBtn = null;

      // Clear contents but don't remove from DOM since it's now part of HTML structure
      if (this.container) {
        this.container.innerHTML = '';
        this.container.style.display = 'none';
      }
      this.container = null;
    }
}

/**
 * Font Popup

/**
 * Extract unique font families from all loaded stylesheets
 * @returns {Array<{name: string, value: string}>}
 */

// ===== Font Popup Creation =====

/**
 * Create a reusable font settings popup.
 * @param {Object} options
 * @returns {{ popup: HTMLElement, toggle: Function, show: Function, hide: Function, getState: Function, setState: Function }}
 */
function createFontPopup(options) {
  const {
    anchor,
    fontSize: initialFontSize = 14,
    fontFamily: initialFontFamily = 'var(--font-sans)',
    fontWeight: initialFontWeight = 'normal',
    onPreview,
    onSave,
    onCancel,
    onReset,
  } = options;

  let originalFontSize = initialFontSize;
  let originalFontFamily = initialFontFamily;
  let originalFontWeight = initialFontWeight;

  let fontSize = initialFontSize;
  let fontFamily = initialFontFamily;
  let fontWeight = initialFontWeight;

  const popup = document.createElement('div');
  popup.className = 'manager-font-popup';

  const fontSizeValue = parseInt(String(fontSize), 10) || 14;

  const fonts = [
    { name: 'Vanadium Sans', value: '"Vanadium Sans", sans-serif' },
    { name: 'Vanadium Mono', value: '"Vanadium Mono", monospace' },
    { name: 'Vanadium Sans Fancy', value: '"Vanadium Sans Fancy", cursive' },
    { name: 'Vanadium Mono Fancy', value: '"Vanadium Mono Fancy", monospace' },
    { name: 'Monospace', value: 'monospace' },
    { name: 'Serif', value: 'serif' },
    { name: 'Sans Serif', value: 'sans-serif' },
  ];
  const fontOptions = fonts.map(f =>
    `<option value="${f.value}" ${fontFamily === f.value ? 'selected' : ''}>${f.name}</option>`
  ).join('\n        ');

  popup.innerHTML = `
    <div class="manager-font-popup-row">
      <label class="manager-font-popup-label">Size</label>
      <input type="number" class="manager-font-popup-input" data-fp="size" value="${fontSizeValue}" min="8" max="32">
    </div>
    <div class="manager-font-popup-row">
      <label class="manager-font-popup-label">Family</label>
      <select class="manager-font-popup-select" data-fp="family">
        ${fontOptions}
      </select>
    </div>
    <div class="manager-font-popup-row">
      <label class="manager-font-popup-label">Style</label>
      <div class="manager-font-popup-styles">
        <button type="button" class="manager-font-popup-style-btn ${fontWeight === 'normal' ? 'active' : ''}" data-fp="style" data-style="normal">Normal</button>
        <button type="button" class="manager-font-popup-style-btn ${fontWeight === 'bold' ? 'active' : ''}" data-fp="style" data-style="bold">Bold</button>
      </div>
    </div>
    <div class="manager-font-popup-preview" data-fp="preview">
      <span class="manager-font-popup-preview-text">The quick brown fox jumps over the lazy dog.<br>0123456789 - !&#64;#$%^&amp;*()[]{};':" ,./&lt;&gt;?~\`|</span>
    </div>
    <div class="manager-font-popup-actions">
      <button type="button" class="manager-font-popup-action-btn manager-font-popup-save-btn" data-fp="save">Save</button>
      <button type="button" class="manager-font-popup-action-btn manager-font-popup-cancel-btn" data-fp="cancel">Cancel</button>
      <button type="button" class="manager-font-popup-action-btn manager-font-popup-reset-btn" data-fp="reset">Default</button>
    </div>
  `;

  const sizeInput = popup.querySelector('[data-fp="size"]');
  const familySelect = popup.querySelector('[data-fp="family"]');
  const styleBtns = popup.querySelectorAll('[data-fp="style"]');
  const preview = popup.querySelector('[data-fp="preview"]');
  const previewText = preview?.querySelector('.manager-font-popup-preview-text');

  const updatePreview = () => {
    if (previewText) {
      const size = parseInt(String(fontSize), 10) || 14;
      previewText.style.fontSize = `${size}px`;
      previewText.style.fontFamily = fontFamily;
      previewText.style.fontWeight = fontWeight;
    }
  };

  const emitPreview = () => {
    updatePreview();
    if (typeof onPreview === 'function') {
      onPreview({ fontSize, fontFamily, fontWeight });
    }
  };

  sizeInput.addEventListener('change', (e) => {
    fontSize = `${e.target.value}px`;
    emitPreview();
  });

  familySelect.addEventListener('change', (e) => {
    fontFamily = e.target.value;
    emitPreview();
  });

  styleBtns.forEach(btn => {
    btn.addEventListener('click', () => {
      styleBtns.forEach(b => b.classList.remove('active'));
      btn.classList.add('active');
      fontWeight = btn.dataset.style;
      emitPreview();
    });
  });

  const saveBtn = popup.querySelector('[data-fp="save"]');
  saveBtn?.addEventListener('click', () => {
    originalFontSize = fontSize;
    originalFontFamily = fontFamily;
    originalFontWeight = fontWeight;
    if (typeof onSave === 'function') {
      onSave({ fontSize, fontFamily, fontWeight });
    }
    hide();
  });

  const cancelBtn = popup.querySelector('[data-fp="cancel"]');
  cancelBtn?.addEventListener('click', () => {
    fontSize = originalFontSize;
    fontFamily = originalFontFamily;
    fontWeight = originalFontWeight;
    const num = parseInt(String(fontSize), 10);
    if (!isNaN(num)) sizeInput.value = num;
    familySelect.value = fontFamily;
    styleBtns.forEach(b => b.classList.toggle('active', b.dataset.style === fontWeight));
    updatePreview();
    if (typeof onCancel === 'function') {
      onCancel();
    }
    hide();
  });

  const resetBtn = popup.querySelector('[data-fp="reset"]');
  resetBtn?.addEventListener('click', () => {
    if (typeof onReset === 'function') {
      const result = onReset();
      if (result && result.fontSize !== undefined) {
        fontSize = result.fontSize;
        fontFamily = result.fontFamily;
        fontWeight = result.fontWeight;
        const num = parseInt(String(fontSize), 10);
        if (!isNaN(num)) sizeInput.value = num;
        familySelect.value = fontFamily;
        styleBtns.forEach(b => b.classList.toggle('active', b.dataset.style === fontWeight));
        updatePreview();
      }
    }
  });

  updatePreview();

  const positionPopup = () => {
    if (!anchor) return;
    const rect = anchor.getBoundingClientRect();
    popup.style.setProperty('position', 'fixed', 'important');
    popup.style.setProperty('top', `${rect.bottom}px`, 'important');
    popup.style.setProperty('right', `${window.innerWidth - rect.right}px`, 'important');
    popup.style.setProperty('left', 'auto', 'important');
    popup.style.setProperty('z-index', '10001', 'important');
  };

  const toggle = (e) => {
    if (e) e.stopPropagation();
    const willShow = !popup.classList.contains('visible');
    if (willShow) {
      document.dispatchEvent(new CustomEvent('close-all-popups'));
      positionPopup();
      popup.classList.add('visible');
      if (anchor) anchor.classList.add('active');
    } else {
      popup.classList.remove('visible');
      if (anchor) anchor.classList.remove('active');
    }
  };

  const show = () => {
    document.dispatchEvent(new CustomEvent('close-all-popups'));
    positionPopup();
    popup.classList.add('visible');
    if (anchor) anchor.classList.add('active');
  };

  const hide = () => {
    popup.classList.remove('visible');
    if (anchor) anchor.classList.remove('active');
  };

  document.body.appendChild(popup);

  const outsideHandler = (e) => {
    if (popup.classList.contains('visible') &&
        !popup.contains(e.target) &&
        anchor && !anchor.contains(e.target)) {
      hide();
    }
  };
  document.addEventListener('click', outsideHandler);

  const closeAllHandler = () => {
    if (popup.classList.contains('visible')) {
      hide();
    }
  };
  document.addEventListener('close-all-popups', closeAllHandler);

  const getState = () => ({ fontSize, fontFamily, fontWeight });
  const setState = (patch) => {
    if (patch.fontSize !== undefined) {
      fontSize = patch.fontSize;
      originalFontSize = patch.fontSize;
      const num = parseInt(String(fontSize), 10);
      if (!isNaN(num)) sizeInput.value = num;
    }
    if (patch.fontFamily !== undefined) {
      fontFamily = patch.fontFamily;
      originalFontFamily = patch.fontFamily;
      familySelect.value = fontFamily;
    }
    if (patch.fontWeight !== undefined) {
      fontWeight = patch.fontWeight;
      originalFontWeight = patch.fontWeight;
      styleBtns.forEach(b => b.classList.toggle('active', b.dataset.style === fontWeight));
    }
    updatePreview();
  };

  const destroy = () => {
    document.removeEventListener('click', outsideHandler);
    document.removeEventListener('close-all-popups', closeAllHandler);
    if (popup && popup.parentNode) {
      popup.parentNode.removeChild(popup);
    }
  };

  return { popup, toggle, show, hide, getState, setState, destroy };
}

export { LithiumEditorFooter as L, createFontPopup as c };
