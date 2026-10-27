/**
 * Course Builder suggestion list (CATCHUP Phase 19 / CB-S).
 *
 * Staff calls go through auth_query. 4515 lists suggestions.
 * 4514 writes the working title and one ledger row. 4516–4522
 * cover staff insert, edit, duplicate, delete, Intake, Suggested,
 * and flag.
 * The receipt email is sent by Build.ReceiveSuggested, which the
 * orchestrator submits. This screen does not call it.
 * 4000 stays the public insert. Cap token, site id, and IP are
 * not on this screen.
 * 4526 is the logged staff override (P22a): any P20a run state,
 * one 'override' ledger row, guarded on the state the screen saw.
 */

import { RUN_STATES } from './course-build-runs.js';

export const SUGGESTION_LIST_QUERY_REF = 4515;
export const WORKING_TITLE_QUERY_REF = 4514;
export const SUGGESTION_INSERT_QUERY_REF = 4516;
export const SUGGESTION_UPDATE_QUERY_REF = 4517;
export const SUGGESTION_DUPLICATE_QUERY_REF = 4518;
export const SUGGESTION_DELETE_QUERY_REF = 4519;
export const INTAKE_QUERY_REF = 4520;
export const SUGGESTED_QUERY_REF = 4522;
export const FLAG_QUERY_REF = 4521;
export const OVERRIDE_QUERY_REF = 4526;
export const BUILDER_TABLE_PATH = 'courses/course-builder';
export const BUILDER_TABLE_KEY_IDX = 15;
export const WORKING_TITLE_MAX = 100;
export const ANNOTATION_MAX = 250;

export const FIELD_LIMITS = {
  course_name: 100,
  summary: 500,
  detail: 20000,
  intended_audience: 500,
  additional_features: 500,
  submitter_name: 50,
  submitter_email: 50,
};

export const LIST_COLUMNS = [
  { field: 'working_title', title: 'Working Title' },
  { field: 'last_activity', title: 'Last activity' },
  { field: 'state_label', title: 'State' },
];

export const SUGGESTION_SEARCH_FIELDS = [
  'working_title',
  'course_name',
  'submitter_name',
  'submitter_email',
];

/**
 * Submission tab. Labels match the Reception Suggest a Course form.
 * Detail is the only HTML field.
 */
export const SUBMISSION_FIELDS = [
  { key: 'course_name', label: 'Suggested Course Name' },
  { key: 'summary', label: 'Provide a course summary' },
  { key: 'detail', label: 'Describe in detail the type of material the course should cover', html: true },
  { key: 'intended_audience', label: 'Describe the intended audience' },
  { key: 'additional_features', label: 'Describe the additional features the course should have' },
  { key: 'submitter_name', label: 'Your Name' },
  { key: 'submitter_email', label: 'Your Email' },
];

/**
 * @param {object|null|undefined} row
 * @param {string} name
 * @returns {*}
 */
export function rawCell(row, name) {
  if (!row || typeof row !== 'object') return undefined;
  if (Object.prototype.hasOwnProperty.call(row, name)) return row[name];
  const upper = name.toUpperCase();
  if (Object.prototype.hasOwnProperty.call(row, upper)) return row[upper];
  return undefined;
}

/**
 * @param {object|null|undefined} row
 * @returns {object|null}
 */
export function suggestionRowData(row) {
  if (!row || typeof row !== 'object') return null;
  if (typeof row.getData === 'function') return row.getData() || null;
  return row;
}

/** Worker-driven P20a states. An override into one gets a red warning. */
export const AUTOMATIC_STATES = [
  'outline', 'research', 'build', 'checklist', 'publishing', 'announcing',
];

/**
 * @param {unknown} value
 * @returns {string}
 */
export function stateLabel(value) {
  const text = String(value ?? '').trim();
  if (text === '') return 'Suggested';
  const key = text.toLowerCase();
  if (RUN_STATES.includes(key)) return key.charAt(0).toUpperCase() + key.slice(1);
  return text;
}

/**
 * Stored run state. A missing draft is Suggested. Any P20a state is
 * returned as is, so research or declined no longer shows as Suggested.
 * @param {object|null|undefined} row
 * @returns {string}
 */
export function storedStage(row) {
  const draft = rawCell(row, 'draft_state');
  const source = draft == null || String(draft).trim() === ''
    ? rawCell(row, 'state_label')
    : draft;
  const key = String(source ?? '').trim().toLowerCase();
  if (RUN_STATES.includes(key)) return key;
  return 'suggested';
}

