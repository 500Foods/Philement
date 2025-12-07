/**
 * Hydrogen Metrics Browser - Chart Functions
 * D3 chart rendering and visualization
 *
 * @version 1.0.0
 * @license MIT
 */

// Extend the global namespace
var HMB = HMB || {};

// Render the D3 chart
HMB.renderChart = function() {
  if (this.state.selectedMetrics.length === 0) {
    console.log('No metrics selected to render');
    return;
  }

  if (!this.state.filteredData || this.state.filteredData.length === 0) {
    this.filterDataByDateRange();
  }

  if (!this.state.filteredData || this.state.filteredData.length === 0) {
    console.log('No data available for selected date range');
    return;
  }

  // Clear previous chart
  d3.select('#metrics-chart').html('');

  // Set up chart dimensions
  const width = this.config.chartSettings.width - this.config.chartSettings.margin.left - this.config.chartSettings.margin.right;
  const height = this.config.chartSettings.height - this.config.chartSettings.margin.top - this.config.chartSettings.margin.bottom;

  // Create SVG container
  const svg = d3.select('#metrics-chart')
    .attr('width', this.config.chartSettings.width)
    .attr('height', this.config.chartSettings.height)
    .append('g')
    .attr('transform', `translate(${this.config.chartSettings.margin.left},${this.config.chartSettings.margin.top})`);

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
};

// Get domain for an axis based on metrics
HMB.getAxisDomain = function(metrics) {
  if (metrics.length === 0) return [0, 1];

  let min = Infinity;
  let max = -Infinity;

  metrics.forEach(metric => {
    this.state.filteredData.forEach(file => {
      if (file.data) {
        const value = this.getNestedValue(file.data, metric.path);
        if (typeof value === 'number') {
          min = Math.min(min, value);
          max = Math.max(max, value);
        }
      }
    });
  });

  // Add 10% padding if we have a valid range
  if (min !== Infinity && max !== -Infinity) {
    const padding = (max - min) * 0.1;
    return [min - padding, max + padding];
  } else {
    return [0, 1];
  }
};

// Draw metrics on the chart
HMB.drawMetrics = function(svg, xScale, leftYScale, rightYScale, width, height) {
  const lineGenerator = d3.line()
    .defined(d => d.value !== null && d.value !== undefined)
    .x(d => xScale(new Date(d.date)))
    .y(d => {
      const metric = this.state.selectedMetrics.find(m => m.path === d.metricPath);
      return metric.axis === 'left' ? leftYScale(d.value) : rightYScale(d.value);
    })
    .curve(d3.curveMonotoneX);

  this.state.selectedMetrics.forEach(metric => {
    // Prepare data for this metric
    const metricData = this.state.filteredData.map(file => {
      const value = file.data ? this.getNestedValue(file.data, metric.path) : null;
      return {
        date: file.date,
        value: typeof value === 'number' ? value : null,
        metricPath: metric.path
      };
    });

    // Determine which scale to use
    const yScale = metric.axis === 'left' ? leftYScale : rightYScale;

    // Draw line
    if (metric.type === 'line') {
      svg.append('path')
        .datum(metricData)
        .attr('class', 'metric-line')
        .attr('d', lineGenerator)
        .attr('stroke', metric.color)
        .attr('stroke-width', 2)
        .attr('fill', 'none');
    }
  });
};