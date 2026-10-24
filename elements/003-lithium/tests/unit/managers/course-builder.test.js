/* globals Event */
import { readFileSync } from 'node:fs';
import { dirname, join } from 'node:path';
import { fileURLToPath } from 'node:url';
import { describe, it, expect, vi, beforeEach } from 'vitest';

const here = dirname(fileURLToPath(import.meta.url));
const listSql = readFileSync(join(
  here,
  '../../../../002-helium/courses/migrations/courses_4102.lua',
), 'utf8');
const saveSql = readFileSync(join(
  here,
  '../../../../002-helium/courses/migrations/courses_4100.lua',
), 'utf8');
const updateSql = readFileSync(join(
  here,
  '../../../../002-helium/courses/migrations/courses_4104.lua',
), 'utf8');
const insertSql = readFileSync(join(
  here,
  '../../../../002-helium/courses/migrations/courses_4103.lua',
), 'utf8');
const intakeSql = readFileSync(join(
  here,
  '../../../../002-helium/courses/migrations/courses_4107.lua',
), 'utf8');
const receiveSql = readFileSync(join(
  here,
  '../../../../002-helium/courses/migrations/courses_4110.lua',
), 'utf8');
const titleRewrite = readFileSync(join(
  here,
  '../../../../002-helium/courses/migrations/courses_4111.lua',
), 'utf8');
const editRewrite = readFileSync(join(
  here,
  '../../../../002-helium/courses/migrations/courses_4112.lua',
), 'utf8');
const copyRewrite = readFileSync(join(
  here,
  '../../../../002-helium/courses/migrations/courses_4113.lua',
), 'utf8');
const receiveRewrite = readFileSync(join(
  here,
  '../../../../002-helium/courses/migrations/courses_4114.lua',
), 'utf8');
const suggestedSql = readFileSync(join(
  here,
  '../../../../002-helium/courses/migrations/courses_4115.lua',
), 'utf8');
const keyRewrite = readFileSync(join(
  here,
  '../../../../002-helium/courses/migrations/courses_4116.lua',
), 'utf8');
const orchestrator = readFileSync(join(
  here,
  '../../../../002-helium/acuranzo/migrations/acuranzo_1401.lua',
), 'utf8');
const tableDef = readFileSync(join(
  here,
  '../../../../002-helium/acuranzo/migrations/acuranzo_1400.lua',
), 'utf8');
const previousDef = readFileSync(join(
  here,
  '../../../../002-helium/acuranzo/migrations/acuranzo_1399.lua',
), 'utf8');

const tableBag = { last: null };

vi.mock('../../../src/shared/conduit.js', () => ({
  authQuery: vi.fn(),
  invokeScript: vi.fn(async () => ({
    status: 'completed',
    result: { ok: true, received: 0, mail_sent: 0, mail_failed: 0 },
  })),
}));

vi.mock('../../../src/shared/confirm.js', () => ({
  confirmDialog: vi.fn(async () => false),
}));

vi.mock('../../../src/shared/toast.js', () => ({
  toast: { error: vi.fn(), info: vi.fn(), success: vi.fn() },
}));

vi.mock('../../../src/core/jwt.js', () => ({
  getClaims: vi.fn(() => null),
  retrieveJWT: vi.fn(() => 'stored-token'),
}));

vi.mock('../../../src/core/icons.js', () => ({
  processIcons: vi.fn(),
}));

vi.mock('../../../src/tables/lithium-table-main.js', () => ({
  default: class LithiumTable {
    constructor(options) {
      this.options = options;
      this.currentData = [];
      this.table = {
        setFilter: vi.fn(),
        clearFilter: vi.fn(),
        deselectRow: vi.fn(),
        selectRow: vi.fn(),
        getRows: vi.fn(() => []),
        print: vi.fn(),
        download: vi.fn(),
      };
      this.init = vi.fn(async () => {});
      this.loadData = vi.fn(async () => {
        this.options.onDataLoaded?.([]);
      });
      this.destroy = vi.fn();
      tableBag.last = this;
    }
  },
}));

