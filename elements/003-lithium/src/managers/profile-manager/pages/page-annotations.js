/**
 * Profile Manager - Annotations Page Handler
 *
 * Handles the Annotations settings page (index: -14)
 */

import { SimpleSettingsPage } from './settings-page-base.js';
import { log, Subsystems, Status } from '../../../core/log.js';

export class AnnotationsPage extends SimpleSettingsPage {
  constructor(options = {}) {
    super({ ...options, index: -14, formSelector: 'form' });
  }
  async onInit() { await super.onInit(); log(Subsystems.MANAGER, Status.DEBUG, '[AnnotationsPage] Initialized (stub)'); }
}

export default AnnotationsPage;
