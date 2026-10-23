import { describe, it, expect, vi, beforeEach } from 'vitest';

const tableBag = { last: null };

vi.mock('../../../src/shared/conduit.js', () => ({
  authQuery: vi.fn(),
  invokeScript: vi.fn(),
}));

vi.mock('../../../src/shared/confirm.js', () => ({
  confirmDialog: vi.fn(async () => false),
}));

vi.mock('../../../src/shared/toast.js', () => ({
  toast: { error: vi.fn(), success: vi.fn() },
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
  COURSE_UPDATE_QUERY_REF,
  ENROLMENT_HISTORY_QUERY_REF,
} from '../../../src/managers/course-manager/course-catalog.js';

const INTRO = 'Manage the 500 Courses catalog, enrollments, commerce, and learner accounts.';

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
    expect(container.querySelector('h2').textContent).toBe('Course Manager');
    expect(container.querySelector('.course-manager-intro').textContent).toBe(INTRO);
    expect(container.textContent).not.toMatch(/lua/i);
    expect(container.textContent).not.toMatch(/phase/i);
    expect(container.querySelector('#course-search')).not.toBeNull();
    expect(container.querySelector('#course-sync-btn').disabled).toBe(true);
    expect(container.querySelector('#course-sync-tip').title).toBe('not enabled on this server');
    expect(container.querySelector('#course-sync-all-btn').disabled).toBe(true);
    expect(container.querySelector('#course-retire-btn').disabled).toBe(true);
    expect(tableBag.last.options.queryRef).toBe(COURSE_LIST_QUERY_REF);
    expect(tableBag.last.options.primaryKeyField).toEqual(['course_id']);
    expect(tableBag.last.options.readonly).toBe(true);
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
    expect(container.querySelector('#course-search')).toBeNull();
    expect(container.querySelector('#course-code')).toBeNull();
  });

  it('shows the lock panel when QueryRef 155 fails for role id 2', async () => {
    authQuery.mockRejectedValueOnce(new Error('not applied'));
    const { container, manager } = mount();
    await manager.render();
    expect(container.querySelector('.course-manager-denied')).not.toBeNull();
    expect(manager._allowed).toBe(false);
    expect(container.querySelector('#course-search')).toBeNull();
  });

  it('filters in the browser and reloads from the list query', async () => {
    staffQuery();
    const { container, manager } = mount();
    await manager.init();
    container.querySelector('#course-search').value = 'bloom';
    container.querySelector('#course-search').dispatchEvent(new Event('input'));
    expect(tableBag.last.table.setFilter).toHaveBeenCalled();
    container.querySelector('#course-search').value = '';
    container.querySelector('#course-retired-chip').checked = false;
    container.querySelector('#course-search').dispatchEvent(new Event('input'));
    expect(tableBag.last.table.clearFilter).toHaveBeenCalled();
    container.querySelector('#course-refresh-btn').click();
    await vi.waitFor(() => {
      expect(tableBag.last.loadData.mock.calls.length).toBeGreaterThan(1);
    });
  });

  it('inserts a course and does not send the list query as the write', async () => {
    staffQuery();
    const { container, manager } = mount();
    await manager.init();
    container.querySelector('#course-code').value = 'BBT';
    container.querySelector('#course-slug').value = 'bloom';
    container.querySelector('#course-title').value = 'Bloom';
    container.querySelector('#course-save-btn').click();
    await vi.waitFor(() => expect(toast.success).toHaveBeenCalled());
    const insert = authQuery.mock.calls.find((call) => call[1] === COURSE_INSERT_QUERY_REF);
    expect(insert).toBeTruthy();
    expect(insert[2].STRING.CODE).toBe('BBT');
    expect(insert[2].STRING.TITLE).toBe('Bloom');
    expect(insert[2].STRING.TAGS).toBe('{}');
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
    container.querySelector('#course-save-btn').click();
    await vi.waitFor(() => expect(toast.success).toHaveBeenCalled());
    const update = authQuery.mock.calls.find((call) => call[1] === COURSE_UPDATE_QUERY_REF);
    expect(update[2].STRING.CODE).toBe('BBT2');
    expect(update[2].INTEGER.COURSEID).toBe(4);
    expect(update[2].INTEGER.USERID).toBe(7);
    expect(update[2].STRING.TITLE).toBeUndefined();
    expect(update[2].STRING.TAGS).toBe('{"topic":"art"}');
  });

  it('stays on the row when the operator declines to save', async () => {
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
    container.querySelector('#course-code').dispatchEvent(new Event('input', { bubbles: true }));
    tableBag.last.options.onRowSelected({
      course_id: 9,
      code: 'OTHER',
      slug: 'other',
      title: 'Other',
    });
    await vi.waitFor(() => expect(tableBag.last.autoSelectRow).toHaveBeenCalledWith(4));
    expect(confirmDialog).toHaveBeenCalled();
    expect(container.querySelector('#course-code').value).toBe('CHANGED');
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
    expect(invokeScript.mock.calls[0][1]).toEqual({
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
    await vi.waitFor(() => expect(invokeScript.mock.calls.length).toBe(2));
    expect(invokeScript.mock.calls[1][1].params).toEqual({ course_id: 4, action: 'unretire' });
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
    expect(container.querySelector('#course-image-preview').classList.contains('is-visible')).toBe(true);
    expect(container.querySelector('#course-image-preview').getAttribute('src'))
      .toBe('https://cdn.example/a.png');
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