import { authQuery, invokeScript } from '../../../src/shared/conduit.js';
import { confirmDialog } from '../../../src/shared/confirm.js';
import { toast } from '../../../src/shared/toast.js';
import { getClaims } from '../../../src/core/jwt.js';
import CourseBuilderManager from '../../../src/managers/course-builder/course-builder.js';
import {
  FLAG_QUERY_REF,
  INTAKE_QUERY_REF,
  SUGGESTED_QUERY_REF,
  SUGGESTION_DUPLICATE_QUERY_REF,
  SUGGESTION_INSERT_QUERY_REF,
  SUGGESTION_LIST_QUERY_REF,
  SUGGESTION_UPDATE_QUERY_REF,
  WORKING_TITLE_QUERY_REF,
  buildInsertParams,
  buildWorkingTitleParams,
  copyCourseName,
  paintSubmission,
  receiptLabel,
  receiptPassResult,
  stageMove,
  stageOptions,
  suggestionListCells,
  submissionValues,
} from '../../../src/managers/course-builder/course-suggestions.js';
import { sanitizeHtml } from '../../../src/core/utils.js';

const FIXTURE = {
  suggestion_id: 7,
  course_name: 'Calculus I',
  summary: 'Limits and derivatives.',
  detail: '<img src="x" onerror="alert(1)"><p>Limits</p>',
  intended_audience: 'First-year students',
  additional_features: 'Quizzes',
  submitter_name: 'Ada',
  submitter_email: 'ada@example.com',
  chacha_token: 'secret-token',
  chacha_site: 'site-key',
  ip_address: '203.0.113.8',
  created_at: '2026-10-01 12:00:00',
  draft_title: null,
  draft_state: null,
  working_title: 'Calculus I',
  last_activity: '2026-10-01 12:00:00',
  state_label: 'Suggested',
};

function mount() {
  const slot = document.createElement('div');
  slot.className = 'manager-slot';
  const footer = document.createElement('div');
  footer.className = 'manager-slot-footer';
  const group = document.createElement('div');
  group.className = 'subpanel-header-group';
  const placeholder = document.createElement('div');
  placeholder.className = 'slot-footer-placeholder';
  group.appendChild(placeholder);
  footer.appendChild(group);
  const container = document.createElement('div');
  slot.appendChild(container);
  slot.appendChild(footer);
  document.body.appendChild(slot);
  const app = {
    api: { post() {} },
    auth: { user: { id: 7 } },
  };
  return { slot, container, app, manager: new CourseBuilderManager(app, container) };
}

function staffQuery() {
  authQuery.mockImplementation(async (_api, ref) => {
    if (ref === 155) return [{ name: 'staff' }];
    return [];
  });
}

