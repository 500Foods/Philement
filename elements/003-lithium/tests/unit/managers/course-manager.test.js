import { readFileSync } from 'node:fs';
import { dirname, join } from 'node:path';
import { fileURLToPath } from 'node:url';
import { describe, it, expect, vi, beforeEach } from 'vitest';

const courseCss = readFileSync(join(
  dirname(fileURLToPath(import.meta.url)),
  '../../../src/managers/course-manager/course-manager.css',
), 'utf8');

const tableBag = { last: null };

vi.mock('../../../src/shared/conduit.js', () => ({
  authQuery: vi.fn(),
  authQueryResponse: vi.fn(),
  invokeScript: vi.fn(),
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
      };
      this.setFilter = this.table.setFilter;
      this.clearFilter = this.table.clearFilter;
      this.autoSelectRow = vi.fn();
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
import { processIcons } from '../../../src/core/icons.js';
import CourseManager from '../../../src/managers/course-manager/course-manager.js';
import { ManagerLoader } from '../../../src/app/manager-loader.js';
import {
  COURSE_INSERT_QUERY_REF,
  COURSE_LIST_QUERY_REF,
  COURSE_PRICES_QUERY_REF,
  COURSE_UPDATE_QUERY_REF,
  ENROLMENT_HISTORY_QUERY_REF,
} from '../../../src/managers/course-manager/course-catalog.js';

function mount() {
  const container = document.createElement('div');
  const app = {
    api: { post() {} },
    auth: { user: { id: 7 } },
  };
  return { container, app, manager: new CourseManager(app, container) };
}

function staffQuery(extra = {}) {
  authQuery.mockImplementation(async (_api, ref) => {
    if (ref === 155) return [{ name: 'staff' }];
    if (ref === 87) return [{ invokable: extra.invokable ? 1 : 0 }];
    if (extra.rows && extra.rows[ref]) return extra.rows[ref];
    return [];
  });
}

describe('CourseManager shell', () => {
  beforeEach(() => {
    authQuery.mockReset();
    invokeScript.mockReset();
    confirmDialog.mockReset();
    confirmDialog.mockResolvedValue(false);
    toast.error.mockReset();
    toast.info.mockReset();
    toast.success.mockReset();
    processIcons.mockClear();
    getClaims.mockReturnValue({ roles: '2' });
    tableBag.last = null;
    authQuery.mockResolvedValue([]);
  });

  it('shows the catalog for a staff name', async () => {
    staffQuery();
    const { container, manager } = mount();
    await manager.init();
    expect(container.querySelector('h2')).toBeNull();
    expect(container.querySelector('.course-manager-intro')).toBeNull();
    expect(container.querySelector('#course-save-btn')).toBeNull();
    expect(container.textContent).not.toMatch(/lua/i);
    expect(container.textContent).not.toMatch(/phase/i);
    expect(container.querySelector('#course-navigator')).not.toBeNull();
    expect(container.querySelector('#course-table').classList.contains('lithium-table-container')).toBe(true);
    expect(container.querySelector('[data-tab="course"]').classList.contains('active')).toBe(true);
    expect(container.querySelector('#course-tab-course #course-code')).not.toBeNull();
    expect(container.querySelector('#course-code').disabled).toBe(true);
    expect(container.querySelector('#course-options')).not.toBeNull();
    expect(container.querySelector('#course-retired-chip')).toBeNull();
    expect(container.querySelector('.course-list-tools')).toBeNull();
    expect(container.querySelector('#course-option-retired')).not.toBeNull();
    expect(container.querySelector('#course-option-featured')).not.toBeNull();
    expect(container.querySelector('[data-tab="canvas"] [fa-graduation-cap]')).not.toBeNull();
    expect(container.querySelector('#course-tab-canvas #course-image-preview')).not.toBeNull();
    expect(container.querySelector('#course-tab-canvas #course-summary')).not.toBeNull();
    expect(container.querySelector('#course-prices-status').textContent).toBe('Select a course.');
    expect(container.querySelector('#course-prices').classList.contains('is-empty')).toBe(true);
    expect(container.querySelector('#course-tab-commerce #course-stripe')).not.toBeNull();
    expect(container.querySelector('#course-tab-history #course-history')).not.toBeNull();
    expect(container.querySelector('[data-tab="learners"]')).not.toBeNull();
    expect(container.querySelector('#course-tab-learners #learner-email')).not.toBeNull();
    expect(container.querySelector('#learner-search-btn')).not.toBeNull();
    expect(container.querySelector('#learner-grant-btn').disabled).toBe(true);
    expect(container.querySelector('#learner-grant-tip').title).toBe('not enabled on this server');
    expect(container.querySelector('#learner-canvas-note').textContent).toContain('Relink is an ops step');
    expect(tableBag.last.options.navigatorContainer).toBe(container.querySelector('#course-navigator'));
    expect(tableBag.last.options.localSearch).toBe(true);
    expect(tableBag.last.options.localSearchFields).toEqual(['code', 'slug', 'title', 'canvas_course_id']);
    expect(container.querySelector('#course-sync-btn').disabled).toBe(true);
    expect(container.querySelector('#course-sync-tip').title).toBe('not enabled on this server');
    expect(container.querySelector('#course-sync-all-btn').disabled).toBe(true);
    expect(container.querySelector('#course-retire-btn').disabled).toBe(true);
    expect(container.querySelector('#course-ensure-btn').disabled).toBe(true);
    expect(container.querySelector('#course-rotate-btn').disabled).toBe(true);
    expect(container.querySelector('#course-ensure-tip').title).toBe('Select a course');
    expect(container.querySelector('#course-stripe-mode').textContent).toBe('Mode unknown');
    expect(container.querySelector('#course-price-save').disabled).toBe(true);
    expect(container.querySelector('#course-price-save-tip').title).toBe('Save the course before editing prices.');
    expect(tableBag.last.options.queryRef).toBe(COURSE_LIST_QUERY_REF);
    expect(tableBag.last.options.primaryKeyField).toEqual(['course_id']);
    expect(tableBag.last.options.readonly).toBe(false);
    expect(processIcons).toHaveBeenCalledWith(container);
  });

  it('shows the lock panel when the resolved name is not staff', async () => {
    getClaims.mockReturnValue({ roles: '1' });
    authQuery.mockResolvedValueOnce([{ name: 'mail_send' }]);
    const { container, manager } = mount();
    await manager.render();
    expect(container.querySelector('.course-manager-denied')).not.toBeNull();
    expect(container.textContent).toMatch(/do not have permission/i);
    expect(container.querySelector('.course-manager-intro')).toBeNull();
    expect(container.querySelector('#course-navigator')).toBeNull();
    expect(container.querySelector('[data-tab]')).toBeNull();
    expect(container.querySelector('#course-code')).toBeNull();
  });

  it('shows the lock panel when QueryRef 155 fails for role id 2', async () => {
    authQuery.mockRejectedValueOnce(new Error('not applied'));
    const { container, manager } = mount();
    await manager.render();
    expect(container.querySelector('.course-manager-denied')).not.toBeNull();
    expect(manager._allowed).toBe(false);
    expect(container.querySelector('#course-code')).toBeNull();
    expect(container.querySelector('[data-tab]')).toBeNull();
  });

  it('lists every course the query returns and reloads from the list query', async () => {
    staffQuery();
    const { container, manager } = mount();
    await manager.init();
    expect(container.querySelector('#course-retired-chip')).toBeNull();
    expect(container.querySelector('#course-option-retired')).not.toBeNull();
    await manager._refresh();
    expect(tableBag.last.loadData.mock.calls.length).toBeGreaterThan(1);
  });

  it('keeps the course fields when another tab is selected', async () => {
    staffQuery();
    const { container, manager } = mount();
    await manager.init();
    container.querySelector('[data-tab="history"]').click();
    expect(container.querySelector('[data-tab="history"]').classList.contains('active')).toBe(true);
    expect(container.querySelector('#course-tab-history').classList.contains('active')).toBe(true);
    expect(container.querySelector('#course-tab-course').classList.contains('active')).toBe(false);
    expect(container.querySelector('#course-code')).not.toBeNull();
  });

  it('inserts a course and does not send the list query as the write', async () => {
    staffQuery();
    const { container, manager } = mount();
    await manager.init();
    container.querySelector('#course-code').value = 'BBT';
    container.querySelector('#course-slug').value = 'bloom';
    container.querySelector('#course-title').value = 'Bloom';
    await tableBag.last.options.onExecuteSave({ getData: () => ({}) });
    expect(toast.success).not.toHaveBeenCalled();
    const insert = authQuery.mock.calls.find((call) => call[1] === COURSE_INSERT_QUERY_REF);
    expect(insert).toBeTruthy();
    expect(insert[2].STRING.CODE).toBe('BBT');
    expect(insert[2].STRING.TITLE).toBe('Bloom');
    expect(insert[2].STRING.TAGS).toBe('{}');
    expect(insert[2].INTEGER.RETIRED).toBe(0);
    expect(insert[2].INTEGER.CREATEDID).toBe(7);
    expect(insert[2].INTEGER.UPDATEDID).toBe(7);
    expect(authQuery.mock.calls.some((call) => call[1] === COURSE_UPDATE_QUERY_REF)).toBe(false);
    expect(container.querySelector('#course-code').value).toBe('');
  });

  it('updates the selected course and leaves the title out of the write', async () => {
    staffQuery();
    const { container, manager } = mount();
    await manager.init();
    tableBag.last.options.onRowSelected({
      course_id: 4,
      code: 'BBT',
      slug: 'bloom',
      title: 'Bloom',
      pricing_type: 'paid',
      is_featured: 1,
      sort_order: 3,
      tags: { topic: 'art' },
      retired: 0,
      summary: 'A card',
      description: 'Longer',
      stripe_product_id: 'prod_123',
      canvas_course_id: 90,
    });
    expect(container.querySelector('#course-title').readOnly).toBe(true);
    expect(container.querySelector('#course-summary').textContent).toBe('A card');
    expect(container.querySelector('#course-stripe').textContent).toBe('prod_123');
    expect(container.querySelector('#course-published').textContent).toBe('Not published');
    container.querySelector('#course-code').value = 'BBT2';
    container.querySelector('#course-option-retired').checked = true;
    await tableBag.last.options.onExecuteSave({ getData: () => ({ course_id: 4 }) });
    expect(toast.success).not.toHaveBeenCalled();
    const update = authQuery.mock.calls.find((call) => call[1] === COURSE_UPDATE_QUERY_REF);
    expect(update[2].STRING.CODE).toBe('BBT2');
    expect(update[2].INTEGER.COURSEID).toBe(4);
    expect(update[2].INTEGER.USERID).toBe(7);
    expect(update[2].INTEGER.RETIRED).toBe(1);
    expect(update[2].INTEGER.IS_FEATURED).toBe(1);
    expect(update[2].STRING.TITLE).toBeUndefined();
    expect(update[2].STRING.TAGS).toBe('{"topic":"art"}');
  });

  it('applies the newly selected course', async () => {
    staffQuery();
    const { container, manager } = mount();
    await manager.init();
    tableBag.last.options.onRowSelected({
      course_id: 4,
      code: 'BBT',
      slug: 'bloom',
      title: 'Bloom',
    });
    container.querySelector('#course-code').value = 'CHANGED';
    tableBag.last.options.onRowSelected({
      course_id: 9,
      code: 'OTHER',
      slug: 'other',
      title: 'Other',
    });
    expect(confirmDialog).not.toHaveBeenCalled();
    expect(container.querySelector('#course-code').value).toBe('OTHER');
  });

  it('refuses navigator delete and copies a row without the course id', async () => {
    staffQuery();
    const { manager } = mount();
    await manager.init();
    const before = authQuery.mock.calls.length;
    tableBag.last.handleDelete();
    expect(toast.info).toHaveBeenCalledWith('Not deleted', expect.objectContaining({
      description: 'Courses stay in the catalog. Set Retired under Options.',
    }));
    expect(authQuery.mock.calls.length).toBe(before);
    const copy = tableBag.last.options.onDuplicate({
      course_id: 4,
      code: 'BBT',
      slug: 'bloom',
      title: 'Bloom',
      canvas_course_id: 90,
      stripe_product_id: 'prod_123',
      is_featured: 1,
      published: 1,
      retired: 1,
    });
    expect(copy.course_id).toBeUndefined();
    expect(copy.code).toBe('BBT-copy');
    expect(copy.slug).toBe('bloom-copy');
    expect(copy.title).toBe('Bloom (Copy)');
    expect(copy.canvas_course_id).toBeNull();
    expect(copy.stripe_product_id).toBeNull();
    expect(copy.published).toBe(0);
    expect(copy.retired).toBe(0);
    expect(copy.is_featured).toBe(1);
  });

  it('lists a price for each currency', async () => {
    staffQuery({
      rows: {
        [COURSE_PRICES_QUERY_REF]: [
          { currency: 'usd', unit_amount_cents: 1999, stripe_price_id: 'price_usd', is_active: 1 },
          { currency: 'eur', unit_amount_cents: 1000, stripe_price_id: 'sk_live_nope', is_active: 0 },
        ],
      },
    });
    const { container, manager } = mount();
    await manager.init();
    tableBag.last.options.onRowSelected({
      course_id: 4,
      code: 'BBT',
      slug: 'bloom',
      title: 'Bloom',
      stripe_product_id: 'prod_123',
    });
    await vi.waitFor(() => {
      expect(container.querySelector('#course-prices').classList.contains('is-empty')).toBe(false);
    });
    const usd = container.querySelector('[data-currency="usd"]');
    expect(usd.querySelector('.course-price-amount').textContent).toBe('19.99');
    expect(usd.querySelector('.course-price-stripe').textContent).toBe('price_usd');
    expect(usd.querySelector('.course-price-state').textContent).toBe('Active');
    const eur = container.querySelector('[data-currency="eur"]');
    expect(eur.querySelector('.course-price-amount').textContent).toBe('10.00');
    expect(eur.querySelector('.course-price-stripe').textContent).toBe('Hidden');
    expect(eur.querySelector('.course-price-state').textContent).toBe('Inactive');
    const cad = container.querySelector('[data-currency="cad"]');
    expect(cad.querySelector('.course-price-amount').textContent).toBe('None');
    expect(cad.querySelector('.course-price-stripe').textContent).toBe('None');
    expect(cad.querySelector('.course-price-state').textContent).toBe('');
    const prices = authQuery.mock.calls.find((call) => call[1] === COURSE_PRICES_QUERY_REF);
    expect(prices[2]).toEqual({ INTEGER: { COURSEID: 4 } });
  });

  it('loads enrolment history for the selected course', async () => {
    staffQuery({
      rows: {
        [ENROLMENT_HISTORY_QUERY_REF]: [{ line: 'User #1 renewed on 2026-Jan-01' }],
      },
    });
    const { container, manager } = mount();
    await manager.init();
    container.querySelector('#course-enrol-history-btn').click();
    expect(toast.error).toHaveBeenCalledWith('Select a course');
    tableBag.last.options.onRowSelected({
      course_id: 4,
      code: 'BBT',
      slug: 'bloom',
      title: 'Bloom',
    });
    container.querySelector('#course-enrol-history-btn').click();
    await vi.waitFor(() => {
      expect(container.querySelector('#course-history').textContent).toBe('User #1 renewed on 2026-Jan-01');
    });
    const history = authQuery.mock.calls.find((call) => call[1] === ENROLMENT_HISTORY_QUERY_REF);
    expect(history[2]).toEqual({ INTEGER: { COURSEID: 4 } });
  });

  it('calls sync only when the script is invokable', async () => {
    staffQuery({ invokable: true });
    invokeScript.mockResolvedValue({ status: 'completed' });
    const { container, manager } = mount();
    await manager.init();
    expect(container.querySelector('#course-sync-all-btn').disabled).toBe(false);
    expect(container.querySelector('#course-sync-btn').disabled).toBe(true);
    expect(container.querySelector('#course-sync-tip').title).toBe('Select a course');
    container.querySelector('#course-sync-all-btn').click();
    await vi.waitFor(() => expect(toast.success).toHaveBeenCalledWith('Sync finished'));
    const syncCall = invokeScript.mock.calls.find((call) => call[1].script === 'Catalog.SyncFromCanvas');
    expect(syncCall[1]).toEqual({
      script: 'Catalog.SyncFromCanvas',
      params: { action: 'all' },
      wait: true,
      timeoutSeconds: 15,
    });
    tableBag.last.options.onRowSelected({
      course_id: 4,
      code: 'BBT',
      slug: 'bloom',
      title: 'Bloom',
      retired: 1,
    });
    expect(container.querySelector('#course-retire-btn').textContent).toBe('Unretire');
    expect(container.querySelector('#course-sync-btn').disabled).toBe(false);
    container.querySelector('#course-retire-btn').click();
    await vi.waitFor(() => {
      const retireCall = invokeScript.mock.calls.find((call) => call[1].script === 'Catalog.Retire');
      expect(retireCall).toBeTruthy();
    });
    const retireCall = invokeScript.mock.calls.find((call) => call[1].script === 'Catalog.Retire');
    expect(retireCall[1].params).toEqual({ course_id: 4, action: 'unretire' });
  });

  it('ensures a product for a paid course when the script is invokable', async () => {
    staffQuery({ invokable: true });
    invokeScript.mockResolvedValue({
      status: 'completed',
      result: { mode: 'test', stripe_product_id: 'prod_new' },
    });
    const { container, manager } = mount();
    await manager.init();
    expect(container.querySelector('#course-stripe-mode').textContent).toBe('Test');
    tableBag.last.options.onRowSelected({
      course_id: 4,
      code: 'BBT',
      slug: 'bloom',
      title: 'Bloom',
      pricing_type: 'paid',
    });
    await vi.waitFor(() => {
      expect(container.querySelector('#course-ensure-btn').disabled).toBe(false);
    });
    expect(container.querySelector('#course-rotate-btn').disabled).toBe(false);
    container.querySelector('#course-ensure-btn').click();
    await vi.waitFor(() => expect(toast.success).toHaveBeenCalledWith('Stripe product checked'));
    const ensureCall = invokeScript.mock.calls.find((call) => call[1].script === 'Stripe.EnsureProduct');
    expect(ensureCall[1]).toEqual({
      script: 'Stripe.EnsureProduct',
      params: { course_id: 4 },
      wait: true,
      timeoutSeconds: 15,
    });
    expect(container.querySelector('#course-stripe').textContent).toBe('prod_new');
  });

  it('leaves a price edit behind with a notice and does not ask to confirm', async () => {
    staffQuery();
    const { container, manager } = mount();
    await manager.init();
    tableBag.last.options.onRowSelected({
      course_id: 4,
      code: 'BBT',
      slug: 'bloom',
      title: 'Bloom',
      pricing_type: 'paid',
    });
    await vi.waitFor(() => {
      expect(container.querySelector('#course-prices').classList.contains('is-empty')).toBe(false);
    });
    manager.editor.setEditable(true);
    expect(container.querySelector('#course-prices').classList.contains('is-editing')).toBe(true);
    const usd = container.querySelector('[data-currency="usd"] .course-price-input');
    usd.value = '19.99';
    usd.dispatchEvent(new window.Event('input', { bubbles: true }));
    tableBag.last.options.onRowSelected({
      course_id: 9,
      code: 'OTHER',
      slug: 'other',
      title: 'Other',
    });
    expect(toast.info).toHaveBeenCalledWith('Price changes were not saved.');
    expect(confirmDialog).not.toHaveBeenCalled();
  });

  it('opens Canvas only for a positive course id', async () => {
    staffQuery();
    const open = vi.spyOn(window, 'open').mockImplementation(() => null);
    const { container, manager } = mount();
    await manager.init();
    expect(container.querySelector('#course-canvas-btn').disabled).toBe(true);
    tableBag.last.options.onRowSelected({
      course_id: 4,
      code: 'BBT',
      slug: 'bloom',
      title: 'Bloom',
      canvas_course_id: 90,
      image_path: 'https://cdn.example/a.png',
    });
    expect(container.querySelector('#course-canvas-btn').disabled).toBe(false);
    const preview = container.querySelector('#course-tab-canvas #course-image-preview');
    expect(preview.classList.contains('is-visible')).toBe(true);
    expect(preview.getAttribute('src')).toBe('https://cdn.example/a.png');
    const previewRule = courseCss.match(/\.course-image-preview\s*\{([^}]*)\}/);
    expect(previewRule).not.toBeNull();
    expect(previewRule[1]).toMatch(/width:\s*524px/);
    expect(previewRule[1]).toMatch(/height:\s*292px/);
    expect(previewRule[1]).toMatch(/object-fit:\s*fill/);
    expect(previewRule[1]).toMatch(/margin-top:\s*var\(--space-5\)/);
    expect(previewRule[1]).toMatch(/border-radius:\s*var\(--border-radius-lg\)/);
    container.querySelector('#course-image').value = '/media/b.png';
    container.querySelector('#course-image').dispatchEvent(new window.Event('input', { bubbles: true }));
    expect(preview.getAttribute('src')).toBe('/media/b.png');
    container.querySelector('#course-canvas-btn').click();
    expect(open).toHaveBeenCalledWith(
      'https://canvas.500courses.com/courses/90',
      '_blank',
      'noopener,noreferrer',
    );
    open.mockRestore();
  });

  it('clears the container on cleanup', async () => {
    staffQuery();
    const { container, manager } = mount();
    await manager.render();
    manager.cleanup();
    expect(container.innerHTML).toBe('');
    expect(tableBag.last.destroy).toHaveBeenCalled();
  });
});

describe('ManagerLoader id 34', () => {
  it('imports CourseManager', async () => {
    const loader = new ManagerLoader({});
    const module = await loader._importManager(34);
    expect(module.default).toBe(CourseManager);
    expect(loader.managerRegistry[34].name).toBe('Course Manager');
  });
});
