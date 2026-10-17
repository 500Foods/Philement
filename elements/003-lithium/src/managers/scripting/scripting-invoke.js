/**
 * Scripting Run tab.
 *
 * Course Manager clones this module. The panel stays out of scripting.js.
 * Wait defaults to true. Timeout is an integer from 1 to 60, default 15.
 * Poll delays are 250, 500, 1000, then 2000 ms until a terminal status,
 * a GET 404, a row change, or close. There is no cancel control.
 */

import { EditorState, EditorView } from '../../core/codemirror.js';
import {
  buildEditorExtensions,
  createReadOnlyCompartment,
  createWordWrapCompartment,
  createBracketMatchCompartment,
  createSelectionHighlightCompartment,
  createCommentContinuationCompartment,
  createWhitespaceCompartment,
  createVirtualColumnsCompartment,
  createIndentUnitCompartment,
  setEditorEditable,
} from '../../core/codemirror-setup.js';
import { LithiumEditorFooter } from '../../core/manager-ui.js';
import { log, Subsystems, Status } from '../../core/log.js';
import { buildScriptPayload, getScriptJob, invokeScript } from '../../shared/conduit.js';
import { scriptInvokable } from './scripting-save.js';

export const POLL_DELAYS_MS = Object.freeze([250, 500, 1000, 2000]);
export const TERMINAL_JOB_STATUSES = Object.freeze(['completed', 'failed', 'killed', 'timeout']);
export const RUN_TIMEOUT_DEFAULT = 15;
export const RUN_TIMEOUT_MIN = 1;
export const RUN_TIMEOUT_MAX = 60;

const CANCEL_POLL_MS = 50;

/**
 * @param {number} attempt - Zero-based poll count
 * @returns {number}
 */
export function nextPollDelay(attempt) {
  const index = Math.max(0, attempt);
  if (index >= POLL_DELAYS_MS.length) return POLL_DELAYS_MS[POLL_DELAYS_MS.length - 1];
  return POLL_DELAYS_MS[index];
}

/**
 * @param {unknown} status
 * @returns {boolean}
 */
export function isTerminalJobStatus(status) {
  return TERMINAL_JOB_STATUSES.includes(status);
}

/**
 * @param {object|null|undefined} row
 * @returns {string}
 */
export function scriptRunName(row) {
  const groupName = row?.group_name == null ? '' : String(row.group_name).trim();
  const scriptName = row?.script_name == null ? '' : String(row.script_name).trim();
  if (!groupName || !scriptName) return '';
  return `${groupName}.${scriptName}`;
}

/**
 * @param {object|null|undefined} row
 * @returns {boolean}
 */
export function isRunEnabled(row) {
  return scriptInvokable(row?.invokable) === 1 && scriptRunName(row) !== '';
}

/**
 * @param {unknown} value
 * @returns {number}
 */
export function clampRunTimeout(value) {
  const parsed = Number.parseInt(String(value ?? ''), 10);
  if (!Number.isInteger(parsed)) return RUN_TIMEOUT_DEFAULT;
  if (parsed < RUN_TIMEOUT_MIN) return RUN_TIMEOUT_MIN;
  if (parsed > RUN_TIMEOUT_MAX) return RUN_TIMEOUT_MAX;
  return parsed;
}

/**
 * @param {unknown} error
 * @returns {string}
 */
function errorCode(error) {
  return error?.serverError?.error || error?.data?.error || '';
}

/**
 * @param {unknown} error
 * @returns {boolean}
 */
export function isScriptNotFound(error) {
  return Number(error?.status) === 404 && errorCode(error) === 'script_not_found';
}

/**
 * @param {unknown} error
 * @returns {boolean}
 */
export function isJobNotFound(error) {
  return Number(error?.status) === 404 && errorCode(error) === 'job_not_found';
}

/**
 * Drop a top-level params._hydrogen key. Nested keys are left alone.
 *
 * @param {object} params
 * @returns {{ params: object, stripped: boolean }}
 */
export function stripHydrogenParams(params) {
  if (!Object.prototype.hasOwnProperty.call(params, '_hydrogen')) {
    return { params, stripped: false };
  }
  const copy = { ...params };
  delete copy._hydrogen;
  return { params: copy, stripped: true };
}

/**
 * @param {unknown} text
 * @returns {{ ok: true, params: object, stripped: boolean } | { ok: false, message: string }}
 */
