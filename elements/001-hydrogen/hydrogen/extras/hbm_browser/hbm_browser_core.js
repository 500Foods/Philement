/**
 * Hydrogen Build Metrics Browser - Core Functions
 * Main application state and initialization
 *
 * @version 1.0.0
 * @license MIT
 */

// Global namespace for the application
var HMB = HMB || {};

// Default configuration
HMB.config = {
  title: 'Hydrogen Build Metrics Browser',
  dateRange: {
    start: '30_days_ago',
    end: 'today'
  },
  chartSettings: {
    width: '100%',
    height: '100%',
    margin: { top: 50, right: 80, bottom: 50, left: 50 }
  },
  metrics: [
    {
      path: "cloc.main.Core_C_Headers.code",
      label: "Core code",
      axis: "left",
      type: "line",
      color: "#000080"
    },
    {
      path: "cloc.main.Test_C_Headers.code",
      label: "Test code",
      axis: "left",
      type: "line",
      color: "#800000"
    },
    {
      path: "cloc.main.Markdown.code",
      label: "Markdown code",
      axis: "left",
      type: "line",
      color: "#87CEEB"
    },
    {
      path: "cloc.main.Shell.code",
      label: "Shell code",
      axis: "left",
      type: "line",
      color: "#008000"
    }
  ],
  dataSources: {
    browser: {
      baseUrl: 'https://raw.githubusercontent.com/500Foods/Philement/main/docs/H/metrics'
    },
    local: {
      basePath: '/docs/H/metrics'
    }
  },
  exportSettings: {
    svg: {
      filename: 'hydrogen_metrics_chart',
      includeStyles: true,
      includeScripts: false,
      quality: 'high',
      backgroundColor: '#000409'
    },
    png: {
      filename: 'hydrogen_metrics_chart',
      scale: 2,
      quality: 0.92
    }
  }
};

// Application state
HMB.state = {
  metricsData: [],
  availableMetrics: [],
  selectedMetrics: [],
  currentDateRange: null,
  isBrowser: typeof window !== 'undefined',
  isHeadless: false,
  loading: false,
  elements: {}
};

// Initialize the application
HMB.initialize = function() {
  if (!this.state.isBrowser) {
    console.log('Running in headless mode');
    this.state.isHeadless = true;
    return;
  }

  // Cache DOM elements
  this.cacheDOMElements();

  // Set up event listeners
  this.setupEventListeners();

  // Initialize UI components
  this.initUIComponents();

  // Set up window resize handler
  this.setupWindowResizeHandler();

  // Load configuration
  this.loadConfiguration();

  // Start loading data in background
  this.initProgressTracking();
  this.loadInitialData();
};



// Set up event listeners
HMB.setupEventListeners = function() {
  console.log('HMB.setupEventListeners called');

  // Chart title click to show controls (now handled in SVG rendering)
  // if (this.state.elements.chartTitle) {
  //   this.state.elements.chartTitle.addEventListener('click', () => {
  //     this.toggleControlPanel();
  //   });
  // }

  // Metric selection dropdown change handler
  const metricSelect = document.getElementById('metric-select');
  if (metricSelect) {
    metricSelect.addEventListener('change', (e) => {
      const addMetricBtn = document.getElementById('add-selected-metric');
      if (addMetricBtn) {
        const metricPath = e.target.value;
        // Enable button only if a metric is selected AND it hasn't been added yet
        if (metricPath) {
          const alreadyAdded = this.state.selectedMetrics.some(m => m.path === metricPath);
          addMetricBtn.disabled = alreadyAdded;
        } else {
          addMetricBtn.disabled = true;
        }
      }
    });
  }

  // Add selected metric button
  const addMetricBtn = document.getElementById('add-selected-metric');
  if (addMetricBtn) {
    addMetricBtn.addEventListener('click', () => {
      this.addSelectedMetric();
    });
  }

  // Update chart button
  const updateChartBtn = document.getElementById('update-chart');
  if (updateChartBtn) {
    updateChartBtn.addEventListener('click', () => {
      this.updateChart();
    });
  }

  // Close panel button
  const closePanelBtn = document.getElementById('close-panel');
  if (closePanelBtn) {
    // console.log('Setting up close button event listener');
    closePanelBtn.addEventListener('click', (e) => {
      // console.log('Close button clicked');
      e.preventDefault();
      e.stopPropagation();
      this.toggleControlPanel();
    });

    // Add visual feedback for testing
    closePanelBtn.addEventListener('mouseover', () => {
      closePanelBtn.style.transform = 'scale(1.1)';
    });

    closePanelBtn.addEventListener('mouseout', () => {
      closePanelBtn.style.transform = 'scale(1)';
    });

  } else {
    console.warn('Close button not found');
  }

  // Retry button for errors
  const retryBtn = document.getElementById('retry-btn');
  if (retryBtn) {
    retryBtn.addEventListener('click', () => {
      this.hideError();
      this.loadInitialData();
    });
  }

  // Export SVG button
  const exportSvgBtn = document.getElementById('export-svg');
  console.log('Looking for export SVG button, found:', !!exportSvgBtn);
  if (exportSvgBtn) {
    console.log('Setting up export SVG button listener');
    exportSvgBtn.addEventListener('click', () => {
      console.log('Export SVG button clicked - starting export');
      this.exportChartAsSVG();
    });
  } else {
    console.error('Export SVG button not found!');
  }

};

