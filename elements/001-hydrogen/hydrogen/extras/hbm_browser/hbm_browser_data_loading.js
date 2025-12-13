/**
 * Hydrogen Build Metrics Browser - Data Loading Functions
 * Data loading, caching, and file discovery
 *
 * @version 1.0.0
 * @license MIT
 */

// Extend the global namespace
var HMB = HMB || {};

// Load initial data with proper two-phase approach
HMB.loadInitialData = function() {
  // Prevent double-loading
  if (this.state.isLoading) {
    console.log('Data loading already in progress, skipping duplicate call');
    return;
  }
  this.state.isLoading = true;

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
      this.loadRemainingDataPhase(31);
      return;
    }

    const currentDate = new Date(today);
    currentDate.setDate(today.getDate() - currentDay);
    const dateStr = currentDate.getFullYear() + '-' + String(currentDate.getMonth() + 1).padStart(2, '0') + '-' + String(currentDate.getDate()).padStart(2, '0');
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

// Phase 2: Load remaining data in background with progress tracking (batch loading)
HMB.loadRemainingDataPhase = function(totalDays) {
  // Show progress bar for background loading
  this.showProgressBar();

  const today = new Date();
  let currentDay = 0;
  let filesLoaded = 0;
  const BATCH_SIZE = 10;
  const startTime = performance.now();

  const loadBatch = async () => {
    if (currentDay >= totalDays) {
      // All files processed
      const endTime = performance.now();
      const elapsed = ((endTime - startTime) / 1000).toFixed(1);
      const perDataset = filesLoaded > 0 ? ((endTime - startTime) / filesLoaded).toFixed(3) : 0;
      console.log(`${filesLoaded} of ${totalDays} datasets loaded in ${elapsed}s @ ${perDataset}ms/dataset`);

      this.hideProgressBar();
      this.state.isLoading = false;

      // Update chart now that all data is loaded
      if (this.state.selectedMetrics.length > 0) {
        this.updateChart();
      }

      return;
    }

    // Prepare batch of files to load
    const batch = [];
    const startDay = currentDay;
    
    for (let i = 0; i < BATCH_SIZE && (startDay + i) < totalDays; i++) {
      const dayOffset = startDay + i;
      const currentDate = new Date(today);
      currentDate.setDate(today.getDate() - dayOffset);
      const dateStr = currentDate.getFullYear() + '-' + String(currentDate.getMonth() + 1).padStart(2, '0') + '-' + String(currentDate.getDate()).padStart(2, '0');

      // Skip if we already have this data from discovery phase
      if (this.isDataCached(dateStr)) {
        currentDay++;
        filesLoaded++;
        continue;
      }

      const [year, month, day] = dateStr.split('-');
      const filePath = `${year}-${month}/${year}-${month}-${day}.json`;

      const url = this.state.isBrowser
        ? `${this.config.dataSources.browser.baseUrl}/${filePath}`
        : `${this.config.dataSources.local.basePath}/${filePath}`;

      batch.push({dateStr, filePath, url});
    }

    // Load all files in the batch in parallel
    const loadPromises = batch.map(item =>
      fetch(item.url)
        .then(response => response.ok ? response.json() : null)
        .then(data => {
          if (data) {
            // Cache the loaded data
            this.cacheLoadedData(item.dateStr, data);

            // Add to our metrics data
            this.processMetricsFiles([{
              date: item.dateStr,
              data: data,
              filePath: item.filePath
            }]);

            filesLoaded++;
          }
          currentDay++;
          return data;
        })
        .catch(error => {
          currentDay++;
          return null;
        })
    );

    // Wait for all files in the batch to complete
    await Promise.all(loadPromises);

    // Update progress
    this.updateProgressTracking(currentDay, filesLoaded, totalDays);

    // Load next batch after small delay
    setTimeout(loadBatch, 20);
  };

  // Start loading first batch
  loadBatch();
};

// Load data for a specific date range with progress tracking
HMB.loadDataForDateRange = async function(startDate, endDate) {
  const start = new Date(startDate);
  const end = new Date(endDate);
  let currentDate = new Date(start);
  let foundFiles = [];
  let daysChecked = 0;
  let filesFound = 0;
  const totalDays = Math.ceil((end - start) / (1000 * 60 * 60 * 24)) + 1;
  let firstFileLoaded = false;

  // Try to load data for the date range
  while (currentDate <= end) {
    const dateStr = currentDate.getFullYear() + '-' + String(currentDate.getMonth() + 1).padStart(2, '0') + '-' + String(currentDate.getDate()).padStart(2, '0');
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

        // After first file, extract metrics and hide progress bar to allow UI interaction
        if (!firstFileLoaded) {
          firstFileLoaded = true;
          this.extractAvailableMetrics();
          this.hideProgressBar();
        }
      }
    } catch (error) {
    }

    daysChecked++;

    // Update progress after each day checked (only if progress bar still shown)
    if (!firstFileLoaded) {
      this.updateProgressTracking(daysChecked, filesFound, totalDays);
    }

    // Go to next day
    currentDate.setDate(currentDate.getDate() + 1);
  }

  if (foundFiles.length === 0) {
    throw new Error(`No metrics files found in the date range ${startDate} to ${endDate}`);
  }

  // Process the found files (remaining files)
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

