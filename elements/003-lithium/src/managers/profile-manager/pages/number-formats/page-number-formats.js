/**
 * Profile Manager - Number Formats Page Handler
 *
 * Handles the Number Formats settings page (index: -10)
 */

import { SimpleSettingsPage } from '../settings-page-base.js';
import { log, Subsystems, Status } from '../../../../core/log.js';

export class NumberFormatsPage extends SimpleSettingsPage {
  constructor(options = {}) {
    super({ ...options, index: -10, formSelector: 'form' });
  }
  async onInit() { await super.onInit(); log(Subsystems.MANAGER, Status.DEBUG, '[NumberFormatsPage] Initialized (stub)'); }
  async loadData() { /* TODO */ }
  async save() { this.setDirty(false); return { success: true }; }
}

export default NumberFormatsPage;
