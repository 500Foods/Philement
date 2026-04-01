/**
 * Annotation Manager
 *
 * Placeholder manager for annotations and notes.
 */

import { processIcons } from '../../core/icons.js';
import './annotation-manager.css';

export default class AnnotationManager {
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
      <div class="annotation-manager-container">
        <div class="placeholder-header">
          <fa fa-note-sticky></fa>
          <h2>Annotation Manager</h2>
        </div>
        <div class="placeholder-content">
          <p>This manager will handle annotations and scratch-pad notes.</p>
          <div class="placeholder-notice">
            <fa fa-info-circle></fa>
            <span>Annotation Manager module is under development.</span>
          </div>
        </div>
      </div>
    `;

    processIcons(this.container);
  }

  setupEventListeners() {
    // Placeholder - no event listeners yet
  }

  teardown() {
    // Cleanup if needed
  }
}
