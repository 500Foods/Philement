/**
 * Session Log Manager
 *
 * Utility Manager — always available from the sidebar footer (fa-scroll).
 * Displays the current session's client-side action log using the same
 * CodeMirror viewer as the login panel's System Logs subpanel.
 *
 * Additional features over the login panel viewer:
 *   - Refresh button — re-reads getRawLog() and updates editor content
 *   - Coverage link button — opens /coverage/index.html in a new tab
 *   - Clear archived sessions button — removes previous-session archives
 *     from localStorage (window.lithiumLogs.archived)
 */

import { log, getRawLog, getArchivedSessions, removeArchivedSession, Subsystems, Status } from '../../core/log.js';
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
  }

  /**
   * Initialize the session log manager
   */
  async init() {
    log(SESSIONLOG, Status.INFO, 'Session Log manager initializing');
    await this.render();
    this._injectSlotHeaderButtons();
    this.setupEventListeners();
    await this.populateLog();
    this.show();
    log(SESSIONLOG, Status.SUCCESS, 'Session Log manager ready');
  }

  /**
   * Render the session log page (workspace content only — no toolbar).
   * Action buttons are injected into the slot header via _injectSlotHeaderButtons().
   */
  async render() {
    try {
      const response = await fetch('/src/managers/session-log/session-log.html');
      const html = await response.text();
      this.container.innerHTML = html;
    } catch (error) {
      this.renderFallback();
    }

    // Cache workspace DOM elements (buttons are in the slot header, not here)
    this.elements = {
      page: this.container.querySelector('#session-log-page'),
      viewer: this.container.querySelector('#session-log-viewer'),
      // refreshBtn, coverageBtn, clearBtn will be populated by _injectSlotHeaderButtons
    };
  }

  /**
   * Inject the Refresh, Coverage, and Clear buttons into the slot header via
   * MainManager.addHeaderButtons().  This keeps all buttons in one unified
   * subpanel-header-group — no separate toolbar, no visual break.
   *
   * Falls back gracefully if MainManager is not available (e.g. tests).
   */
  _injectSlotHeaderButtons() {
    const mainMgr = this.app?._getMainManager?.();
    if (!mainMgr) {
      return;
    }

    const slotId = mainMgr._utilitySlotId('session-log');

    mainMgr.addHeaderButtons(slotId, [
      {
        id:      'session-log-refresh-btn',
        icon:    'fa-rotate',
        title:   'Refresh log',
        tooltip: 'Refresh',
        onClick: () => {
          log(SESSIONLOG, Status.INFO, 'Button clicked: Refresh');
          this.refreshLog();
        },
      },
      {
        id:      'session-log-coverage-btn',
        icon:    'fa-chart-simple-horizontal',
        title:   'View coverage report',
        tooltip: 'Coverage',
        onClick: () => {
          log(SESSIONLOG, Status.INFO, 'Button clicked: Coverage report');
          window.open('/coverage/index.html', '_blank');
        },
      },
      {
        id:      'session-log-clear-btn',
        icon:    'fa-trash-can',
        title:   'Clear archived sessions',
        tooltip: 'Clear archived',
        onClick: () => {
          log(SESSIONLOG, Status.INFO, 'Button clicked: Clear archived sessions');
          this.clearArchivedSessions();
        },
      },
    ]);

    // Cache references to the injected buttons now that they're in the DOM
    const area = mainMgr.elements?.managerArea;
    if (area) {
      const slot = area.querySelector(`#${slotId}`);
      if (slot) {
        this.elements.refreshBtn  = slot.querySelector('#session-log-refresh-btn');
        this.elements.coverageBtn = slot.querySelector('#session-log-coverage-btn');
        this.elements.clearBtn    = slot.querySelector('#session-log-clear-btn');
      }
    }
  }

  /**
   * Minimal fallback if template fetch fails (no toolbar — buttons are in slot header)
   */
  renderFallback() {
    this.container.innerHTML = `
      <div id="session-log-page">
        <div class="session-log-viewer" id="session-log-viewer">
          <div class="log-placeholder"><p>Loading session log...</p></div>
        </div>
      </div>
    `;
  }

  /**
   * Set up event listeners.
   * Note: action button listeners are wired inline in _injectSlotHeaderButtons();
   * this method handles any remaining workspace-level listeners.
   */
  setupEventListeners() {
    // Action button listeners are already attached in _injectSlotHeaderButtons().
    // Add any workspace-level event listeners here if needed in the future.
  }

  /**
   * Refresh the log by re-reading the buffer and updating the CodeMirror editor.
   * If the editor is not yet initialized, fall back to a full populate.
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
   * Clear all previous-session log archives from localStorage
   */
  clearArchivedSessions() {
    const archives = getArchivedSessions();
    if (archives.length === 0) {
      log(SESSIONLOG, Status.INFO, 'No archived sessions to clear');
      return;
    }
    for (const { key } of archives) {
      removeArchivedSession(key);
    }
    log(SESSIONLOG, Status.SUCCESS, `Cleared ${archives.length} archived session(s) from localStorage`);
  }

  /**
   * Build the display text from the raw log buffer.
   * Mirrors the login panel's populateLogsPanel() rendering logic exactly.
   * @returns {string} Formatted log text for CodeMirror
   */
  _buildLogText() {
    const entries = getRawLog();

    if (entries.length === 0) {
      return '(No log entries yet)';
    }

    // Determine max subsystem width for fixed-width column alignment.
    // Bracketed entries (EventBus) are excluded from this calculation because their
    // brackets already account for the two-space column separator.
    const maxSubsystemLen = entries.reduce((max, e) => {
      const s = e.subsystem || '';
      return s.startsWith('[') ? max : Math.max(max, s.length);
    }, 0);

    // Build display lines, expanding grouped entries ({ title, items }) to multiple lines.
    const lines = [];
    for (const entry of entries) {
      const date = new Date(entry.timestamp);
      const time = String(date.getHours()).padStart(2, '0') + ':' +
        String(date.getMinutes()).padStart(2, '0') + ':' +
        String(date.getSeconds()).padStart(2, '0') + '.' +
        String(date.getMilliseconds()).padStart(3, '0');
      const raw = entry.subsystem || '';
      const isBracketed = raw.startsWith('[');
      const subsystem = isBracketed ? raw : raw.padEnd(maxSubsystemLen);
      const pre = isBracketed ? ' ' : '  ';
      const sep = isBracketed ? ' ' : '  ';
      const desc = entry.description;

      if (desc && typeof desc === 'object' && desc.title !== undefined && Array.isArray(desc.items)) {
        // Grouped entry: render title + ― continuation lines
        lines.push(`${time}${pre}${subsystem}${sep}${desc.title}`);
        for (const item of desc.items) {
          lines.push(`${time}${pre}${subsystem}${sep}― ${item}`);
        }
      } else {
        lines.push(`${time}${pre}${subsystem}${sep}${desc}`);
      }
    }

    return lines.join('\n');
  }

  /**
   * Populate the log viewer with the current session log using CodeMirror.
   * If CodeMirror is already initialized, updates the content in-place.
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

    // Initialize CodeMirror — same config as login panel System Logs subpanel
    try {
      const [
        { EditorView, lineNumbers, highlightActiveLine, highlightActiveLineGutter, drawSelection, keymap },
        { EditorState },
        { defaultKeymap, historyKeymap, history },
        { oneDark },
        { sql },
      ] = await Promise.all([
        import('@codemirror/view'),
        import('@codemirror/state'),
        import('@codemirror/commands'),
        import('@codemirror/theme-one-dark'),
        import('@codemirror/lang-sql'),
      ]);

      // Custom 4-digit zero-padded line number formatter
      const zeroPaddedLineNumbers = lineNumbers({
        formatNumber: (n) => String(n).padStart(4, '0'),
      });

      const extensions = [
        // Visual chrome
        zeroPaddedLineNumbers,
        highlightActiveLine(),
        highlightActiveLineGutter(),
        drawSelection(),
        history(),
        keymap.of([...defaultKeymap, ...historyKeymap]),

        // SQL language (pre-warms module; also highlights any SQL snippets in logs)
        sql(),

        // Dark theme
        oneDark,

        // Read-only
        EditorState.readOnly.of(true),

        // Sizing and font
        EditorView.theme({
          '&': {
            height: '100%',
            fontSize: '10px',
            fontFamily: "'Vanadium Mono', 'Courier New', Courier, monospace",
          },
          '.cm-scroller': {
            overflow: 'auto',
            scrollbarWidth: 'thin',
            scrollbarColor: '#555 #222',
          },
          '.cm-scroller::-webkit-scrollbar': { width: '8px', height: '8px' },
          '.cm-scroller::-webkit-scrollbar-track': { background: '#1e1e2e' },
          '.cm-scroller::-webkit-scrollbar-thumb': { background: '#555', borderRadius: '4px' },
          '.cm-scroller::-webkit-scrollbar-corner': { background: '#1e1e2e' },
          '.cm-content': { whiteSpace: 'pre' },
        }),
      ];

      const state = EditorState.create({ doc: logText, extensions });
      viewer.innerHTML = '';
      this._logEditor = new EditorView({ state, parent: viewer });
    } catch (error) {
      viewer.innerHTML = `<pre class="log-content" style="padding:1em;font-size:10px;overflow:auto;height:100%;margin:0;">${logText}</pre>`;
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
   * Teardown the session log manager
   */
  teardown() {
    // Destroy the CodeMirror editor if present
    if (this._logEditor) {
      this._logEditor.destroy();
      this._logEditor = null;
    }

    // Clear references
    this.elements = {};
  }
}
