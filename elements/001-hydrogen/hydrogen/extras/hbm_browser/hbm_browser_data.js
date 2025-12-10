/**
 * Hydrogen Build Metrics Browser - Data Functions
 * Data processing, file discovery, and metric extraction
 *
 * @version 1.0.0
 * @license MIT
 */

// Extend the global namespace
var HMB = HMB || {};

// Load initial data with proper two-phase approach
HMB.loadInitialData = function() {
  // Step 1: Show loading overlay immediately
  this.showLoading();
  this.initProgressTracking();

  // Phase 1: Load just enough data to determine available metrics
  // Look back up to 10 days to find the first available metrics file
  this.loadMetricsDiscoveryPhase(10);
};

// Phase 1: Load just enough data to determine available metrics
HMB.loadMetricsDiscoveryPhase = function(daysToCheck) {
  const today = new Date();
  let filesFound = 0;
  let currentDay = 0;

  const checkNextDay = () => {
    if (filesFound >= 1 || currentDay >= daysToCheck) {
      // We found at least one file or checked all days
      if (filesFound === 0) {
        console.error('No metrics files found in discovery phase');
        this.showError('No metrics files found. Please check your data source.');
        return;
      }

      // Extract metrics and move to phase 2
      this.extractAvailableMetrics();
      this.hideLoading(); // Hide the initial loading overlay

      // Start phase 2: Load remaining data in background
      this.loadRemainingDataPhase(30);
      return;
    }

    const currentDate = new Date(today);
    currentDate.setDate(today.getDate() - currentDay);
    const dateStr = currentDate.toISOString().split('T')[0];
    const [year, month, day] = dateStr.split('-');
    const filePath = `${year}-${month}/${year}-${month}-${day}.json`;

    const url = this.state.isBrowser
      ? `${this.config.dataSources.browser.baseUrl}/${filePath}`
      : `${this.config.dataSources.local.basePath}/${filePath}`;

    fetch(url)
      .then(response => {
        if (response.ok) {
          return response.json();
        }
        return null;
      })
      .then(data => {
        if (data) {
          filesFound++;

          // Cache the data
          this.cacheLoadedData(dateStr, data);

          // Add to our metrics data
          this.processMetricsFiles([{
            date: dateStr,
            data: data,
            filePath: filePath
          }]);
        }

        currentDay++;
        // Continue checking next day
        setTimeout(checkNextDay, 50);
      })
      .catch(error => {
        currentDay++;
        // Continue checking next day
        setTimeout(checkNextDay, 50);
      });
  };

  // Start checking from most recent day
  checkNextDay();
};

// Phase 2: Load remaining data in background with progress tracking
HMB.loadRemainingDataPhase = function(totalDays) {
  // Show progress bar for background loading
  this.showProgressBar();

  const today = new Date();
  let currentDay = 0;
  let filesLoaded = 0;

  const loadNextFile = () => {
    if (currentDay >= totalDays) {
      // All files processed
      this.hideProgressBar();

      // Update chart now that all data is loaded
      if (this.state.selectedMetrics.length > 0) {
        this.updateChart();
      }

      return;
    }

    const currentDate = new Date(today);
    currentDate.setDate(today.getDate() - currentDay);
    const dateStr = currentDate.toISOString().split('T')[0];

    // Skip if we already have this data from discovery phase
    if (this.isDataCached(dateStr)) {
      currentDay++;
      filesLoaded++;
      this.updateProgressTracking(currentDay, filesLoaded, totalDays);
      setTimeout(loadNextFile, 20);
      return;
    }

    const [year, month, day] = dateStr.split('-');
    const filePath = `${year}-${month}/${year}-${month}-${day}.json`;

    const url = this.state.isBrowser
      ? `${this.config.dataSources.browser.baseUrl}/${filePath}`
      : `${this.config.dataSources.local.basePath}/${filePath}`;

    fetch(url)
      .then(response => {
        if (response.ok) {
          return response.json();
        }
        return null;
      })
      .then(data => {
        if (data) {
          // Cache the data
          this.cacheLoadedData(dateStr, data);

          // Add to our metrics data
          this.processMetricsFiles([{
            date: dateStr,
            data: data,
            filePath: filePath
          }]);

          filesLoaded++;
        }

        currentDay++;
        // Update progress
        this.updateProgressTracking(currentDay, filesLoaded, totalDays);

        // Load next file after small delay
        setTimeout(loadNextFile, 20);
      })
      .catch(error => {
        currentDay++;
        // Update progress even on error
        this.updateProgressTracking(currentDay, filesLoaded, totalDays);
        // Continue with next file
        setTimeout(loadNextFile, 20);
      });
  };

  // Start loading from beginning
  loadNextFile();
};