// Batch loading of additional files
HMB.loadAdditionalFiles = async function(dates, currentIndex) {
  const BATCH_SIZE = 10;
  
  // Track loading stats on first call
  if (currentIndex === 0) {
    this.state.additionalLoadStart = performance.now();
    this.state.additionalFilesLoaded = 0;
  }

  if (currentIndex >= dates.length) {
    // All additional files loaded, update and finish
    const endTime = performance.now();
    const elapsed = ((endTime - this.state.additionalLoadStart) / 1000).toFixed(1);
    const filesLoaded = this.state.additionalFilesLoaded;
    const perDataset = filesLoaded > 0 ? ((endTime - this.state.additionalLoadStart) / filesLoaded).toFixed(3) : 0;
    console.log(`${filesLoaded} of ${dates.length} datasets loaded in ${elapsed}s @ ${perDataset}ms/dataset`);
    
    this.filterDataByDateRange();
    this.renderChart();
    this.hideProgressBar();

    // Auto-update chart after additional data loading completes
    if (this.state.selectedMetrics.length > 0) {
      this.updateChart();
    }
    return;
  }

  // Prepare batch of files to load
  const batch = [];
  for (let i = 0; i < BATCH_SIZE && (currentIndex + i) < dates.length; i++) {
    const dateStr = dates[currentIndex + i];
    const [year, month, day] = dateStr.split('-');
    const filePath = `${year}-${month}/${year}-${month}-${day}.json`;

    const url = this.state.isBrowser
      ? `${this.config.dataSources.browser.baseUrl}/${filePath}`
      : `${this.config.dataSources.local.basePath}/${filePath}`;

    batch.push({dateStr, filePath, url});
  }

  // Load all files in the batch in parallel
  const loadPromises = batch.map(item =>
    fetch(item.url)
      .then(response => response.ok ? response.json() : null)
      .then(data => {
        if (data) {
          // Cache the loaded data
          this.cacheLoadedData(item.dateStr, data);

          // Add to our metrics data
          this.processMetricsFiles([{
            date: item.dateStr,
            data: data,
            filePath: item.filePath
          }]);
          
          this.state.additionalFilesLoaded++;
        }
        return data;
      })
      .catch(error => null)
  );

  // Wait for all files in the batch to complete
  await Promise.all(loadPromises);

  // Update progress
  this.updateProgressTracking(currentIndex + batch.length, this.state.metricsData.length, dates.length);

  // Load next batch after small delay
  setTimeout(() => {
    this.loadAdditionalFiles(dates, currentIndex + batch.length);
  }, 100);
};

// Load additional data for specific dates (batch loading)
HMB.loadAdditionalData = async function(dates) {
  let filesLoaded = 0;
  const BATCH_SIZE = 10;
  const startTime = performance.now();

  for (let i = 0; i < dates.length; i += BATCH_SIZE) {
    // Prepare batch of files to load
    const batch = [];
    
    for (let j = i; j < Math.min(i + BATCH_SIZE, dates.length); j++) {
      const dateStr = dates[j];
      const [year, month, day] = dateStr.split('-');
      const filePath = `${year}-${month}/${year}-${month}-${day}.json`;

      const url = this.state.isBrowser
        ? `${this.config.dataSources.browser.baseUrl}/${filePath}`
        : `${this.config.dataSources.local.basePath}/${filePath}`;

      batch.push({dateStr, filePath, url});
    }

    // Load all files in the batch in parallel
    const loadPromises = batch.map(item =>
      fetch(item.url)
        .then(response => response.ok ? response.json() : null)
        .then(data => {
          if (data) {
            // Cache the loaded data
            this.cacheLoadedData(item.dateStr, data);

            // Add to our metrics data
            this.processMetricsFiles([{
              date: item.dateStr,
              data: data,
              filePath: item.filePath
            }]);

            filesLoaded++;
          }
          return data;
        })
        .catch(error => null)
    );

    // Wait for all files in the batch to complete
    await Promise.all(loadPromises);

    // Update progress
    this.updateProgressTracking(i + batch.length, filesLoaded, dates.length);
  }

  // Log completion stats
  const endTime = performance.now();
  const elapsed = ((endTime - startTime) / 1000).toFixed(1);
  const perDataset = filesLoaded > 0 ? ((endTime - startTime) / filesLoaded).toFixed(3) : 0;
  console.log(`${filesLoaded} of ${dates.length} datasets loaded in ${elapsed}s @ ${perDataset}ms/dataset`);

  return filesLoaded;
};

// Get all dates in a range
HMB.getDatesInRange = function(startDate, endDate) {
  const dates = [];
  let currentDate = new Date(startDate);
  const end = new Date(endDate);

  while (currentDate <= end) {
    dates.push(currentDate.getFullYear() + '-' + String(currentDate.getMonth() + 1).padStart(2, '0') + '-' + String(currentDate.getDate()).padStart(2, '0'));
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