// Cache DOM elements
HMB.cacheDOMElements = function() {
  this.state.elements = {
    chartContainer: document.getElementById('chart-container'),
    metricsChart: document.getElementById('metrics-chart'),
    tooltip: document.getElementById('d3-tooltip'),
    controlPanel: document.getElementById('control-panel'),
    metricsList: document.getElementById('metrics-list'),
    selectedMetrics: document.getElementById('selected-metrics'),
    colorPicker: document.getElementById('color-picker'),
    startDate: document.getElementById('start-date'),
    endDate: document.getElementById('end-date'),
    loadingOverlay: document.getElementById('loading-overlay'),
    errorMessage: document.getElementById('error-message'),
    progressContainer: document.getElementById('data-loading-progress'),
    progressBar: document.getElementById('progress-bar'),
    progressText: document.getElementById('progress-text'),
    progressPercentage: document.getElementById('progress-percentage')
  };
};

// Initialize UI components
HMB.initUIComponents = function() {
  this.initDatePickers();
  // Chart title is now handled in SVG rendering, no need to set textContent here
};

// Initialize progress tracking
HMB.initProgressTracking = function(totalDays = 30) {
  // Initialize progress tracking
  this.state.progress = {
    totalDays: totalDays,
    loadedDays: 0,
    totalFiles: 0,
    loadedFiles: 0
  };
};

// Set up window resize handler for responsive design
HMB.setupWindowResizeHandler = function() {
  if (typeof window !== 'undefined') {
    let resizeTimeout;
    window.addEventListener('resize', () => {
      clearTimeout(resizeTimeout);
      resizeTimeout = setTimeout(() => {
        this.handleWindowResize();
      }, 200); // Debounce resize events
    });
  }
};

// Handle window resize
HMB.handleWindowResize = function() {
  if (this.state.selectedMetrics.length > 0) {
    this.renderChart(); // Re-render chart with new dimensions
  }
};

// Update progress tracking
HMB.updateProgressTracking = function(daysLoaded, filesLoaded, totalDays) {
  if (!this.state.progress) {
    this.initProgressTracking();
  }

  this.state.progress.loadedDays = daysLoaded || 0;
  this.state.progress.loadedFiles = filesLoaded || 0;

  // Update total days if provided
  if (totalDays) {
    this.state.progress.totalDays = totalDays;
  }

  // Update UI
  this.updateProgressUI();
};