// Load data for a specific number of days with progress tracking
HMB.loadDataForDateRange = async function(days) {
  const today = new Date();
  let currentDate = new Date(today);
  let foundFiles = [];
  let daysChecked = 0;
  let filesFound = 0;

  // Try to load data for the specified number of days
  for (let i = 0; i < days; i++) {
    const dateStr = currentDate.toISOString().split('T')[0];
    const [year, month, day] = dateStr.split('-');
    const filePath = `${year}-${month}/${year}-${month}-${day}.json`;

    try {
      const url = this.state.isBrowser
        ? `${this.config.dataSources.browser.baseUrl}/${filePath}`
        : `${this.config.dataSources.local.basePath}/${filePath}`;

      const response = await fetch(url);
      if (response.ok) {
        const data = await response.json();
        foundFiles.push({
          date: dateStr,
          data: data,
          filePath: filePath
        });
        filesFound++;

        // Cache the loaded data
        this.cacheLoadedData(dateStr, data);
      }
    } catch (error) {
    }

    daysChecked++;

    // Update progress after each day checked
    this.updateProgressTracking(daysChecked, filesFound, days);

    // Go back one day
    currentDate.setDate(currentDate.getDate() - 1);
  }

  if (foundFiles.length === 0) {
    throw new Error(`No metrics files found in the last ${days} days`);
  }

  // Process the found files
  this.processMetricsFiles(foundFiles);

  return foundFiles;
};

// Cache loaded data to avoid reloading
HMB.cacheLoadedData = function(date, data) {
  if (!this.state.loadedDataCache) {
    this.state.loadedDataCache = {};
  }

  // Only cache if we don't already have this date
  if (!this.state.loadedDataCache[date]) {
    this.state.loadedDataCache[date] = data;
  }
};

// Check if data is already cached for a date
HMB.isDataCached = function(date) {
  return this.state.loadedDataCache && this.state.loadedDataCache[date];
};

// Get cached data for a date
HMB.getCachedData = function(date) {
  if (this.state.loadedDataCache && this.state.loadedDataCache[date]) {
    return this.state.loadedDataCache[date];
  }
  return null;
};

// Process metrics files and extract data
HMB.processMetricsFiles = function(files) {
  // Combine with existing data if we have any
  if (!this.state.metricsData) {
    this.state.metricsData = [];
  }

  // Add new files to our data collection, avoiding duplicates
  files.forEach(newFile => {
    const existingIndex = this.state.metricsData.findIndex(f => f.date === newFile.date);
    if (existingIndex === -1) {
      this.state.metricsData.push(newFile);
    } else {
      // Update existing entry if we have newer data
      this.state.metricsData[existingIndex] = newFile;
    }
  });

  // Sort by date (oldest first)
  this.state.metricsData.sort((a, b) => new Date(a.date) - new Date(b.date));

  // Update progress tracking with total files
  if (this.state.progress) {
    this.state.progress.totalFiles = this.state.metricsData.length;
    this.state.progress.totalDays = this.state.progress.totalDays || 30;
  }
};

