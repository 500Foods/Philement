/**
 * Hydrogen Build Metrics Browser - UI Functions
 * UI components and event handling
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
  const thirtyDaysAgo = new Date();
  thirtyDaysAgo.setDate(today.getDate() - 30);

  // Set default date range
  this.state.currentDateRange = {
    start: thirtyDaysAgo.toISOString().split('T')[0],
    end: today.toISOString().split('T')[0]
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

// Note: toggleControlPanel is implemented in core.js for collapse animation
// This file previously had a conflicting implementation that used 'hidden' class

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

    metricElement.innerHTML = `
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



// Initialize and manage the color picker
HMB.initColorPicker = function() {
  // console.log('Initializing color picker...');

  // Check if already initialized to prevent duplicate event listeners
  if (this.state.colorPicker && this.state.colorPicker.initialized) {
    // console.log('Color picker already initialized, skipping');
    return;
  }

  // Initialize color picker elements
  this.state.colorPicker = {
    isActive: false,
    initialized: true,
    currentColor: '#87CEEB', // Default sky blue
    hue: 200, // Blue hue
    saturation: 0.6,
    value: 0.9
  };

  // Set up color picker close button
  const closeColorPickerBtn = document.getElementById('close-color-picker');
  if (closeColorPickerBtn) {
    // console.log('Setting up close button event listener');
    closeColorPickerBtn.addEventListener('click', () => {
      // console.log('Close button clicked');
      this.hideColorPicker();
    });
  } else {
    console.warn('Close color picker button not found');
  }

  // Set up color picker hex input
  const hexInput = document.getElementById('color-picker-hex');
  if (hexInput) {
    // console.log('Setting up HEX input event listener');
    hexInput.addEventListener('input', (e) => {
      // console.log('HEX input changed:', e.target.value);
      this.updateColorFromHex(e.target.value);
    });
  }

  // Initialize color swatch click handler
  const colorSwatch = document.getElementById('color-preview');
  if (colorSwatch) {
    // Check if event listener already exists
    if (!colorSwatch.dataset.colorPickerInitialized) {
      // console.log('Setting up color swatch click handler');
      colorSwatch.addEventListener('click', () => {
        // console.log('Color swatch clicked - toggling color picker');
        this.toggleColorPicker();
      });
      colorSwatch.dataset.colorPickerInitialized = 'true';
    } else {
      // console.log('Color swatch event listener already set up');
    }
  } else {
    console.warn('Color preview element not found');
  }

  // Initialize color input change handler
  const colorInput = document.getElementById('metric-color');
  if (colorInput) {
    // console.log('Setting up color input change handler');
    colorInput.addEventListener('input', (e) => {
      // console.log('Color input changed:', e.target.value);
      const preview = document.getElementById('color-preview');
      if (preview) {
        preview.style.backgroundColor = e.target.value || 'transparent';
      }
    });
  } else {
    console.warn('Metric color input not found');
  }

  // Initialize color picker interaction
  this.initColorPickerInteractions();
  // console.log('Color picker initialization complete');
};

// Toggle color picker visibility
HMB.toggleColorPicker = function() {
  // console.log('toggleColorPicker called');
  const colorPicker = document.getElementById('vanilla-color-picker');
  if (colorPicker) {
    // console.log('Color picker element found, current display:', colorPicker.style.display);
    if (colorPicker.style.display === 'block') {
      // console.log('Hiding color picker');
      this.hideColorPicker();
    } else {
      // console.log('Showing color picker');
      this.showColorPicker();
    }
  } else {
    console.warn('Color picker element not found');
  }
};

// Show color picker
HMB.showColorPicker = function() {
  // console.log('showColorPicker called');
  const colorPicker = document.getElementById('vanilla-color-picker');
  if (colorPicker) {
    // console.log('Showing color picker element');

    // Center the color picker in the window
    colorPicker.style.position = 'fixed';
    colorPicker.style.left = '50%';
    colorPicker.style.top = '50%';
    colorPicker.style.transform = 'translate(-50%, -50%)';
    colorPicker.style.zIndex = '10000';

    colorPicker.style.display = 'block';
    this.state.colorPicker.isActive = true;

    // Initialize color picker with current color
    const currentColor = document.getElementById('metric-color').value || '#87CEEB';
    // console.log('Initializing color picker with color:', currentColor);
    this.updateColorPicker(currentColor);
  } else {
    console.warn('Color picker element not found in showColorPicker');
  }
};

// Hide color picker
HMB.hideColorPicker = function() {
  // console.log('hideColorPicker called');
  const colorPicker = document.getElementById('vanilla-color-picker');
  if (colorPicker) {
    // console.log('Hiding color picker element');
    colorPicker.style.display = 'none';
    this.state.colorPicker.isActive = false;
  } else {
    console.warn('Color picker element not found in hideColorPicker');
  }
};

// Update color picker UI with current color
HMB.updateColorPicker = function(colorHex) {
  // Validate color format
  if (!/^#([0-9A-F]{3}){1,2}$/i.test(colorHex)) {
    colorHex = '#87CEEB'; // Default to sky blue
  }

  // Update state
  this.state.colorPicker.currentColor = colorHex;

  // Update hex input
  const hexInput = document.getElementById('color-picker-hex');
  if (hexInput) {
    hexInput.value = colorHex;
  }

  // Update color picker preview
  const preview = document.getElementById('color-picker-preview');
  if (preview) {
    preview.style.backgroundColor = colorHex;
  }

  // Convert hex to HSV for saturation/hue pickers
  const hsv = this.hexToHsv(colorHex);
  if (hsv) {
    this.state.colorPicker.hue = hsv.h;
    this.state.colorPicker.saturation = hsv.s;
    this.state.colorPicker.value = hsv.v;

    // Update saturation picker to reflect current hue using canvas
    const saturationPicker = document.getElementById('color-picker-saturation');
    if (saturationPicker) {
      // Create canvas if it doesn't exist
      let canvas = saturationPicker.querySelector('canvas');
      if (!canvas) {
        canvas = document.createElement('canvas');
        canvas.width = saturationPicker.offsetWidth;
        canvas.height = saturationPicker.offsetHeight;
        canvas.style.position = 'absolute';
        canvas.style.top = '0';
        canvas.style.left = '0';
        saturationPicker.appendChild(canvas);
      }

      // Draw proper HSV color space on canvas with current hue
      this.drawHSVSaturationPicker(canvas, hsv.h);
    }

    // Update hue picker
    this.updateHuePicker(hsv.h);

    // Update saturation/value cursor position
    this.updateSaturationCursor(hsv.s, hsv.v);
  }
};

// Draw HSV saturation picker on canvas
HMB.drawHSVSaturationPicker = function(canvas, hue) {
  if (!canvas) return;

  const ctx = canvas.getContext('2d');
  const width = canvas.width;
  const height = canvas.height;

  // Create proper HSV color space by drawing each pixel individually
  // This ensures the top-right corner shows the pure hue
  const imageData = ctx.createImageData(width, height);
  const data = imageData.data;

  for (let y = 0; y < height; y++) {
    for (let x = 0; x < width; x++) {
      // Calculate saturation (left to right: 0.0 to 1.0)
      const saturation = x / (width - 1);

      // Calculate value (top to bottom: 1.0 to 0.0)
      const value = 1 - (y / (height - 1));

      // Convert HSV to RGB for this pixel
      const rgb = this.hsvToRgb(hue, saturation, value);

      // Set pixel color
      const index = (y * width + x) * 4;
      data[index] = rgb.r;     // R
      data[index + 1] = rgb.g; // G
      data[index + 2] = rgb.b; // B
      data[index + 3] = 255;   // A (fully opaque)
    }
  }

  // Draw the image data to canvas
  ctx.putImageData(imageData, 0, 0);
};

// Helper function to convert HSV to RGB
HMB.hsvToRgb = function(h, s, v) {
  h = Math.min(360, Math.max(0, h));
  s = Math.min(1, Math.max(0, s));
  v = Math.min(1, Math.max(0, v));

  const c = v * s;
  const x = c * (1 - Math.abs(((h / 60) % 2) - 1));
  const m = v - c;

  let r, g, b;
  if (h >= 0 && h < 60) {
    [r, g, b] = [c, x, 0];
  } else if (h >= 60 && h < 120) {
    [r, g, b] = [x, c, 0];
  } else if (h >= 120 && h < 180) {
    [r, g, b] = [0, c, x];
  } else if (h >= 180 && h < 240) {
    [r, g, b] = [0, x, c];
  } else if (h >= 240 && h < 300) {
    [r, g, b] = [x, 0, c];
  } else {
    [r, g, b] = [c, 0, x];
  }

  return {
    r: Math.round((r + m) * 255),
    g: Math.round((g + m) * 255),
    b: Math.round((b + m) * 255)
  };
};

// Initialize color picker interactions
HMB.initColorPickerInteractions = function() {
  // Saturation/Value picker interaction
  const saturationPicker = document.getElementById('color-picker-saturation');
  if (saturationPicker) {
    saturationPicker.addEventListener('click', (e) => {
      this.handleSaturationClick(e);
    });
  }

  // Hue picker interaction
  const huePicker = document.getElementById('color-picker-hue');
  if (huePicker) {
    huePicker.addEventListener('click', (e) => {
      this.handleHueClick(e);
    });
  }
};

// Handle saturation/value picker click
HMB.handleSaturationClick = function(e) {
  const saturationPicker = document.getElementById('color-picker-saturation');
  if (!saturationPicker) return;

  const rect = saturationPicker.getBoundingClientRect();
  const x = e.clientX - rect.left;
  const y = e.clientY - rect.top;

  // Calculate saturation and value from position
  // X-axis: Saturation (left to right: 0.0 to 1.0)
  // Y-axis: Value/Brightness - CSS gradient goes from black (bottom) to transparent (top)
  // So bottom = black = low value, top = bright = high value
  // But cursor positioning uses (1 - value), so we need to match that
  const saturation = Math.min(1, Math.max(0, x / rect.width));
  const value = 1 - Math.min(1, Math.max(0, y / rect.height)); // Invert to match cursor positioning

  // Update state
  this.state.colorPicker.saturation = saturation;
  this.state.colorPicker.value = value;

  // Update cursor position
  this.updateSaturationCursor(saturation, value);

  // Convert to hex and update everything
  const hex = this.hsvToHex(
    this.state.colorPicker.hue,
    saturation,
    value
  );
  this.updateColorFromHex(hex);
};

// Handle hue picker click
HMB.handleHueClick = function(e) {
  const huePicker = document.getElementById('color-picker-hue');
  if (!huePicker) return;

  const rect = huePicker.getBoundingClientRect();
  const x = e.clientX - rect.left;

  // Calculate hue from position (0-360 degrees)
  // The hue picker is horizontal, so we use X position instead of Y
  const hue = 360 * Math.min(1, Math.max(0, x / rect.width));

  // Update state
  this.state.colorPicker.hue = hue;

  // Update hue cursor position
  this.updateHueCursor(hue);

  // Convert to hex and update everything
  const hex = this.hsvToHex(
    hue,
    this.state.colorPicker.saturation,
    this.state.colorPicker.value
  );
  this.updateColorFromHex(hex);
};

// Update saturation cursor position
HMB.updateSaturationCursor = function(saturation, value) {
  const cursor = document.getElementById('color-picker-cursor');
  if (cursor) {
    const saturationPicker = document.getElementById('color-picker-saturation');
    if (saturationPicker) {
      const x = saturation * saturationPicker.offsetWidth;
      const y = (1 - value) * saturationPicker.offsetHeight;
      cursor.style.left = `${x}px`;
      cursor.style.top = `${y}px`;
    }
  }
};

// Update hue cursor position
HMB.updateHueCursor = function(hue) {
  const cursor = document.getElementById('color-picker-hue-cursor');
  if (cursor) {
    const huePicker = document.getElementById('color-picker-hue');
    if (huePicker) {
      // The hue picker is a horizontal bar, so we position along the X-axis
      // hue ranges from 0-360, we map this to the width of the hue picker
      const x = (hue / 360) * huePicker.offsetWidth;
      cursor.style.left = `${x}px`;
      cursor.style.top = '50%'; // Center vertically
      cursor.style.transform = 'translateY(-50%)'; // Perfect vertical centering
    }
  }
};

// Update hue picker gradient
HMB.updateHuePicker = function(hue) {
  // Hue picker shows the current hue selection
  const huePicker = document.getElementById('color-picker-hue');
  if (huePicker) {
    // This could be enhanced with a proper hue gradient
  }
};

// Update color from hex input
HMB.updateColorFromHex = function(hex) {
  // Validate hex format
  if (!/^#([0-9A-F]{3}){1,2}$/i.test(hex)) {
    return;
  }

  // Update main color swatch
  const colorSwatch = document.getElementById('color-preview');
  if (colorSwatch) {
    colorSwatch.style.backgroundColor = hex;
  }

  // Update main color input field
  const colorInput = document.getElementById('metric-color');
  if (colorInput) {
    colorInput.value = hex;
  }

  // Update color picker state and UI
  this.updateColorPicker(hex);
};

// Convert HEX to HSV
HMB.hexToHsv = function(hex) {
  // Remove # if present
  hex = hex.replace('#', '');

  // Parse r, g, b values
  let r, g, b;
  if (hex.length === 3) {
    r = parseInt(hex[0] + hex[0], 16) / 255;
    g = parseInt(hex[1] + hex[1], 16) / 255;
    b = parseInt(hex[2] + hex[2], 16) / 255;
  } else if (hex.length === 6) {
    r = parseInt(hex.substring(0, 2), 16) / 255;
    g = parseInt(hex.substring(2, 4), 16) / 255;
    b = parseInt(hex.substring(4, 6), 16) / 255;
  } else {
    return null;
  }

  // Find min, max, delta
  const max = Math.max(r, g, b);
  const min = Math.min(r, g, b);
  const delta = max - min;

  // Calculate hue
  let h = 0;
  if (delta !== 0) {
    if (max === r) {
      h = ((g - b) / delta) % 6;
    } else if (max === g) {
      h = (b - r) / delta + 2;
    } else {
      h = (r - g) / delta + 4;
    }
  }
  h = Math.round(h * 60);
  if (h < 0) h += 360;

  // Calculate saturation
  const s = max === 0 ? 0 : delta / max;

  // Calculate value
  const v = max;

  return { h, s, v };
};

// Convert HSV to HEX
HMB.hsvToHex = function(h, s, v) {
  h = Math.min(360, Math.max(0, h));
  s = Math.min(1, Math.max(0, s));
  v = Math.min(1, Math.max(0, v));

  const c = v * s;
  const x = c * (1 - Math.abs(((h / 60) % 2) - 1));
  const m = v - c;

  let r, g, b;
  if (h >= 0 && h < 60) {
    [r, g, b] = [c, x, 0];
  } else if (h >= 60 && h < 120) {
    [r, g, b] = [x, c, 0];
  } else if (h >= 120 && h < 180) {
    [r, g, b] = [0, c, x];
  } else if (h >= 180 && h < 240) {
    [r, g, b] = [0, x, c];
  } else if (h >= 240 && h < 300) {
    [r, g, b] = [x, 0, c];
  } else {
    [r, g, b] = [c, 0, x];
  }

  r = Math.round((r + m) * 255);
  g = Math.round((g + m) * 255);
  b = Math.round((b + m) * 255);

  return `#${r.toString(16).padStart(2, '0')}${g.toString(16).padStart(2, '0')}${b.toString(16).padStart(2, '0')}`.toUpperCase();
};
