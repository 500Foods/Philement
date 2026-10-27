/**
 * Course Builder Run tab — human gates (CATCHUP Phase 21).
 *
 * The left suggestion list (Lookup 059 key 15) is the queue. This tab
 * shows the selected suggestion's run: state, artifacts (4524), and
 * ledger events (4525), plus the gate buttons for that state. Buttons
 * call Build.Accept / Build.Decline / Build.Approve / Build.Abandon
 * through invokeScript with { run_id }. Nothing here reaches research
 * except the human Approve. There is no Research button.
 */

import { log, Subsystems, Status } from '../../core/log.js';
import { authQuery, invokeScript } from '../../shared/conduit.js';
import { confirmDialog } from '../../shared/confirm.js';
import { toast } from '../../shared/toast.js';
import { staffOutcome } from '../course-manager/course-learners.js';
import {
  RUN_EVENTS_QUERY_REF,
  RUN_GET_QUERY_REF,
  RUN_LIST_QUERY_REF,
  buildRunIdParams,
  buildRunListParams,
} from './course-build-runs.js';

export const GATE_SCRIPTS = {
  accept: 'Build.Accept',
  decline: 'Build.Decline',
  approve: 'Build.Approve',
  abandon: 'Build.Abandon',
};

/** P20a human gates. state -> buttons, in display order. */
export const GATE_ACTIONS = {
  submission: [
    { key: 'accept', label: 'Accept', script: GATE_SCRIPTS.accept, to: 'accepted', confirm: false },
    { key: 'decline', label: 'Decline', script: GATE_SCRIPTS.decline, to: 'declined', confirm: true },
  ],
  approve: [
    { key: 'approve', label: 'Approve', script: GATE_SCRIPTS.approve, to: 'research', confirm: true },
    { key: 'abandon', label: 'Abandon', script: GATE_SCRIPTS.abandon, to: 'abandoned', confirm: true },
  ],
};

export const EVENT_LIMIT = 10;

/**
 * @param {string} state
 * @returns {Array<object>}
 */
export function gateActionsFor(state) {
  const key = String(state || '').trim().toLowerCase();
  return GATE_ACTIONS[key] ? GATE_ACTIONS[key].slice() : [];
}

/**
 * invokeScript options for one gate button.
 * @param {string} script
 * @param {number|string} runId
 */
export function buildGateScriptOptions(script, runId) {
  const id = Number(runId);
  if (!Number.isInteger(id) || id < 1) throw new Error('run_id is required');
  if (!Object.values(GATE_SCRIPTS).includes(script)) throw new Error(`Unknown gate script ${script}`);
  return { script, params: { run_id: id }, wait: true, timeoutSeconds: 15 };
}

/**
 * Normalise the script envelope. Business failures come back as
 * { ok:false, code, message } from Build.Gate.
 */
export function gateOutcome(envelope) {
  return staffOutcome(envelope);
}

/**
 * @param {Array<object>} rows - 4523 rows
 * @param {number} suggestionId
 */
export function findRunForSuggestion(rows, suggestionId) {
  if (!Array.isArray(rows)) return null;
  const id = Number(suggestionId);
  return rows.find((row) => Number(row && row.suggestion_id) === id) || null;
}

/**
 * 4524 returns one row per artifact (LEFT JOIN). Split into run + list.
 * @param {Array<object>} rows
 */
export function splitRunDetail(rows) {
  if (!Array.isArray(rows) || rows.length === 0) return { run: null, artifacts: [] };
  const run = { ...rows[0] };
  const artifacts = rows
    .filter((row) => row && row.artifact_id !== null && row.artifact_id !== undefined)
    .map((row) => ({
      artifact_id: row.artifact_id,
      kind: row.artifact_kind,
      module_index: row.module_index,
      body: row.artifact_body,
      model_id: row.model_id,
      created_at: row.artifact_created_at,
    }));
  return { run, artifacts };
}

/** Newest first, capped. */
export function recentEvents(rows, limit = EVENT_LIMIT) {
  if (!Array.isArray(rows)) return [];
  return rows.slice().reverse().slice(0, limit);
}

function node(tag, className, text) {
  const el = document.createElement(tag);
  if (className) el.className = className;
  if (text !== undefined && text !== null) el.textContent = String(text);
  return el;
}

function preview(value, max = 160) {
  const text = value === null || value === undefined ? '' : String(value);
  return text.length > max ? `${text.slice(0, max)}…` : text;
}

/**
 * Loads and paints the Run tab for the manager's selected suggestion.
 * @param {object} mgr - CourseBuilderManager
 */
