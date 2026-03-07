/**
 * Lookups Manager
 * 
 * Server-provided reference data management.
 * Placeholder implementation.
 */

export default class LookupsManager {
  constructor(app, container) {
    this.app = app;
    this.container = container;
  }

  async init() {
    this.container.innerHTML = `
      <div style="
        display: flex;
        flex-direction: column;
        align-items: center;
        justify-content: center;
        height: 100%;
        color: var(--text-secondary);
      ">
        <fa fa-list style="font-size: 48px; margin-bottom: 16px; opacity: 0.5;"></fa>
        <h2>Lookups</h2>
        <p>Reference data management coming soon</p>
      </div>
    `;
  }

  teardown() {
    // Cleanup
  }
}
