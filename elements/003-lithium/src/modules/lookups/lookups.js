// Lookups Module for Lithium PWA
// Placeholder lookups interface

import htmlTemplate from './lookups.html?raw';

class LookupsModule {
  constructor(app, container) {
    this.app = app;
    this.container = container;
    this.logger = app.logger;
  }

  async init() {
    this.logger.info('Initializing lookups module');

    // Load HTML template
    this.container.innerHTML = htmlTemplate;

    this.logger.info('Lookups module initialized');
  }

  destroy() {
    this.logger.info('Destroying lookups module');
  }
}

export default LookupsModule;