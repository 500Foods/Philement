# Hydrogen Metrics Browser Implementation Plan

## Overview

This document outlines the detailed implementation plan for the Hydrogen Metrics Browser (hm_browser) application.

## Architecture Summary

- **Browser Mode**: GitHub-based data access only
- **Command Line Mode**: Local file access with SVG output
- **Dual Configuration**: Separate JSON configs for browser vs CLI modes
- **D3 Visualization**: Interactive charts with dual Y-axes
- **Responsive Design**: 1920x1080 aspect ratio targeting

## Implementation Details

### 1. CSS Implementation (hm_browser.css)

```css
/* Base Styles */
:root {
  --bg-color: #000409;
  --text-color: #e0e0e0;
  --accent-color: #4a90e2;
  --chart-bg: rgba(0, 4, 9, 0.8);
}

/* Layout */
.body {
  margin: 0;
  padding: 0;
  font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
  background-color: var(--bg-color);
  color: var(--text-color);
  height: 100vh;
  width: 100vw;
  overflow: hidden;
}

/* Chart Container */
#chart-container {
  width: 100%;
  height: calc(100% - 60px);
  position: relative;
}

/* Control Panel */
.control-panel {
  position: absolute;
  top: 10px;
  right: 10px;
  background: var(--chart-bg);
  border-radius: 8px;
  padding: 15px;
  z-index: 100;
}

/* Responsive Design */
@media (max-width: 768px) {
  .control-panel {
    width: 90%;
    left: 5%;
    bottom: 20px;
    top: auto;
  }
}
```

### 2. HTML Structure (hm_browser.html)

```html
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Hydrogen Metrics Browser</title>
  <link rel="stylesheet" href="hm_browser.css">
  <!-- CDN Dependencies -->
  <link rel="stylesheet" href="https://cdn.jsdelivr.net/npm/flatpickr/dist/flatpickr.min.css">
  <link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.0.0/css/all.min.css">
  <script src="https://cdn.jsdelivr.net/npm/d3@7"></script>
  <script src="https://cdn.jsdelivr.net/npm/flatpickr"></script>
  <script src="https://cdn.jsdelivr.net/npm/@jaames/iro@5"></script>
</head>
<body>
  <div id="chart-container"></div>
  <div class="control-panel">
    <h3><i class="fas fa-chart-line"></i> Metrics Browser</h3>
    <div id="controls">
      <div class="date-range">
        <input type="text" id="start-date" placeholder="Start Date">
        <input type="text" id="end-date" placeholder="End Date">
      </div>
      <div id="metrics-list"></div>
      <div id="color-picker-container"></div>
      <button id="add-metric">Add Metric</button>
      <button id="export-svg">Export SVG</button>
    </div>
  </div>
  <script src="hm_browser.js"></script>
</body>
</html>
```

### 3. JavaScript Core (hm_browser.js)

```javascript
// Configuration
const CONFIG = {
  githubBaseUrl: 'https://raw.githubusercontent.com/500Foods/Philement/main/docs/H/metrics',
  localBasePath: '/docs/H/metrics',
  defaultConfig: 'hm_browser_defaults.json',
  autoConfig: 'hm_browser_auto.json'
};

// Main Application Class
class MetricsBrowser {
  constructor() {
    this.metricsData = [];
    this.availableMetrics = [];
    this.chartConfig = {};
    this.currentDateRange = {start: null, end: null};
  }

  // File Discovery with fallback
  async discoverMetricsFiles() {
    const today = new Date();
    let currentDate = new Date(today);

    // Try up to 30 days back
    for (let i = 0; i < 30; i++) {
      const dateStr = currentDate.toISOString().split('T')[0];
      const [year, month, day] = dateStr.split('-');
      const filePath = `${year}-${month}/${year}-${month}-${day}.json`;

      try {
        const url = this.isBrowser()
          ? `${CONFIG.githubBaseUrl}/${filePath}`
          : `${CONFIG.localBasePath}/${filePath}`;

        const response = await fetch(url);
        if (response.ok) {
          return {data: await response.json(), date: dateStr};
        }
      } catch (error) {
        // Continue to next day
      }

      // Go back one day
      currentDate.setDate(currentDate.getDate() - 1);
    }

    throw new Error('No metrics files found in the last 30 days');
  }

  // Numeric value extraction
  extractNumericValues(data, path = '') {
    const results = [];

    for (const [key, value] of Object.entries(data)) {
      const newPath = path ? `${path}.${key}` : key;

      if (typeof value === 'number') {
        results.push({path: newPath, value});
      } else if (typeof value === 'object' && value !== null) {
        results.push(...this.extractNumericValues(value, newPath));
      }
      // Skip arrays for now, handle separately if needed
    }

    return results;
  }

  // D3 Chart Rendering
  renderChart() {
    // Implementation will include:
    // - Dual Y-axis support
    // - Line/Bar chart types
    // - Responsive sizing
    // - Interactive tooltips
    // - Legend with color coding
  }

  // Utility methods
  isBrowser() {
    return typeof window !== 'undefined';
  }
}

// Initialization
document.addEventListener('DOMContentLoaded', () => {
  const browser = new MetricsBrowser();
  browser.initialize();
});
```

