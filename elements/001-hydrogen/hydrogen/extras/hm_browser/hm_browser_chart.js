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
      console.log('Chart title clicked!');
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
      console.log('SVG overlay clicked! Panel collapsed:', this.state.elements.controlPanel.classList.contains('collapsed'));

      // Only toggle if panel is collapsed
      if (this.state.elements.controlPanel.classList.contains('collapsed')) {
        console.log('Toggling control panel from SVG overlay');
        this.toggleControlPanel();
      } else {
        console.log('Panel not collapsed, ignoring click');
      }
    });

  // Log overlay creation
  console.log('SVG overlay created with dimensions:', width, 'x', height);
  console.log('Panel collapsed state:', this.state.elements.controlPanel.classList.contains('collapsed'));

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

// Get domain for an axis based on metrics
HMB.getAxisDomain = function(metrics) {
  if (metrics.length === 0) return [0, 1];

  let min = 0; // Always start at 0
  let max = 0;

  metrics.forEach(metric => {
    this.state.filteredData.forEach(file => {
      if (file.data) {
        const value = this.getNestedValue(file.data, metric.path);
        if (typeof value === 'number') {
          max = Math.max(max, value);
        }
      }
    });
  });

  // Add 10% padding if we have a valid max value
  if (max > 0) {
    const padding = max * 0.1;
    return [0, max + padding];
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

      // Add dots for each data point
      svg.selectAll('.metric-dot-' + metric.path.replace(/\./g, '-'))
        .data(metricData.filter(d => d.value !== null && d.value !== undefined))
        .enter()
        .append('circle')
        .attr('class', 'metric-dot')
        .attr('cx', d => xScale(new Date(d.date)))
        .attr('cy', d => yScale(d.value))
        .attr('r', 4)
        .attr('fill', metric.color)
        .attr('stroke', '#fff')
        .attr('stroke-width', 1);
    }
  });
};

// Draw legend in SVG
HMB.drawLegend = function(svg, width, height) {
  // Create legend group - position it within the visible chart area
  const legendGroup = svg.append('g')
    .attr('class', 'chart-legend')
    .attr('transform', `translate(${width - 200}, 20)`);

  // Add legend title
  legendGroup.append('text')
    .attr('class', 'legend-title')
    .attr('x', 0)
    .attr('y', 0)
    .attr('fill', 'var(--text-color)')
    .attr('font-size', '0.9rem')
    .attr('font-weight', '600')
    .text('Metrics Legend');

  // Add legend items
  const legendItems = legendGroup.selectAll('.legend-item')
    .data(this.state.selectedMetrics)
    .enter()
    .append('g')
    .attr('class', 'legend-item')
    .attr('transform', (d, i) => `translate(0, ${20 + i * 20})`);

  // Add color swatches
  legendItems.append('rect')
    .attr('class', 'legend-color')
    .attr('width', 15)
    .attr('height', 3)
    .attr('x', 0)
    .attr('y', -2)
    .attr('fill', d => d.color)
    .attr('rx', 1);

  // Add legend labels
  legendItems.append('text')
    .attr('class', 'legend-label')
    .attr('x', 20)
    .attr('y', 0)
    .attr('fill', 'var(--text-color)')
    .attr('font-size', '0.85rem')
    .text(d => d.label);
};