// Update progress UI
HMB.updateProgressUI = function() {
  if (!this.state.progress) return;

  const progressElement = document.getElementById('progress-text');
  if (progressElement) {
    const daysText = `${this.state.progress.loadedDays} of ${this.state.progress.totalDays} days`;
    const filesText = this.state.progress.totalFiles > 0
      ? ` (${this.state.progress.loadedFiles} of ${this.state.progress.totalFiles} files)`
      : '';
    progressElement.textContent = daysText + filesText;
  }

  // Update progress bar
  const progressBar = document.getElementById('progress-bar');
  if (progressBar) {
    const progressPercent = this.state.progress.totalDays > 0
      ? (this.state.progress.loadedDays / this.state.progress.totalDays) * 100
      : 0;
    progressBar.style.width = `${progressPercent}%`;
  }

  // Update progress percentage text
  const progressPercentage = document.getElementById('progress-percentage');
  if (progressPercentage) {
    const percentage = this.state.progress.totalDays > 0
      ? Math.round((this.state.progress.loadedDays / this.state.progress.totalDays) * 100)
      : 0;
    progressPercentage.textContent = `${percentage}%`;
  }
};

// Show loading overlay
HMB.showLoading = function() {
  if (this.state.elements.loadingOverlay) {
    this.state.elements.loadingOverlay.style.display = 'flex';
  }
};

// Initialize date pickers
HMB.initDatePickers = function() {
  const today = new Date();
  const thirtyDaysAgo = new Date();
  thirtyDaysAgo.setDate(today.getDate() - 30);

  // Set default date range
  this.state.currentDateRange = {
    start: thirtyDaysAgo.toISOString().split('T')[0],
    end: today.toISOString().split('T')[0]
  };

  // Also set the input values directly in case flatpickr doesn't initialize properly
  if (this.state.elements.startDate) {
    this.state.elements.startDate.value = this.state.currentDateRange.start;
  }
  if (this.state.elements.endDate) {
    this.state.elements.endDate.value = this.state.currentDateRange.end;
  }

  // Check if elements exist before initializing
  if (this.state.elements.startDate && this.state.elements.endDate) {
    // Initialize date pickers
    if (typeof flatpickr !== 'undefined') {
      const startPicker = flatpickr(this.state.elements.startDate, {
        dateFormat: 'Y-m-d',
        defaultDate: this.state.currentDateRange.start,
        maxDate: today,
        onChange: (selectedDates) => {
          if (selectedDates.length > 0) {
            this.state.currentDateRange.start = selectedDates[0].toISOString().split('T')[0];
            this.handleDateRangeChange();
          }
        }
      });

      const endPicker = flatpickr(this.state.elements.endDate, {
        dateFormat: 'Y-m-d',
        defaultDate: this.state.currentDateRange.end,
        maxDate: today,
        onChange: (selectedDates) => {
          if (selectedDates.length > 0) {
            this.state.currentDateRange.end = selectedDates[0].toISOString().split('T')[0];
            this.handleDateRangeChange();
          }
        }
      });
    } else {
      // Fallback if flatpickr is not available
      this.state.elements.startDate.value = this.state.currentDateRange.start;
      this.state.elements.endDate.value = this.state.currentDateRange.end;
    }
  }
};