describe('suggestion display', () => {
  it('shows Working Title, Last activity, and Suggested for a fixture', () => {
    const bare = {
      course_name: 'Calculus I',
      created_at: '2026-10-01 12:00:00',
    };
    expect(suggestionListCells(bare)).toEqual({
      workingTitle: 'Calculus I',
      lastActivity: '2026-10-01 12:00:00',
      state: 'Suggested',
    });
    expect(suggestionListCells(FIXTURE).state).toBe('Suggested');
    expect(suggestionListCells({
      working_title: 'Working calculus',
      draft_title: 'Working calculus',
      last_activity: '2026-10-23 09:00:00',
      state_label: 'Suggested',
      course_name: 'Calculus I',
    })).toEqual({
      workingTitle: 'Working calculus',
      lastActivity: '2026-10-23 09:00:00',
      state: 'Suggested',
    });
  });

  it('paints the seven fields and does not run detail markup', () => {
    const root = document.createElement('div');
    root.innerHTML = `
      <div data-field="course_name"></div>
      <div data-field="summary"></div>
      <div data-field="detail"></div>
      <div data-field="intended_audience"></div>
      <div data-field="additional_features"></div>
      <div data-field="submitter_name"></div>
      <div data-field="submitter_email"></div>
    `;
    paintSubmission(root, FIXTURE, sanitizeHtml);
    expect(submissionValues(FIXTURE).course_name).toBe('Calculus I');
    expect(root.querySelector('[data-field="course_name"]').textContent).toBe('Calculus I');
    expect(root.querySelector('[data-field="summary"]').textContent).toBe('Limits and derivatives.');
    expect(root.querySelector('[data-field="intended_audience"]').textContent).toBe('First-year students');
    expect(root.querySelector('[data-field="additional_features"]').textContent).toBe('Quizzes');
    expect(root.querySelector('[data-field="submitter_name"]').textContent).toBe('Ada');
    expect(root.querySelector('[data-field="submitter_email"]').textContent).toBe('ada@example.com');
    const detail = root.querySelector('[data-field="detail"]');
    expect(detail.querySelector('script')).toBeNull();
    expect(detail.innerHTML.toLowerCase()).not.toContain('onerror');
    expect(detail.querySelector('p').textContent).toBe('Limits');
    expect(root.textContent).not.toContain('secret-token');
    expect(root.textContent).not.toContain('site-key');
    expect(root.textContent).not.toContain('203.0.113.8');
  });

  it('keeps course_name out of the working-title save', () => {
    const built = buildWorkingTitleParams(7, '  Working calculus  ', 4);
    expect(built.ok).toBe(true);
    expect(built.params).toEqual({
      INTEGER: { SUGGESTION_ID: 7, USERID: 4 },
      STRING: { WORKING_TITLE: 'Working calculus' },
    });
    expect(JSON.stringify(built.params)).not.toContain('course_name');
    expect(JSON.stringify(built.params)).not.toContain('Calculus I');
    expect(buildWorkingTitleParams(7, '   ', 4).ok).toBe(false);
    expect(buildWorkingTitleParams(7, 'x'.repeat(101), 4).ok).toBe(false);
    expect(saveSql).not.toMatch(/UPDATE\s+\$\{SCHEMA\}course_suggestions/);
    expect(saveSql).not.toMatch(/SET\s+course_name/);
    expect(saveSql).toContain('working_title');
    expect(saveSql).toContain('course_build_actions');
    expect(saveSql.match(/INSERT INTO \$\{SCHEMA\}course_build_actions/g)).toHaveLength(1);
    expect(listSql).not.toContain('chacha_token');
    expect(listSql).not.toContain('chacha_site');
    expect(listSql).not.toContain('ip_address');
    expect(listSql).toContain('course_name');
    expect(listSql).toContain("WHEN r.state = 'received' THEN 'Received'");
    expect(listSql).toContain("WHEN r.state = 'intake' THEN 'Intake'");
    expect(listSql).toContain('r.receipt_mail');
    expect(listSql).toContain('4515');
    expect(updateSql).toMatch(/UPDATE\s+\$\{SCHEMA\}course_suggestions/);
    expect(updateSql).not.toMatch(/receipt_mail\s*=/);
    expect(updateSql).toContain("'edit'");
    expect(insertSql).not.toContain('course_build_runs');
    expect(intakeSql).toContain("state = 'intake'");
    expect(intakeSql).not.toMatch(/receipt_mail\s*=/);
    expect(receiveSql).toContain('suggestion-receipt-');
    expect(receiveSql).toContain("receipt_mail = 'failed'");
    expect(receiveSql).toContain("receipt_mail = 'sent'");
    expect(receiveSql).toContain('receipt_mail IS NULL');
    expect(receiveSql).toContain('${FROM_ONE}');
    expect(receiveSql).toContain('CAST(:SUGGESTION_ID AS ${CAST_INTEGER})');
    expect(receiveSql).not.toMatch(/phase/i);
    expect(titleRewrite).toContain('CAST(:WORKING_TITLE AS ${CAST_TITLE})');
    expect(titleRewrite).not.toMatch(/UPDATE\s+\$\{SCHEMA\}course_suggestions/);
    expect(titleRewrite).not.toMatch(/SET\s+course_name/);
    expect(titleRewrite).toContain("'working_title'");
    expect(editRewrite).toContain('CAST(:WORKING_TITLE AS ${CAST_TITLE})');
    expect(editRewrite).toContain('CAST(:ANNOTATION AS ${CAST_NOTE})');
    expect(editRewrite).toMatch(/UPDATE\s+\$\{SCHEMA\}course_suggestions/);
    expect(editRewrite).toContain('detail = :DETAIL');
    expect(editRewrite).not.toMatch(/receipt_mail\s*=/);
    expect(copyRewrite).toContain('CAST(:COURSE_NAME AS ${CAST_TITLE})');
    const freshReceipt = receiveRewrite.slice(
      receiveRewrite.indexOf('local sql_new = [==['),
      receiveRewrite.indexOf(']==]', receiveRewrite.indexOf('local sql_new = [==[')),
    );
    expect(freshReceipt).toContain('local FROM_ADDRESS = ""');
    expect(freshReceipt).toContain('local REPLY_TO = ""');
    expect(freshReceipt).toContain('local actor = 0');
    expect(freshReceipt).toContain('suggestion-receipt-');
    expect(freshReceipt).not.toContain('Enroll.StaffGate');
    expect(receiveRewrite).toContain('invokable = 0');
    expect(receiveRewrite).toContain('Enroll.StaffGate');
    const freshOrch = orchestrator.slice(
      orchestrator.indexOf('local sql_new = [==['),
      orchestrator.indexOf(']==]', orchestrator.indexOf('local sql_new = [==[')),
    );
    expect(freshOrch).toContain('Build.ReceiveSuggested');
    expect(freshOrch).toContain('function submit_receipt');
    expect(freshOrch).toContain('prune_terminal');
    expect(freshOrch).toContain('EnsureCanvasUser');
    expect(freshOrch).toContain('r.receipt_mail IS NULL');
    expect(suggestedSql).toContain("state = 'suggested'");
    expect(suggestedSql).toContain('receipt_mail = NULL');
    expect(suggestedSql).toContain("state IN ('received', 'intake')");
    expect(suggestedSql).toContain("'suggested'");
    expect(suggestedSql).not.toMatch(/UPDATE\s+\$\{SCHEMA\}course_suggestions/);
    const freshKey = keyRewrite.slice(
      keyRewrite.indexOf('local sql_new = [==['),
      keyRewrite.indexOf(']==]', keyRewrite.indexOf('local sql_new = [==[')),
    );
    expect(freshKey).toContain('tostring(os.time())');
    expect(freshKey).toContain('suggestion-receipt-');
    expect(keyRewrite.slice(0, keyRewrite.indexOf('local sql_new = [==['))).not.toContain('os.time()');
    expect(keyRewrite).toContain('invokable = 0');
    expect(orchestrator.slice(0, orchestrator.indexOf('local sql_new = [==['))).not.toContain('submit_receipt');
    expect(previousDef).toContain('"queryRef": 4513');
    expect(previousDef).toContain('"readonly": true');
    expect(tableDef).toContain('"queryRef": 4515');
    expect(tableDef).toContain('"readonly": false');
    expect(tableDef).not.toContain('insertQueryRef');
    expect(tableDef).not.toContain('updateQueryRef');
    expect(tableDef).not.toContain('deleteQueryRef');
    expect(tableDef).not.toMatch(/phase/i);
    const workingAt = tableDef.indexOf('"title": "Working Title"');
    const activityAt = tableDef.indexOf('"title": "Last activity"');
    const stateAt = tableDef.indexOf('"title": "State"');
    expect(workingAt).toBeGreaterThan(-1);
    expect(workingAt).toBeLessThan(activityAt);
    expect(activityAt).toBeLessThan(stateAt);
  });

  it('labels receipt, stage, and a copied name', () => {
    expect(stageOptions('suggested').options.map((option) => option.label)).toEqual(['Suggested', 'Intake']);
    expect(stageOptions('received').options.map((option) => option.value)).toEqual(['suggested', 'received', 'intake']);
    expect(stageOptions('received').canMove).toBe(true);
    expect(stageOptions('intake').canMove).toBe(true);
    expect(stageOptions('intake').options.map((option) => option.value)).toEqual(['suggested', 'intake']);
    expect(stageMove('received', 'suggested').queryRef).toBe(SUGGESTED_QUERY_REF);
    expect(stageMove('intake', 'suggested').message).toContain('sent again');
    expect(stageMove('suggested', 'intake').queryRef).toBe(INTAKE_QUERY_REF);
    expect(stageMove('suggested', 'received')).toBeNull();
    expect(receiptLabel('sent', true)).toBe('Receipt email: sent');
    expect(receiptLabel('failed', true)).toBe('Receipt email: failed');
    expect(receiptLabel(null, true)).toBe('Receipt email: not sent');
    expect(receiptLabel('sent', false)).toBe('');
    expect(copyCourseName('Calculus I')).toBe('Calculus I (Copy)');
    expect(copyCourseName('x'.repeat(100))).toHaveLength(100);
    expect(receiptPassResult({
      status: 'completed',
      result: '{"ok":true,"received":2,"mail_sent":1,"mail_failed":1}',
    })).toEqual({
      ok: true,
      status: 'completed',
      received: 2,
      sent: 1,
      failed: 1,
    });
    const built = buildInsertParams({
      course_name: 'Staff idea',
      summary: 'From a conversation',
      detail: '',
      intended_audience: '',
      additional_features: '',
      submitter_name: '',
      submitter_email: '',
    }, 7);
    expect(built.ok).toBe(true);
    expect(built.params.STRING.COURSE_NAME).toBe('Staff idea');
    expect(JSON.stringify(built.params)).not.toContain('chacha');
    expect(suggestionListCells({ state_label: 'Received', working_title: 'A', last_activity: 't' }).state).toBe('Received');
  });
});

