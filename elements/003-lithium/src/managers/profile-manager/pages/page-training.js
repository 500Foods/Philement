/**
 * Profile Manager - Training Page Handler
 *
 * Handles the Training settings page (index: -16)
 */

import { SimpleSettingsPage } from './settings-page-base.js';
import { log, Subsystems, Status } from '../../../core/log.js';

export class TrainingPage extends SimpleSettingsPage {
  constructor(options = {}) {
    super({ ...options, index: -16, formSelector: 'form' });
  }
  async onInit() { await super.onInit(); log(Subsystems.MANAGER, Status.DEBUG, '[TrainingPage] Initialized (stub)'); }
}

export default TrainingPage;
