/**
 * Profile Manager - Tokens Page Handler
 *
 * Handles the Tokens settings page (index: -7)
 */

import { SimpleSettingsPage } from '../settings-page-base.js';
import { log, Subsystems, Status } from '../../../../core/log.js';

export class TokensPage extends SimpleSettingsPage {
  constructor(options = {}) {
    super({ ...options, index: -7, formSelector: 'form' });
  }
  async onInit() { await super.onInit(); log(Subsystems.MANAGER, Status.DEBUG, '[TokensPage] Initialized (stub)'); }
}

export default TokensPage;
