/**
 * Profile Manager - Startup Page Handler
 *
 * Handles the Startup settings page (index: -11)
 */

import { SimpleSettingsPage } from './settings-page-base.js';
import { log, Subsystems, Status } from '../../../core/log.js';

export class StartupPage extends SimpleSettingsPage {
  constructor(options = {}) {
    super({ ...options, index: -11, formSelector: 'form' });
  }
  async onInit() { await super.onInit(); log(Subsystems.MANAGER, Status.DEBUG, '[StartupPage] Initialized (stub)'); }
}

export default StartupPage;