// Render the D3 chart
HMB.renderChart = function() {
  if (this.state.selectedMetrics.length === 0) {
    // console.log('No metrics selected to render - clearing chart');
    // Clear the chart
    d3.select('#metrics-chart').html('');
    return;
  }

  if (!this.state.filteredData || this.state.filteredData.length === 0) {
    this.filterDataByDateRange();
  }

  if (!this.state.filteredData || this.state.filteredData.length === 0) {
    // console.log('No data available for selected date range');
    return;
  }

  // Clear previous chart
  d3.select('#metrics-chart').html('');

  // Get container dimensions for responsive design
  const container = document.getElementById('chart-container');
  const width = container.clientWidth - this.config.chartSettings.margin.left - this.config.chartSettings.margin.right;
  const height = container.clientHeight - this.config.chartSettings.margin.top - this.config.chartSettings.margin.bottom;

  // Create SVG container with responsive dimensions
  const svg = d3.select('#metrics-chart')
    .attr('width', container.clientWidth)
    .attr('height', container.clientHeight)
    .append('g')
    .attr('transform', `translate(${this.config.chartSettings.margin.left},${this.config.chartSettings.margin.top})`);

  // Add chart title to SVG
  const chartTitle = svg.append('text')
    .attr('class', 'chart-title')
    .attr('x', width / 2)
    .attr('y', -this.config.chartSettings.margin.top / 2)
    .attr('text-anchor', 'middle')
    .attr('fill', 'var(--text-color)')
    .attr('font-size', '1.3rem')
    .attr('font-weight', '600')
    .attr('cursor', 'pointer')
    .text(this.config.title)
    .on('click', () => {
      // console.log('Chart title clicked!');
      this.toggleControlPanel();
    });

  // Add transparent overlay for SVG click area - always present but only active when panel is collapsed
  const svgOverlay = svg.append('rect')
    .attr('class', 'svg-overlay')
    .attr('width', width)
    .attr('height', height)
    .attr('fill', 'transparent')
    .attr('cursor', 'pointer')
    .attr('opacity', 0)
    .on('click', () => {
      // console.log('SVG overlay clicked! Panel collapsed:', this.state.elements.controlPanel.classList.contains('collapsed'));

      // Only toggle if panel is collapsed
      if (this.state.elements.controlPanel.classList.contains('collapsed')) {
        // console.log('Toggling control panel from SVG overlay');
        this.toggleControlPanel();
      } else {
        // console.log('Panel not collapsed, ignoring click');
      }
    });

  // Log overlay creation
  // console.log('SVG overlay created with dimensions:', width, 'x', height);
  // console.log('Panel collapsed state:', this.state.elements.controlPanel.classList.contains('collapsed'));

  // Add click handler to the SVG element itself as fallback
  d3.select('#metrics-chart').on('click', function() {
    if (HMB.state.elements.controlPanel.classList.contains('collapsed')) {
      HMB.toggleControlPanel();
    }
  });

  // Set up scales
  const xScale = d3.scaleTime()
    .domain(d3.extent(this.state.filteredData, d => new Date(d.date)))
    .range([0, width]);

  // Create scales for each axis
  const leftAxisMetrics = this.state.selectedMetrics.filter(m => m.axis === 'left');
  const rightAxisMetrics = this.state.selectedMetrics.filter(m => m.axis === 'right');

  const leftYScale = d3.scaleLinear()
    .domain(this.getAxisDomain(leftAxisMetrics))
    .range([height, 0]);

  const rightYScale = d3.scaleLinear()
    .domain(this.getAxisDomain(rightAxisMetrics))
    .range([height, 0]);

  // Add X axis
  svg.append('g')
    .attr('class', 'x-axis')
    .attr('transform', `translate(0,${height})`)
    .call(d3.axisBottom(xScale).ticks(5).tickFormat(d3.timeFormat('%Y-%m-%d')));

  // Add left Y axis
  svg.append('g')
    .attr('class', 'y-axis left-axis')
    .call(d3.axisLeft(leftYScale).ticks(5));

  // Add right Y axis if needed
  if (rightAxisMetrics.length > 0) {
    svg.append('g')
      .attr('class', 'y-axis right-axis')
      .attr('transform', `translate(${width},0)`)
      .call(d3.axisRight(rightYScale).ticks(5));
  }

  // Draw metrics
  this.drawMetrics(svg, xScale, leftYScale, rightYScale, width, height);

  // Add legend to SVG
  this.drawLegend(svg, width, height);
};

// Update selected metrics UI
HMB.updateSelectedMetricsUI = function() {
  const selectedMetricsList = document.getElementById('selected-metrics');
  if (!selectedMetricsList) return;

  selectedMetricsList.innerHTML = '';

  this.state.selectedMetrics.forEach(metric => {
    const metricElement = document.createElement('div');
    metricElement.className = 'selected-metric-item';
    metricElement.dataset.path = metric.path;

    metricElement.innerHTML = `
      <div class="metric-info-selected">
        <div class="metric-label-selected" title="Path: ${metric.path}">${metric.displayLabel || metric.label}</div>
        <div class="metric-details-selected">
          <span class="metric-color-preview" style="background-color: ${metric.color}"></span>
          <span class="metric-axis-badge ${metric.axis}">${metric.axis}</span>
          <span class="metric-type-badge ${metric.type}">${metric.type}</span>
        </div>
      </div>
      <button class="remove-metric-btn" title="Remove metric" data-metric-path="${metric.path}">
        <i class="fas fa-trash-alt"></i>
      </button>
    `;

    // Add click handler for remove button
    const removeBtn = metricElement.querySelector('.remove-metric-btn');
    if (removeBtn) {
      removeBtn.addEventListener('click', (e) => {
        e.stopPropagation();
        this.removeMetric(metric.path);
      });
    }

    // Add click handler for metric label to copy path to clipboard
    const metricLabel = metricElement.querySelector('.metric-label-selected');
    if (metricLabel) {
      metricLabel.addEventListener('click', (e) => {
        e.stopPropagation();
        this.copyMetricPathToClipboard(metric.path);
      });
    }

    selectedMetricsList.appendChild(metricElement);
  });
};

