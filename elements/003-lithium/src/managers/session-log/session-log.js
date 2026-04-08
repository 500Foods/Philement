/**
 * Session Log Manager
 *
 * Utility Manager — always available from the sidebar footer (fa-scroll).
 * Displays the current session's client-side action log using CodeMirror.
 *
 * Features:
 *   - Toolbar with Refresh, Font selector, and Coverage link
 *   - Animated refresh button (matches LithiumTable navigator)
 *   - Font settings popup (size, family, weight)
 *   - Auto-refresh and scroll-to-bottom when opened
 */

import { log, getRawLog, Subsystems, Status } from '../../core/log.js';
import { formatLogText } from '../../shared/log-formatter.js';
import { createFontPopup, initToolbars } from '../../core/manager-ui.js';
import { processIcons } from '../../core/icons.js';
import './session-log.css';

// Convenience alias for this module's subsystem
const SESSIONLOG = Subsystems.SESSIONLOG;

/**
 * Session Log Manager Class
 */
export default class SessionLogManager {
  constructor(app, container) {
    this.app = app;
    this.container = container;
    this.elements = {};
    this._logEditor = null;
    this._fontPopup = null;
    
    // Font settings state
    this._fontSize = 10;
    this._fontFamily = "'Vanadium Mono', 'Courier New', Courier, monospace";
    this._fontWeight = 'normal';
  }

  /**
   * Initialize the session log manager
   */
  async init() {
    log(SESSIONLOG, Status.INFO, 'Session Log manager initializing');
    await this.render();
    this.setupEventListeners();
    await this.populateLog();
    this.show();
    
    // Auto-refresh and scroll to bottom when opened
    await this.refreshLog();
    this.scrollToBottom();
    
    log(SESSIONLOG, Status.SUCCESS, 'Session Log manager ready');
  }

  /**
   * Render the session log page with toolbar and viewer.
   */
  async render() {
    try {
      const response = await fetch('/src/managers/session-log/session-log.html');
      const html = await response.text();
      this.container.innerHTML = html;
    } catch (error) {
      this.renderFallback();
    }

    // Cache DOM elements
    this.elements = {
      page: this.container.querySelector('#session-log-page'),
      toolbar: this.container.querySelector('#session-log-toolbar'),
      viewer: this.container.querySelector('#session-log-viewer'),
      refreshBtn: this.container.querySelector('#session-log-refresh-btn'),
      fontBtn: this.container.querySelector('#session-log-font-btn'),
      coverageBtn: this.container.querySelector('#session-log-coverage-btn'),
    };

    // Initialize toolbar collapse behavior
    initToolbars();
    processIcons(this.container);
  }

  /**
   * Set up event listeners for toolbar buttons.
   */
  setupEventListeners() {
    // Refresh button with animation
    this.elements.refreshBtn?.addEventListener('click', () => {
      log(SESSIONLOG, Status.INFO, 'Button clicked: Refresh');
      this.refreshLogWithAnimation();
    });

    // Font selector popup
    this.elements.fontBtn?.addEventListener('click', (e) => {
      log(SESSIONLOG, Status.INFO, 'Button clicked: Font');
      this.toggleFontPopup(e);
    });

    // Coverage link
    this.elements.coverageBtn?.addEventListener('click', () => {
      log(SESSIONLOG, Status.INFO, 'Button clicked: Coverage report');
      window.open('/coverage/index.html', '_blank');
    });
  }

  /**
   * Toggle the font popup.
   */
  toggleFontPopup() {
    if (!this._fontPopup) {
      this._initFontPopup();
    }
    this._fontPopupToggle?.();
  }

  /**
   * Initialize the font popup (created once and reused).
   */
  _initFontPopup() {
    const { popup, toggle, hide } = createFontPopup({
      anchor: this.elements.fontBtn,
      fontSize: this._fontSize,
      fontFamily: this._fontFamily,
      fontWeight: this._fontWeight,
      onSave: ({ fontSize, fontFamily, fontWeight }) => {
        this._fontSize = parseInt(fontSize, 10) || 10;
        this._fontFamily = fontFamily;
        this._fontWeight = fontWeight;
        this.applyFontSettings();
      },
    });

    this._fontPopup = popup;
    this._fontPopupToggle = toggle;
    this._fontPopupHide = hide;
  }

  /**
   * Apply current font settings to the CodeMirror editor.
   */
  applyFontSettings() {
    if (!this._logEditor) return;

    const editorEl = this._logEditor.dom;
    if (editorEl) {
      const scroller = editorEl.querySelector('.cm-scroller');
      const content = editorEl.querySelector('.cm-content');
      
      if (scroller) {
        scroller.style.fontSize = `${this._fontSize}px`;
        scroller.style.fontFamily = this._fontFamily;
        scroller.style.fontWeight = this._fontWeight;
      }
      
      if (content) {
        content.style.fontSize = `${this._fontSize}px`;
        content.style.fontFamily = this._fontFamily;
        content.style.fontWeight = this._fontWeight;
      }
    }

    log(SESSIONLOG, Status.INFO, `Font settings applied: ${this._fontSize}px, ${this._fontFamily}`);
  }

