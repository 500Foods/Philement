/**
 * Course Manager — Manager ID 34
 *
 * First official 500 Courses operator deployment.
 * Authz for v1: JWT roles claim (staff/admin) — see CATCHUP L3.
 *
 * Full feature set: CATCHUP Band D (Phases 12–18).
 */

import { processIcons } from '../../core/icons.js';
import { parseRoleIds, isStaffRoleSet } from '../../core/utils.js';
import { getClaims } from '../../core/jwt.js';
import { authQuery } from '../../shared/conduit.js';
import { log, Subsystems, Status } from '../../core/log.js';
import './course-manager.css';

/** Role IDs seeded for staff/admin (acuranzo_1380). */
const STAFF_ROLE_IDS = [2, 3];

export default class CourseManager {
  constructor(app, container) {
    this.app = app;
    this.container = container;
    this.elements = {};
    this._isStaff = false;
    this._roleNames = [];
  }

  async init() {
    await this.render();
    this.setupEventListeners();
  }

  async _checkStaffAccess() {
    const claims = getClaims();
    if (!claims) {
      this._isStaff = false;
      this._roleNames = [];
      log(Subsystems.MANAGER, Status.INFO, 'Course Manager: no JWT claims');
      return;
    }
    const roleIds = parseRoleIds(claims.roles);
    log(Subsystems.MANAGER, Status.INFO,
      `Course Manager: role_ids=[${roleIds.join(',')}] resolving names`);

    if (roleIds.length === 0) {
      this._isStaff = false;
      this._roleNames = [];
      return;
    }

    this._roleNames = await this._resolveRoleNames(roleIds);
    this._isStaff = isStaffRoleSet(this._roleNames);
    log(Subsystems.MANAGER, Status.INFO,
      `Course Manager: resolved names=[${this._roleNames.join(',')}] staff=${this._isStaff}`);
  }

  async _resolveRoleNames(roleIds) {
    if (!this.app.api) {
      const names = roleIds
        .map(id => STAFF_ROLE_IDS.includes(id) ? (id === 2 ? 'staff' : 'admin') : null)
        .filter(Boolean);
      return names;
    }

    try {
      const rows = await authQuery(this.app.api, 155, {
        INTEGER: roleIds.reduce((acc, id, i) => { acc[`ID${i}`] = id; return acc; }, {}),
      });
      return rows.map(r => r.name);
    } catch (err) {
      log(Subsystems.MANAGER, Status.WARN,
        `Course Manager: role name resolution failed, falling back to ID check: ${err.message}`);
      return roleIds
        .map(id => STAFF_ROLE_IDS.includes(id) ? (id === 2 ? 'staff' : 'admin') : null)
        .filter(Boolean);
    }
  }

  async render() {
    await this._checkStaffAccess();

    if (!this._isStaff) {
      this._renderDenied();
      return;
    }

    this.container.innerHTML = `
      <div class="course-manager-container">
        <div class="course-manager-header">
          <fa fa-book-open></fa>
          <h2>Course Manager</h2>
        </div>
        <div class="course-manager-body">
          <p class="course-manager-intro">
            Manage 500 Courses catalog, enrollments, commerce, and learner administration.
          </p>
          <div class="course-manager-placeholder">
            <fa fa-info-circle></fa>
            <span>Course Manager UI is coming in Phase 12 (CATCHUP Band D).</span>
          </div>
        </div>
      </div>
    `;

    processIcons(this.container);
  }

  _renderDenied() {
    this.container.innerHTML = `
      <div class="course-manager-container">
        <div class="course-manager-denied">
          <fa fa-lock></fa>
          <h2>Course Manager</h2>
          <p class="denied-message">
            You do not have permission to access Course Manager.
            Contact your system administrator if you believe this is an error.
          </p>
        </div>
      </div>
    `;
    processIcons(this.container);
  }

  setupEventListeners() {
  }

  cleanup() {
    if (this.container) {
      this.container.innerHTML = '';
    }
  }
}