// Extract available numeric metrics from the data
HMB.extractAvailableMetrics = function() {
  if (this.state.metricsData.length === 0) return;

  // Use the most recent file to extract available metrics
  const latestData = this.state.metricsData[this.state.metricsData.length - 1].data;

  this.state.availableMetrics = this.extractNumericValues(latestData);

  if (this.state.availableMetrics.length > 0) {
    this.state.availableMetrics.slice(0, 5).forEach((m, i) => {
    });

    // Check for test_results metrics specifically
    const testMetrics = this.state.availableMetrics.filter(m => m.path.includes('test_results'));
    if (testMetrics.length > 0) {
      testMetrics.slice(0, 3).forEach((m, i) => {
      });
    }
  }

  // Auto-select default metrics from configuration
  if (this.config.metrics && this.config.metrics.length > 0) {
    // Clone the config metrics and add display labels
    this.state.selectedMetrics = this.config.metrics.map(metric => {
      const clonedMetric = JSON.parse(JSON.stringify(metric));
      // Add display label for consistency
      clonedMetric.displayLabel = this.createDisplayLabel(metric.path, {});
      // Also add context for reference
      clonedMetric.context = this.getContextForPath(metric.path);
      // Ensure lineStyle has a default
      if (!clonedMetric.lineStyle) {
        clonedMetric.lineStyle = 'regular';
      }
      return clonedMetric;
    });
  }

  // Update metric count display
  const metricCountElement = document.getElementById('metric-count');
  if (metricCountElement) {
    metricCountElement.textContent = `(${this.state.availableMetrics.length})`;
  }

  // Update the selected metrics UI
  this.updateSelectedMetricsUI();

  // Also populate the metrics dropdown
  this.populateMetricDropdown();

  // Initialize button state after metrics are loaded
  const metricSelect = document.getElementById('metric-select');
  const addMetricBtn = document.getElementById('add-selected-metric');
  if (metricSelect && addMetricBtn) {
    const selectedMetricPath = metricSelect.value;
    if (selectedMetricPath) {
      // Check if the currently selected metric is already added
      const alreadyAdded = this.state.selectedMetrics.some(m => m.path === selectedMetricPath);
      addMetricBtn.disabled = alreadyAdded;
    } else {
      addMetricBtn.disabled = true;
    }
  }

  // Filter data and render chart immediately
  this.filterDataByDateRange();
  this.renderChart();

  // Auto-update chart after data loading completes
  if (this.state.selectedMetrics.length > 0) {
    this.updateChart();
  }
};

// Recursively extract numeric values from nested JSON with enhanced labeling
HMB.extractNumericValues = function(data, path = '', context = {}) {
  const results = [];

  for (const [key, value] of Object.entries(data)) {
    // Build path based on data type
    let newPath;
    if (Array.isArray(data)) {
      // When data is an array, we're iterating over indices
      // The path should include the array index
      newPath = `${path}[${key}]`;
    } else {
      // For regular objects, use dot notation
      newPath = path ? `${path}.${key}` : key;
    }

    const newContext = {...context};

    // Handle special cases for better labeling
    if (Array.isArray(data)) {
      // For arrays, use the index but also capture identifying fields
      const index = parseInt(key);
      if (data[index] && typeof data[index] === 'object') {
        // Capture identifying fields from array items
        if (data[index].test_id) newContext.test_id = data[index].test_id;
        if (data[index].language) newContext.language = data[index].language;
        if (data[index].file_path) newContext.file_path = data[index].file_path;
        if (data[index].metric) newContext.metric = data[index].metric;
      }
    }

    // Handle numeric strings (percentages, comma-separated numbers, and numbers with units)
    if (typeof value === 'string') {
      // Clean the string: remove commas, spaces, and units
      const cleanedValue = value.replace(/,/g, '').replace(/\s*(KB|MB|GB|%)?$/i, '');
      const numericValue = parseFloat(cleanedValue);
      if (!isNaN(numericValue)) {
        const cleanPath = this.createCleanMetricPath(newPath, newContext);
        results.push({
          path: cleanPath, // Use clean path as primary identifier
          value: numericValue,
          label: this.createEnhancedMetricLabel(cleanPath, newContext), // Use clean path for labeling too
          originalPath: newPath, // Store original path for data lookup
          context: newContext // Store context for reference
        });
      }
    }
    else if (typeof value === 'number') {
      const cleanPath = this.createCleanMetricPath(newPath, newContext);
      results.push({
        path: cleanPath, // Use clean path as primary identifier
        value: value,
        label: this.createEnhancedMetricLabel(cleanPath, newContext), // Use clean path for labeling too
        originalPath: newPath, // Store original path for data lookup
        context: newContext // Store context for reference
      });
    }
    else if (typeof value === 'object' && value !== null) {
      // Handle arrays and objects with enhanced context
      if (Array.isArray(value)) {
        value.forEach((item, index) => {
          const itemPath = `${path}.${key}[${index}]`;
          if (typeof item === 'object' && item !== null) {
            // Create array-specific context
            const arrayContext = {...newContext};
            if (item.test_id) arrayContext.test_id = item.test_id;
            if (item.language) arrayContext.language = item.language;
            if (item.file_path) arrayContext.file_path = item.file_path;
            if (item.metric) arrayContext.metric = item.metric;

            // Pass the correct array path with index
            results.push(...this.extractNumericValues(item, itemPath, arrayContext));
          } else if (typeof item === 'number') {
            const arrayContext = {...newContext};
            const cleanPath = this.createCleanMetricPath(itemPath, arrayContext);
            results.push({
              path: cleanPath,
              value: item,
              label: this.createEnhancedMetricLabel(itemPath, arrayContext),
              originalPath: itemPath,
              context: arrayContext
            });
          } else if (typeof item === 'string') {
            // Clean the string: remove commas, spaces, and units
            const cleanedValue = item.replace(/,/g, '').replace(/\s*(KB|MB|GB|%)?$/i, '');
            const numericValue = parseFloat(cleanedValue);
            if (!isNaN(numericValue)) {
              const arrayContext = {...newContext};
              const cleanPath = this.createCleanMetricPath(itemPath, arrayContext);
              results.push({
                path: cleanPath,
                value: numericValue,
                label: this.createEnhancedMetricLabel(itemPath, arrayContext),
                originalPath: itemPath,
                context: arrayContext
              });
            }
          }
        });
      } else {
        results.push(...this.extractNumericValues(value, newPath, newContext));
      }
    }
  }

  return results;
};

