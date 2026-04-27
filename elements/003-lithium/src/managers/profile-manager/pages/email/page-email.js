/**
 * Profile Manager - E-Mail Page Handler
 *
 * Handles the E-Mail settings page (index: -4)
 */

import { SimpleSettingsPage } from '../settings-page-base.js';
import { log, Subsystems, Status } from '../../../../core/log.js';

/**
 * Email Page Handler
 */
export class EmailPage extends SimpleSettingsPage {
  constructor(options = {}) {
    super({
      ...options,
      index: -4,
      formSelector: 'form',
    });
  }

  async onInit() {
    await super.onInit();
    log(Subsystems.MANAGER, Status.DEBUG, '[EmailPage] Initialized (stub)');
  }

  async loadData() {
    // TODO: Load from API
  }

  async save() {
    // TODO: Save to API
    this.setDirty(false);
    return { success: true };
  }
}

export default EmailPage;