// Copy metric path to clipboard
HMB.copyMetricPathToClipboard = function(metricPath) {
  try {
    // Create a temporary input element
    const tempInput = document.createElement('input');
    tempInput.value = metricPath;
    document.body.appendChild(tempInput);

    // Select and copy the text
    tempInput.select();
    tempInput.setSelectionRange(0, 99999); // For mobile devices

    // Copy to clipboard
    const success = document.execCommand('copy');

    // Clean up
    document.body.removeChild(tempInput);

    // Show feedback
    if (success) {
      // Show a temporary notification
      const existingNotification = document.getElementById('copy-notification');
      if (existingNotification) {
        existingNotification.remove();
      }

      const notification = document.createElement('div');
      notification.id = 'copy-notification';
      notification.className = 'copy-notification';
      notification.textContent = `Copied: ${metricPath}`;
      document.body.appendChild(notification);

      // Remove notification after 2 seconds
      setTimeout(() => {
        notification.remove();
      }, 2000);
    }

    return success;
  } catch (error) {
    console.error('Failed to copy metric path:', error);
    return false;
  }
};

// Hide loading overlay
HMB.hideLoading = function() {
  if (this.state.elements.loadingOverlay) {
    this.state.elements.loadingOverlay.style.display = 'none';
  }
};

// Show error message
HMB.showError = function(message) {
  if (this.state.elements.errorMessage) {
    this.state.elements.errorMessage.querySelector('#error-details').textContent = message;
    this.state.elements.errorMessage.style.display = 'block';
  }
  console.error('Error:', message);
};

// SIMPLE toggle function - direct and reliable
HMB.toggleControlPanel = function() {
  // console.log('SIMPLE TOGGLE CALLED');

  if (this.state.elements.controlPanel) {
    const isCollapsed = this.state.elements.controlPanel.classList.contains('collapsed');
    // console.log('Panel currently collapsed:', isCollapsed);

    // Use the same logic as window resize - just call handleWindowResize after a short delay
    setTimeout(() => {
      // console.log('Calling handleWindowResize after panel toggle');
      this.handleWindowResize();
    }, 350); // Match the CSS transition duration

    if (isCollapsed) {
      // SHOW PANEL - DIRECT AND SIMPLE
      // console.log('SHOWING PANEL');
      this.state.elements.controlPanel.classList.remove('collapsed');
    } else {
      // HIDE PANEL - DIRECT AND SIMPLE
      // console.log('HIDING PANEL');
      this.state.elements.controlPanel.classList.add('collapsed');
    }
  } else {
    console.warn('Control panel element not found!');
  }
};

// Update chart
HMB.updateChart = function() {
  this.filterDataByDateRange();
  this.renderChart();
};