/**
 * Stage control. Normal moves first (Suggested, Intake), then an
 * "Override (staff)" group with every P20a state (4526, logged).
 * @param {unknown} stage
 * @returns {{ current: string, canMove: boolean, options: Array<{value: string, label: string}>,
 *   groups: Array<{label: string, options: Array<{value: string, label: string}>}> }}
 */
export function stageOptions(stage) {
  const optionLabel = (value) => (AUTOMATIC_STATES.includes(value) ? `${stateLabel(value)} (Auto)` : stateLabel(value));
  const key = String(stage ?? '').trim().toLowerCase();
  const current = RUN_STATES.includes(key) ? key : 'suggested';
  const early = ['suggested', 'received', 'intake'];
  const normalMoves = {
    suggested: ['suggested', 'intake'],
    received: ['suggested', 'received', 'intake'],
    intake: ['suggested', 'intake'],
  };
  const normal = (early.includes(current) ? normalMoves[current] : [current])
    .map((value) => ({ value, label: optionLabel(value) }));
  const override = RUN_STATES
    .filter((value) => value !== current)
    .map((value) => ({ value: `override:${value}`, label: optionLabel(value) }));
  const groups = [
    { label: '', options: normal },
    { label: 'Override (staff)', options: override },
  ];
  const options = [...normal, ...override];
  return { current, canMove: options.length > 1, options, groups };
}

/**
 * @param {unknown} value
 * @returns {{ override: boolean, state: string }}
 */
export function parseStageValue(value) {
  const text = String(value ?? '').trim().toLowerCase();
  if (text.startsWith('override:')) return { override: true, state: text.slice(9) };
  return { override: false, state: text };
}

/**
 * Manual stage change. Suggested clears the receipt mark.
 * Intake leaves that mark alone. Any other target is a logged staff
 * override on 4526 (FROM_STATE / TO_STATE).
 * @param {unknown} previous
 * @param {unknown} next
 * @returns {{ queryRef: number, override: boolean, from: string, to: string, danger: boolean, warning: string,
 *   title: string, message: string, confirmLabel: string, success: string } | null}
 */
export function stageMove(previous, next) {
  const from = String(previous ?? '').trim().toLowerCase();
  const parsed = parseStageValue(next);
  const to = parsed.state;
  if (!RUN_STATES.includes(to) || to === from) return null;
  if (!parsed.override && to === 'intake' && (from === 'suggested' || from === 'received')) {
    return {
      queryRef: INTAKE_QUERY_REF,
      override: false,
      from,
      to,
      danger: false,
      warning: '',
      title: 'Move to Intake',
      message: 'Move this suggestion to Intake. The receipt email stays as it is.',
      confirmLabel: 'Move to Intake',
      success: 'Moved to Intake',
    };
  }
  if (!parsed.override && to === 'suggested' && (from === 'received' || from === 'intake')) {
    return {
      queryRef: SUGGESTED_QUERY_REF,
      override: false,
      from,
      to,
      danger: false,
      warning: '',
      title: 'Move to Suggested',
      message: 'Move this suggestion to Suggested. The receipt mark is cleared and the email is sent again.',
      confirmLabel: 'Move to Suggested',
      success: 'Moved to Suggested',
    };
  }
  const label = stateLabel(to);
  const automatic = AUTOMATIC_STATES.includes(to);
  const parts = [
    `Staff override: set this run from ${stateLabel(from)} to ${label}. One override row is written to the ledger.`,
  ];
  if (to === 'suggested') {
    parts.push('The receipt mark is cleared and the email is sent again.');
  }
  return {
    queryRef: OVERRIDE_QUERY_REF,
    override: true,
    from,
    to,
    danger: automatic,
    warning: automatic
      ? `Warning: ${label} is an automatic stage. Workers may act on this run as soon as it lands there.`
      : '',
    title: `Override to ${label}`,
    message: parts.join(' '),
    confirmLabel: `Set ${label}`,
    success: `Run set to ${label}`,
  };
}

/**
 * Params for 4526.
 * @param {unknown} suggestionId
 * @param {unknown} from
 * @param {unknown} to
 * @param {number} userId
 */
export function buildOverrideParams(suggestionId, from, to, userId) {
  const checked = requireSuggestionId(suggestionId);
  if (!checked.ok) return checked;
  const fromKey = String(from ?? '').trim().toLowerCase();
  const toKey = String(to ?? '').trim().toLowerCase();
  if (!RUN_STATES.includes(fromKey) || !RUN_STATES.includes(toKey)) {
    return { ok: false, message: 'Unknown run state.' };
  }
  return {
    ok: true,
    params: {
      INTEGER: { SUGGESTION_ID: checked.id, USERID: userId },
      STRING: { FROM_STATE: fromKey, TO_STATE: toKey },
    },
  };
}

