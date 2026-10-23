import { describe, it, expect, vi, beforeEach } from 'vitest';

vi.mock('../../../src/shared/conduit.js', () => ({
  authQuery: vi.fn(),
  invokeScript: vi.fn(),
}));

vi.mock('../../../src/shared/confirm.js', () => ({
  confirmDialog: vi.fn(async () => false),
}));

vi.mock('../../../src/shared/toast.js', () => ({
  toast: { error: vi.fn(), info: vi.fn(), success: vi.fn() },
}));

import { authQuery, invokeScript } from '../../../src/shared/conduit.js';
import { confirmDialog } from '../../../src/shared/confirm.js';
import { toast } from '../../../src/shared/toast.js';
import * as logApi from '../../../src/core/log.js';
import {
  CANVAS_OPS_NOTE,
  CourseLearners,
  archiveButtonState,
  buildLearnerSearchParams,
  canvasUserUrl,
  grantButtonState,
  learnerAccountParams,
  renewButtonState,
  staffArchiveOptions,
  staffGrantOptions,
  staffOutcome,
  staffRenewOptions,
  stripeCustomerLabel,
} from '../../../src/managers/course-manager/course-learners.js';

function manager() {
  return {
    app: { api: {} },
    _mode: 'edit',
    _selectedId: 16,
    elements: {
      pricing: { value: 'free' },
      retired: { checked: false },
      published: { textContent: 'Published' },
    },
    commerce: { _mode: 'test' },
  };
}

function mount() {
  const pane = document.createElement('div');
  const learners = new CourseLearners(manager());
  learners.mount(pane);
  return { pane, learners };
}

describe('learner search and seat helpers', () => {
  it('lets an account id win and still sends the other binds blank', () => {
    expect(buildLearnerSearchParams({
      accountId: '42',
      subject: 'kc-sub',
      email: 'ada@example.com',
    })).toEqual({
      ok: true,
      field: 'account',
      params: {
        INTEGER: { ACCOUNTID: 42 },
        STRING: { SUBJECT: '', EMAIL: '' },
      },
    });
    expect(buildLearnerSearchParams({ subject: 'kc-sub', email: 'ada@example.com' })).toEqual({
      ok: true,
      field: 'subject',
      params: {
        INTEGER: { ACCOUNTID: 0 },
        STRING: { SUBJECT: 'kc-sub', EMAIL: '' },
      },
    });
    expect(buildLearnerSearchParams({ email: 'ada@example.com' }).field).toBe('email');
    expect(buildLearnerSearchParams({}).ok).toBe(false);
    expect(buildLearnerSearchParams({ accountId: '2abc' }).message).toBe('Account id must be a positive integer.');
    expect(learnerAccountParams(0)).toBeNull();
    expect(learnerAccountParams(42)).toEqual({ INTEGER: { ACCOUNTID: 42 } });
  });

  it('builds Canvas user links and staff script calls', () => {
    expect(canvasUserUrl(99)).toBe('https://canvas.500courses.com/users/99');
    expect(canvasUserUrl('15abc')).toBe('');
    expect(canvasUserUrl(0)).toBe('');
    expect(staffGrantOptions(42, 16)).toEqual({
      script: 'Enroll.StaffGrant',
      params: { account_id: 42, course_id: 16 },
      wait: true,
      timeoutSeconds: 15,
    });
    expect(staffGrantOptions(1.5, 16)).toBeNull();
    expect(staffArchiveOptions(42, 16, 'archive').params.action).toBe('archive');
    expect(staffArchiveOptions(42, 16, 'drop')).toBeNull();
    expect(staffRenewOptions(42, 16).script).toBe('Enroll.StaffRenew');
    expect(staffGrantOptions(42, 16).params._hydrogen).toBeUndefined();
  });

  it('keeps grant, archive, and renew off until the seat is allowed', () => {
    expect(grantButtonState({ invokable: false }).title).toBe('not enabled on this server');
    expect(grantButtonState({
      invokable: true, courseId: 0, accountId: 42,
    }).title).toBe('Select a course');
    expect(grantButtonState({
      invokable: true, courseId: 16, accountId: 0,
    }).title).toBe('Search for an account');
    expect(grantButtonState({
      invokable: true, courseId: 16, accountId: 42, retired: true, pricingType: 'free', published: 'Published',
    }).title).toBe('Retired courses are not granted.');
    expect(grantButtonState({
      invokable: true, courseId: 16, accountId: 42, pricingType: 'paid', published: 'Published',
    }).title).toBe('Free courses only. Paid seats stay on checkout.');
    expect(grantButtonState({
      invokable: true, courseId: 16, accountId: 42, pricingType: 'free', published: 'Not published',
    }).title).toBe('The course is not published.');
    expect(grantButtonState({
      invokable: true, courseId: 16, accountId: 42, pricingType: 'free', published: 'Published',
    }).disabled).toBe(false);

    expect(archiveButtonState({ invokable: false, archived: true })).toEqual({
      disabled: true,
      title: 'not enabled on this server',
      label: 'Unarchive',
    });
    expect(archiveButtonState({ invokable: true, archived: false }).label).toBe('Archive');
    expect(renewButtonState({ invokable: false }).title).toBe('not enabled on this server');
    expect(renewButtonState({
      invokable: true, archived: true, renewPolicy: 'free_renew',
    }).title).toBe('Unarchive the seat before renewing it.');
    expect(renewButtonState({
      invokable: true, renewPolicy: 'paid_renew',
    }).title).toBe('Paid renewal stays on checkout.');
    expect(renewButtonState({
      invokable: true, renewPolicy: 'no_renew',
    }).title).toBe('This seat cannot be renewed.');
    expect(renewButtonState({
      invokable: true, renewPolicy: 'free_renew',
    }).disabled).toBe(false);
  });

  it('hides a secret customer id and treats a false script body as failure', () => {
    expect(stripeCustomerLabel('')).toBe('None');
    expect(stripeCustomerLabel('cus_abc')).toBe('cus_abc');
    expect(stripeCustomerLabel('sk_live_secret')).toBe('Hidden');
    expect(staffOutcome({
      status: 'completed',
      result: { ok: false, code: 'not_offered', message: 'Free courses only. Paid seats stay on checkout.' },
    })).toEqual({
      ok: false,
      code: 'not_offered',
      message: 'Free courses only. Paid seats stay on checkout.',
    });
    expect(staffOutcome({
      status: 'completed',
      result: '{"ok":false,"code":"paid_renew","message":"Paid renewal stays on checkout."}',
    }).code).toBe('paid_renew');
    expect(staffOutcome({
      status: 'completed',
      result: { ok: true, already: true, enrollment_id: 8 },
    }).ok).toBe(true);
    expect(staffOutcome({ status: 'failed', error: 'nope' }).message).toBe('nope');
  });
});