  /**
   * Refresh the log with spinning animation (matches LithiumTable navigator).
   */
  async refreshLogWithAnimation() {
    // Add spin animation class
    if (this.elements.refreshBtn) {
      this.elements.refreshBtn.classList.add('session-log-refresh-spin');
      setTimeout(() => {
        this.elements.refreshBtn?.classList.remove('session-log-refresh-spin');
      }, 750);
    }

    await this.refreshLog();
  }

  /**
   * Refresh the log by re-reading the buffer and updating the CodeMirror editor.
   */
  async refreshLog() {
    if (this._logEditor) {
      const logText = this._buildLogText();
      this._logEditor.dispatch({
        changes: { from: 0, to: this._logEditor.state.doc.length, insert: logText },
      });
      log(SESSIONLOG, Status.INFO, 'Log refreshed');
    } else {
      await this.populateLog();
    }
  }

  /**
   * Scroll to the bottom of the log viewer.
   */
  scrollToBottom() {
    if (!this._logEditor) return;

    // Use CodeMirror's scrollIntoView to go to the end of the document
    const docLength = this._logEditor.state.doc.length;
    this._logEditor.dispatch({
      effects: [],
      selection: { anchor: docLength },
      scrollIntoView: true,
    });

    log(SESSIONLOG, Status.DEBUG, 'Scrolled to bottom of log');
  }

  /**
   * Build the display text from the raw log buffer.
   * @returns {string} Formatted log text for CodeMirror
   */
  _buildLogText() {
    const entries = getRawLog();
    return formatLogText(entries);
  }

  /**
   * Populate the log viewer with the current session log using CodeMirror.
   */
  async populateLog() {
    const viewer = this.elements.viewer;
    if (!viewer) {
      return;
    }

    const logText = this._buildLogText();

    // If CodeMirror is already initialized, just update the content
    if (this._logEditor) {
      this._logEditor.dispatch({
        changes: { from: 0, to: this._logEditor.state.doc.length, insert: logText },
      });
      return;
    }

    // Initialize CodeMirror
    try {
      const { EditorState, EditorView } = await import('../../core/codemirror.js');
      const { buildEditorExtensions, createReadOnlyCompartment } = await import('../../core/codemirror-setup.js');

      const roCompartment = createReadOnlyCompartment();
      const extensions = buildEditorExtensions({
        language: 'log',
        readOnlyCompartment: roCompartment,
        readOnly: true,
        fontSize: this._fontSize,
        fontFamily: this._fontFamily,
      });

      const state = EditorState.create({ doc: logText, extensions });
      viewer.innerHTML = '';
      this._logEditor = new EditorView({ state, parent: viewer });
      
      // Apply current font settings
      this.applyFontSettings();
    } catch (error) {
      viewer.innerHTML = `<pre class="log-content" style="padding:1em;font-size:${this._fontSize}px;overflow:auto;height:100%;margin:0;font-family:${this._fontFamily};">${logText}</pre>`;
    }
  }

  /**
   * Show the session log page with fade-in
   */
  show() {
    requestAnimationFrame(() => {
      this.elements.page?.classList.add('visible');
    });
  }

  /**
   * Minimal fallback if template fetch fails
   */
  renderFallback() {
    this.container.innerHTML = `
      <div id="session-log-page">
        <div class="lithium-toolbar" id="session-log-toolbar">
          <button type="button" class="lithium-toolbar-btn" id="session-log-refresh-btn">
            <fa fa-arrows-rotate></fa>
            <span>Refresh</span>
          </button>
          <div class="lithium-toolbar-placeholder"></div>
          <button type="button" class="lithium-toolbar-btn" id="session-log-coverage-btn">
            <fa fa-chart-simple-horizontal></fa>
            <span>Coverage</span>
          </button>
        </div>
        <div class="session-log-viewer" id="session-log-viewer">
          <div class="log-placeholder"><p>Loading session log...</p></div>
        </div>
      </div>
    `;
  }

  /**
   * Teardown the session log manager
   */
  teardown() {
    // Hide font popup if open
    if (this._fontPopupHide) {
      this._fontPopupHide();
      this._fontPopupHide = null;
      this._fontPopup = null;
    }

    // Destroy the CodeMirror editor if present
    if (this._logEditor) {
      this._logEditor.destroy();
      this._logEditor = null;
    }

    // Clear references
    this.elements = {};
  }
}