// Helper function to extract context from a path
HMB.getContextForPath = function(path) {
  const context = {};

  // Try to extract context from the path pattern
  if (path.includes('test_results.data')) {
    // Extract test_id from path like "test_results.data.01-CMP.elapsed"
    const match = path.match(/test_results\.data\.([^.]+)\./);
    if (match && match[1]) {
      context.test_id = match[1];
    }
  }
  else if (path.includes('cloc.main')) {
    // Extract language from path like "cloc.main.C.1000"
    const match = path.match(/cloc\.main\.([^.]+)\./);
    if (match && match[1]) {
      context.language = match[1];
    }
  }
  else if (path.includes('coverage.data')) {
    // Extract file info from path like "coverage.data.src_main_c.95.5"
    const match = path.match(/coverage\.data\.([^.]+)\./);
    if (match && match[1]) {
      context.file_path = match[1];
    }
  }
  else if (path.includes('stats')) {
    // Extract metric from path like "stats.performance.avg_time"
    const match = path.match(/stats\.([^.]+)\./);
    if (match && match[1]) {
      context.metric = match[1];
    }
  }

  return context;
};

// Create a human-readable label from a metric path
HMB.createMetricLabel = function(path) {
  return path
    .replace(/\./g, ' ')
    .replace(/\[/g, ' ')
    .replace(/\]/g, '')
    .replace(/([A-Z])/g, ' $1')
    .trim()
    .replace(/\s+/g, ' ')
    .replace(/\w\S*/g, (txt) => txt.charAt(0).toUpperCase() + txt.substr(1).toLowerCase());
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

// Create a clean metric path that uses descriptive identifiers instead of array indices
HMB.createCleanMetricPath = function(path, context) {
  // Handle test_results.data array items
  if (path.includes('test_results.data') && context.test_id) {
    return path.replace(/test_results\.data\[\d+\]/, `test_results.data.${context.test_id}`);
  }
  // Handle cloc.main array items - this is the key fix for the Shell/Markdown issue
  else if (path.includes('cloc.main') && context.language) {
    return path.replace(/cloc\.main\[\d+\]/, `cloc.main.${context.language.replace(/\s+/g, '_').replace(/\//g, '_')}`);
  }
  // Handle stats array items
  else if (path.includes('stats') && context.metric) {
    return path.replace(/stats\[\d+\]/, `stats.${context.metric}`);
  }
  // Handle coverage.data array items
  else if (path.includes('coverage.data') && context.file_path) {
    // Clean up file_path by removing color codes and .c suffix
    const cleanFilePath = context.file_path
      .replace(/\{.*?\}/g, '') // Remove {COLOR} codes
      .replace(/\.c$/, '')     // Remove .c suffix
      .replace(/[^a-zA-Z0-9_\.\/]/g, '_'); // Replace special chars with underscore
    return path.replace(/coverage\.data\[\d+\]/, `coverage.data.${cleanFilePath}`);
  }
  // For other arrays, try to find a meaningful identifier
  else if (path.includes('[') && path.includes(']')) {
    // If we have any context, use it
    if (context.test_id) {
      return path.replace(/\[\d+\]/, `.${context.test_id}`);
    } else if (context.language) {
      return path.replace(/\[\d+\]/, `.${context.language.replace(/\s+/g, '_')}`);
    } else if (context.metric) {
      return path.replace(/\[\d+\]/, `.${context.metric}`);
    }
  }

  // Default: return original path if no special handling
  return path;
};

// Create an enhanced metric label with descriptive information
HMB.createEnhancedMetricLabel = function(path, context) {
  // Handle test_results.data items
  if (path.includes('test_results.data') && context.test_id) {
    const baseLabel = path
      .replace('test_results.data', `Test ${context.test_id}`)
      .replace(/\[/g, ' ')
      .replace(/\]/g, '')
      .replace(/\./g, ' ');
    return `${baseLabel} (${context.test_id})`;
  }
  // Handle cloc.main items
  else if (path.includes('cloc.main') && context.language) {
    const baseLabel = path
      .replace('cloc.main', `CLOC ${context.language}`)
      .replace(/\[/g, ' ')
      .replace(/\]/g, '')
      .replace(/\./g, ' ');
    return `${baseLabel} (${context.language})`;
  }
  // Handle stats items
  else if (path.includes('stats') && context.metric) {
    const baseLabel = path
      .replace('stats', `Stats ${context.metric}`)
      .replace(/\[/g, ' ')
      .replace(/\]/g, '')
      .replace(/\./g, ' ');
    return `${baseLabel} (${context.metric})`;
  }
  // Handle coverage.data items
  else if (path.includes('coverage.data') && context.file_path) {
    const cleanFilePath = context.file_path
      .replace(/\{.*?\}/g, '') // Remove {COLOR} codes
      .replace(/\.c$/, '');    // Remove .c suffix
    const baseLabel = path
      .replace('coverage.data', `Coverage ${cleanFilePath}`)
      .replace(/\[/g, ' ')
      .replace(/\]/g, '')
      .replace(/\./g, ' ');
    return `${baseLabel} (${cleanFilePath})`;
  }

  // Default labeling for other cases
  return this.createMetricLabel(path);
};

// Get nested value from object using dot notation (enhanced to handle our clean paths)
HMB.getNestedValue = function(obj, path) {
  // Always use the clean path with identifier-based lookup
  // This ensures we find the correct data regardless of array order in each file
  const result = this.getNestedValueByPath(obj, path);
  return result;
};

// Helper function to get value by exact path
// Enhanced to handle both clean paths (test_results.data.01-CMP.elapsed)
// and original array paths (test_results.data[0].elapsed)
HMB.getNestedValueByPath = function(obj, path) {
  // First, try to handle clean paths by converting them to array access
  if (path.includes('test_results.data.') && !path.includes('[')) {
    // Convert clean path like "test_results.data.01-CMP.elapsed" to array access
    const match = path.match(/test_results\.data\.([^.]+)\.(.+)/);
    if (match) {
      const testIdFromPath = match[1];
      const remainingPath = match[2];

      // Find the array index for this test_id
      const dataArray = obj?.test_results?.data;
      if (Array.isArray(dataArray)) {
        for (let i = 0; i < dataArray.length; i++) {
          // Normalize both identifiers for comparison (though test IDs are typically already normalized)
          const jsonTestId = dataArray[i]?.test_id;
          if (jsonTestId) {
            const normalizedJsonTestId = jsonTestId.replace(/\s+/g, '_').replace(/\//g, '_');
            if (normalizedJsonTestId === testIdFromPath || jsonTestId === testIdFromPath) {
              // Found the matching test, now get the remaining path
              const result = this.getNestedValueByPath(dataArray[i], remainingPath);
              return result;
            }
          }
        }
      }
      return undefined;
    }
  }
  else if (path.includes('cloc.main.') && !path.includes('[')) {
    // Convert clean path like "cloc.main.C.1000" or "cloc.main.C_C++_Header.code" to array access
    const match = path.match(/cloc\.main\.([^.]+)\.(.+)/);
    if (match) {
      const languageFromPath = match[1];
      const remainingPath = match[2];

      // Find the array index for this language
      const mainArray = obj?.cloc?.main;
      if (Array.isArray(mainArray)) {
        for (let i = 0; i < mainArray.length; i++) {
          // Normalize both the path language and JSON language for comparison
          // The path may have underscores where the JSON has spaces or slashes
          const jsonLanguage = mainArray[i]?.language;
          if (jsonLanguage) {
            const normalizedJsonLanguage = jsonLanguage.replace(/\s+/g, '_').replace(/\//g, '_');
            if (normalizedJsonLanguage === languageFromPath) {
              // Found the matching language, now get the remaining path
              return this.getNestedValueByPath(mainArray[i], remainingPath);
            }
          }
        }
      }
      return undefined;
    }
  }
  else if (path.includes('coverage.data.') && !path.includes('[')) {
      // Convert clean path like "coverage.data.src/api/api_service.c.coverage_percentage" to array access
      // Need to handle file paths that contain dots by matching from the end
      const lastDotIndex = path.lastIndexOf('.');
      const filePathFromPath = path.substring('coverage.data.'.length, lastDotIndex);
      const remainingPath = path.substring(lastDotIndex + 1);

      // Find the array index for this file_path
      const dataArray = obj?.coverage?.data;
      if (Array.isArray(dataArray)) {
          for (let i = 0; i < dataArray.length; i++) {
              // Normalize both file paths for comparison
              const jsonFilePath = dataArray[i]?.file_path;
              if (jsonFilePath) {
                  const normalizedJsonFilePath = jsonFilePath.replace(/\s+/g, '_').replace(/\//g, '_');
                  if (normalizedJsonFilePath === filePathFromPath || jsonFilePath === filePathFromPath) {
                      // Found the matching file, now get the remaining path
                      return this.getNestedValueByPath(dataArray[i], remainingPath);
                  }
              }
          }
      }
      return undefined;
  }
  else if (path.includes('stats.') && !path.includes('[')) {
    // Convert clean path like "stats.performance.avg_time" to array access
    const match = path.match(/stats\.([^.]+)\.(.+)/);
    if (match) {
      const metricFromPath = match[1];
      const remainingPath = match[2];

      // Find the array index for this metric
      const statsArray = obj?.stats;
      if (Array.isArray(statsArray)) {
        for (let i = 0; i < statsArray.length; i++) {
          // Normalize both identifiers for comparison
          const jsonMetric = statsArray[i]?.metric;
          if (jsonMetric) {
            const normalizedJsonMetric = jsonMetric.replace(/\s+/g, '_').replace(/\//g, '_');
            if (normalizedJsonMetric === metricFromPath || jsonMetric === metricFromPath) {
              // Found the matching metric, now get the remaining path
              return this.getNestedValueByPath(statsArray[i], remainingPath);
            }
          }
        }
      }
      return undefined;
    }
  }

  // Handle original array notation paths like test_results.data[0].elapsed
  let result = path.split('.').reduce((o, p) => {
    // Handle array access like item[0]
    if (p.includes('[')) {
      const arrayPart = p.match(/^([^\[]+)\[(\d+)\]/);
      if (arrayPart) {
        const arrayName = arrayPart[1];
        const index = parseInt(arrayPart[2]);
        return (o || {})[arrayName] ? (o || {})[arrayName][index] : undefined;
      }
    }
    return (o || {})[p];
  }, obj);

  // Parse string values that contain numbers
  if (typeof result === 'string') {
    const cleanedValue = result.replace(/,/g, '').replace(/\s*(KB|MB|GB|%)?$/i, '');
    const numericValue = parseFloat(cleanedValue);
    if (!isNaN(numericValue)) {
      return numericValue;
    }
  }

  return result;
};

// Filter data by current date range
HMB.filterDataByDateRange = function() {
  if (!this.state.currentDateRange || !this.state.metricsData) return;

  const startDate = new Date(this.state.currentDateRange.start);
  const endDate = new Date(this.state.currentDateRange.end);
  const allDates = [];
  const dateMap = new Map();

  // Add all dates in range
  let currentDate = new Date(startDate);
  while (currentDate <= endDate) {
    const dateStr = currentDate.toISOString().split('T')[0];
    allDates.push(dateStr);
    dateMap.set(dateStr, null);
    currentDate.setDate(currentDate.getDate() + 1);
  }

  // Add actual data points - prioritize cached data if available
  this.state.metricsData.forEach(file => {
    if (file.date >= this.state.currentDateRange.start && file.date <= this.state.currentDateRange.end) {
      // Check if we have cached data for this date (might be more recent)
      const cachedData = this.getCachedData(file.date);
      const dataToUse = cachedData || file.data;
      dateMap.set(file.date, {
        date: file.date,
        data: dataToUse,
        filePath: file.filePath
      });
    }
  });

  // Create filtered data array
  this.state.filteredData = allDates.map(dateStr => {
    const entry = dateMap.get(dateStr);
    const data = entry?.data || null;
    const hasData = entry !== null;
    return {
      date: dateStr,
      data: data,
      hasData: hasData
    };
  });
};


// Handle date range changes and load additional data if needed
HMB.handleDateRangeChange = function() {
  if (!this.state.currentDateRange || !this.state.currentDateRange.start || !this.state.currentDateRange.end) {
    return;
  }

  // Check if we need to load additional data for the new date range
  const datesToLoad = this.getDatesInRange(this.state.currentDateRange.start, this.state.currentDateRange.end);
  const datesNeedLoading = datesToLoad.filter(date => !this.isDataCached(date));

  if (datesNeedLoading.length > 0) {
    // Show progress bar for additional data loading
    this.showProgressBar();
    this.initProgressTracking(datesNeedLoading.length);

    // Start loading additional files
    this.loadAdditionalFiles(datesNeedLoading, 0);
  } else {
    // We have all the data cached, just filter and render
    this.filterDataByDateRange();
    this.renderChart();
  }
};

// Simple event-driven loading of additional files
HMB.loadAdditionalFiles = function(dates, currentIndex) {
  if (currentIndex >= dates.length) {
    // All additional files loaded, update and finish
    this.filterDataByDateRange();
    this.renderChart();
    this.hideProgressBar();

    // Auto-update chart after additional data loading completes
    if (this.state.selectedMetrics.length > 0) {
      this.updateChart();
    }
    return;
  }

  const dateStr = dates[currentIndex];
  const [year, month, day] = dateStr.split('-');
  const filePath = `${year}-${month}/${year}-${month}-${day}.json`;

  const url = this.state.isBrowser
    ? `${this.config.dataSources.browser.baseUrl}/${filePath}`
    : `${this.config.dataSources.local.basePath}/${filePath}`;

  // Load this file
  fetch(url)
    .then(response => {
      if (response.ok) {
        return response.json();
      }
      return null;
    })
    .then(data => {
      if (data) {
        // Cache the loaded data
        this.cacheLoadedData(dateStr, data);

        // Add to our metrics data
        this.processMetricsFiles([{
          date: dateStr,
          data: data,
          filePath: filePath
        }]);
      }

      // Update progress
      this.updateProgressTracking(currentIndex + 1, this.state.metricsData.length, dates.length);

      // Load next file after small delay
      setTimeout(() => {
        this.loadAdditionalFiles(dates, currentIndex + 1);
      }, 100);
    })
    .catch(error => {
      // Continue with next file
      setTimeout(() => {
        this.loadAdditionalFiles(dates, currentIndex + 1);
      }, 100);
    });
};

// Load additional data for specific dates
HMB.loadAdditionalData = async function(dates) {
  let filesLoaded = 0;

  for (let i = 0; i < dates.length; i++) {
    const dateStr = dates[i];
    const [year, month, day] = dateStr.split('-');
    const filePath = `${year}-${month}/${year}-${month}-${day}.json`;

    try {
      const url = this.state.isBrowser
        ? `${this.config.dataSources.browser.baseUrl}/${filePath}`
        : `${this.config.dataSources.local.basePath}/${filePath}`;

      const response = await fetch(url);
      if (response.ok) {
        const data = await response.json();

        // Cache the loaded data
        this.cacheLoadedData(dateStr, data);

        // Add to our metrics data
        this.processMetricsFiles([{
          date: dateStr,
          data: data,
          filePath: filePath
        }]);

        filesLoaded++;
      }
    } catch (error) {
    }

    // Update progress
    this.updateProgressTracking(i + 1, filesLoaded, dates.length);
  }

  return filesLoaded;
};

// Get all dates in a range
HMB.getDatesInRange = function(startDate, endDate) {
  const dates = [];
  let currentDate = new Date(startDate);
  const end = new Date(endDate);

  while (currentDate <= end) {
    dates.push(currentDate.toISOString().split('T')[0]);
    currentDate.setDate(currentDate.getDate() + 1);
  }

  return dates;
};

// Show progress bar for background data loading
HMB.showProgressBar = function() {
  const progressContainer = document.getElementById('data-loading-progress');
  if (progressContainer) {
    progressContainer.style.display = 'block';
  }
};

// Hide progress bar when loading is complete
HMB.hideProgressBar = function() {
  const progressContainer = document.getElementById('data-loading-progress');
  if (progressContainer) {
    progressContainer.style.display = 'none';
  }
};
