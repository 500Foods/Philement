#!/usr/bin/env node

/**
 * Hydrogen Build Metrics Browser - CLI Version
 * Headless Node.js implementation for generating SVG reports
 *
 * Usage: node hbm_browser_cli.js [config_file] [output_file]
 */

const fs = require('fs');
const path = require('path');
const { JSDOM } = require('jsdom');
const d3 = require('d3');
const iro = require('@jaames/iro');
const flatpickr = require('flatpickr');

// Import the main browser class and modules
const HydrogenMetricsBrowser = require('./hbm_browser.js');
const HydrogenMetricsBrowserCore = require('./hbm_browser_core.js');
const HydrogenMetricsBrowserData = require('./hbm_browser_data.js');
const HydrogenMetricsBrowserChart = require('./hbm_browser_chart.js');
const HydrogenMetricsBrowserUI = require('./hbm_browser_ui.js');
const HydrogenMetricsBrowserUtils = require('./hbm_browser_utils.js');

/**
 * CLI Main Function
 */
async function main() {
  // Parse command line arguments
  const args = process.argv.slice(2);
  const configFile = args[0] || 'hbm_browser_auto.json';
  const outputFile = args[1] || 'hydrogen_metrics_report.svg';

  console.log('Hydrogen Build Metrics Browser - CLI Mode');
  console.log('=================================');
  console.log(`Config: ${configFile}`);
  console.log(`Output: ${outputFile}`);
  console.log('');

  try {
    // Read configuration file
    const config = readConfigFile(configFile);
    console.log('Configuration loaded successfully');

    // Set up headless environment
    const dom = await setupHeadlessEnvironment();
    global.window = dom.window;
    global.document = dom.window.document;
    global.navigator = dom.window.navigator;

    // Initialize D3 and other libraries in headless mode
    setupGlobalLibraries();

    // Create browser instance in headless mode
    const browser = new HydrogenMetricsBrowser(config);
    browser.isHeadless = true;
    browser.isBrowser = false;

    // Override file discovery to use local filesystem
    browser.discoverMetricsFiles = discoverLocalMetricsFiles;

    // Run the metrics processing
    console.log('Discovering metrics files...');
    await browser.discoverMetricsFiles();

    console.log('Processing metrics data...');
    browser.filterDataByDateRange();
    browser.renderChart();

    // Export SVG
    console.log('Generating SVG output...');
    await exportSVGHeadless(browser, outputFile);

    console.log('✅ Report generation completed successfully!');
    process.exit(0);

  } catch (error) {
    console.error('❌ Error:', error.message);
    console.error(error.stack);
    process.exit(1);
  }
}

/**
 * Read configuration file
 * @param {string} filePath - Path to config file
 * @returns {Object} Configuration object
 */
function readConfigFile(filePath) {
  try {
    const configContent = fs.readFileSync(filePath, 'utf8');
    return JSON.parse(configContent);
  } catch (error) {
    console.error(`Failed to read config file ${filePath}:`, error.message);
    throw error;
  }
}

/**
 * Set up headless JSDOM environment
 * @returns {JSDOM} JSDOM instance
 */
async function setupHeadlessEnvironment() {
  const htmlContent = fs.readFileSync(path.join(__dirname, 'hbm_browser.html'), 'utf8');

  return new JSDOM(htmlContent, {
    runScripts: 'dangerously',
    resources: 'usable',
    pretendToBeVisual: true,
    url: 'http://localhost'
  });
}

/**
 * Set up global libraries for headless mode
 */
function setupGlobalLibraries() {
  // Make D3 available globally
  global.d3 = d3;

  // Set up a minimal console for browser-like behavior
  if (!global.console) {
    global.console = {
      log: console.log,
      warn: console.warn,
      error: console.error,
      info: console.info,
      debug: console.debug
    };
  }
}

/**
 * Headless version of metrics file discovery using local filesystem
 * @returns {Promise<Array>} Array of metrics file data
 */
async function discoverLocalMetricsFiles() {
  const today = new Date();
  let currentDate = new Date(today);
  const foundFiles = [];

  // Try up to 30 days back
  for (let i = 0; i < 30; i++) {
    const dateStr = currentDate.toISOString().split('T')[0];
    const [year, month, day] = dateStr.split('-');
    const filePath = path.join(__dirname, '..', '..', '..', '..', 'docs', 'H', 'metrics', year + '-' + month, year + '-' + month + '-' + day + '.json');

    try {
      if (fs.existsSync(filePath)) {
        const fileContent = fs.readFileSync(filePath, 'utf8');
        const data = JSON.parse(fileContent);
        foundFiles.push({
          date: dateStr,
          data: data,
          filePath: filePath
        });
        console.log(`Found metrics file: ${filePath}`);
      }
    } catch (error) {
      console.log(`No file found for ${currentDate.toDateString()}`);
    }

    // Go back one day
    currentDate.setDate(currentDate.getDate() - 1);

    // If we have at least one file and it's within our date range, we can stop
    if (foundFiles.length > 0 && i > 0) {
      break;
    }
  }

  if (foundFiles.length === 0) {
    throw new Error('No metrics files found in the last 30 days');
  }

  return foundFiles;
}

/**
 * Export SVG in headless mode
 * @param {HydrogenMetricsBrowser} browser - Browser instance
 * @param {string} outputFile - Output file path
 */
async function exportSVGHeadless(browser, outputFile) {
  // Get the SVG element
  const svgElement = browser.elements.metricsChart;
  if (!svgElement) {
    throw new Error('SVG element not found');
  }

  // Serialize SVG to string
  const serializer = new browser.elements.chartContainer.ownerDocument.defaultView.XMLSerializer();
  const svgData = serializer.serializeToString(svgElement);

  // Write to file
  fs.writeFileSync(outputFile, svgData, 'utf8');

  console.log(`SVG report saved to: ${outputFile}`);
  console.log(`File size: ${fs.statSync(outputFile).size} bytes`);
}

// Run the CLI
main().catch(error => {
  console.error('Fatal error:', error);
  process.exit(1);
});