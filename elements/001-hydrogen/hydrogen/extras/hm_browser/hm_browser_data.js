/**
 * Hydrogen Metrics Browser - Data Functions
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

  console.log('Starting two-phase data loading process...');

  // Phase 1: Load just enough data to determine available metrics
  // Look back up to 10 days to find the first available metrics file
  this.loadMetricsDiscoveryPhase(10);
};

// Phase 1: Load just enough data to determine available metrics
HMB.loadMetricsDiscoveryPhase = function(daysToCheck) {
  console.log(`Starting metrics discovery phase - checking last ${daysToCheck} days`);

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

      console.log(`Found ${filesFound} metrics files during discovery phase`);
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

    console.log(`Checking for metrics file: ${filePath}`);

    fetch(url)
      .then(response => {
        if (response.ok) {
          return response.json();
        }
        return null;
      })
      .then(data => {
        if (data) {
          console.log(`Found metrics file for ${dateStr}`);
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
        console.log(`Error loading file for ${dateStr}:`, error.message);
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
  console.log('Starting remaining data loading phase in background...');

  // Show progress bar for background loading
  this.showProgressBar();

  const today = new Date();
  let currentDay = 0;
  let filesLoaded = 0;

  const loadNextFile = () => {
    if (currentDay >= totalDays) {
      // All files processed
      console.log(`Completed loading phase - loaded ${filesLoaded} files total`);
      this.hideProgressBar();
      return;
    }

    const currentDate = new Date(today);
    currentDate.setDate(today.getDate() - currentDay);
    const dateStr = currentDate.toISOString().split('T')[0];

    // Skip if we already have this data from discovery phase
    if (this.isDataCached(dateStr)) {
      console.log(`Already have data for ${dateStr}, skipping...`);
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
          console.log(`Loaded additional data for ${dateStr}`);

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
        console.log(`Error loading file for ${dateStr}:`, error.message);
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
        console.log(`Found metrics file for ${dateStr}`);

        // Cache the loaded data
        this.cacheLoadedData(dateStr, data);
      }
    } catch (error) {
      console.log(`Error loading file for ${currentDate.toDateString()}:`, error.message);
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
    console.log(`Cached data for ${date}`);
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

  console.log(`Processed ${this.state.metricsData.length} metrics files`);
};

// Extract available numeric metrics from the data
HMB.extractAvailableMetrics = function() {
  if (this.state.metricsData.length === 0) return;

  // Use the most recent file to extract available metrics
  const latestData = this.state.metricsData[this.state.metricsData.length - 1].data;
  this.state.availableMetrics = this.extractNumericValues(latestData);

  // Auto-select default metrics from configuration
  if (this.config.metrics && this.config.metrics.length > 0) {
    this.state.selectedMetrics = JSON.parse(JSON.stringify(this.config.metrics));
  }

  // Debug: Log what we found
  console.log('Available metrics found:', this.state.availableMetrics.length);
  if (this.state.availableMetrics.length > 0) {
    console.log('Sample metrics:', this.state.availableMetrics.slice(0, 5));
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
};

// Recursively extract numeric values from nested JSON
HMB.extractNumericValues = function(data, path = '') {
  const results = [];

  for (const [key, value] of Object.entries(data)) {
    const newPath = path ? `${path}.${key}` : key;

    if (typeof value === 'number') {
      results.push({
        path: newPath,
        value: value,
        label: this.createMetricLabel(newPath)
      });
    } else if (typeof value === 'object' && value !== null) {
      // Handle arrays and objects
      if (Array.isArray(value)) {
        value.forEach((item, index) => {
          if (typeof item === 'object' && item !== null) {
            results.push(...this.extractNumericValues(item, `${newPath}[${index}]`));
          } else if (typeof item === 'number') {
            results.push({
              path: `${newPath}[${index}]`,
              value: item,
              label: this.createMetricLabel(`${newPath}[${index}]`)
            });
          }
        });
      } else {
        results.push(...this.extractNumericValues(value, newPath));
      }
    }
  }

  return results;
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
      dateMap.set(file.date, {
        date: file.date,
        data: cachedData || file.data,
        filePath: file.filePath
      });
    }
  });

  // Create filtered data array
  this.state.filteredData = allDates.map(dateStr => ({
    date: dateStr,
    data: dateMap.get(dateStr)?.data || null,
    hasData: dateMap.get(dateStr) !== null
  }));
};

// Get nested value from object using dot notation
HMB.getNestedValue = function(obj, path) {
  return path.split('.').reduce((o, p) => (o || {})[p], obj);
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
    // Show loading overlay for additional data loading
    this.showLoading();
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
    this.hideLoading();
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
        console.log(`Loaded additional data for ${dateStr}`);

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
      console.log(`Error loading additional file for ${dateStr}:`, error.message);

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
        console.log(`Loaded additional data for ${dateStr}`);
      }
    } catch (error) {
      console.log(`Error loading additional file for ${dateStr}:`, error.message);
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
