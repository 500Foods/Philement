/**
 * Profile Manager - Tour Manager Page Handler
 *
 * Handles the Tour Manager settings page (index: 6)
 */

import { SimpleSettingsPage } from '../settings-page-base.js';
import { log, Subsystems, Status } from '../../../../core/log.js';

export class TourManagerPage extends SimpleSettingsPage {
  constructor(options = {}) {
    super({ ...options, index: 6, formSelector: 'form' });
  }
  async onInit() { await super.onInit(); log(Subsystems.MANAGER, Status.DEBUG, '[TourManagerPage] Initialized'); }
}

export default TourManagerPage;
