import { describe, it, expect, vi, beforeEach } from 'vitest';
import { readFileSync } from 'fs';
import { resolve } from 'path';

vi.mock('../../../src/core/jwt.js', () => ({
  getClaims: vi.fn((t) => (t ? { username: 'andrew', email: 'a@x', roles: [], database: 'Acuranzo' } : null)),
  retrieveJWT: vi.fn(() => 'stored-token'),
}));

const { getClaims } = await import('../../../src/core/jwt.js');
const { AccountPage } = await import('../../../src/managers/profile-manager/pages/account/page-account.js');

describe('getClaims callers pass the stored JWT', () => {
  beforeEach(() => getClaims.mockClear());

  it('AccountPage.loadUserInfo fills fields from claims', () => {
    const page = new AccountPage({});
    page.container = document.createElement('div');
    page.container.innerHTML = '<span id="profile-username"></span><span id="profile-email"></span><span id="profile-roles"></span><span id="profile-database"></span>';
    page.loadUserInfo();
    expect(getClaims).toHaveBeenCalledWith('stored-token');
    expect(page.container.querySelector('#profile-username').textContent).toBe('andrew');
  });

  it('no bare getClaims() calls remain in src', () => {
    const files = [
      'src/managers/profile-manager/profile-manager.js',
      'src/managers/style-manager/style-manager.js',
      'src/managers/main/main-state.js',
      'src/managers/profile-manager/pages/account/page-account.js',
      'src/managers/profile-manager/pages/login-history/page-login-history.js',
    ];
    for (const f of files) {
      const code = readFileSync(resolve(process.cwd(), f), 'utf8').replace(/\/\/.*$/gm, '');
      expect(code, f).not.toMatch(/getClaims\(\s*\)/);
    }
  });
});
