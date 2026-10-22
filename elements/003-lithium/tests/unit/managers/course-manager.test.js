import { describe, it, expect, vi, beforeEach } from 'vitest';

vi.mock('../../../src/shared/conduit.js', () => ({
  authQuery: vi.fn(),
}));

vi.mock('../../../src/core/jwt.js', () => ({
  getClaims: vi.fn(() => null),
}));

vi.mock('../../../src/core/icons.js', () => ({
  processIcons: vi.fn(),
}));

import { authQuery } from '../../../src/shared/conduit.js';
import { getClaims } from '../../../src/core/jwt.js';
import { processIcons } from '../../../src/core/icons.js';
import CourseManager from '../../../src/managers/course-manager/course-manager.js';
import { ManagerLoader } from '../../../src/app/manager-loader.js';

function mount() {
  const container = document.createElement('div');
  const app = { api: { post() {} } };
  return { container, manager: new CourseManager(app, container) };
}

describe('CourseManager shell', () => {
  beforeEach(() => {
    authQuery.mockReset();
    processIcons.mockClear();
    getClaims.mockReturnValue({ roles: '2' });
  });

  it('shows the title and one sentence for a staff name', async () => {
    authQuery.mockResolvedValueOnce([{ name: 'staff' }]);
    const { container, manager } = mount();
    await manager.init();
    expect(container.querySelector('h2').textContent).toBe('Course Manager');
    expect(container.querySelector('.course-manager-intro').textContent)
      .toBe('Manage the 500 Courses catalog, enrollments, commerce, and learner accounts.');
    expect(container.textContent).not.toMatch(/lua/i);
    expect(container.textContent).not.toMatch(/phase/i);
    expect(container.querySelector('button')).toBeNull();
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
  });

  it('shows the lock panel when QueryRef 155 fails for role id 2', async () => {
    authQuery.mockRejectedValueOnce(new Error('not applied'));
    const { container, manager } = mount();
    await manager.render();
    expect(container.querySelector('.course-manager-denied')).not.toBeNull();
    expect(manager._allowed).toBe(false);
  });

  it('clears the container on cleanup', async () => {
    authQuery.mockResolvedValueOnce([{ name: 'admin' }]);
    const { container, manager } = mount();
    await manager.render();
    manager.cleanup();
    expect(container.innerHTML).toBe('');
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
