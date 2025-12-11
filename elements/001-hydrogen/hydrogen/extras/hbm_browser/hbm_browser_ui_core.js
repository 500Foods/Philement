/**
 * Hydrogen Build Metrics Browser - UI Core Functions
 * Core UI components and event handling
 *
 * @version 1.0.0
 * @license MIT
 */

// Extend the global namespace
var HMB = HMB || {};

// Set up event listeners
HMB.setupEventListeners = function() {
  // Chart title click to show controls
  if (this.state.elements.chartTitle) {
    this.state.elements.chartTitle.addEventListener('click', () => {
      this.toggleControlPanel();
    });
  }

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

  // Retry button for errors
  const retryBtn = document.getElementById('retry-btn');
  if (retryBtn) {
    retryBtn.addEventListener('click', () => {
      this.hideError();
      this.showLoading();
      this.discoverMetricsFiles();
    });
  }

  // Metrics filter input
  const filterInput = document.getElementById('metrics-filter');
  if (filterInput) {
    let filterTimeout;
    filterInput.addEventListener('input', () => {
      clearTimeout(filterTimeout);
      filterTimeout = setTimeout(() => {
        this.applyMetricsFilter();
      }, 300);
    });
  }

  // Filter buttons
  const clearFilterBtn = document.getElementById('clear-filter');
  const applyFilterBtn = document.getElementById('apply-filter');
  if (clearFilterBtn && applyFilterBtn) {
    clearFilterBtn.addEventListener('click', () => {
      if (filterInput) {
        filterInput.value = '';
        this.applyMetricsFilter();
      }
    });

    applyFilterBtn.addEventListener('click', () => {
      this.applyMetricsFilter();
    });
  }
};

// Create a simplified display label for the dropdown
// Creates concise but informative labels like "Test 01-CMP elapsed" instead of "Test Results Data Elapsed"
HMB.createDisplayLabel = function(path, context) {
  // Handle different metric types with specific formatting

  // 1. Test Results - Format as "Test <test_id> <metric_name>"
  if (path.includes('test_results.data') && context && context.test_id) {
    // Extract the metric name (last part of path)
    const parts = path.split('.');
    const metricName = parts[parts.length - 1];

    // Create concise format: "Test <test_id> <metric_name>"
    return `Test ${context.test_id} ${metricName}`;
  }

  // 2. CLOC (Code Lines) - Format as "CLOC <language> <metric_name>"
  else if (path.includes('cloc.main') && context && context.language) {
    // Extract the metric name (last part of path)
    const parts = path.split('.');
    const metricName = parts[parts.length - 1];

    // Create concise format: "CLOC <language> <metric_name>"
    return `CLOC ${context.language} ${metricName}`;
  }

  // 3. Coverage - Format as "Coverage <file> <metric_name>"
  else if (path.includes('coverage.data') && context && context.file_path) {
    // Extract the metric name (last part of path)
    const parts = path.split('.');
    const metricName = parts[parts.length - 1];

    // Clean up file path for display
    const cleanFilePath = context.file_path
      .replace(/\{.*?\}/g, '') // Remove {COLOR} codes
      .replace(/\.c$/, '')     // Remove .c suffix
      .replace(/[^a-zA-Z0-9_\.\/]/g, '_'); // Replace special chars

    // Create concise format: "Coverage <file> <metric_name>"
    return `Coverage ${cleanFilePath} ${metricName}`;
  }

  // 4. Stats - Format as "Stats <metric> <stat_name>"
  else if (path.includes('stats') && context && context.metric) {
    // Extract the metric name (last part of path)
    const parts = path.split('.');
    const metricName = parts[parts.length - 1];

    // Create concise format: "Stats <metric> <metric_name>"
    return `Stats ${context.metric} ${metricName}`;
  }

  // 5. General case - Create clean label with underscores as spaces
  else {
    // Start with basic cleaning
    let displayLabel = path
      .replace(/\./g, ' ')
      .replace(/\[/g, ' ')
      .replace(/\]/g, '')
      .replace(/([A-Z])/g, ' $1')
      .trim()
      .replace(/\s+/g, ' ');

    // Replace underscores with spaces for better readability
    displayLabel = displayLabel.replace(/_/g, ' ');

    // Capitalize properly
    displayLabel = displayLabel.replace(/\w\S*/g, (txt) => txt.charAt(0).toUpperCase() + txt.substr(1).toLowerCase());

    return displayLabel.trim();
  }
};

