
/**
 * Hydrogen Build Metrics Browser - Simple Main Entry
 * Main application using function calls instead of classes
 *
 * @version 1.0.0
 * @license MIT
 */

// Global application state
const hmBrowser = {
  config: {},
  metricsData: [],
  availableMetrics: [],
  selectedMetrics: [],
  currentDateRange: null,
  elements: {},
  isBrowser: typeof window !== 'undefined',
  isHeadless: false,
  loading: false
};

// Initialize the application
function initHmBrowser() {
  console.log('Initializing Hydrogen Build Metrics Browser (simple version)...');

  // Set default configuration
  hmBrowser.config = {
    title: 'Hydrogen Build Metrics Browser',
    dateRange: {
      start: '30_days_ago',
      end: 'today'
    },
    chartSettings: {
      width: 1920,
      height: 1080,
      margin: { top: 50, right: 80, bottom: 50, left: 350 }
    },
    metrics: [
      {
        path: "summary.total_tests",
        label: "Total Tests",
        axis: "left",
        type: "line",
        color: "#4a90e2"
      },
      {
        path: "summary.combined_coverage",
        label: "Combined Coverage %",
        axis: "right",
        type: "line",
        color: "#7ed321"
      }
    ],
    dataSources: {
      browser: {
        baseUrl: 'https://raw.githubusercontent.com/500Foods/Philement/main/docs/H/metrics'
      },
      local: {
        basePath: '/docs/H/metrics'
      }
    }
  };

  // Cache DOM elements
  cacheDOMElements();

  // Set up event listeners
  setupEventListeners();

  // Initialize UI components
  initUIComponents();

  // Start loading data
  showLoading();
  discoverMetricsFiles();
}

// DOM Element Caching
function cacheDOMElements() {
  hmBrowser.elements = {
    chartContainer: document.getElementById('chart-container'),
    chartTitle: document.getElementById('chart-title'),
    chartLegend: document.getElementById('chart-legend'),
    metricsChart: document.getElementById('metrics-chart'),
    tooltip: document.getElementById('d3-tooltip'),
    controlPanel: document.getElementById('control-panel'),
    metricsList: document.getElementById('metrics-list'),
    selectedMetrics: document.getElementById('selected-metrics'),
    colorPicker: document.getElementById('color-picker'),
    startDate: document.getElementById('start-date'),
    endDate: document.getElementById('end-date'),
    loadingOverlay: document.getElementById('loading-overlay'),
    errorMessage: document.getElementById('error-message')
  };
}

// Event Listeners
function setupEventListeners() {
  if (hmBrowser.elements.chartTitle) {
    hmBrowser.elements.chartTitle.addEventListener('click', toggleControlPanel);
  }

  if (hmBrowser.elements.chartLegend) {
    hmBrowser.elements.chartLegend.addEventListener('click', toggleControlPanel);
  }
}
