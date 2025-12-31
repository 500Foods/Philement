// Dashboard Module for Lithium PWA
// Placeholder dashboard with basic information

import htmlTemplate from './dashboard.html?raw';

class DashboardModule {
  constructor(app, container) {
    this.app = app;
    this.container = container;
    this.logger = app.logger;
  }

  async init() {
    this.logger.info('Initializing dashboard module');

    // Load HTML template
    this.container.innerHTML = htmlTemplate;

    // Initialize dashboard content
    this.initializeDashboard();

    this.logger.info('Dashboard module initialized');
  }

  initializeDashboard() {
    // Update app info
    const appInfo = this.container.querySelector('#app-info');
    if (appInfo) {
      const state = this.app.getState();
      appInfo.innerHTML = `
        <p><strong>Version:</strong> ${state.version}</p>
        <p><strong>Online:</strong> ${state.online ? 'Yes' : 'No'}</p>
        <p><strong>Timestamp:</strong> ${new Date(state.timestamp).toLocaleString()}</p>
      `;
    }

    // Add some placeholder widgets
    this.createPlaceholderWidgets();
  }

  createPlaceholderWidgets() {
    const widgetsContainer = this.container.querySelector('#dashboard-widgets');
    if (widgetsContainer) {
      widgetsContainer.innerHTML = `
        <div class="row">
          <div class="col-md-6 mb-4">
            <div class="card">
              <div class="card-header">
                <h5 class="card-title">System Status</h5>
              </div>
              <div class="card-body">
                <p>All systems operational</p>
                <div class="progress">
                  <div class="progress-bar bg-success" style="width: 100%"></div>
                </div>
              </div>
            </div>
          </div>
          <div class="col-md-6 mb-4">
            <div class="card">
              <div class="card-header">
                <h5 class="card-title">Recent Activity</h5>
              </div>
              <div class="card-body">
                <ul class="list-unstyled">
                  <li>• Application started</li>
                  <li>• User logged in</li>
                  <li>• Dashboard loaded</li>
                </ul>
              </div>
            </div>
          </div>
        </div>
      `;
    }
  }

  destroy() {
    // Clean up if needed
    this.logger.info('Destroying dashboard module');
  }
}

export default DashboardModule;