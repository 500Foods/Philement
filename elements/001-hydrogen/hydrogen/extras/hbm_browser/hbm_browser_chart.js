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
  let width, height;
  if (this.state.isHeadless) {
    // Use configured dimensions for headless mode
    width = this.config.chartSettings.width - this.config.chartSettings.margin.left - this.config.chartSettings.margin.right;
    height = this.config.chartSettings.height - this.config.chartSettings.margin.top - this.config.chartSettings.margin.bottom - 90;
  } else {
    width = container.clientWidth - this.config.chartSettings.margin.left - this.config.chartSettings.margin.right;
    height = container.clientHeight - this.config.chartSettings.margin.top - this.config.chartSettings.margin.bottom - 90;
  }

  // Initialize zoom transform if not exists
  this.state.zoomTransform = this.state.zoomTransform || d3.zoomIdentity;

  // Add defs for gradients at SVG root level
  const defs = d3.select('#metrics-chart').append('defs');

  // Create SVG container with responsive dimensions
  const svgRoot = d3.select('#metrics-chart')
    .attr('width', this.state.isHeadless ? this.config.chartSettings.width : container.clientWidth)
    .attr('height', this.state.isHeadless ? this.config.chartSettings.height : container.clientHeight);

  // Add background rectangle for headless mode
  if (this.state.isHeadless) {
    svgRoot.insert('rect', ':first-child')
      .attr('width', '100%')
      .attr('height', '100%')
      .attr('fill', '#1e1e1e')
      .attr('rx', 6);
  }

  const svg = svgRoot.append('g')
    .attr('transform', `translate(${this.config.chartSettings.margin.left},${this.config.chartSettings.margin.top})`);

  // Add clip path for x-axis to prevent labels from extending beyond chart
  defs.append('clipPath')
    .attr('id', 'x-axis-clip')
    .append('rect')
    .attr('x', 0)
    .attr('y', -50)
    .attr('width', width)
    .attr('height', 150);

  // Add clip path for chart area to clip lines and bars at boundaries
  defs.append('clipPath')
    .attr('id', 'chart-clip')
    .append('rect')
    .attr('x', 0)
    .attr('y', 0)
    .attr('width', width)
    .attr('height', height);

  // Add chart title to SVG
  const chartTitle = svg.append('text')
    .attr('class', 'chart-title')
    .attr('x', width / 2)
    .attr('y', -20 )
    .attr('text-anchor', 'middle')
    .attr('fill', this.state.isHeadless ? '#ffffff' : 'var(--text-color)')
    .attr('font-family', 'Cairo, "Segoe UI", Tahoma, Geneva, Verdana, sans-serif')
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
  // Generate full date range for domain to ensure all dates are displayed
  // Use UTC dates constructed directly from date parts to avoid timezone conversions
  const dates = this.state.filteredData.map(d => {
    const [year, month, day] = d.date.split('-').map(Number);
    return new Date(Date.UTC(year, month - 1, day, 0, 0, 0));
  });
  const minDate = d3.min(dates);
  const maxDate = d3.max(dates);
  // Add one day padding on each end for comfortable bar/point display
  const paddedMinDate = d3.utcDay.offset(minDate, -1);
  const paddedMaxDate = d3.utcDay.offset(maxDate, 1);
  const allDates = d3.utcDay.range(minDate, d3.utcDay.offset(maxDate, 1));

  // Debug: Log missing dates within the selected date range
  const logMissingDates = () => {
    const startParts = this.state.currentDateRange.start.split('-').map(Number);
    const endParts = this.state.currentDateRange.end.split('-').map(Number);
    const startDate = new Date(Date.UTC(startParts[0], startParts[1] - 1, startParts[2]));
    const endDate = new Date(Date.UTC(endParts[0], endParts[1] - 1, endParts[2]));
    const missingDates = [];
    let currentDate = new Date(startDate);
    while (currentDate <= endDate) {
      const dateStr = d3.utcFormat('%Y-%m-%d')(currentDate);
      const file = this.state.filteredData.find(d => d.date === dateStr);
      if (!file || !file.data) {
        missingDates.push(dateStr);
      } else {
        const hasAllMetrics = this.state.selectedMetrics.every(metric => {
          const value = this.getNestedValue(file.data, metric.path);
          return typeof value === 'number' && value !== null && value !== undefined;
        });
        if (!hasAllMetrics) {
          missingDates.push(dateStr);
        }
      }
      currentDate.setUTCDate(currentDate.getUTCDate() + 1);
    }
//    console.log('Missing dates:', missingDates);
//    console.log('Count of missing dates:', missingDates.length);
    this.state.missingDates = missingDates;
  };
  logMissingDates();

  let xScale = d3.scaleUtc()
    .domain([paddedMinDate, paddedMaxDate])
    .range([0, width]);

  // Create scales for each axis
  const leftAxisMetrics = this.state.selectedMetrics.filter(m => m.axis === 'left');
  const rightAxisMetrics = this.state.selectedMetrics.filter(m => m.axis === 'right');

  let leftYScale = d3.scaleLinear()
    .domain(this.getAxisDomain(leftAxisMetrics))
    .range([height, 0]);

  let rightYScale = d3.scaleLinear()
    .domain(this.getAxisDomain(rightAxisMetrics))
    .range([height, 0]);

  // Store original scales for zooming
  this.originalXScale = xScale.copy();
  this.originalLeftYScale = leftYScale.copy();
  this.originalRightYScale = rightYScale.copy();

  // Apply rescale if zoom transform exists
  if (this.state.zoomTransform) {
    xScale = this.state.zoomTransform.rescaleX(this.originalXScale);
    leftYScale = this.state.zoomTransform.rescaleY(this.originalLeftYScale);
    rightYScale = this.state.zoomTransform.rescaleY(this.originalRightYScale);
  }

  // Store current xScale for tooltip date calculation
  this.currentXScale = xScale;

  // Helper function to get day of week from YYYY-MM-DD string
  const getDayOfWeek = (dateStr) => {
    const [year, month, day] = dateStr.split('-').map(Number);
    const date = new Date(Date.UTC(year, month - 1, day, 0, 0, 0));
    return date.getUTCDay(); // 0 = Sunday
  };

  // Add X axis - use UTC formatting to match UTC dates
  const xAxis = svg.append('g')
    .attr('class', 'x-axis')
    .attr('transform', `translate(0,${height - 0})`)
    .attr('clip-path', 'url(#x-axis-clip)')
    .call(d3.axisBottom(xScale).tickValues(allDates).tickFormat(d => {
      const dayOfWeek = d.getUTCDay();
      if (dayOfWeek === 0) { // Sunday
        return d3.utcFormat('%Y-W%W')(d);
      } else {
        return d3.utcFormat('%Y-%m-%d')(d);
      }
    }));

  // Rotate tick labels 90 degrees
  xAxis.selectAll('text')
    .attr('transform', 'rotate(90)')
    .attr('text-anchor', 'start')
    .attr('dx', '0.8em')
    .attr('dy', '-0.25em')
    .attr('fill', this.state.isHeadless ? '#ffffff' : 'var(--text-color)')
    .attr('font-family', 'Cairo, "Segoe UI", Tahoma, Geneva, Verdana, sans-serif')
    .classed('darken-dates', d => this.state.missingDates.includes(d3.utcFormat('%Y-%m-%d')(d)))
    .style('font-weight', d => d.getUTCDay() === 0 ? 'bold' : 'normal');

  // In headless mode, set fill for missing dates
  if (this.state.isHeadless) {
    xAxis.selectAll('text')
      .filter(d => this.state.missingDates.includes(d3.utcFormat('%Y-%m-%d')(d)))
      .attr('fill', '#666666');
  }

  // In headless mode, remove clip-path from x-axis to show labels
  if (this.state.isHeadless) {
    xAxis.attr('clip-path', null);
  }

  // Add major axis lines on Sundays
  const sundayDates = allDates.filter(d => d.getUTCDay() === 0 && xScale(d) >= 0 && xScale(d) <= width);
  sundayDates.forEach(d => {
    svg.append('line')
      .attr('class', 'sunday-line')
      .attr('x1', xScale(d))
      .attr('y1', 0)
      .attr('x2', xScale(d))
      .attr('y2', height)
      .attr('stroke', '#333')
      .attr('stroke-width', 1)
      .attr('stroke-dasharray', '2,2');
  });

  // Add chart boundary lines
  svg.append('line')
    .attr('class', 'x-axis-line')
    .attr('x1', 0)
    .attr('y1', height)
    .attr('x2', width)
    .attr('y2', height)
    .attr('stroke', '#333')
    .attr('stroke-width', 1);

  svg.append('line')
    .attr('class', 'x-axis-line')
    .attr('x1', 0)
    .attr('y1', 0)
    .attr('x2', width)
    .attr('y2', 0)
    .attr('stroke', '#333')
    .attr('stroke-width', 1);

    svg.append('line')
    .attr('class', 'x-axis-line')
    .attr('x1', 0)
    .attr('y1', 0)
    .attr('x2', 0)
    .attr('y2', height)
    .attr('stroke', '#333')
    .attr('stroke-width', 1);

    svg.append('line')
    .attr('class', 'x-axis-line')
    .attr('x1', width)
    .attr('y1', 0)
    .attr('x2', width)
    .attr('y2', height)
    .attr('stroke', '#333')
    .attr('stroke-width', 1);

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
    .call(d3.axisLeft(leftYScale).ticks(5).tickFormat(abbreviateNumber))
    .selectAll('text')
    .attr('fill', this.state.isHeadless ? '#ffffff' : 'var(--text-color)')
    .attr('font-family', 'Cairo, "Segoe UI", Tahoma, Geneva, Verdana, sans-serif');

  // Add right Y axis if needed
  if (rightAxisMetrics.length > 0) {
    svg.append('g')
      .attr('class', 'y-axis right-axis')
      .attr('transform', `translate(${width},0)`)
      .call(d3.axisRight(rightYScale).ticks(5).tickFormat(abbreviateNumber))
      .selectAll('text')
      .attr('fill', this.state.isHeadless ? '#ffffff' : 'var(--text-color)')
      .attr('font-family', 'Cairo, "Segoe UI", Tahoma, Geneva, Verdana, sans-serif');
  }

  // Create gradients for bar metrics
  this.state.selectedMetrics.forEach(metric => {
    if (metric.type === 'bar') {
      const gradientId = `gradient-${metric.path.replace(/[^a-zA-Z0-9]/g, '_')}`;
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

  // Add zoom and pan (skip in headless mode)
  if (!this.state.isHeadless) {
    const zoom = d3.zoom()
      .scaleExtent([0.5, 5])
      .translateExtent([[-width * 2, -height * 2], [width * 2, height * 2]])
      .on('zoom', (event) => {
        this.state.zoomTransform = event.transform;
        const rescaledX = event.transform.rescaleX(this.originalXScale);
        const rescaledLeftY = event.transform.rescaleY(this.originalLeftYScale);
        const rescaledRightY = event.transform.rescaleY(this.originalRightYScale);

        // Update current xScale for tooltip
        this.currentXScale = rescaledX;

        // Update x axis
        xAxis.call(d3.axisBottom(rescaledX).tickValues(allDates).tickFormat(d => {
          const dayOfWeek = d.getUTCDay();
          if (dayOfWeek === 0) {
            return d3.utcFormat('%Y-W%W')(d);
          } else {
            return d3.utcFormat('%Y-%m-%d')(d);
          }
        }));

        // Rotate tick labels 90 degrees
        xAxis.selectAll('text')
          .attr('transform', 'rotate(90)')
          .attr('text-anchor', 'start')
          .attr('dx', '0.8em')
          .attr('dy', '-0.25em')
          .attr('fill', (d) => {
            const dateStr = d3.utcFormat('%Y-%m-%d')(d);
            const isMissing = this.state.missingDates.includes(dateStr);
            if (this.state.isHeadless) {
              return isMissing ? '#666666' : '#ffffff'; // Dark gray for missing dates, white for available
            } else {
              return 'var(--text-color)';
            }
          })
          .attr('font-family', 'Cairo, "Segoe UI", Tahoma, Geneva, Verdana, sans-serif')
          .classed('darken-dates', d => this.state.missingDates.includes(d3.utcFormat('%Y-%m-%d')(d)))
          .style('font-weight', d => d.getUTCDay() === 0 ? 'bold' : 'normal');

        // Update sunday lines
        svg.selectAll('.sunday-line').remove();
        const sundayDates = allDates.filter(d => d.getUTCDay() === 0 && rescaledX(d) >= 0 && rescaledX(d) <= width);
        sundayDates.forEach(d => {
          svg.append('line')
            .attr('class', 'sunday-line')
            .attr('x1', rescaledX(d))
            .attr('y1', 0)
            .attr('x2', rescaledX(d))
            .attr('y2', height)
            .attr('stroke', '#333')
            .attr('stroke-width', 1)
            .attr('stroke-dasharray', '2,2');
        });

        // Update y axes
        svg.select('.left-axis').call(d3.axisLeft(rescaledLeftY).ticks(5).tickFormat(abbreviateNumber));
        if (rightAxisMetrics.length > 0) {
          svg.select('.right-axis').call(d3.axisRight(rescaledRightY).ticks(5).tickFormat(abbreviateNumber));
        }

        // Redraw metrics
        svg.selectAll('.metric-line, .metric-dot, .metric-hover, .metric-bar, .metric-bar-hover').remove();
        this.drawMetrics(svg, rescaledX, rescaledLeftY, rescaledRightY, width, height);
      });

    d3.select('#metrics-chart').call(zoom);
    d3.select('#metrics-chart').call(zoom.transform, this.state.zoomTransform);
    d3.select('#metrics-chart').on('dblclick.zoom', () => {
      svg.transition().call(zoom.transform, d3.zoomIdentity);
    });
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
    .x(d => xScale(d.date))
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
       const [year, month, day] = file.date.split('-').map(Number);
       return {
         date: new Date(Date.UTC(year, month - 1, day, 0, 0, 0)),
         value: typeof value === 'number' ? value : null,
         metricPath: metric.path
       };
     });

    // Determine which scale to use
    const yScale = metric.axis === 'left' ? leftYScale : rightYScale;

    const [minX, maxX] = xScale.domain();
    const [minY, maxY] = yScale.domain();
    const validPoints = metric.type === 'line'
      ? metricData.filter(d => d.value !== null && d.value !== undefined)
      : metricData.filter(d => d.value !== null && d.value !== undefined && d.date >= minX && d.date <= maxX && d.value >= minY && d.value <= maxY);
    // console.log(`  ✅ Valid data points for "${metric.path}": ${validPoints.length}/${metricData.length}`);
    // if (validPoints.length === 0) {
    //   console.log(`  ❌ NO VALID DATA POINTS - this is why the metric doesn't show!`);
    // }

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
        .attr('clip-path', 'url(#chart-clip)')
        .style('stroke-width', lineWidth + 'px');

      if (strokeDasharray) {
        linePath.style('stroke-dasharray', strokeDasharray);
      }

      // Add invisible larger hover areas for each data point
      svg.selectAll('.metric-hover-' + metric.path.replace(/\./g, '-'))
        .data(validPoints)
        .enter()
        .append('circle')
        .attr('class', 'metric-hover')
        .attr('cx', d => xScale(d.date))
        .attr('cy', d => yScale(d.value))
        .attr('r', 10)
        .attr('fill', 'transparent')
        .attr('stroke', 'none')
        .attr('clip-path', 'url(#chart-clip)')
        .on('mouseover', (event, d) => {
          this.showTooltip(event, d, metric);
        })
        .on('mouseout', () => {
          this.hideTooltip();
        });

      // Add visible dots for each data point
      svg.selectAll('.metric-dot-' + metric.path.replace(/\./g, '-'))
        .data(validPoints)
        .enter()
        .append('circle')
        .attr('class', 'metric-dot')
        .attr('cx', d => xScale(d.date))
        .attr('cy', d => yScale(d.value))
        .attr('r', 4)
        .attr('fill', `url(#gradient-${metric.path.replace(/[^a-zA-Z0-9]/g, '_')})`)
        .attr('stroke', '#fff')
        .attr('stroke-width', 1)
        .attr('clip-path', 'url(#chart-clip)');
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

      // Calculate bar width - use consistent width based on date range
      const dateRange = xScale.domain();
      const daysDiff = Math.ceil((dateRange[1] - dateRange[0]) / (24 * 60 * 60 * 1000)) + 1;
      const barWidth = Math.max(4, width / (daysDiff * 2));
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
          const barX = xScale(d.date) - barWidth / 2;
          const barY = yScale(d.value);
          const barHeight = height - yScale(d.value);
          return roundedTopRect(barX, barY, barWidth, barHeight, cornerRadius);
        })
        .attr('fill', `url(#gradient-${metric.path.replace(/[^a-zA-Z0-9]/g, '_')})`)
        .attr('clip-path', 'url(#chart-clip)')
        .style('stroke', metric.color)
        .style('stroke-width', strokeWidth + 'px');

      // Add invisible larger hover areas for each bar
      svg.selectAll('.metric-bar-hover-' + metric.path.replace(/\./g, '-'))
        .data(validPoints)
        .enter()
        .append('rect')
        .attr('class', 'metric-bar-hover')
        .attr('x', d => xScale(d.date) - barWidth / 2 - 5)
        .attr('y', 0)
        .attr('width', barWidth + 10)
        .attr('height', height)
        .attr('fill', 'transparent')
        .attr('stroke', 'none')
        .attr('clip-path', 'url(#chart-clip)')
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

  // Get chart container bounds for proper positioning
  const chartContainer = this.state.elements.chartContainer;
  const containerRect = chartContainer ? chartContainer.getBoundingClientRect() : null;

  // Format the tooltip content
  const seriesName = metric.displayLabel || metric.label;
  // Calculate date from mouse position using current rescaled x-scale
  const mouseX = event.clientX - containerRect.left - this.config.chartSettings.margin.left;
  const dateObj = this.currentXScale.invert(mouseX);
  const date = d3.utcFormat('%Y-%m-%d')(dateObj);
  const value = typeof d.value === 'number' ? d.value.toLocaleString() : d.value;

  // Set tooltip content with line breaks
  tooltip.innerHTML = `
    <div style="font-size: 0.8rem; line-height: 1.2;">
      <div><strong>${seriesName}</strong></div>
      <div>${date}</div>
      <div>Value: ${value}</div>
    </div>
  `;

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
    .attr('transform', `translate(0, ${height + 115})`);


  // Add legend items
  const sortedMetrics = this.state.selectedMetrics.slice().sort((a, b) => (a.displayLabel || a.label).localeCompare(b.displayLabel || b.label));

  // Calculate widths for each legend item
  const tempText = legendGroup.append('text')
    .attr('font-size', '0.85rem')
    .style('visibility', 'hidden');
  const itemWidths = sortedMetrics.map(d => {
    tempText.text(d.displayLabel || d.label);
    let textWidth;
    if (this.state.isHeadless) {
      textWidth = (d.displayLabel || d.label).length * 8;
    } else {
      textWidth = tempText.node().getBBox().width;
    }
    return 15 + 5 + textWidth + 5; // color 15 + space 5 + text + padding 5
  });
  tempText.remove();

  const totalLegendWidth = itemWidths.reduce((sum, w, i) => sum + w + (i < itemWidths.length - 1 ? 20 : 0), 0);
  const startX = (width - totalLegendWidth) / 2;
  const positions = [];
  let currentX = startX;
  for (let i = 0; i < itemWidths.length; i++) {
    positions.push(currentX);
    currentX += itemWidths[i] + (i < itemWidths.length - 1 ? 20 : 0);
  }

  const legendItems = legendGroup.selectAll('.legend-item')
    .data(sortedMetrics)
    .enter()
    .append('g')
    .attr('class', 'legend-item')
    .attr('transform', (d, i) => `translate(${positions[i]}, 0)`);

  // Add color swatches
  legendItems.append('rect')
    .attr('class', 'legend-color')
    .attr('width', 15)
    .attr('height', 4)
    .attr('x', 0)
    .attr('y', -6)
    .attr('fill', d => d.color)
    .attr('rx', 1);

  // Add legend labels using display label for consistency
  legendItems.append('text')
    .attr('class', 'legend-label')
    .attr('x', 20)
    .attr('y', 0)
    .attr('fill', this.state.isHeadless ? '#ffffff' : 'var(--text-color)')
    .attr('font-family', 'Cairo, "Segoe UI", Tahoma, Geneva, Verdana, sans-serif')
    .attr('font-size', '0.85rem')
    .text(d => d.displayLabel || d.label);
};