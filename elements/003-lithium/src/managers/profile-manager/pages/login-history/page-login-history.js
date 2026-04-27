/**
 * Profile Manager - Login History Page Handler
 *
 * Handles the Login History settings page (index: -17)
 */

import { BaseSettingsPage } from '../settings-page-base.js';
import { log, Subsystems, Status } from '../../../../core/log.js';

export class LoginHistoryPage extends BaseSettingsPage {
  constructor(options = {}) {
    super({ ...options, index: -17 });
  }
  async onInit() {
    log(Subsystems.MANAGER, Status.DEBUG, '[LoginHistoryPage] Initialized (stub)');
  }
}

export default LoginHistoryPage;