export function parseRunParams(text) {
  const source = String(text ?? '').trim();
  if (source === '') return { ok: true, params: {}, stripped: false };
  let parsed;
  try {
    parsed = JSON.parse(source);
  } catch {
    return { ok: false, message: 'Params must be a JSON object' };
  }
  if (parsed === null || typeof parsed !== 'object' || Array.isArray(parsed)) {
    return { ok: false, message: 'Params must be a JSON object' };
  }
  const stripped = stripHydrogenParams(parsed);
  return { ok: true, params: stripped.params, stripped: stripped.stripped };
}

/**
 * @param {object} args
 * @param {object|null|undefined} args.row
 * @param {unknown} args.paramsText
 * @param {boolean} args.wait
 * @param {unknown} args.timeoutSeconds
 * @returns {{ script: string, params: object, wait: boolean, timeout_seconds: number, stripped: boolean }}
 */
export function buildRunPayload({ row, paramsText, wait, timeoutSeconds }) {
  const parsed = parseRunParams(paramsText);
  if (!parsed.ok) throw runError('invalid_params', parsed.message);
  const payload = buildScriptPayload({
    script: scriptRunName(row),
    params: parsed.params,
    wait: Boolean(wait),
    timeoutSeconds: clampRunTimeout(timeoutSeconds),
  });
  return { ...payload, stripped: parsed.stripped };
}

/**
 * @param {object} [fields]
 * @param {string} [fields.script]
 * @param {string} [fields.status]
 * @param {string} [fields.jobId]
 * @param {string} [fields.body]
 * @returns {string}
 */
export function formatRunPane({ script = '', status = '', jobId = '', body = '' } = {}) {
  const lines = [
    `script: ${script}`,
    `status: ${status}`,
    `job_id: ${jobId}`,
  ];
  const text = body == null ? '' : String(body);
  if (text !== '') {
    lines.push('', text);
  }
  return lines.join('\n');
}

/**
 * @param {string} script
 * @param {object|null|undefined} envelope
 * @returns {string}
 */
export function formatJobEnvelope(script, envelope) {
  const status = envelope?.status || '';
  const jobId = envelope?.job_id || '';
  let body = '';
  if (envelope?.error) body = String(envelope.error);
  if (envelope?.result !== undefined && envelope?.result !== null) {
    const json = JSON.stringify(envelope.result, null, 2);
    const empty = json === '{}' || json === 'null';
    if (!empty) body = body ? `${body}\n\n${json}` : json;
    else if (!body) body = json;
  }
  return formatRunPane({ script, status, jobId, body });
}

/**
 * @param {string} script
 * @param {unknown} error
 * @returns {string}
 */
export function formatRunFailure(script, error) {
  if (isScriptNotFound(error)) {
    return formatRunPane({ script, status: 'not_runnable', jobId: '', body: 'Not runnable' });
  }
  if (isJobNotFound(error)) {
    return formatRunPane({ script, status: 'missing', jobId: '', body: 'Job not found' });
  }
  const message = error?.serverError?.message || error?.message || 'Run failed';
  return formatRunPane({ script, status: 'error', jobId: '', body: message });
}

/**
 * Poll until a terminal status, a GET 404, or cancellation.
 * The first wait is 250 ms. A pending or running job keeps the last delay.
 *
 * @param {object} args
 * @param {string} args.jobId
 * @param {(jobId: string) => Promise<object>} args.getJob
 * @param {(ms: number, isCancelled?: () => boolean) => Promise<void>} args.sleep
 * @param {() => boolean} [args.isCancelled]
 * @returns {Promise<{ stopped: boolean, missing?: boolean, notRunnable?: boolean, envelope?: object, error?: unknown }>}
 */
export async function pollScriptJob({ jobId, getJob, sleep, isCancelled }) {
  let attempt = 0;
  let outcome = null;
  while (outcome === null) {
    if (isCancelled?.()) {
      outcome = { stopped: true };
      break;
    }
    await sleep(nextPollDelay(attempt), isCancelled);
    attempt += 1;
    if (isCancelled?.()) {
      outcome = { stopped: true };
      break;
    }
    let envelope;
    try {
      envelope = await getJob(jobId);
    } catch (error) {
      if (Number(error?.status) === 404) {
        outcome = {
          stopped: false,
          missing: true,
          notRunnable: isScriptNotFound(error),
          error,
        };
        break;
      }
      throw error;
    }
    if (isCancelled?.()) {
      outcome = { stopped: true };
      break;
    }
    if (isTerminalJobStatus(envelope?.status)) {
      outcome = { stopped: false, envelope };
    }
  }
  return outcome;
}

