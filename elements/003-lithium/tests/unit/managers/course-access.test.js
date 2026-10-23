import { describe, it, expect, vi, beforeEach } from 'vitest';

vi.mock('../../../src/shared/conduit.js', () => ({
  authQuery: vi.fn(),
}));

vi.mock('../../../src/core/jwt.js', () => ({
  getClaims: vi.fn(),
  retrieveJWT: vi.fn(),
}));

import { authQuery } from '../../../src/shared/conduit.js';
import { getClaims, retrieveJWT } from '../../../src/core/jwt.js';
import {
  COURSE_MANAGER_ID,
  ROLE_NAME_QUERY_REF,
  permittedManagersForMenu,
  resolveCourseManagerAccess,
  visibleMenuGroups,
} from '../../../src/managers/course-manager/course-access.js';

const learning = {
  id: 6,
  name: 'Learning',
  items: [
    { managerId: 34, name: 'Course Manager' },
    { managerId: 35, name: 'Course Builder' },
  ],
};

describe('visibleMenuGroups', () => {
  it('leaves the menu in place when access is allowed', () => {
    const groups = [learning];
    expect(visibleMenuGroups(groups, true)).toBe(groups);
  });

  it('omits manager 34 and keeps the rest of the group', () => {
    const result = visibleMenuGroups([learning], false);
    expect(result).toHaveLength(1);
    expect(result[0].items.map((item) => item.managerId)).toEqual([35]);
    expect(learning.items).toHaveLength(2);
  });

  it('drops a group that only contained manager 34', () => {
    const only = { id: 6, name: 'Learning', items: [{ managerId: COURSE_MANAGER_ID }] };
    expect(visibleMenuGroups([only], false)).toEqual([]);
  });

  it('returns an empty list for a missing menu', () => {
    expect(visibleMenuGroups(null, false)).toEqual([]);
  });
});

describe('permittedManagersForMenu', () => {
  it('keeps 34 when access is allowed', () => {
    expect(permittedManagersForMenu([33, 34, 35], true)).toEqual([33, 34, 35]);
  });

  it('removes 34 when access is denied', () => {
    expect(permittedManagersForMenu([33, 34, 35], false)).toEqual([33, 35]);
  });
});

describe('resolveCourseManagerAccess', () => {
  beforeEach(() => {
    authQuery.mockReset();
    getClaims.mockReset();
    retrieveJWT.mockReset();
  });

  it('reads roles from the stored token when claims are omitted', async () => {
    retrieveJWT.mockReturnValue('stored-token');
    getClaims.mockReturnValue({ roles: '2,3' });
    authQuery
      .mockResolvedValueOnce([{ name: 'staff' }])
      .mockResolvedValueOnce([{ name: 'admin' }]);
    const access = await resolveCourseManagerAccess({ post() {} });
    expect(getClaims).toHaveBeenCalledWith('stored-token');
    expect(access.allowed).toBe(true);
    expect(access.roleIds).toEqual([2, 3]);
  });

  it('denies when no token is stored', async () => {
    retrieveJWT.mockReturnValue(null);
    getClaims.mockReturnValue(null);
    const access = await resolveCourseManagerAccess({ post() {} });
    expect(access.allowed).toBe(false);
    expect(access.roleIds).toEqual([]);
    expect(authQuery).not.toHaveBeenCalled();
  });

  it('allows a resolved staff name', async () => {
    authQuery.mockResolvedValueOnce([{ name: 'staff' }]);
    const access = await resolveCourseManagerAccess({ post() {} }, { roles: '2' });
    expect(access.allowed).toBe(true);
    expect(access.roleNames).toEqual(['staff']);
    expect(authQuery).toHaveBeenCalledWith(
      expect.anything(),
      ROLE_NAME_QUERY_REF,
      { INTEGER: { ROLEID: 2 } },
    );
  });

  it('allows a resolved admin name among other roles', async () => {
    authQuery
      .mockResolvedValueOnce([{ name: 'mail_send' }])
      .mockResolvedValueOnce([{ name: 'admin' }]);
    const access = await resolveCourseManagerAccess({ post() {} }, { roles: [1, 3] });
    expect(access.allowed).toBe(true);
    expect(access.roleNames).toEqual(['mail_send', 'admin']);
  });

  it('denies a resolved name that is not staff or admin', async () => {
    authQuery.mockResolvedValueOnce([{ name: 'mail_send' }]);
    const access = await resolveCourseManagerAccess({ post() {} }, { roles: '1' });
    expect(access.allowed).toBe(false);
  });

  it('denies role id 2 when QueryRef 155 fails', async () => {
    authQuery.mockRejectedValueOnce(new Error('query missing'));
    const access = await resolveCourseManagerAccess({ post() {} }, { roles: '2' });
    expect(access.allowed).toBe(false);
    expect(access.roleNames).toEqual([]);
  });

  it('denies when the client is missing', async () => {
    const access = await resolveCourseManagerAccess(null, { roles: '2,3' });
    expect(access.allowed).toBe(false);
    expect(authQuery).not.toHaveBeenCalled();
  });

  it('denies when the token has no role ids', async () => {
    const access = await resolveCourseManagerAccess({ post() {} }, { roles: 'staff' });
    expect(access.allowed).toBe(false);
    expect(access.roleIds).toEqual([]);
    expect(authQuery).not.toHaveBeenCalled();
  });

  it('denies a non-array QueryRef 155 payload', async () => {
    authQuery.mockResolvedValueOnce({ name: 'staff' });
    const access = await resolveCourseManagerAccess({ post() {} }, { roles: 2 });
    expect(access.allowed).toBe(false);
  });
});
