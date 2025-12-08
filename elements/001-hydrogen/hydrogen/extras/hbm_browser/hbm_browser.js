/**
 * Hydrogen Build Metrics Browser - Main Entry Point
 * Simple function-based approach
 *
 * @version 1.0.0
 * @license MIT
 */

// Global namespace
var HMB = HMB || {};

// Load configuration
HMB.loadConfiguration = function() {
  try {
    // Try relative path first for local development
    fetch('./hbm_browser_defaults.json')
      .then(response => {
        if (!response.ok) {
          throw new Error('Config file not found at relative path');
        }
        return response.json();
      })
      .then(config => {
        HMB.config = { ...HMB.config, ...config };
        console.log('Configuration loaded successfully');
      })
      .catch(error => {
        console.warn('Using default configuration (CORS expected in local dev):', error.message);

        // For local development, we can use default config
        if (window.location.protocol === 'file:') {
          console.log('Running in local development mode - using default config');
        }
      });
  } catch (error) {
    console.warn('Configuration loading failed:', error);
  }
};

// Initialize the application when DOM is loaded
if (typeof window !== 'undefined') {
  document.addEventListener('DOMContentLoaded', function() {
    console.log('Initializing Hydrogen Build Metrics Browser...');
    HMB.loadConfiguration();
    HMB.initialize();
  });
}

// Export for Node.js/CLI usage
if (typeof module !== 'undefined' && module.exports) {
  module.exports = HMB;
}