export async function loadRunPanel(mgr) {
  const host = mgr.elements && mgr.elements.runPane;
  if (!host) return;
  const suggestionId = mgr._selectedId;
  const api = mgr.app && mgr.app.api;
  host.replaceChildren();
  if (!Number.isInteger(suggestionId) || !api) {
    host.append(node('p', 'builder-run-empty', 'Select a suggestion.'));
    return;
  }
  host.append(node('p', 'builder-run-empty', 'Loading run…'));
  try {
    const list = await authQuery(api, RUN_LIST_QUERY_REF, buildRunListParams(''));
    const listed = findRunForSuggestion(list, suggestionId);
    if (mgr._selectedId !== suggestionId) return;
    if (!listed) {
      host.replaceChildren(node('p', 'builder-run-empty', 'No run yet. The run starts when the receipt is sent.'));
      return;
    }
    const params = buildRunIdParams(listed.run_id);
    const [detailRows, eventRows] = await Promise.all([
      authQuery(api, RUN_GET_QUERY_REF, params),
      authQuery(api, RUN_EVENTS_QUERY_REF, params),
    ]);
    if (mgr._selectedId !== suggestionId) return;
    const { run, artifacts } = splitRunDetail(detailRows);
    paintRunPanel(mgr, run || listed, artifacts, recentEvents(eventRows));
  } catch (error) {
    log(Subsystems.MANAGER, Status.ERROR, `Course Builder run load failed: ${error.message}`);
    host.replaceChildren(node('p', 'builder-run-error', `Run failed to load: ${error.message}`));
  }
}

/**
 * @param {object} mgr
 * @param {object} run
 * @param {Array<object>} artifacts
 * @param {Array<object>} events
 */
export function paintRunPanel(mgr, run, artifacts, events) {
  const host = mgr.elements.runPane;
  host.replaceChildren();
  const head = node('div', 'builder-run-head');
  head.append(
    node('span', 'builder-run-id', `Run ${run.run_id}`),
    node('span', 'builder-run-state', String(run.state || '')),
  );
  if (run.error_text) head.append(node('span', 'builder-run-error', run.error_text));
  host.append(head);

  const actions = node('div', 'builder-run-actions');
  for (const action of gateActionsFor(run.state)) {
    const btn = node('button', 'btn btn-sm btn-secondary builder-run-btn', action.label);
    btn.type = 'button';
    btn.dataset.gate = action.key;
    btn.addEventListener('click', () => runGate(mgr, action, run.run_id));
    actions.append(btn);
  }
  if (actions.childElementCount === 0) {
    actions.append(node('span', 'builder-run-none', 'No human action at this state.'));
  }
  host.append(actions);

  host.append(node('h4', 'builder-run-sub', `Artifacts (${artifacts.length})`));
  const artList = node('ul', 'builder-run-list');
  for (const art of artifacts) {
    const item = node('li', 'builder-run-item');
    const label = art.module_index !== null && art.module_index !== undefined
      ? `${art.kind} #${art.module_index}` : String(art.kind || '');
    item.append(node('strong', '', label), node('span', 'builder-run-body', preview(art.body)));
    artList.append(item);
  }
  host.append(artList);

  host.append(node('h4', 'builder-run-sub', 'Recent events'));
  const evList = node('ul', 'builder-run-list');
  for (const ev of events) {
    evList.append(node('li', 'builder-run-item',
      `${ev.created_at || ''} — ${ev.action_name || ''} (actor ${ev.actor_account_id ?? '—'})`));
  }
  host.append(evList);
}

/**
 * @param {object} mgr
 * @param {object} action - entry from GATE_ACTIONS
 * @param {number} runId
 */
export async function runGate(mgr, action, runId) {
  if (mgr._busy) return;
  if (action.confirm) {
    const ok = await confirmDialog({
      title: `${action.label} run ${runId}?`,
      message: `This moves the run to ${action.to}.`,
      confirmLabel: action.label,
      cancelLabel: 'Cancel',
    });
    if (!ok) return;
  }
  const api = mgr.app && mgr.app.api;
  mgr._busy = true;
  try {
    const envelope = await invokeScript(api, buildGateScriptOptions(action.script, runId));
    const result = gateOutcome(envelope);
    log(Subsystems.MANAGER, Status.INFO,
      `${action.script} run ${runId} ok=${result.ok} code=${result.ok ? '' : result.code}`);
    if (!result.ok) {
      toast.error(`${action.label} failed`, { description: result.message });
    } else {
      toast.success(`${action.label}: run ${runId} is ${result.body.state || action.to}`);
    }
  } catch (error) {
    toast.error(`${action.label} failed`, { description: error.message });
  } finally {
    mgr._busy = false;
  }
  await mgr._reload?.(mgr._selectedId);
  await loadRunPanel(mgr);
}
