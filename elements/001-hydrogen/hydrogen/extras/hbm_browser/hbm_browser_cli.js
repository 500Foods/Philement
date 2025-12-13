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

/**
 * CLI Main Function
 */
async function main() {
  // Parse command line arguments
  const args = process.argv.slice(2);
  const configFile = args[0] || 'hbm_browser_defaults.json';
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

    // Set up headless environment with browser scripts loaded
    const dom = await setupHeadlessEnvironment();
    global.window = dom.window;
    global.document = dom.window.document;
    global.navigator = dom.window.navigator;

    // Initialize D3 and other libraries in headless mode
    setupGlobalLibraries();

    // Make libraries available in JSDOM window context
    dom.window.d3 = d3;
    dom.window.iro = iro;
    dom.window.flatpickr = flatpickr;

    // Load browser application scripts in correct order
    await loadBrowserScripts(dom);

    // Access the HMB object created by the browser scripts in the JSDOM window
    const browser = dom.window.HMB;
    if (!browser) {
      throw new Error('Failed to load browser application - HMB object not found');
    }

    // Configure for headless mode
    browser.state.isHeadless = true;
    browser.state.isBrowser = false;

    // Override data loading to use local filesystem
    browser.discoverMetricsFiles = discoverLocalMetricsFiles.bind(browser);
    browser.loadInitialData = loadInitialDataHeadless.bind(browser);

    // Initialize the browser (sets up elements)
    browser.initialize();

    // Load configuration into browser
    Object.assign(browser.config, config);

    // Handle "all" date range mode - find actual date range from available files
    if (config.dateRange && config.dateRange.mode === "all") {
      const actualDateRange = await findActualDateRange();
      browser.config.dateRange = {
        start: actualDateRange.start,
        end: actualDateRange.end,
        mode: "absolute"
      };
      console.log(`Using full date range: ${actualDateRange.start} to ${actualDateRange.end}`);
    }

    // Set current date range in state (used by filterDataByDateRange)
    browser.state.currentDateRange = {
      start: browser.config.dateRange.start,
      end: browser.config.dateRange.end
    };

    // Set selected metrics from config
    browser.state.selectedMetrics = config.metrics || [];

    // Set default chart dimensions for headless mode
    browser.config.chartSettings.width = 1920;
    browser.config.chartSettings.height = 1080;

    // Run the metrics processing
    console.log('Discovering metrics files...');
    await browser.loadInitialData();

    console.log('Missing dates:', browser.state.missingDates ? browser.state.missingDates.length : 'none');

    console.log('Processing metrics data...');
    browser.filterDataByDateRange();
    browser.renderChart();

    // Export SVG
    console.log('Generating SVG output...');
    await exportSVGHeadless(browser, outputFile, dom);

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
 * Load browser application scripts in the correct order
 * @param {JSDOM} dom - JSDOM instance
 */
async function loadBrowserScripts(dom) {
  const scripts = [
    'hbm_browser_core.js',
    'hbm_browser_data_loading.js',
    'hbm_browser_data_processing.js',
    'hbm_browser_chart.js',
    'hbm_browser_ui_core.js',
    'hbm_browser_ui_color.js',
    'hbm_browser.js'
  ];

  for (const scriptName of scripts) {
    const scriptPath = path.join(__dirname, scriptName);
    const scriptContent = fs.readFileSync(scriptPath, 'utf8');

    // Execute the script in the JSDOM context
    dom.window.eval(scriptContent);
  }
}

/**
 * Find the actual date range from available metrics files
 * @returns {Promise<{start: string, end: string}>}
 */
async function findActualDateRange() {
  const docsRoot = process.env.HYDROGEN_DOCS_ROOT || path.join(__dirname, '..', '..', '..', '..', 'docs', 'H');
  const metricsDir = path.join(docsRoot, 'metrics');

  const dates = [];

  try {
    // Scan all year directories
    const yearDirs = fs.readdirSync(metricsDir).filter(dir =>
      fs.statSync(path.join(metricsDir, dir)).isDirectory() &&
      /^\d{4}-\d{2}$/.test(dir)
    );

    for (const yearDir of yearDirs) {
      const yearPath = path.join(metricsDir, yearDir);
      try {
        const files = fs.readdirSync(yearPath).filter(file =>
          file.endsWith('.json') && /^\d{4}-\d{2}-\d{2}\.json$/.test(file)
        );

        files.forEach(file => {
          const dateStr = file.replace('.json', '');
          dates.push(dateStr);
        });
      } catch (error) {
        // Skip directories that can't be read
      }
    }
  } catch (error) {
    console.warn('Could not scan metrics directory for date range:', error.message);
    // Fall back to default range
    return {
      start: new Date(Date.now() - 30 * 24 * 60 * 60 * 1000).toISOString().split('T')[0],
      end: new Date().toISOString().split('T')[0]
    };
  }

  if (dates.length === 0) {
    throw new Error('No metrics files found');
  }

  dates.sort();
  return {
    start: dates[0],
    end: dates[dates.length - 1]
  };
}

/**
 * Headless version of initial data loading
 */
async function loadInitialDataHeadless() {
  // Prevent double-loading
  if (this.state.isLoading) {
    console.log('Data loading already in progress, skipping duplicate call');
    return;
  }
  this.state.isLoading = true;

  // Step 1: Show loading overlay (skip in headless)
  // this.showLoading();

  // Load metrics data using the overridden discoverMetricsFiles
  const foundFiles = await this.discoverMetricsFiles();

  // Process the found files
  this.processMetricsFiles(foundFiles);
  console.log(`Loaded ${foundFiles.length} metrics files:`);
  foundFiles.forEach(file => console.log(`  ${file.filePath}`));

  // Extract metrics
  this.extractAvailableMetrics();
  console.log(`Extracted ${this.state.availableMetrics.length} available metrics`);

  this.state.isLoading = false;
}

/**
 * Headless version of metrics file discovery using local filesystem
 * @returns {Promise<Array>} Array of metrics file data
 */
async function discoverLocalMetricsFiles() {
  // Use HYDROGEN_DOCS_ROOT if available, otherwise use relative path
  const docsRoot = process.env.HYDROGEN_DOCS_ROOT || path.join(__dirname, '..', '..', '..', '..', '..', 'docs', 'H');

  // Get the date range from config
  const startDate = new Date(this.config.dateRange.start);
  const endDate = new Date(this.config.dateRange.end);
  const foundFiles = [];
  const missingDates = [];

  // Scan from end date back to start date
  let currentDate = new Date(endDate);

  while (currentDate >= startDate) {
    const dateStr = currentDate.toISOString().split('T')[0];
    const [year, month, day] = dateStr.split('-');
    const filePath = path.join(docsRoot, 'metrics', year + '-' + month, year + '-' + month + '-' + day + '.json');

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
      } else {
        // Track missing dates
        missingDates.push(dateStr);
      }
    } catch (error) {
      // Track missing dates
      missingDates.push(dateStr);
    }

    // Go back one day
    currentDate.setDate(currentDate.getDate() - 1);
  }

  if (foundFiles.length === 0) {
    throw new Error(`No metrics files found in date range ${this.config.dateRange.start} to ${this.config.dateRange.end}`);
  }

  // Sort by date (oldest first)
  foundFiles.sort((a, b) => new Date(a.date) - new Date(b.date));

  // Set missing dates on the browser instance
  this.state.missingDates = missingDates;

  return foundFiles;
}

/**
 * Export SVG in headless mode
 * @param {HydrogenMetricsBrowser} browser - Browser instance
 * @param {string} outputFile - Output file path
 */
async function exportSVGHeadless(browser, outputFile, dom) {
  // Get the SVG element directly from DOM
  const svgElement = document.getElementById('metrics-chart');
  if (!svgElement) {
    throw new Error('SVG element not found in DOM');
  }

  // Serialize SVG to string using JSDOM's XMLSerializer
  const serializer = new dom.window.XMLSerializer();
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