/**
 * True when a 4526 response shows no row moved (stale screen or bad
 * state). Looks for affected_rows anywhere in the envelope; when none
 * is reported the move is treated as done.
 * @param {unknown} response
 * @returns {boolean}
 */
export function overrideMissed(response) {
  const counts = [];
  const walk = (node, depth) => {
    if (!node || typeof node !== 'object' || depth > 6) return;
    if (Array.isArray(node)) { node.forEach((item) => walk(item, depth + 1)); return; }
    Object.entries(node).forEach(([key, value]) => {
      if (key.toLowerCase() === 'affected_rows' && typeof value === 'number') counts.push(value);
      else walk(value, depth + 1);
    });
  };
  walk(response, 0);
  return counts.length > 0 && counts.every((n) => n === 0);
}

/**
 * @param {unknown} value
 * @param {boolean} active
 * @returns {string}
 */
export function receiptLabel(value, active) {
  if (!active) return '';
  const text = String(value ?? '').trim().toLowerCase();
  if (text === 'sent') return 'Receipt email: sent';
  if (text === 'failed') return 'Receipt email: failed';
  return 'Receipt email: not sent';
}

/**
 * @param {object|null|undefined} row
 * @returns {boolean}
 */
export function isFlagged(row) {
  return Number(rawCell(row, 'flagged')) === 1;
}

/**
 * @param {unknown} name
 * @returns {string}
 */
export function copyCourseName(name) {
  const suffix = ' (Copy)';
  const base = String(name ?? '').trim() || 'Suggestion';
  return `${base.slice(0, WORKING_TITLE_MAX - suffix.length)}${suffix}`;
}

/**
 * Editable working title. A stored draft wins. Otherwise the
 * submitted course name.
 * @param {object|null|undefined} row
 * @returns {string}
 */
export function workingTitleForEdit(row) {
  const draft = rawCell(row, 'draft_title');
  if (draft != null && String(draft) !== '') return String(draft);
  const shown = rawCell(row, 'working_title');
  if (shown != null && String(shown) !== '') return String(shown);
  const name = rawCell(row, 'course_name');
  return name == null ? '' : String(name);
}

/**
 * @param {object|null|undefined} row
 * @returns {string}
 */
export function lastActivity(row) {
  const activity = rawCell(row, 'last_activity');
  if (activity != null && activity !== '') return String(activity);
  const updated = rawCell(row, 'updated_at');
  const created = rawCell(row, 'created_at');
  const draft = rawCell(row, 'draft_title');
  if (draft != null && draft !== '' && updated != null && updated !== '') return String(updated);
  if (created != null && created !== '') return String(created);
  if (updated != null && updated !== '') return String(updated);
  return '';
}

/**
 * The three list cells, in display order.
 * @param {object|null|undefined} row
 * @returns {{ workingTitle: string, lastActivity: string, state: string }}
 */
export function suggestionListCells(row) {
  const state = rawCell(row, 'state_label');
  const stored = state == null || state === ''
    ? rawCell(row, 'draft_state') ?? rawCell(row, 'state')
    : state;
  return {
    workingTitle: workingTitleForEdit(row),
    lastActivity: lastActivity(row),
    state: stateLabel(stored),
  };
}

/**
 * @param {object|null|undefined} row
 * @returns {Object<string, string>}
 */
export function submissionValues(row) {
  const values = {};
  SUBMISSION_FIELDS.forEach((field) => {
    const value = rawCell(row, field.key);
    values[field.key] = value == null ? '' : String(value);
  });
  return values;
}

/**
 * Paint the seven read-only fields. Text fields use text nodes.
 * Detail is sanitized HTML.
 * @param {ParentNode} root
 * @param {object|null|undefined} row
 * @param {(html: string) => string} sanitize
 */
export function paintSubmission(root, row, sanitize) {
  const values = submissionValues(row);
  SUBMISSION_FIELDS.forEach((field) => {
    const node = root.querySelector(`[data-field="${field.key}"]`);
    if (!node) return;
    const text = values[field.key];
    if (field.html) {
      node.textContent = '';
      node.innerHTML = sanitize(text);
      return;
    }
    node.textContent = text;
  });
}

