/**
 * Profile Manager - Notifications Page Handler
 *
 * Handles the Notifications settings page (index: -12)
 */

import { SimpleSettingsPage } from './settings-page-base.js';
import { log, Subsystems, Status } from '../../../core/log.js';

export class NotificationsPage extends SimpleSettingsPage {
  constructor(options = {}) {
    super({ ...options, index: -12, formSelector: 'form' });
  }
  async onInit() { await super.onInit(); log(Subsystems.MANAGER, Status.DEBUG, '[NotificationsPage] Initialized (stub)'); }
}

export default NotificationsPage;
