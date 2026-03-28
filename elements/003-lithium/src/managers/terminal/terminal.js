/**
 * Terminal Manager
 *
 * Placeholder manager for command line terminal.
 */

import { processIcons } from '../../core/icons.js';
import './terminal.css';

export default class TerminalManager {
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
      <div class="terminal-container">
        <div class="placeholder-header">
          <fa fa-terminal></fa>
          <h2>Terminal</h2>
        </div>
        <div class="placeholder-content">
          <p>This manager will provide a command line terminal interface.</p>
          <div class="placeholder-notice">
            <fa fa-info-circle></fa>
            <span>Terminal module is under development.</span>
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
