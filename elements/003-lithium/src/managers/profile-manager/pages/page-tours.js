/**
 * Profile Manager - Tours Page Handler
 *
 * Handles the Tours settings page (index: -15)
 */

import { SimpleSettingsPage } from './settings-page-base.js';
import { log, Subsystems, Status } from '../../../core/log.js';

export class ToursPage extends SimpleSettingsPage {
  constructor(options = {}) {
    super({ ...options, index: -15, formSelector: 'form' });
  }
  async onInit() { await super.onInit(); log(Subsystems.MANAGER, Status.DEBUG, '[ToursPage] Initialized (stub)'); }
}

export default ToursPage;