/**
 * @param {number} ms
 * @param {() => boolean} [isCancelled]
 * @returns {Promise<void>}
 */
function delay(ms, isCancelled) {
  return new Promise((resolve) => {
    let poll = null;
    const timer = setTimeout(() => {
      if (poll) clearInterval(poll);
      resolve();
    }, ms);
    if (typeof isCancelled !== 'function') return;
    poll = setInterval(() => {
      if (!isCancelled()) return;
      clearTimeout(timer);
      clearInterval(poll);
      resolve();
    }, CANCEL_POLL_MS);
  });
}

/**
 * @param {string} code
 * @param {string} message
 * @returns {Error}
 */
function runError(code, message) {
  const error = new Error(message);
  error.code = code;
  return error;
}

/**
 * @param {string} script
 * @param {string} status
 * @param {string} [jobId]
 */
function logRun(script, status, jobId = '') {
  log(Subsystems.MANAGER, Status.INFO, `[Scripting] Run ${script} status=${status} job_id=${jobId}`);
}

/**
 * @param {object} manager
 * @param {object} [deps]
 * @returns {object}
 */
export function createScriptInvoke(manager, deps = {}) {
  const invoke = deps.invokeScript || invokeScript;
  const readJob = deps.getScriptJob || getScriptJob;
  const sleep = deps.sleep || delay;

  const panel = {
    manager,
    view: null,
    footer: null,
    generation: 0,
    running: false,
    mounted: false,
    button: null,
    waitInput: null,
    timeoutInput: null,
    result: null,
    pane: null,
    editorHost: null,
    footerHost: null,

    mount() {
      if (this.mounted) return;
      const root = this.manager.container;
      if (!root) return;
      this.button = root.querySelector('#scripting-run-btn');
      this.waitInput = root.querySelector('#scripting-run-wait');
      this.timeoutInput = root.querySelector('#scripting-run-timeout');
      this.result = root.querySelector('#scripting-run-result');
      this.pane = root.querySelector('#scripting-tab-run');
      this.editorHost = root.querySelector('#scripting-run-editor');
      this.footerHost = root.querySelector('#scripting-run-editor-footer');
      if (!this.button) return;
      this.mounted = true;
      this.button.addEventListener('click', this._onRunClick);
      this.timeoutInput?.addEventListener('change', this._onTimeoutChange);
      this.sync();
    },

    async show() {
      await this.ensureEditor();
      this.sync();
    },

    onRowChanged() {
      this.generation += 1;
      this._showText('');
      this._setRunning(false);
    },

    sync() {
      if (!this.button) return;
      this.button.disabled = this.running || !isRunEnabled(this.manager.currentScript);
    },

    async run() {
      const row = this.manager.currentScript;
      if (!isRunEnabled(row)) return;
      const generation = ++this.generation;
      const alive = () => generation === this.generation;
      const script = scriptRunName(row);
      this._setRunning(true);
      try {
        const built = buildRunPayload({
          row,
          paramsText: this.paramsText(),
          wait: this.waitChecked(),
          timeoutSeconds: this.timeoutValue(),
        });
        if (this.timeoutInput) this.timeoutInput.value = String(built.timeout_seconds);
        if (built.stripped) {
          log(Subsystems.MANAGER, Status.INFO, '[Scripting] Stripped reserved params._hydrogen');
        }
        const envelope = await invoke(this.manager.app?.api, {
          script: built.script,
          params: built.params,
          wait: built.wait,
          timeoutSeconds: built.timeout_seconds,
        });
        if (!alive()) return;
        this._showEnvelope(script, envelope);
        if (built.wait === false && envelope?.job_id && !isTerminalJobStatus(envelope.status)) {
          await this._poll(script, envelope.job_id, alive);
        }
      } catch (error) {
        if (!alive()) return;
        this._showText(formatRunFailure(script, error));
        const status = isScriptNotFound(error) ? 'not_runnable' : 'error';
        logRun(script, status);
      } finally {
        if (alive()) this._setRunning(false);
      }
    },

    destroy() {
      this.generation += 1;
      this._setRunning(false);
      this.button?.removeEventListener('click', this._onRunClick);
      this.timeoutInput?.removeEventListener('change', this._onTimeoutChange);
      this.mounted = false;
      this.footer?.destroy();
      this.footer = null;
      this.view?.destroy();
      this.view = null;
    },

    paramsText() {
      if (!this.view?.state?.doc) return '{}';
      return this.view.state.doc.toString();
    },

    waitChecked() {
      return this.waitInput ? this.waitInput.checked : true;
    },

    timeoutValue() {
      return clampRunTimeout(this.timeoutInput ? this.timeoutInput.value : RUN_TIMEOUT_DEFAULT);
    },

    async _poll(script, jobId, alive) {
      const polled = await pollScriptJob({
        jobId,
        getJob: (id) => readJob(this.manager.app?.api, id),
        sleep,
        isCancelled: () => !alive(),
      });
      if (!alive() || polled.stopped) return;
      if (polled.missing) {
        const status = polled.notRunnable ? 'not_runnable' : 'missing';
        const body = polled.notRunnable ? 'Not runnable' : 'Job not found';
        this._showText(formatRunPane({ script, status, jobId, body }));
        logRun(script, status, jobId);
        return;
      }
      this._showEnvelope(script, polled.envelope);
    },

    _showEnvelope(script, envelope) {
      this._showText(formatJobEnvelope(script, envelope));
      logRun(script, envelope?.status || '', envelope?.job_id || '');
    },

    _showText(text) {
      if (this.result) this.result.textContent = text;
    },

    _setRunning(running) {
      this.running = running;
      this.pane?.classList.toggle('is-running', running);
      if (this.pane) this.pane.setAttribute('aria-busy', running ? 'true' : 'false');
      this.sync();
    },

    async ensureEditor() {
      if (this.view || !this.editorHost) return;
      try {
        const readOnlyCompartment = createReadOnlyCompartment();
        const wordWrapCompartment = createWordWrapCompartment();
        const bracketMatchCompartment = createBracketMatchCompartment();
        const selectionHighlightCompartment = createSelectionHighlightCompartment();
        const commentContinuationCompartment = createCommentContinuationCompartment();
        const whitespaceCompartment = createWhitespaceCompartment();
        const virtualColumnsCompartment = createVirtualColumnsCompartment();
        const indentUnitCompartment = createIndentUnitCompartment();
        const keymap = this.manager.editHelper?.getCodeMirrorKeymapOptions?.() || {};
        const extensions = buildEditorExtensions({
          language: 'json',
          readOnlyCompartment,
          readOnly: false,
          fontSize: 14,
          wordWrapCompartment,
          bracketMatchCompartment,
          selectionHighlightCompartment,
          commentContinuationCompartment,
          whitespaceCompartment,
          virtualColumnsCompartment,
          indentUnitCompartment,
          wordWrap: false,
          bracketMatch: true,
          selectionHighlight: true,
          commentContinuation: true,
          showWhitespace: false,
          onUpdate: (update) => {
            if (update.selectionSet) {
              requestAnimationFrame(() => this.footer?.updateCursorPosition());
            }
          },
          ...keymap,
        });
        this.view = new EditorView({
          state: EditorState.create({ doc: '{}', extensions }),
          parent: this.editorHost,
        });
        setEditorEditable(this.view, readOnlyCompartment, true, this.editorHost);
        if (this.footerHost) {
          this.footer = new LithiumEditorFooter({
            container: this.footerHost,
            editorView: this.view,
            wordWrapCompartment,
            bracketMatchCompartment,
            selectionHighlightCompartment,
            commentContinuationCompartment,
            whitespaceCompartment,
            indentUnitCompartment,
            virtualColumnsCompartment,
            initialWordWrap: false,
            initialBracketMatch: true,
            initialSelectionHighlight: true,
            initialCommentContinuation: true,
            initialWhitespace: false,
            initialVirtualColumns: true,
            storageKey: 'editors.scripting.run',
          });
          this.footer.init();
        }
      } catch (error) {
        log(Subsystems.MANAGER, Status.ERROR, `[Scripting] Run editor failed: ${error.message}`);
      }
    },
  };

  panel._onRunClick = () => {
    panel._lastRun = panel.run();
  };
  panel._onTimeoutChange = () => {
    if (!panel.timeoutInput) return;
    panel.timeoutInput.value = String(clampRunTimeout(panel.timeoutInput.value));
  };
  return panel;
}