// Initialize date pickers
HMB.initDatePickers = function() {
  const today = new Date();
  const thirtyDaysAgo = new Date(today.getTime() - 30 * 24 * 60 * 60 * 1000);

  // Set default date range
  this.state.currentDateRange = {
    start: thirtyDaysAgo.getFullYear() + '-' + String(thirtyDaysAgo.getMonth() + 1).padStart(2, '0') + '-' + String(thirtyDaysAgo.getDate()).padStart(2, '0'),
    end: today.getFullYear() + '-' + String(today.getMonth() + 1).padStart(2, '0') + '-' + String(today.getDate()).padStart(2, '0')
  };

  // Initialize date pickers
  flatpickr(this.state.elements.startDate, {
    dateFormat: 'Y-m-d',
    defaultDate: this.state.currentDateRange.start,
    maxDate: today,
    onChange: (selectedDates, dateStr) => {
      // console.log('Start date changed to:', dateStr);
      this.state.currentDateRange.start = dateStr;
      this.handleDateRangeChange();
    }
  });

  flatpickr(this.state.elements.endDate, {
    dateFormat: 'Y-m-d',
    defaultDate: this.state.currentDateRange.end,
    maxDate: today,
    onChange: (selectedDates, dateStr) => {
      // console.log('End date changed to:', dateStr);
      this.state.currentDateRange.end = dateStr;
      this.handleDateRangeChange();
    }
  });
};

