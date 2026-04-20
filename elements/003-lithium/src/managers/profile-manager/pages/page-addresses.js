/**
 * Profile Manager - Addresses Page Handler
 *
 * Handles the Addresses settings page (index: -3)
 */

import { SimpleSettingsPage } from './settings-page-base.js';
import { log, Subsystems, Status } from '../../../core/log.js';

/**
 * Addresses Page Handler
 */
export class AddressesPage extends SimpleSettingsPage {
  constructor(options = {}) {
    super({
      ...options,
      index: -3,
      formSelector: 'form',
    });
  }

  async onInit() {
    await super.onInit();
    log(Subsystems.MANAGER, Status.DEBUG, '[AddressesPage] Initialized (stub)');
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

export default AddressesPage;
