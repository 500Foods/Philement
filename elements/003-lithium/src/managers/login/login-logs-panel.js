/**
 * Login Logs Panel
 *
 * Manages the logs sub-panel on the login page: renders the action log via
 * a CodeMirror editor in read-only mode, with a `<pre>` fallback if CodeMirror
 * fails to load. Extracted from login.js to keep files under 1,000 lines per
 * LITHIUM-INS.md rule #2.
 */

import { getRawLog } from '../../core/log.js';
import { formatLogText } from '../../shared/log-formatter.js';
import { scrollbarManager } from '../../core/scrollbar-manager.js';

/**
 * LogsPanel class
 *
 * Owns the CodeMirror editor lifecycle for the logs panel. Follows the
 * show/hide/destroy pattern established by PasswordManager and LanguagePanel.
 *
 * The panel has trivial show/hide semantics: `show()` populates (or refreshes)
 * the editor; `hide()` is a no-op (the editor stays alive in case the user
 * reopens the panel). `destroy()` tears everything down.
 */
export class LogsPanel {
  /**
   * @param {Object} options
   * @param {Object} options.elements - DOM element references; must contain `logViewer`
   * @param {Object} [options.imports] - Optional import overrides for testing.
   *   `imports.codemirror` and `imports.codemirrorSetup` may be supplied as
   *   pre-resolved modules; otherwise they are dynamically imported on first
   *   `show()`.
   */
  constructor({ elements, imports } = {}) {
    this.elements = elements || {};
    this._imports = imports || null;
    this._editor = null;
  }

  /**
   * Populate (or refresh) the logs panel. Called when the panel becomes visible.
   * Idempotent: subsequent calls update the editor's content rather than
   * reinitialising the editor.
   */
  async show() {
    const logViewer = this.elements.logViewer;
    if (!logViewer) {
      console.warn('[LogsPanel] logViewer element not found');
      return;
    }

    const entries = getRawLog();
    const logText = formatLogText(entries);

    // If CodeMirror is already initialised, just update the content.
    if (this._editor) {
      this._editor.dispatch({
        changes: { from: 0, to: this._editor.state.doc.length, insert: logText }
      });
      return;
    }

    // First-time init: try CodeMirror, fall back to <pre> on import failure.
    try {
      const cm = this._imports?.codemirror
        ? this._imports.codemirror
        : await import('../../core/codemirror.js');
      const cmSetup = this._imports?.codemirrorSetup
        ? this._imports.codemirrorSetup
        : await import('../../core/codemirror-setup.js');

      const { EditorState, EditorView } = cm;
      const { buildEditorExtensions, createReadOnlyCompartment } = cmSetup;

      const roCompartment = createReadOnlyCompartment();
      const extensions = buildEditorExtensions({
        language: 'log',
        readOnlyCompartment: roCompartment,
        readOnly: true,
        fontSize: 10,
        fontFamily: "'Vanadium Mono', 'Courier New', Courier, monospace",
      });

      const state = EditorState.create({ doc: logText, extensions });

      logViewer.innerHTML = '';
      this._editor = new EditorView({ state, parent: logViewer });
    } catch (error) {
      console.warn('[LogsPanel] CodeMirror failed to load, using plain text:', error);
      logViewer.innerHTML = `<pre class="log-content">${logText}</pre>`;
    }
  }

  /**
   * Called when the logs panel is being hidden. Currently a no-op:
   * the editor instance persists so reopening is instant.
   */
  hide() {
    // Intentional no-op.
  }

  /**
   * Destroy the editor and any associated scrollbar instance.
   * Called from LoginManager.teardown().
   */
  destroy() {
    // Clean up CodeMirror OverlayScrollbars first.
    if (this._editor && this._editor._osbInstance) {
      scrollbarManager.destroy(this._editor._osbInstance);
      this._editor._osbInstance = null;
    }

    // Destroy the editor view.
    if (this._editor) {
      this._editor.destroy();
      this._editor = null;
    }
  }

  /**
   * Test-only accessor for the editor instance. Avoid in production code.
   * @internal
   */
  get _editorInstance() {
    return this._editor;
  }
}
