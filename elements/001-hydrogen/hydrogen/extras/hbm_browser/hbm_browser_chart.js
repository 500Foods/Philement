/**
 * Hydrogen Build Metrics Browser - Chart Functions
 * D3 chart rendering and visualization
 *
 * @version 1.0.0
 * @license MIT
 */

// Extend the global namespace
var HMB = HMB || {};

// Render the D3 chart
HMB.renderChart = function() {
  // console.log('🎨 renderChart called, selectedMetrics:', this.state.selectedMetrics.length);
  // if (this.state.selectedMetrics.length > 0) {
  //   console.log('📊 Selected metrics:');
  //   this.state.selectedMetrics.forEach((m, i) => {
  //     console.log(`  ${i+1}. "${m.path}" (${m.label})`);
  //   });
  // }

  if (this.state.selectedMetrics.length === 0) {
    // console.log('❌ No metrics selected to render');
    // Clear the chart completely
    const svgElement = d3.select('#metrics-chart');
    svgElement.selectAll('*').remove();
    return;
  }

  if (!this.state.filteredData || this.state.filteredData.length === 0) {
    // console.log('🔄 Calling filterDataByDateRange...');
    this.filterDataByDateRange();
  }

  if (!this.state.filteredData || this.state.filteredData.length === 0) {
    // console.log('❌ No data available for selected date range');
    return;
  }

  // console.log('✅ Proceeding with chart rendering...');

  // Clear previous chart completely
  const svgElement = d3.select('#metrics-chart');
  svgElement.selectAll('*').remove();
  svgElement.html('');

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

  // Add defs for gradients
  const defs = svg.append('defs');

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

  // Helper function to abbreviate numbers
  const abbreviateNumber = (num) => {
    if (num >= 1000000) {
      return (num / 1000000).toFixed(1).replace(/\.0$/, '') + 'M';
    }
    if (num >= 1000) {
      return (num / 1000).toFixed(1).replace(/\.0$/, '') + 'K';
    }
    return num.toString();
  };

  // Add left Y axis
  svg.append('g')
    .attr('class', 'y-axis left-axis')
    .call(d3.axisLeft(leftYScale).ticks(5).tickFormat(abbreviateNumber));

  // Add right Y axis if needed
  if (rightAxisMetrics.length > 0) {
    svg.append('g')
      .attr('class', 'y-axis right-axis')
      .attr('transform', `translate(${width},0)`)
      .call(d3.axisRight(rightYScale).ticks(5).tickFormat(abbreviateNumber));
  }

  // Create gradients for bar metrics
  this.state.selectedMetrics.forEach(metric => {
    if (metric.type === 'bar') {
      const gradientId = `gradient-${metric.path.replace(/\./g, '-')}`;
      const gradient = defs.append('linearGradient')
        .attr('id', gradientId)
        .attr('x1', '0%')
        .attr('y1', '0%')
        .attr('x2', '0%')
        .attr('y2', '100%');

      // Function to darken color by 50%
      const darkenColor = (color) => {
        // Simple darkening by reducing RGB values
        const hex = color.replace('#', '');
        const r = Math.max(0, parseInt(hex.substr(0, 2), 16) * 0.5);
        const g = Math.max(0, parseInt(hex.substr(2, 2), 16) * 0.5);
        const b = Math.max(0, parseInt(hex.substr(4, 2), 16) * 0.5);
        return `rgb(${Math.round(r)}, ${Math.round(g)}, ${Math.round(b)})`;
      };

      gradient.append('stop')
        .attr('offset', '0%')
        .attr('stop-color', metric.color);

      gradient.append('stop')
        .attr('offset', '100%')
        .attr('stop-color', darkenColor(metric.color));
    }
  });

  // Draw metrics
  this.drawMetrics(svg, xScale, leftYScale, rightYScale, width, height);

  // Add legend to SVG only if there are metrics
  if (this.state.selectedMetrics.length > 0) {
    this.drawLegend(svg, width, height);
  }
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
    .x(d => xScale(new Date(d.date)))
    .y(d => {
      const metric = this.state.selectedMetrics.find(m => m.path === d.metricPath);
      return metric.axis === 'left' ? leftYScale(d.value) : rightYScale(d.value);
    })
    .curve(d3.curveMonotoneX);

  this.state.selectedMetrics.forEach(metric => {
    // console.log(`📈 Processing metric: "${metric.path}" (${metric.label})`);

    // Prepare data for this metric
    const metricData = this.state.filteredData.map(file => {
      // console.log(`  📅 ${file.date}: data exists = ${!!file.data}`);
      // if (file.data) {
      //   console.log(`    Data keys: ${Object.keys(file.data).join(', ')}`);
      //   if (file.data.test_results) {
      //     console.log(`    test_results exists, data array length: ${file.data.test_results.data ? file.data.test_results.data.length : 'no data array'}`);
      //   }
      // }
      const value = file.data ? this.getNestedValue(file.data, metric.path) : null;
      // console.log(`    📊 path "${metric.path}" -> ${value} (type: ${typeof value})`);
      return {
        date: file.date,
        value: typeof value === 'number' ? value : null,
        metricPath: metric.path
      };
    });

    const validPoints = metricData.filter(d => d.value !== null && d.value !== undefined);
    // console.log(`  ✅ Valid data points for "${metric.path}": ${validPoints.length}/${metricData.length}`);
    // if (validPoints.length === 0) {
    //   console.log(`  ❌ NO VALID DATA POINTS - this is why the metric doesn't show!`);
    // }

    // Determine which scale to use
    const yScale = metric.axis === 'left' ? leftYScale : rightYScale;

    // Draw line
    if (metric.type === 'line') {
      // Determine line width and dash array based on lineStyle
      let lineWidth = 2;
      let strokeDasharray = null;

      switch (metric.lineStyle) {
        case 'thin':
          lineWidth = 1;
          break;
        case 'regular':
          lineWidth = 3;
          break;
        case 'thick':
          lineWidth = 6;
          break;
        case 'dashed':
          lineWidth = 3;
          strokeDasharray = '8,4';
          break;
        case 'dotted':
          lineWidth = 3;
          strokeDasharray = '2,4';
          break;
        default:
          lineWidth = 3;
      }

      const linePath = svg.append('path')
        .datum(validPoints)
        .attr('class', 'metric-line')
        .attr('d', lineGenerator)
        .attr('stroke', metric.color)
        .attr('fill', 'none')
        .style('stroke-width', lineWidth + 'px');

      if (strokeDasharray) {
        linePath.style('stroke-dasharray', strokeDasharray);
      }

      // Add invisible larger hover areas for each data point
      svg.selectAll('.metric-hover-' + metric.path.replace(/\./g, '-'))
        .data(metricData.filter(d => d.value !== null && d.value !== undefined))
        .enter()
        .append('circle')
        .attr('class', 'metric-hover')
        .attr('cx', d => xScale(new Date(d.date)))
        .attr('cy', d => yScale(d.value))
        .attr('r', 10)
        .attr('fill', 'transparent')
        .attr('stroke', 'none')
        .on('mouseover', (event, d) => {
          this.showTooltip(event, d, metric);
        })
        .on('mouseout', () => {
          this.hideTooltip();
        });

      // Add visible dots for each data point
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
    } else if (metric.type === 'bar') {
      // Determine bar stroke width based on lineStyle
      let strokeWidth = 1;

      switch (metric.lineStyle) {
        case 'thin':
          strokeWidth = 1;
          break;
        case 'regular':
          strokeWidth = 2;
          break;
        case 'thick':
          strokeWidth = 3;
          break;
        case 'dashed':
          strokeWidth = 2;
          break;
        case 'dotted':
          strokeWidth = 2;
          break;
        default:
          strokeWidth = 2;
      }

      // Calculate bar width - simple approach: fixed width based on number of points
      const barWidth = Math.max(4, width / (validPoints.length * 2));
      const cornerRadius = 3;

      // Function to create rounded top rectangle path
      const roundedTopRect = (x, y, w, h, r) => {
        return `M${x} ${y + r}
                Q${x} ${y} ${x + r} ${y}
                L${x + w - r} ${y}
                Q${x + w} ${y} ${x + w} ${y + r}
                L${x + w} ${y + h}
                L${x} ${y + h}
                Z`;
      };

      svg.selectAll('.metric-bar-' + metric.path.replace(/\./g, '-'))
        .data(validPoints)
        .enter()
        .append('path')
        .attr('class', 'metric-bar')
        .attr('d', d => {
          const barX = xScale(new Date(d.date)) - barWidth / 2;
          const barY = yScale(d.value);
          const barHeight = height - yScale(d.value);
          return roundedTopRect(barX, barY, barWidth, barHeight, cornerRadius);
        })
        .attr('fill', `url(#gradient-${metric.path.replace(/\./g, '-')})`)
        .style('stroke', metric.color)
        .style('stroke-width', strokeWidth + 'px');

      // Add invisible larger hover areas for each bar
      svg.selectAll('.metric-bar-hover-' + metric.path.replace(/\./g, '-'))
        .data(validPoints)
        .enter()
        .append('rect')
        .attr('class', 'metric-bar-hover')
        .attr('x', d => xScale(new Date(d.date)) - barWidth / 2 - 5)
        .attr('y', 0)
        .attr('width', barWidth + 10)
        .attr('height', height)
        .attr('fill', 'transparent')
        .attr('stroke', 'none')
        .on('mouseover', (event, d) => {
          this.showTooltip(event, d, metric);
        })
        .on('mouseout', () => {
          this.hideTooltip();
        });
    }
  });
};

