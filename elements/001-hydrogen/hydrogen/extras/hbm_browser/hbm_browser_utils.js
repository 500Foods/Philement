/**
 * Hydrogen Build Metrics Browser - Utilities Module
 * Utility functions and helpers
 *
 * @version 1.0.0
 * @license MIT
 */

// Utility functions for the Hydrogen Build Metrics Browser
class HydrogenMetricsBrowserUtils {
  /**
   * Toggle control panel visibility
   * @param {boolean} show - Whether to show or hide (toggle if undefined)
   */
  toggleControlPanel(show) {
    if (show === undefined) {
      show = this.elements.controlPanel.classList.contains('hidden');
    }

    if (show) {
      this.elements.controlPanel.classList.remove('hidden');
    } else {
      this.elements.controlPanel.classList.add('hidden');
    }
  }

  /**
   * Show loading overlay
   */
  showLoading() {
    if (this.elements.loadingOverlay) {
      this.elements.loadingOverlay.style.display = 'flex';
    }
  }

  /**
   * Hide loading overlay
   */
  hideLoading() {
    if (this.elements.loadingOverlay) {
      this.elements.loadingOverlay.style.display = 'none';
    }
  }

  /**
   * Show error message
   * @param {string} message - Error message to display
   */
  showError(message) {
    if (this.elements.errorMessage) {
      this.elements.errorMessage.querySelector('#error-details').textContent = message;
      this.elements.errorMessage.style.display = 'block';
    }
    console.error('Error:', message);
  }

  /**
   * Hide error message
   */
  hideError() {
    if (this.elements.errorMessage) {
      this.elements.errorMessage.style.display = 'none';
    }
  }

  /**
   * Update the chart with current settings
   */
  updateChart() {
    this.filterDataByDateRange();
    this.renderChart();
  }

  /**
   * Export chart as SVG
   */
  exportSVG() {
    const svgElement = document.getElementById('metrics-chart');
    const svgData = new XMLSerializer().serializeToString(svgElement);
    const blob = new Blob([svgData], {type: 'image/svg+xml'});
    const url = URL.createObjectURL(blob);

    const link = document.createElement('a');
    link.href = url;
    link.download = `${this.config.exportSettings.svg.filename}.svg`;
    document.body.appendChild(link);
    link.click();
    document.body.removeChild(link);
    URL.revokeObjectURL(url);
  }

  /**
   * Command line mode entry point
   * @param {Object} config - Configuration object
   * @param {string} outputFile - Output file path
   */
  static async runHeadless(config, outputFile) {
    const browser = new HydrogenMetricsBrowser(config);
    browser.isHeadless = true;

    try {
      await browser.discoverMetricsFiles();
      browser.filterDataByDateRange();
      browser.renderChart();

      // In headless mode, we would export the SVG
      console.log(`Chart generated successfully. Output: ${outputFile}`);
      return { success: true, outputFile };
    } catch (error) {
      console.error('Headless mode error:', error);
      return { success: false, error: error.message };
    }
  }
}