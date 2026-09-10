/**
 * Course Builder — Manager ID 35
 *
 * Placeholder manager for the 500 Courses Course Builder pipeline UI.
 * Full implementation: CATCHUP Band E.
 */

import { processIcons } from '../../core/icons.js';
import './course-builder.css';

export default class CourseBuilderManager {
  constructor(app, container) {
    this.app = app;
    this.container = container;
    this.elements = {};
  }

  async init() {
    await this.render();
    this.setupEventListeners();
  }

  async render() {
    this.container.innerHTML = `
      <div class="course-builder-container">
        <div class="placeholder-header">
          <fa fa-pencil-ruler></fa>
          <h2>Course Builder</h2>
        </div>
        <div class="placeholder-content">
          <p>This manager will handle the 500 Courses Course Builder pipeline — operator queue, human gates, and script invocation.</p>
          <div class="placeholder-notice">
            <fa fa-info-circle></fa>
            <span>Course Builder module is under development.</span>
          </div>
        </div>
      </div>
    `;

    processIcons(this.container);
  }

  setupEventListeners() {
  }

  teardown() {
  }
}