### 4. Configuration Files

#### **hm_browser_defaults.json**

```json
{
  "title": "Hydrogen Metrics Browser",
  "dateRange": {
    "start": "30_days_ago",
    "end": "today"
  },
  "chartSettings": {
    "width": 1920,
    "height": 1080,
    "margin": { "top": 50, "right": 80, "bottom": 50, "left": 80 }
  },
  "metrics": [
    {
      "path": "summary.total_tests",
      "axis": "left",
      "type": "line",
      "color": "#4a90e2",
      "label": "Total Tests"
    },
    {
      "path": "summary.combined_coverage",
      "axis": "right",
      "type": "line",
      "color": "#7ed321",
      "label": "Combined Coverage %"
    }
  ],
  "uiSettings": {
    "showLegend": true,
    "showGrid": true,
    "showTooltip": true
  }
}
```

#### **hm_browser_auto.json**

```json
{
  "title": "Hydrogen Metrics Auto Report",
  "dateRange": {
    "start": "30_days_ago",
    "end": "today"
  },
  "chartSettings": {
    "width": 1920,
    "height": 1080,
    "margin": { "top": 50, "right": 80, "bottom": 50, "left": 80 },
    "outputFormat": "svg"
  },
  "metrics": [
    {
      "path": "summary.total_tests",
      "axis": "left",
      "type": "line",
      "color": "#4a90e2",
      "label": "Total Tests"
    },
    {
      "path": "summary.total_passed",
      "axis": "left",
      "type": "bar",
      "color": "#7ed321",
      "label": "Tests Passed"
    },
    {
      "path": "summary.combined_coverage",
      "axis": "right",
      "type": "line",
      "color": "#f5a623",
      "label": "Coverage %"
    }
  ],
  "outputFile": "metrics_report.svg"
}
```

### 5. Command Line Interface (hm_browser.sh)

```bash
#!/bin/bash

# hm_browser.sh - Hydrogen Metrics Browser CLI
# Usage: ./hm_browser.sh <config.json> <output.svg>

CONFIG_FILE=$1
OUTPUT_FILE=$2
SCRIPT_DIR=$(dirname "$0")

# Validate inputs
if [ -z "$CONFIG_FILE" ] || [ -z "$OUTPUT_FILE" ]; then
  echo "Usage: $0 <config.json> <output.svg>"
  exit 1
fi

if [ ! -f "$CONFIG_FILE" ]; then
  echo "Error: Config file $CONFIG_FILE not found"
  exit 1
fi

# Run the browser in headless mode
node "$SCRIPT_DIR/hm_browser.js" --config "$CONFIG_FILE" --output "$OUTPUT_FILE" --headless

if [ $? -eq 0 ]; then
  echo "Successfully generated $OUTPUT_FILE"
else
  echo "Error generating metrics report"
  exit 1
fi
```

## Implementation Roadmap

### Phase 1: Core Infrastructure

1. ✅ Create architecture documentation
2. Create CSS foundation
3. Create HTML structure
4. Implement basic JavaScript class structure
5. Implement file discovery logic

### Phase 2: Data Processing

1. Implement numeric value extraction
2. Create configuration system
3. Implement date range handling
4. Add error handling and validation

### Phase 3: Visualization

1. Implement D3 chart rendering
2. Add dual axis support
3. Implement interactive controls
4. Add color picker integration

### Phase 4: Command Line

1. Create CLI script
2. Implement headless mode
3. Add SVG export functionality
4. Test with sample data

### Phase 5: Testing & Documentation

1. Create test cases
2. Write usage documentation
3. Add examples
4. Final validation

## Technical Considerations

- **Performance**: Optimize JSON parsing for large metrics files
- **Error Handling**: Graceful degradation when files are missing
- **Accessibility**: Ensure UI is keyboard-navigable
- **Responsiveness**: Test across device sizes
- **Security**: Validate all user inputs

## Success Criteria

1. Browser version successfully loads and displays metrics from GitHub
2. Command line version generates SVG reports from local files
3. Interactive controls work for adding/removing metrics
4. Dual axis charting works correctly
5. Date range selection functions properly
6. Configuration system allows customization