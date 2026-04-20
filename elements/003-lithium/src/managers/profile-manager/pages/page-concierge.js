/**
 * Profile Manager - Concierge Page Handler
 *
 * Handles the Concierge settings page (index: -13)
 */

import { SimpleSettingsPage } from './settings-page-base.js';
import { log, Subsystems, Status } from '../../../core/log.js';

export class ConciergePage extends SimpleSettingsPage {
  constructor(options = {}) {
    super({ ...options, index: -13, formSelector: 'form' });
  }
  async onInit() { await super.onInit(); log(Subsystems.MANAGER, Status.DEBUG, '[ConciergePage] Initialized (stub)'); }
}

export default ConciergePage;