describe('CourseLearners', () => {
  beforeEach(() => {
    authQuery.mockReset();
    invokeScript.mockReset();
    confirmDialog.mockReset();
    confirmDialog.mockResolvedValue(false);
    toast.error.mockReset();
    toast.success.mockReset();
    authQuery.mockResolvedValue([]);
  });

  it('searches by account id, hides secrets, and opens the Canvas user', async () => {
    const realLog = vi.spyOn(logApi, 'log');
    authQuery.mockImplementation(async (_api, ref) => {
      if (ref === 87) return [{ invokable: 0 }];
      if (ref === 4508) {
        return [{ account_id: 42, name: 'Ada', email: 'ada@example.com', subject: 'kc-sub' }];
      }
      if (ref === 4509) {
        return [{
          account_id: 42,
          name: 'Ada',
          stripe_customer_id: 'sk_live_secret',
          notify_new_course: 1,
        }];
      }
      if (ref === 4510) {
        return [{
          enrollment_id: 8,
          course_id: 16,
          title: 'Bloom',
          status: 'active',
          renew_policy: 'paid_renew',
        }];
      }
      if (ref === 4511) {
        return [{
          order_id: 3,
          order_number: 'R-3',
          account_id: 42,
          status: 'paid',
          currency: 'cad',
          total_cents: 4999,
          stripe_intent_id: 'pi_abc',
          stripe_customer_id: 'cus_abc',
        }];
      }
      if (ref === 4512) {
        return [{ canvas_user_id: 99, canvas_email: 'ada@canvas.test', last_seen_at: '2026-10-23' }];
      }
      return [];
    });
    const open = vi.spyOn(window, 'open').mockImplementation(() => null);
    const { pane, learners } = mount();
    await learners.loadFlags();
    pane.querySelector('#learner-email').value = 'ada@example.com';
    pane.querySelector('#learner-subject').value = 'kc-sub';
    pane.querySelector('#learner-account').value = '42';
    pane.querySelector('#learner-search-btn').click();
    await vi.waitFor(() => {
      expect(pane.querySelector('#learner-detail').hidden).toBe(false);
    });
    const search = authQuery.mock.calls.find((call) => call[1] === 4508);
    expect(search[2]).toEqual({
      INTEGER: { ACCOUNTID: 42 },
      STRING: { SUBJECT: '', EMAIL: '' },
    });
    const logged = realLog.mock.calls.map((call) => String(call[2])).join('\n');
    expect(logged).toContain('Learner search by account 42');
    expect(logged).not.toContain('ada@example.com');
    expect(logged).not.toContain('kc-sub');
    expect(logged).not.toContain('sk_live_secret');
    expect(pane.textContent).not.toMatch(/lua/i);
    expect(pane.textContent).not.toMatch(/phase/i);
    expect(pane.textContent).toContain('Hidden');
    expect(pane.textContent).not.toContain('sk_live_secret');
    expect(pane.textContent).toContain(CANVAS_OPS_NOTE);
    expect(pane.querySelector('#learner-grant-btn').disabled).toBe(true);
    expect(pane.querySelector('#learner-grant-tip').title).toBe('not enabled on this server');
    expect(pane.querySelector('.course-learner-renew').disabled).toBe(true);
    expect(pane.querySelector('.course-learner-renew').parentElement.title).toBe('not enabled on this server');
    expect(pane.querySelector('.course-learner-seat').textContent).toContain('paid_renew');
    expect(pane.querySelector('.course-learner-archive').disabled).toBe(true);
    pane.querySelector('.course-learner-archive').click();
    expect(confirmDialog).not.toHaveBeenCalled();
    const payment = pane.querySelector('.course-learner-order-link');
    expect(payment.getAttribute('href')).toBe('https://dashboard.stripe.com/test/payments/pi_abc');
    pane.querySelector('#learner-canvas-btn').click();
    expect(open).toHaveBeenCalledWith(
      'https://canvas.500courses.com/users/99',
      '_blank',
      'noopener,noreferrer',
    );
    open.mockRestore();
    realLog.mockRestore();
  });

  it('leaves several matches as buttons and does not query a blank search', async () => {
    authQuery.mockImplementation(async (_api, ref) => {
      if (ref === 4508) {
        return [
          { account_id: 1, name: 'Ada', email: 'a@example.com' },
          { account_id: 2, name: 'Grace', email: 'g@example.com' },
        ];
      }
      return [];
    });
    const { pane } = mount();
    pane.querySelector('#learner-email').value = 'a@example.com';
    pane.querySelector('#learner-search-btn').click();
    await vi.waitFor(() => {
      expect(pane.querySelectorAll('.course-learner-hit').length).toBe(2);
    });
    expect(pane.querySelector('#learner-detail').hidden).toBe(true);
    authQuery.mockClear();
    pane.querySelector('#learner-email').value = '';
    pane.querySelector('#learner-search-btn').click();
    await vi.waitFor(() => {
      expect(toast.error).toHaveBeenCalledWith('Search failed', {
        description: 'Enter an email, a Keycloak sub, or an account id.',
      });
    });
    expect(authQuery).not.toHaveBeenCalled();
  });

  it('reports a completed script whose body says no', async () => {
    authQuery.mockImplementation(async (_api, ref) => {
      if (ref === 87) return [{ invokable: 1 }];
      if (ref === 4508) return [{ account_id: 42, name: 'Ada' }];
      if (ref === 4509) return [{ account_id: 42, name: 'Ada', stripe_customer_id: 'cus_abc' }];
      if (ref === 4510) {
        return [{
          enrollment_id: 8,
          course_id: 16,
          title: 'Bloom',
          status: 'active',
          renew_policy: 'paid_renew',
        }];
      }
      if (ref === 4511 || ref === 4512) return [];
      return [];
    });
    invokeScript.mockResolvedValue({
      status: 'completed',
      result: { ok: false, code: 'not_offered', message: 'Free courses only. Paid seats stay on checkout.' },
    });
    const { pane, learners } = mount();
    await learners.loadFlags();
    pane.querySelector('#learner-account').value = '42';
    pane.querySelector('#learner-search-btn').click();
    await vi.waitFor(() => {
      expect(pane.querySelector('#learner-grant-btn').disabled).toBe(false);
    });
    expect(pane.querySelector('.course-learner-renew').disabled).toBe(true);
    expect(pane.querySelector('.course-learner-renew').parentElement.title).toBe('Paid renewal stays on checkout.');
    pane.querySelector('#learner-grant-btn').click();
    expect(invokeScript).not.toHaveBeenCalled();
    confirmDialog.mockResolvedValue(true);
    pane.querySelector('#learner-grant-btn').click();
    await vi.waitFor(() => {
      expect(toast.error).toHaveBeenCalledWith('Action failed', {
        description: 'Free courses only. Paid seats stay on checkout.',
      });
    });
    expect(invokeScript).toHaveBeenCalledWith(expect.anything(), {
      script: 'Enroll.StaffGrant',
      params: { account_id: 42, course_id: 16 },
      wait: true,
      timeoutSeconds: 15,
    });
  });
});
