import { describe, it, expect } from 'vitest';
import {
  buildOrderParams,
  buildPriceSaveParams,
  deactivateOptions,
  ensureProductOptions,
  refundOptions,
  stripeDashboardUrl,
  stripeModeFromEnvelope,
} from '../../../src/managers/course-manager/course-commerce.js';
import {
  priceSaveAccepted,
  priceSaveRequestItems,
  priceSaveResponseItems,
} from '../../../src/managers/course-manager/course-price-log.js';

describe('course commerce helpers', () => {
  it('posts Ensure Product with course_id and no action', () => {
    expect(ensureProductOptions(0)).toBeNull();
    expect(ensureProductOptions(4)).toEqual({
      script: 'Stripe.EnsureProduct',
      params: { course_id: 4 },
      wait: true,
      timeoutSeconds: 15,
    });
    expect(ensureProductOptions(4, 'rotate').params).toEqual({
      course_id: 4,
      action: 'rotate',
    });
  });

  it('keeps a cleared currency and writes a new amount in cents', () => {
    const built = buildPriceSaveParams(4, 7, [
      { currency: 'cad', text: '', hadRow: true, storedCents: 1999, active: true },
      { currency: 'usd', text: '19.99', hadRow: false, storedCents: 0, active: true },
      { currency: 'eur', text: '', hadRow: false, storedCents: 0, active: false },
      { currency: 'gbp', text: '10', hadRow: true, storedCents: 500, active: false },
    ]);
    expect(built).toEqual({
      ok: true,
      params: {
        INTEGER: {
          COURSEID: 4,
          USERID: 7,
          CAD_PRESENT: 1,
          CAD_KEEP: 1,
          CAD_CENTS: 1999,
          CAD_ACTIVE: 0,
          USD_PRESENT: 1,
          USD_KEEP: 0,
          USD_CENTS: 1999,
          USD_ACTIVE: 1,
          EUR_PRESENT: 0,
          EUR_KEEP: 0,
          EUR_CENTS: 0,
          EUR_ACTIVE: 0,
          GBP_PRESENT: 1,
          GBP_KEEP: 0,
          GBP_CENTS: 1000,
          GBP_ACTIVE: 0,
        },
      },
    });
  });

  it('lists this course and treats a blank account id as every account', () => {
    expect(buildOrderParams(4, '').params).toEqual({
      INTEGER: { COURSEID: 4, ACCOUNTID: 0 },
    });
    expect(buildOrderParams(4, '12').params.INTEGER.ACCOUNTID).toBe(12);
    expect(buildOrderParams(4, 'nope').ok).toBe(false);
  });

  it('builds a dashboard link only for test or live and a safe id', () => {
    expect(stripeDashboardUrl('test', 'payment', 'pi_abc')).toBe(
      'https://dashboard.stripe.com/test/payments/pi_abc',
    );
    expect(stripeDashboardUrl('live', 'customer', 'cus_abc')).toBe(
      'https://dashboard.stripe.com/customers/cus_abc',
    );
    expect(stripeDashboardUrl('test', 'payment', 'sk_live_x')).toBe('');
    expect(stripeDashboardUrl('', 'customer', 'cus_abc')).toBe('');
  });

  it('reads test or live from a completed mode result', () => {
    expect(stripeModeFromEnvelope({
      status: 'completed',
      result: { mode: 'live' },
    })).toBe('live');
    expect(stripeModeFromEnvelope({
      status: 'completed',
      result: { mode: 'sk_live_x' },
    })).toBe('');
    expect(stripeModeFromEnvelope({ status: 'failed', result: { mode: 'live' } })).toBe('');
    expect(stripeModeFromEnvelope(undefined)).toBe('');
  });

  it('sends a refund reason and a deactivate account id', () => {
    expect(refundOptions(9, 'late')).toEqual({
      script: 'Stripe.Refund',
      params: { order_id: 9, reason: 'late' },
      wait: true,
      timeoutSeconds: 15,
    });
    expect(deactivateOptions(3).params).toEqual({ account_id: 3 });
  });
});

describe('price save log', () => {
  const params = {
    INTEGER: {
      COURSEID: 16,
      USERID: 99,
      CAD_PRESENT: 1,
      CAD_KEEP: 0,
      CAD_CENTS: 5000,
      CAD_ACTIVE: 1,
      USD_PRESENT: 1,
      USD_KEEP: 1,
      USD_CENTS: 3900,
      USD_ACTIVE: 0,
      EUR_PRESENT: 0,
      EUR_KEEP: 0,
      EUR_CENTS: 0,
      EUR_ACTIVE: 0,
      GBP_PRESENT: 1,
      GBP_KEEP: 0,
      GBP_CENTS: 3000,
      GBP_ACTIVE: 1,
    },
  };

  it('logs the binds and leaves out the staff id', () => {
    const lines = priceSaveRequestItems(16, params).join('\n');
    expect(lines).toContain('query_ref=4506');
    expect(lines).toContain('course_id=16');
    expect(lines).toContain('CAD present=1 keep=0 cents=5000 active=1');
    expect(lines).toContain('USD present=1 keep=1 cents=3900 active=0');
    expect(lines).not.toContain('99');
    expect(lines).not.toContain('USERID');
  });

  it('logs statement counts and not the row body', () => {
    const lines = priceSaveResponseItems({
      success: true,
      query_ref: 4506,
      affected_rows: 1,
      row_count: 8,
      rows: [
        { statement: 1, block: 1, affected: 1, success: true, rows: [{ stripe_price_id: 'price_secret' }] },
      ],
    }).join('\n');
    expect(lines).toContain('affected_rows=1');
    expect(lines).toContain('block=1 statement=1 affected=1 success=true');
    expect(lines).not.toContain('price_secret');
  });

  it('accepts a write that changed a row', () => {
    expect(priceSaveAccepted({
      success: true,
      affected_rows: 1,
      rows: [{ statement: 1, block: 1, affected: 1, success: true }],
    })).toEqual({ ok: true });
    expect(priceSaveAccepted({ success: true, affected_rows: 4, rows: [] })).toEqual({ ok: true });
  });

  it('refuses an empty success and a fail body', () => {
    expect(priceSaveAccepted({
      success: true,
      affected_rows: 0,
      rows: [],
    }).ok).toBe(false);
    expect(priceSaveAccepted({
      success: 'fail',
      message: 'queryref not found or not public',
      rows: [],
    }).message).toBe('queryref not found or not public');
  });
});
