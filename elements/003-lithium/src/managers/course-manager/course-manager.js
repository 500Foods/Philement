/**
 * Course Manager — Manager ID 34
 *
 * Staff shell for the 500 Courses operator surface.
 * Authz is CATCHUP L3: QueryRef 155 names, then staff or admin.
 * Catalog, commerce, learners, and inbox land in later Band D phases.
 */

import { processIcons } from '../../core/icons.js';
import { resolveCourseManagerAccess } from './course-access.js';
import './course-manager.css';

const SHELL_TEXT = 'Manage the 500 Courses catalog, enrollments, commerce, and learner accounts.';

export default class CourseManager {
  constructor(app, container) {
    this.app = app;
    this.container = container;
    this.elements = {};
    this._allowed = false;
  }

  async init() {
    await this.render();
  }

  async render() {
    const access = await resolveCourseManagerAccess(this.app && this.app.api);
    this._allowed = access.allowed;
    if (!access.allowed) {
      this._renderDenied();
      return;
    }
    this._renderShell();
  }

  _renderShell() {
    this.container.innerHTML = `
      <div class="course-manager-container">
        <div class="course-manager-header">
          <fa fa-book-open></fa>
          <h2>Course Manager</h2>
        </div>
        <p class="course-manager-intro">${SHELL_TEXT}</p>
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

  cleanup() {
    if (this.container) {
      this.container.innerHTML = '';
    }
  }
}