describe('CourseBuilder shell', () => {
  beforeEach(() => {
    document.body.innerHTML = '';
    authQuery.mockReset();
    invokeScript.mockReset();
    invokeScript.mockResolvedValue({
      status: 'completed',
      result: { ok: true, received: 0, mail_sent: 0, mail_failed: 0 },
    });
    confirmDialog.mockReset();
    confirmDialog.mockResolvedValue(false);
    toast.error.mockReset();
    toast.info.mockReset();
    toast.success.mockReset();
    getClaims.mockReturnValue({ roles: '2' });
    tableBag.last = null;
    authQuery.mockResolvedValue([]);
  });

  it('shows the suggestion table and the Submission tab for staff', async () => {
    staffQuery();
    const { container, manager } = mount();
    await manager.init();
    expect(container.textContent).not.toMatch(/lua/i);
    expect(container.textContent).not.toMatch(/phase/i);
    expect(container.textContent).not.toMatch(/Accept|Decline|Junk|Canvas/);
    expect(container.querySelector('#builder-table')).not.toBeNull();
    expect(container.querySelector('#builder-navigator')).not.toBeNull();
    const rightKids = [...container.querySelector('.builder-right-panel').children];
    expect(rightKids[0].id).toBe('builder-toolbar');
    expect(rightKids[1].classList.contains('builder-title-bar')).toBe(true);
    expect(container.querySelector('#builder-working-title').maxLength).toBe(100);
    expect(container.querySelector('#builder-working-title').disabled).toBe(true);
    expect(container.querySelector('[data-tab="submission"]').textContent).toContain('Submission');
    expect(container.querySelectorAll('[data-tab]')).toHaveLength(1);
    const labels = [...container.querySelectorAll('.builder-field-label')].map((node) => node.textContent);
    expect(labels).toEqual([
      'Suggested Course Name',
      'Provide a course summary',
      'Describe in detail the type of material the course should cover',
      'Describe the intended audience',
      'Describe the additional features the course should have',
      'Your Name',
      'Your Email',
      'Annotation',
    ]);
    expect(tableBag.last.options.queryRef).toBe(SUGGESTION_LIST_QUERY_REF);
    expect(SUGGESTION_LIST_QUERY_REF).toBe(4515);
    expect(tableBag.last.options.readonly).toBe(false);
    expect(typeof tableBag.last.handleFlag).toBe('function');
    expect(typeof tableBag.last.handleAnnotate).toBe('function');
    expect(invokeScript).not.toHaveBeenCalled();
    expect(tableBag.last.options.primaryKeyField).toEqual(['suggestion_id']);
    expect(typeof manager.cleanup).toBe('function');
    expect(manager.teardown).toBeUndefined();
    manager.cleanup();
    expect(tableBag.last.destroy).toHaveBeenCalled();
  });

  it('shows the lock panel when the resolved name is not staff', async () => {
    getClaims.mockReturnValue({ roles: '1' });
    authQuery.mockResolvedValueOnce([{ name: 'mail_send' }]);
    const { container, manager } = mount();
    await manager.init();
    expect(container.querySelector('h2').textContent).toBe('Course Builder');
    expect(container.textContent).toContain('do not have permission');
    expect(container.querySelector('#builder-table')).toBeNull();
    expect(tableBag.last).toBeNull();
  });

  it('paints a selected suggestion and saves the working title only', async () => {
    staffQuery();
    const { container, manager } = mount();
    await manager.init();
    await tableBag.last.options.onRowSelected({ getData: () => FIXTURE });
    expect(container.querySelector('#builder-working-title').value).toBe('Calculus I');
    expect(container.querySelector('#builder-working-title').disabled).toBe(false);
    expect(container.querySelector('[data-field="course_name"]').textContent).toBe('Calculus I');
    expect(container.querySelector('[data-field="detail"]').innerHTML.toLowerCase()).not.toContain('onerror');
    expect(container.querySelector('[data-field="detail"] p').textContent).toBe('Limits');
    expect(container.textContent).not.toContain('secret-token');
    expect(document.querySelector('#manager-footer-save').disabled).toBe(true);

    container.querySelector('#builder-working-title').value = 'Working calculus';
    container.querySelector('#builder-working-title').dispatchEvent(new Event('input'));
    expect(document.querySelector('#manager-footer-save').disabled).toBe(false);

    document.querySelector('#manager-footer-save').click();
    await vi.waitFor(() => {
      expect(authQuery).toHaveBeenCalledWith(
        expect.anything(),
        WORKING_TITLE_QUERY_REF,
        {
          INTEGER: { SUGGESTION_ID: 7, USERID: 7 },
          STRING: { WORKING_TITLE: 'Working calculus' },
        },
      );
      expect(toast.success).toHaveBeenCalled();
    });
    expect(container.querySelector('[data-field="course_name"]').textContent).toBe('Calculus I');
    expect(authQuery.mock.calls.map((call) => call[1])).not.toContain(SUGGESTION_UPDATE_QUERY_REF);
  });

  it('edits, annotates, flags, copies, and moves to Intake without another email', async () => {
    staffQuery();
    const { container, manager } = mount();
    await manager.init();
    await tableBag.last.options.onRowSelected({ getData: () => FIXTURE });
    const stage = container.querySelector('#builder-stage');
    expect([...stage.options].map((option) => option.textContent)).toEqual(['Suggested', 'Intake']);
    expect(stage.disabled).toBe(false);
    expect(container.querySelector('#builder-receipt').textContent).toBe('Receipt email: not sent');

    stage.value = 'intake';
    stage.dispatchEvent(new Event('change'));
    await vi.waitFor(() => expect(confirmDialog).toHaveBeenCalled());
    expect(stage.value).toBe('suggested');
    expect(authQuery.mock.calls.map((call) => call[1])).not.toContain(INTAKE_QUERY_REF);

    confirmDialog.mockResolvedValue(true);
    stage.value = 'intake';
    stage.dispatchEvent(new Event('change'));
    await vi.waitFor(() => {
      expect(authQuery).toHaveBeenCalledWith(expect.anything(), INTAKE_QUERY_REF, {
        INTEGER: { SUGGESTION_ID: 7, USERID: 7 },
      });
    });
    expect(invokeScript).not.toHaveBeenCalled();

    await tableBag.last.options.onRowSelected({
      getData: () => ({ ...FIXTURE, draft_state: 'received', receipt_mail: 'failed', flagged: 1 }),
    });
    expect([...container.querySelector('#builder-stage').options].map((option) => option.value))
      .toEqual(['suggested', 'received', 'intake']);
    const receivedStage = container.querySelector('#builder-stage');
    receivedStage.value = 'suggested';
    receivedStage.dispatchEvent(new Event('change'));
    await vi.waitFor(() => {
      expect(authQuery).toHaveBeenCalledWith(expect.anything(), SUGGESTED_QUERY_REF, {
        INTEGER: { SUGGESTION_ID: 7, USERID: 7 },
      });
    });
    expect(invokeScript).not.toHaveBeenCalled();
    expect(container.querySelector('#builder-receipt').textContent).toBe('Receipt email: failed');
    expect(container.querySelector('#builder-flag').textContent).toBe('Flagged');

    await tableBag.last.handleFlag();
    await vi.waitFor(() => {
      expect(authQuery).toHaveBeenCalledWith(expect.anything(), FLAG_QUERY_REF, {
        INTEGER: { SUGGESTION_ID: 7, USERID: 7 },
      });
    });

    await tableBag.last.handleAnnotate();
    const note = container.querySelector('#builder-annotation');
    expect(note.disabled).toBe(false);
    note.value = 'Talked about this';
    note.dispatchEvent(new Event('input'));
    document.querySelector('#manager-footer-save').click();
    await vi.waitFor(() => {
      expect(authQuery).toHaveBeenCalledWith(
        expect.anything(),
        SUGGESTION_UPDATE_QUERY_REF,
        expect.objectContaining({
          STRING: expect.objectContaining({
            ANNOTATION: 'Talked about this',
            COURSE_NAME: 'Calculus I',
            DETAIL: FIXTURE.detail,
          }),
        }),
      );
    });
    await vi.waitFor(() => expect(manager._busy).toBe(false));

    await tableBag.last.handleEdit();
    expect(container.querySelector('#builder-stage').disabled).toBe(true);
    const name = container.querySelector('[data-edit="course_name"]');
    name.value = 'Calculus II';
    name.dispatchEvent(new Event('input', { bubbles: true }));
    document.querySelector('#manager-footer-save').click();
    await vi.waitFor(() => {
      expect(authQuery).toHaveBeenCalledWith(
        expect.anything(),
        SUGGESTION_UPDATE_QUERY_REF,
        expect.objectContaining({
          STRING: expect.objectContaining({ COURSE_NAME: 'Calculus II' }),
        }),
      );
    });
    await vi.waitFor(() => expect(manager._busy).toBe(false));

    await tableBag.last.handleAdd();
    expect(container.querySelector('#builder-working-title').disabled).toBe(true);
    container.querySelector('[data-edit="course_name"]').value = 'Staff idea';
    container.querySelector('[data-edit="summary"]').value = 'From a conversation';
    container.querySelector('[data-edit="course_name"]').dispatchEvent(new Event('input', { bubbles: true }));
    document.querySelector('#manager-footer-save').click();
    await vi.waitFor(() => {
      expect(authQuery).toHaveBeenCalledWith(
        expect.anything(),
        SUGGESTION_INSERT_QUERY_REF,
        expect.objectContaining({
          INTEGER: { USERID: 7 },
          STRING: expect.objectContaining({
            COURSE_NAME: 'Staff idea',
            SUMMARY: 'From a conversation',
          }),
        }),
      );
    });
    await vi.waitFor(() => expect(manager._busy).toBe(false));

    await tableBag.last.options.onRowSelected({ getData: () => FIXTURE });
    await tableBag.last.handleDuplicate();
    await vi.waitFor(() => {
      expect(authQuery).toHaveBeenCalledWith(
        expect.anything(),
        SUGGESTION_DUPLICATE_QUERY_REF,
        expect.objectContaining({
          STRING: { COURSE_NAME: 'Calculus I (Copy)' },
        }),
      );
    });
    expect(invokeScript).not.toHaveBeenCalled();
  });

  it('opens the list without sending a receipt', async () => {
    staffQuery();
    const { container, manager } = mount();
    await manager.init();
    expect(invokeScript).not.toHaveBeenCalled();
    expect(toast.info).not.toHaveBeenCalled();
    expect(toast.error).not.toHaveBeenCalled();
    expect(container.querySelector('#builder-table')).not.toBeNull();
    manager.cleanup();
  });
});
