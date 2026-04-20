/**
 * Profile Manager - Authentication Page Handler
 *
 * Handles the Authentication settings page (index: -6)
 */

import { BaseSettingsPage } from './settings-page-base.js';
import { log, Subsystems, Status } from '../../../core/log.js';

export class AuthenticationPage extends BaseSettingsPage {
  constructor(options = {}) {
    super({ ...options, index: -6 });
  }
  async onInit() {
    log(Subsystems.MANAGER, Status.DEBUG, '[AuthenticationPage] Initialized (stub)');
  }
}

export default AuthenticationPage;