// Add selected metric
HMB.addSelectedMetric = function() {
  const metricPath = document.getElementById('metric-select').value;
  if (!metricPath) return;

  const metric = this.state.availableMetrics.find(m => m.path === metricPath);
  if (!metric) return;

  // Check if already added
  const existing = this.state.selectedMetrics.find(m => m.path === metricPath);
  if (existing) {
    // console.log('Metric already added');
    // Update the button state in case this was called directly
    const addMetricBtn = document.getElementById('add-selected-metric');
    if (addMetricBtn) {
      addMetricBtn.disabled = true;
    }
    return;
  }

  // Read current UI selections
  const axisSelect = document.getElementById('metric-axis');
  const typeSelect = document.getElementById('metric-type');
  const colorInput = document.getElementById('metric-color');

  // Create metric configuration with current UI selections
  // Use the new display label format for consistency
  // console.log('Adding metric with path:', metric.path, 'and context:', metric.context);
  const displayLabel = this.createDisplayLabel(metric.path, metric.context || {});
  // console.log('Created display label:', displayLabel);
  const metricConfig = {
    path: metric.path,
    label: metric.label,
    displayLabel: displayLabel, // Store the simplified display label
    context: metric.context, // Also store the context for reference
    axis: axisSelect ? axisSelect.value : 'left',
    type: typeSelect ? typeSelect.value : 'line',
    color: colorInput && colorInput.value ? colorInput.value : this.getRandomColor()
  };

  this.state.selectedMetrics.push(metricConfig);
  this.updateSelectedMetricsUI();
  this.updateChart();

  // Update the button state after adding
  const addMetricBtn = document.getElementById('add-selected-metric');
  if (addMetricBtn) {
    addMetricBtn.disabled = true;
  }
};

// Hide error message
HMB.hideError = function() {
  if (this.state.elements.errorMessage) {
    this.state.elements.errorMessage.style.display = 'none';
  }
};


// Remove a metric from selected metrics
HMB.removeMetric = function(metricPath) {
  this.state.selectedMetrics = this.state.selectedMetrics.filter(m => m.path !== metricPath);
  this.updateSelectedMetricsUI();
  this.updateChart();

  // Update the add button state in case the removed metric was the one selected in dropdown
  const metricSelect = document.getElementById('metric-select');
  const addMetricBtn = document.getElementById('add-selected-metric');
  if (metricSelect && addMetricBtn) {
    const selectedMetricPath = metricSelect.value;
    if (selectedMetricPath) {
      // Check if the currently selected metric in dropdown is now available to add
      const alreadyAdded = this.state.selectedMetrics.some(m => m.path === selectedMetricPath);
      addMetricBtn.disabled = alreadyAdded;
    } else {
      addMetricBtn.disabled = true;
    }
  }
};





// Export the current chart as SVG
HMB.exportChartAsSVG = function() {
  console.log('Export SVG function called - starting export process');

  try {
    const svgElement = document.getElementById('metrics-chart');
    if (!svgElement) {
      console.error('SVG element not found');
      alert('Error: Chart element not found. Please ensure a chart is displayed before exporting.');
      return;
    }

    // Check if SVG has content
    const svgContent = svgElement.innerHTML.trim();
    if (!svgContent) {
      console.warn('SVG element is empty');
      alert('Warning: The chart appears to be empty. Please add some metrics and render a chart before exporting.');
      return;
    }

    console.log('SVG element found, cloning...');
    // Clone the SVG to avoid modifying the original
    const svgClone = svgElement.cloneNode(true);

    // Ensure SVG has proper namespace
    if (!svgClone.hasAttribute('xmlns')) {
      svgClone.setAttribute('xmlns', 'http://www.w3.org/2000/svg');
    }

    // Get the SVG as a string
    const svgString = new XMLSerializer().serializeToString(svgClone);
    console.log('SVG serialized, length:', svgString.length);

    // Use Blob method for better compatibility
    const blob = new Blob([svgString], { type: 'image/svg+xml;charset=utf-8' });
    const url = URL.createObjectURL(blob);

    // Create a temporary link and trigger download
    const link = document.createElement('a');
    link.href = url;
    link.download = (this.config.exportSettings?.svg?.filename || 'chart') + '.svg';
    document.body.appendChild(link);

    // Try to click the link
    link.click();
    console.log('Download triggered successfully');

    // Clean up
    document.body.removeChild(link);
    URL.revokeObjectURL(url);

    console.log('SVG export completed successfully');
  } catch (error) {
    console.error('SVG export failed:', error);
    alert('Export failed: ' + error.message);
  }
};

// Initialize the application when DOM is loaded
if (typeof window !== 'undefined') {
  document.addEventListener('DOMContentLoaded', function() {
    console.log('Initializing Hydrogen Build Metrics Browser...');
    HMB.initialize();
  });
}