/**
 * @param {unknown} value
 * @returns {{ ok: true, title: string } | { ok: false, message: string }}
 */
export function validateWorkingTitle(value) {
  const title = String(value ?? '').trim();
  if (!title) return { ok: false, message: 'Working title is required.' };
  if (title.length > WORKING_TITLE_MAX) {
    return { ok: false, message: 'Working title is limited to 100 characters.' };
  }
  return { ok: true, title };
}

/**
 * @param {unknown} suggestionId
 * @param {unknown} title
 * @param {unknown} userId
 * @returns {{ ok: true, title: string, params: object } | { ok: false, message: string }}
 */
export function buildWorkingTitleParams(suggestionId, title, userId) {
  const checked = validateWorkingTitle(title);
  if (!checked.ok) return checked;
  const id = Number(suggestionId);
  if (!Number.isInteger(id) || id <= 0) {
    return { ok: false, message: 'Select a suggestion.' };
  }
  const uid = Number(userId);
  return {
    ok: true,
    title: checked.title,
    params: {
      INTEGER: {
        SUGGESTION_ID: id,
        USERID: Number.isInteger(uid) ? uid : 0,
      },
      STRING: {
        WORKING_TITLE: checked.title,
      },
    },
  };
}

/**
 * @param {object|null|undefined} app
 * @returns {number}
 */
export function builderUserId(app) {
  const id = Number(app && app.auth && app.auth.user && app.auth.user.id);
  return Number.isInteger(id) ? id : 0;
}

/**
 * @param {unknown} value
 * @param {number} max
 * @returns {string}
 */
function clip(value, max) {
  return String(value ?? '').trim().slice(0, max);
}

/**
 * @param {Object<string, string>} values
 * @returns {{ ok: true, values: Object<string, string> } | { ok: false, message: string }}
 */
export function validateSubmission(values) {
  const courseName = clip(values && values.course_name, FIELD_LIMITS.course_name);
  const summary = clip(values && values.summary, FIELD_LIMITS.summary);
  if (!courseName) return { ok: false, message: 'Suggested course name is required.' };
  if (!summary) return { ok: false, message: 'A course summary is required.' };
  return {
    ok: true,
    values: {
      course_name: courseName,
      summary,
      detail: String(values.detail ?? '').slice(0, FIELD_LIMITS.detail),
      intended_audience: clip(values.intended_audience, FIELD_LIMITS.intended_audience),
      additional_features: clip(values.additional_features, FIELD_LIMITS.additional_features),
      submitter_name: clip(values.submitter_name, FIELD_LIMITS.submitter_name),
      submitter_email: clip(values.submitter_email, FIELD_LIMITS.submitter_email),
    },
  };
}

/**
 * @param {number} suggestionId
 * @param {number} userId
 * @returns {{ INTEGER: { SUGGESTION_ID: number, USERID: number } }}
 */
function idParams(suggestionId, userId) {
  return {
    INTEGER: {
      SUGGESTION_ID: suggestionId,
      USERID: userId,
    },
  };
}

/**
 * @param {unknown} suggestionId
 * @returns {{ ok: true, id: number } | { ok: false, message: string }}
 */
function requireSuggestionId(suggestionId) {
  const id = Number(suggestionId);
  if (!Number.isInteger(id) || id <= 0) return { ok: false, message: 'Select a suggestion.' };
  return { ok: true, id };
}

/**
 * @param {Object<string, string>} values
 * @returns {Object<string, string>}
 */
function submissionBinds(values) {
  return {
    COURSE_NAME: values.course_name,
    SUMMARY: values.summary,
    DETAIL: values.detail,
    INTENDED_AUDIENCE: values.intended_audience,
    ADDITIONAL_FEATURES: values.additional_features,
    SUBMITTER_NAME: values.submitter_name,
    SUBMITTER_EMAIL: values.submitter_email,
  };
}

/**
 * @param {Object<string, string>} values
 * @param {unknown} userId
 * @returns {{ ok: true, params: object, courseName: string } | { ok: false, message: string }}
 */
export function buildInsertParams(values, userId) {
  const checked = validateSubmission(values);
  if (!checked.ok) return checked;
  const uid = Number(userId);
  return {
    ok: true,
    courseName: checked.values.course_name,
    params: {
      INTEGER: { USERID: Number.isInteger(uid) ? uid : 0 },
      STRING: submissionBinds(checked.values),
    },
  };
}

/**
 * @param {unknown} suggestionId
 * @param {Object<string, string>} values
 * @param {unknown} title
 * @param {unknown} annotation
 * @param {unknown} userId
 * @returns {{ ok: true, params: object, title: string } | { ok: false, message: string }}
 */
