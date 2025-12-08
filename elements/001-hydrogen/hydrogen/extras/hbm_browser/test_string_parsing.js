// Test script to verify string number parsing in extractNumericValues
// Load the data module
const fs = require('fs');

// Mock the HMB object and required functions
global.HMB = {
  createCleanMetricPath: function(path, context) { return path; },
  createEnhancedMetricLabel: function(path, context) { return path; }
};

// Copy the extractNumericValues function from hbm_browser_data.js
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
          path: cleanPath,
          value: numericValue,
          label: this.createEnhancedMetricLabel(newPath, newContext),
          originalPath: newPath, // Store original path for data lookup
          context: newContext // Store context for reference
        });
      }
    }
    else if (typeof value === 'number') {
      const cleanPath = this.createCleanMetricPath(newPath, newContext);
      results.push({
        path: cleanPath,
        value: value,
        label: this.createEnhancedMetricLabel(newPath, newContext),
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

// Test data with the examples from the task
const testData = {
  "coverage_lines": {
    "metric": "Instrumented Tests",
    "value": "62,867",
    "description": "Lines of instrumented code - Test articles"
  },
  "coverage_percentages": {
    "metric": "Unity Ratio",
    "value": "190.480 %",
    "description": "Test/Core C/Headers         {GREEN}Target: 100 % - 200 %{RESET}"
  },
  "file_sizes": {
    "metric": "Hydrogen Payload",
    "value": "529 KB",
    "description": "Payload with Swagger, Migrations, Terminal, OIDC"
  }
};

console.log('Testing extractNumericValues with string number parsing...\n');

// Run the extraction
const results = HMB.extractNumericValues(testData);

console.log('Results:');
results.forEach(result => {
  console.log(`Path: ${result.path}`);
  console.log(`Value: ${result.value} (type: ${typeof result.value})`);
  console.log(`Original value was: ${JSON.stringify(testData[result.path.split('.')[0]][result.path.split('.')[1]])}`);
  console.log('---');
});

// Expected results:
// - "62,867" should become 62867
// - "190.480 %" should become 190.480
// - "529 KB" should become 529

console.log('\nExpected values:');
console.log('- "62,867" -> 62867');
console.log('- "190.480 %" -> 190.480');
console.log('- "529 KB" -> 529');