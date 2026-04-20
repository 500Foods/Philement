/**
 * Profile Manager - Account Page Handler
 *
 * Handles the Account settings page (index: -1)
 * Displays user information from JWT claims
 */

import { BaseSettingsPage } from './settings-page-base.js';
import { getClaims } from '../../../core/jwt.js';
import { log, Subsystems, Status } from '../../../core/log.js';

/**
 * Account Page Handler
 */
export class AccountPage extends BaseSettingsPage {
  constructor(options = {}) {
    super({
      ...options,
      index: -1,
    });
  }

  /**
   * Called when the page is initialized
   */
  async onInit() {
    // Load and display user info
    this.loadUserInfo();
    log(Subsystems.MANAGER, Status.DEBUG, '[AccountPage] Initialized');
  }

  /**
   * Load user information from JWT claims
   */
  loadUserInfo() {
    const claims = getClaims();
    if (!claims || !this.container) return;

    const fields = {
      '#profile-username': claims.username || '-',
      '#profile-email': claims.email || '-',
      '#profile-roles': Array.isArray(claims.roles) ? claims.roles.join(', ') : claims.roles || '-',
      '#profile-database': claims.database || '-',
    };

    Object.entries(fields).forEach(([selector, value]) => {
      const el = this.container.querySelector(selector);
      if (el) el.textContent = value;
    });
  }

  /**
   * Called when the page becomes visible
   */
  onShow() {
    // Refresh user info when page is shown (in case claims changed)
    this.loadUserInfo();
  }

  /**
   * Refresh the page data
   */
  refresh() {
    this.loadUserInfo();
  }
}

export default AccountPage;
