/**
 * Chats Manager
 *
 * Placeholder manager for chats.
 */

import { processIcons } from '../../core/icons.js';
import './chats.css';

export default class ChatsManager {
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
      <div class="chats-container">
        <div class="placeholder-header">
          <fa fa-comments></fa>
          <h2>Chats</h2>
        </div>
        <div class="placeholder-content">
          <p>This manager will handle chat conversations and history.</p>
          <div class="placeholder-notice">
            <fa fa-info-circle></fa>
            <span>Chats module is under development.</span>
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