// Apply metrics filter based on search input
HMB.applyMetricsFilter = function() {
  const filterInput = document.getElementById('metrics-filter');
  if (!filterInput) return;

  const filterText = filterInput.value.trim();
  if (!filterText) {
    // If filter is empty, show all metrics
    this.populateMetricDropdown();
    return;
  }

  // Filter available metrics with enhanced search
  // Search both displayed values and underlying data for comprehensive filtering
  const filterLower = filterText.toLowerCase();

  // Create regex pattern that treats spaces as wildcards
  const filterPattern = filterLower.replace(/\s+/g, '.*');

  const filteredMetrics = this.state.availableMetrics.filter(metric => {
    // Search in multiple fields for better coverage
    const labelLower = metric.label.toLowerCase();
    const pathLower = metric.path.toLowerCase();

    // Create display label for searching
    const displayLabel = this.createDisplayLabel(metric.path, metric.context || {});
    const displayLabelLower = displayLabel.toLowerCase();

    // Check if the pattern matches in any of the relevant fields
    const regex = new RegExp(filterPattern);
    return regex.test(displayLabelLower) ||
           regex.test(labelLower) ||
           regex.test(pathLower) ||
           (metric.context && metric.context.test_id && regex.test(metric.context.test_id.toLowerCase())) ||
           (metric.context && metric.context.language && regex.test(metric.context.language.toLowerCase())) ||
           (metric.context && metric.context.metric && regex.test(metric.context.metric.toLowerCase()));
  });

  // Update the metric dropdown with filtered results
  const dropdown = document.getElementById('metric-select');
  if (dropdown) {
    // Clear existing options (keep the default)
    while (dropdown.options.length > 1) {
      dropdown.remove(1);
    }

    // Add filtered metrics to dropdown with improved display format
    filteredMetrics.forEach(metric => {
      const option = document.createElement('option');
      option.value = metric.path;

      // Use simplified display label for filtered results too
      const displayLabel = this.createDisplayLabel(metric.path, metric.context || {});
      option.textContent = displayLabel;

      // Store original data for reference
      option.dataset.originalLabel = metric.label;
      option.dataset.metricPath = metric.path;

      dropdown.appendChild(option);
    });

    // Update the metrics count to show filtered results
    const metricCountElement = document.getElementById('metric-count');
    if (metricCountElement) {
      metricCountElement.textContent = `(${filteredMetrics.length})`;
    }

    // console.log(`Filtered dropdown to ${filteredMetrics.length} metrics`);
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
  const lineStyleSelect = document.getElementById('metric-line-style');

  // Create metric configuration with current UI selections
  const metricConfig = {
    path: metric.path,
    label: metric.label,
    displayLabel: this.createDisplayLabel(metric.path, metric.context || {}),
    axis: axisSelect ? axisSelect.value : 'left',
    type: typeSelect ? typeSelect.value : 'line',
    color: colorInput && colorInput.value ? colorInput.value : this.getRandomColor(),
    lineStyle: lineStyleSelect ? lineStyleSelect.value : 'regular'
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

// Update selected metrics UI
HMB.updateSelectedMetricsUI = function() {
  const selectedMetricsList = document.getElementById('selected-metrics');
  if (!selectedMetricsList) return;

  selectedMetricsList.innerHTML = '';

  this.state.selectedMetrics.forEach(metric => {
    const metricElement = document.createElement('div');
    metricElement.className = 'selected-metric-item';
    metricElement.dataset.path = metric.path;
    metricElement.draggable = true;

    metricElement.innerHTML = `
      <button class="reorder-handle-btn" title="Drag to reorder" data-metric-path="${metric.path}">
        <i class="fas fa-grip-vertical"></i>
      </button>
      <div class="metric-info-selected">
        <div class="metric-label-selected" title="Path: ${metric.path}">${metric.displayLabel || metric.label}</div>
        <div class="metric-details-selected">
          <span class="metric-color-preview" style="background-color: ${metric.color}"></span>
          <span class="metric-axis-badge ${metric.axis}">${metric.axis}</span>
          <span class="metric-type-badge ${metric.type}">${metric.type}</span>
          <span class="metric-style-badge ${metric.lineStyle}">${metric.lineStyle}</span>
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

    // Add drag and drop handlers for reordering
    metricElement.addEventListener('dragstart', (e) => {
      e.dataTransfer.effectAllowed = 'move';
      e.dataTransfer.setData('text/html', metricElement.outerHTML);
      metricElement.classList.add('dragging');
      this.state.dragIndex = Array.from(selectedMetricsList.children).indexOf(metricElement);
    });

    metricElement.addEventListener('dragend', (e) => {
      metricElement.classList.remove('dragging');
      // Remove drag over classes from all items
      selectedMetricsList.querySelectorAll('.selected-metric-item').forEach(item => {
        item.classList.remove('drag-over');
      });
    });

    metricElement.addEventListener('dragover', (e) => {
      e.preventDefault();
      e.dataTransfer.dropEffect = 'move';
    });

    metricElement.addEventListener('dragenter', (e) => {
      e.preventDefault();
      if (metricElement !== e.relatedTarget && !metricElement.contains(e.relatedTarget)) {
        metricElement.classList.add('drag-over');
      }
    });

    metricElement.addEventListener('dragleave', (e) => {
      if (metricElement !== e.relatedTarget && !metricElement.contains(e.relatedTarget)) {
        metricElement.classList.remove('drag-over');
      }
    });

    metricElement.addEventListener('drop', (e) => {
      e.preventDefault();
      metricElement.classList.remove('drag-over');

      const draggedIndex = this.state.dragIndex;
      const targetIndex = Array.from(selectedMetricsList.children).indexOf(metricElement);

      if (draggedIndex !== targetIndex && draggedIndex !== undefined) {
        // Reorder the array
        const draggedMetric = this.state.selectedMetrics.splice(draggedIndex, 1)[0];
        this.state.selectedMetrics.splice(targetIndex, 0, draggedMetric);

        // Update UI and chart
        this.updateSelectedMetricsUI();
        this.updateChart();
      }
    });

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

// Generate random color
HMB.getRandomColor = function() {
  const letters = '0123456789ABCDEF';
  let color = '#';
  for (let i = 0; i < 6; i++) {
    color += letters[Math.floor(Math.random() * 16)];
  }
  return color;
};

// Populate the metric dropdown with available metrics
HMB.populateMetricDropdown = function() {
  const dropdown = document.getElementById('metric-select');
  if (!dropdown) return;

  // Clear existing options (keep the default)
  while (dropdown.options.length > 1) {
    dropdown.remove(1);
  }

  // Add available metrics to dropdown with improved display format
  this.state.availableMetrics.forEach(metric => {
    const option = document.createElement('option');
    option.value = metric.path;

    // Use simplified display label instead of complex format with redundant info
    const displayLabel = this.createDisplayLabel(metric.path, metric.context || {});
    option.textContent = displayLabel;

    // Store original data for filtering and reference
    option.dataset.originalLabel = metric.label;
    option.dataset.metricPath = metric.path;

    dropdown.appendChild(option);
  });

  // Set default color to skyblue (#87CEEB)
  const colorInput = document.getElementById('metric-color');
  if (colorInput) {
    colorInput.value = '#87CEEB'; // Sky blue
    const colorPreview = document.getElementById('color-preview');
    if (colorPreview) {
      colorPreview.style.backgroundColor = '#87CEEB';
    }
  }

  // Initialize color picker after elements are created
  this.initColorPicker();

  // Update the metrics count to show all available metrics
  const metricCountElement = document.getElementById('metric-count');
  if (metricCountElement) {
    metricCountElement.textContent = `(${this.state.availableMetrics.length})`;
  }

  // console.log(`Populated dropdown with ${this.state.availableMetrics.length} metrics`);
};