// Show tooltip on hover
HMB.showTooltip = function(event, d, metric) {
  // Clear any pending hide timeout
  if (this.tooltipHideTimeout) {
    clearTimeout(this.tooltipHideTimeout);
    this.tooltipHideTimeout = null;
  }

  const tooltip = this.state.elements.tooltip;
  if (!tooltip) return;

  // Format the tooltip content
  const seriesName = metric.displayLabel || metric.label;
  // Use the date string directly to avoid timezone conversion issues
  // d.date is already in YYYY-MM-DD format
  const date = d.date;
  const value = typeof d.value === 'number' ? d.value.toLocaleString() : d.value;

  // Set tooltip content with line breaks
  tooltip.innerHTML = `
    <div style="font-size: 0.8rem; line-height: 1.2;">
      <div><strong>${seriesName}</strong></div>
      <div>${date}</div>
      <div>Value: ${value}</div>
    </div>
  `;

  // Get chart container bounds for proper positioning
  const chartContainer = this.state.elements.chartContainer;
  const containerRect = chartContainer ? chartContainer.getBoundingClientRect() : null;

  // Position tooltip near mouse cursor (relative to chart container)
  let x = event.clientX - containerRect.left + 10;
  let y = event.clientY - containerRect.top - 10;

  // If we have container bounds, ensure tooltip stays within chart area
  if (containerRect) {
    // Adjust if tooltip would go outside chart container
    if (x + 200 > containerRect.width) { // Assume tooltip width ~200px
      x = event.clientX - containerRect.left - 210;
    }
    if (y + 80 > containerRect.height) { // Assume tooltip height ~80px
      y = event.clientY - containerRect.top - 90;
    }

    // Ensure tooltip stays within container bounds
    x = Math.max(10, Math.min(x, containerRect.width - 210));
    y = Math.max(10, Math.min(y, containerRect.height - 90));
  } else {
    // Fallback to viewport bounds if container not found
    if (x + 200 > window.innerWidth) {
      x = event.clientX - 210;
    }
    if (y + 80 > window.innerHeight) {
      y = event.clientY - 90;
    }
    x = Math.max(10, Math.min(x, window.innerWidth - 210));
    y = Math.max(10, Math.min(y, window.innerHeight - 90));
  }

  tooltip.style.left = `${x}px`;
  tooltip.style.top = `${y}px`;

  // Set fade-in transition and show
  tooltip.style.transition = 'opacity 0.1s ease';
  tooltip.style.opacity = '1';
  tooltip.classList.add('visible');
};

// Hide tooltip with delay to reduce flicker
HMB.hideTooltip = function() {
  const tooltip = this.state.elements.tooltip;
  if (tooltip) {
    // Clear any existing hide timeout
    if (this.tooltipHideTimeout) {
      clearTimeout(this.tooltipHideTimeout);
    }
    // Delay hide by 200ms to reduce flicker
    this.tooltipHideTimeout = setTimeout(() => {
      tooltip.style.transition = 'opacity 0.5s ease';
      tooltip.style.opacity = '0';
      tooltip.classList.remove('visible');
      this.tooltipHideTimeout = null;
    }, 200);
  }
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

  // Add legend labels using display label for consistency
  legendItems.append('text')
    .attr('class', 'legend-label')
    .attr('x', 20)
    .attr('y', 0)
    .attr('fill', 'var(--text-color)')
    .attr('font-size', '0.85rem')
    .text(d => d.displayLabel || d.label);
};