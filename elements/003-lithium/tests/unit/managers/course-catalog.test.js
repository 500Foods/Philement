import { describe, it, expect } from 'vitest';
import {
  COURSE_INSERT_QUERY_REF,
  COURSE_LIST_QUERY_REF,
  COURSE_PRICES_QUERY_REF,
  COURSE_UPDATE_QUERY_REF,
  ENROLMENT_HISTORY_QUERY_REF,
  MANAGEMENT_HISTORY_QUERY_REF,
  actionButtonState,
  actionOutcome,
  buildCourseInsertParams,
  buildCourseUpdateParams,
  canvasCourseUrl,
  courseDraftSignature,
  courseDuplicateRow,
  courseFormFromRow,
  formatPriceAmount,
  priceListView,
  courseMatchesSearch,
  historyText,
  parseTags,
  safeImageUrl,
  scriptIsInvokable,
  stripeLabel,
} from '../../../src/managers/course-manager/course-catalog.js';

describe('course catalog helpers', () => {
  it('uses caller refs that are not migration file numbers', () => {
    expect(COURSE_LIST_QUERY_REF).toBe(4500);
    expect(COURSE_INSERT_QUERY_REF).toBe(4501);
    expect(COURSE_UPDATE_QUERY_REF).toBe(4502);
    expect(ENROLMENT_HISTORY_QUERY_REF).toBe(4503);
    expect(MANAGEMENT_HISTORY_QUERY_REF).toBe(4504);
    expect(COURSE_PRICES_QUERY_REF).toBe(4505);
  });

  it('matches code, slug, title, and canvas id', () => {
    const row = { code: 'BBT', slug: 'bloom', title: 'Bloom', canvas_course_id: 90, retired: 1 };
    expect(courseMatchesSearch(row, '90')).toBe(true);
    expect(courseMatchesSearch(row, 'nope')).toBe(false);
    expect(courseMatchesSearch(row, '')).toBe(true);
  });

  it('accepts http(s) and site paths for images', () => {
    expect(safeImageUrl('https://cdn.example/a.png')).toBe('https://cdn.example/a.png');
    expect(safeImageUrl('/media/a.png')).toBe('/media/a.png');
    expect(safeImageUrl('//cdn.example/a.png')).toBe('');
    expect(safeImageUrl('javascript:alert(1)')).toBe('');
  });

  it('builds a Canvas course url from a positive id', () => {
    expect(canvasCourseUrl(90)).toBe('https://canvas.500courses.com/courses/90');
    expect(canvasCourseUrl(0)).toBe('');
    expect(canvasCourseUrl('')).toBe('');
  });

  it('hides secret-shaped stripe values', () => {
    expect(stripeLabel('prod_123')).toBe('prod_123');
    expect(stripeLabel('sk_live_secret')).toBe('Hidden');
    expect(stripeLabel('')).toBe('None');
  });

  it('requires a JSON object for tags', () => {
    expect(parseTags('')).toEqual({ ok: true, json: '{}' });
    expect(parseTags('{"a":1}').json).toBe('{"a":1}');
    expect(parseTags('[1]').ok).toBe(false);
    expect(parseTags('nope').ok).toBe(false);
  });

  it('builds an insert with a title and a blank canvas id', () => {
    const built = buildCourseInsertParams({
      code: 'BBT',
      slug: 'bloom',
      title: 'Bloom',
      imagePath: '',
      canvasCourseId: '',
      pricingType: 'free',
      featured: false,
      sortOrder: '',
      tags: '',
    }, 7);
    expect(built.ok).toBe(true);
    expect(built.params.STRING.CANVAS_COURSE_ID).toBe('');
    expect(built.params.STRING.TITLE).toBe('Bloom');
    expect(built.params.INTEGER).toEqual({
      IS_FEATURED: 0,
      RETIRED: 0,
      SORT_ORDER: 0,
      CREATEDID: 7,
      UPDATEDID: 7,
    });
    expect(buildCourseInsertParams({ code: 'BBT', slug: 'bloom', title: '' }, 7).ok).toBe(false);
  });

  it('builds an update without a title and refuses a missing course', () => {
    const built = buildCourseUpdateParams({
      courseId: 4,
      code: 'BBT',
      slug: 'bloom',
      imagePath: '/a.png',
      canvasCourseId: '90',
      pricingType: 'paid',
      featured: true,
      sortOrder: '2',
      tags: '{"topic":"art"}',
    }, 7);
    expect(built.ok).toBe(true);
    expect(built.params.STRING.TITLE).toBeUndefined();
    expect(built.params.INTEGER.COURSEID).toBe(4);
    expect(built.params.INTEGER.RETIRED).toBe(0);
    expect(built.params.STRING.CANVAS_COURSE_ID).toBe('90');
    expect(buildCourseUpdateParams({
      courseId: 4,
      code: 'A',
      slug: 'a',
      pricingType: 'free',
      retired: true,
    }, 7).params.INTEGER.RETIRED).toBe(1);
    expect(buildCourseUpdateParams({ courseId: 0, code: 'A', slug: 'a' }, 7).ok).toBe(false);
  });

  it('maps a row onto the form and tracks the editable draft', () => {
    const form = courseFormFromRow({
      course_id: 4,
      CODE: 'BBT',
      slug: 'bloom',
      title: 'Bloom',
      retired: 1,
      tags: { topic: 'art' },
    });
    expect(form.courseId).toBe(4);
    expect(form.code).toBe('BBT');
    expect(form.retired).toBe(true);
    expect(form.tags).toContain('topic');
    const left = courseDraftSignature({ code: 'A', slug: 'a', title: 'T' });
    const right = courseDraftSignature({ code: 'A', slug: 'a', title: 'T' });
    expect(left).toBe(right);
    expect(courseDraftSignature({ code: 'B', slug: 'a', title: 'T' })).not.toBe(left);
  });

  it('copies a brochure row and formats the four currencies', () => {
    const copy = courseDuplicateRow({
      course_id: 4,
      code: 'BBT',
      slug: 'bloom',
      title: 'Bloom',
      canvas_course_id: 90,
      stripe_product_id: 'prod_123',
    });
    expect(copy.course_id).toBeUndefined();
    expect(copy.code).toBe('BBT-copy');
    expect(copy.title).toBe('Bloom (Copy)');
    expect(copy.canvas_course_id).toBeNull();
    expect(copy.published).toBe(0);
    expect(copy.retired).toBe(0);
    expect(formatPriceAmount(1999)).toBe('19.99');
    expect(formatPriceAmount('nope')).toBe('None');
    const view = priceListView([
      { currency: 'GBP', unit_amount_cents: 250, stripe_price_id: 'price_gbp', is_active: 1 },
    ]);
    expect(view.map((row) => row.currency)).toEqual(['cad', 'usd', 'eur', 'gbp']);
    expect(view[0]).toEqual({ currency: 'cad', amount: 'None', stripe: 'None', active: '' });
    expect(view[3].amount).toBe('2.50');
    expect(view[3].stripe).toBe('price_gbp');
    expect(view[3].active).toBe('Active');
  });

  it('keeps script buttons disabled until invokable is 1', () => {
    expect(scriptIsInvokable([{ invokable: 0 }])).toBe(false);
    expect(scriptIsInvokable([{ INVOKABLE: 1 }])).toBe(true);
    expect(actionButtonState({ invokable: false, needsRow: true, hasRow: true }).title)
      .toBe('not enabled on this server');
    expect(actionButtonState({ invokable: true, needsRow: true, hasRow: false }).title)
      .toBe('Select a course');
    expect(actionButtonState({ invokable: true, needsRow: false, hasRow: false }).disabled)
      .toBe(false);
  });

  it('reads history lines and treats a non-completed job as a failure', () => {
    expect(historyText([{ line: 'one' }, { LINE: 'two' }])).toBe('one\ntwo');
    expect(historyText([])).toBe('');
    expect(actionOutcome({ status: 'completed' }).ok).toBe(true);
    expect(actionOutcome({ status: 'failed', error: 'nope' }).detail).toBe('nope');
  });
});