export function buildUpdateParams(suggestionId, values, title, annotation, userId) {
  const idCheck = requireSuggestionId(suggestionId);
  if (!idCheck.ok) return idCheck;
  const checked = validateSubmission(values);
  if (!checked.ok) return checked;
  const titleCheck = validateWorkingTitle(title);
  if (!titleCheck.ok) return titleCheck;
  const uid = Number(userId);
  return {
    ok: true,
    title: titleCheck.title,
    params: {
      INTEGER: {
        SUGGESTION_ID: idCheck.id,
        USERID: Number.isInteger(uid) ? uid : 0,
      },
      STRING: {
        ...submissionBinds(checked.values),
        WORKING_TITLE: titleCheck.title,
        ANNOTATION: clip(annotation, ANNOTATION_MAX),
      },
    },
  };
}

/**
 * @param {unknown} suggestionId
 * @param {unknown} courseName
 * @param {unknown} userId
 * @returns {{ ok: true, courseName: string, params: object } | { ok: false, message: string }}
 */
export function buildDuplicateParams(suggestionId, courseName, userId) {
  const idCheck = requireSuggestionId(suggestionId);
  if (!idCheck.ok) return idCheck;
  const uid = Number(userId);
  const name = copyCourseName(courseName);
  return {
    ok: true,
    courseName: name,
    params: {
      ...idParams(idCheck.id, Number.isInteger(uid) ? uid : 0),
      STRING: { COURSE_NAME: name },
    },
  };
}

/**
 * @param {unknown} suggestionId
 * @param {unknown} userId
 * @returns {{ ok: true, params: object } | { ok: false, message: string }}
 */
export function buildIdParams(suggestionId, userId) {
  const idCheck = requireSuggestionId(suggestionId);
  if (!idCheck.ok) return idCheck;
  const uid = Number(userId);
  return { ok: true, params: idParams(idCheck.id, Number.isInteger(uid) ? uid : 0) };
}

/**
 * @param {HTMLSelectElement|null|undefined} select
 * @param {unknown} stage
 * @param {boolean} enabled
 */
export function fillStageSelect(select, stage, enabled) {
  if (!select) return;
  const model = stageOptions(stage);
  select.replaceChildren();
  model.groups.forEach((group) => {
    let parent = select;
    if (group.label) {
      parent = document.createElement('optgroup');
      parent.label = group.label;
      select.appendChild(parent);
    }
    group.options.forEach((option) => {
      const node = document.createElement('option');
      node.value = option.value;
      node.textContent = option.label;
      parent.appendChild(node);
    });
  });
  select.value = model.current;
  select.dataset.stage = model.current;
  select.disabled = !enabled || !model.canMove;
}

/**
 * @param {ParentNode} root
 * @param {Object<string, string>} values
 */
export function showSubmissionEditor(root, values) {
  SUBMISSION_FIELDS.forEach((field) => {
    const node = root.querySelector(`[data-field="${field.key}"]`);
    if (!node) return;
    node.textContent = '';
    const input = document.createElement(field.html ? 'textarea' : 'input');
    input.className = field.html ? 'input builder-area' : 'input';
    if (!field.html) input.type = 'text';
    input.maxLength = FIELD_LIMITS[field.key];
    input.value = values[field.key] || '';
    input.dataset.edit = field.key;
    input.autocomplete = 'off';
    node.appendChild(input);
  });
}

/**
 * @param {ParentNode} root
 * @returns {Object<string, string>}
 */
export function readSubmissionEditor(root) {
  const values = {};
  SUBMISSION_FIELDS.forEach((field) => {
    const input = root.querySelector(`[data-edit="${field.key}"]`);
    values[field.key] = input ? input.value : '';
  });
  return values;
}

/**
 * @param {object|null|undefined} envelope
 * @returns {{ ok: boolean, status: string, received: number, sent: number, failed: number }}
 */
export function receiptPassResult(envelope) {
  const status = envelope && envelope.status ? String(envelope.status) : '';
  let body = envelope ? envelope.result : null;
  if (typeof body === 'string') {
    try {
      body = JSON.parse(body);
    } catch {
      body = null;
    }
  }
  if (!body || typeof body !== 'object') body = {};
  return {
    ok: status === 'completed' && body.ok === true,
    status,
    received: Number(body.received) || 0,
    sent: Number(body.mail_sent) || 0,
    failed: Number(body.mail_failed) || 0,
  };
}
