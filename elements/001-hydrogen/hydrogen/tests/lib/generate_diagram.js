import fs from 'fs';
import * as d3 from 'd3';
import { JSDOM } from 'jsdom';

// Sample JSON data
const data = [
  {
    table: "Customer",
    columns: [
      { name: "customer_id", type: "int", isPrimary: true },
      { name: "name", type: "varchar" }
    ],
    position: { x: 50, y: 50 }
  },
  {
    table: "Order",
    columns: [
      { name: "order_id", type: "int", isPrimary: true },
      { name: "customer_id", type: "int", isForeign: true }
    ],
    position: { x: 300, y: 50 }
  },
  {
    relationship: {
      from: { table: "Customer", column: "customer_id" },
      to: { table: "Order", column: "customer_id" },
      type: "one-to-many"
    }
  }
];

// Create virtual DOM
const dom = new JSDOM('<!DOCTYPE html><body><svg width="600" height="400"></svg></body>');
const svg = d3.select(dom.window.document.querySelector('svg'));

const tableWidth = 150;
const tableHeight = 80;
const columnHeight = 20;

// Add styles
const style = `
  <style>
    .table { stroke: black; fill: white; }
    .table-name { font-weight: bold; }
    .primary-key { font-weight: bold; fill: blue; }
    .relationship { stroke: black; stroke-width: 2; }
    .crow-foot { stroke: black; stroke-width: 2; }
  </style>
`;
dom.window.document.head.innerHTML += style;

// Draw tables
const tables = data.filter(d => d.table);
svg.selectAll('.table-group')
  .data(tables)
  .enter()
  .append('g')
  .attr('class', 'table-group')
  .attr('transform', d => `translate(${d.position.x}, ${d.position.y})`)
  .each(function(d) {
    const group = d3.select(this);
    group.append('rect')
      .attr('class', 'table')
      .attr('width', tableWidth)
      .attr('height', tableHeight);
    group.append('text')
      .attr('class', 'table-name')
      .attr('x', 10)
      .attr('y', 20)
      .text(d.table);
    group.selectAll('.column')
      .data(d.columns)
      .enter()
      .append('text')
      .attr('class', d => d.isPrimary ? 'column primary-key' : 'column')
      .attr('x', 10)
      .attr('y', (d, i) => 40 + i * columnHeight)
      .text(d => `${d.name} (${d.type})`);
  });

// Draw relationships
const relationships = data.filter(d => d.relationship);
relationships.forEach(rel => {
  const fromTable = tables.find(t => t.table === rel.relationship.from.table);
  const toTable = tables.find(t => t.table === rel.relationship.to.table);
  const startX = fromTable.position.x + tableWidth;
  const startY = fromTable.position.y + tableHeight / 2;
  const endX = toTable.position.x;
  const endY = toTable.position.y + tableHeight / 2;
  const midX = (startX + endX) / 2;

  svg.append('path')
    .attr('class', 'relationship')
    .attr('d', `M${startX},${startY} H${midX} V${endY} H${endX}`)
    .attr('fill', 'none');

  if (rel.relationship.type === 'one-to-many') {
    const crowLength = 10;
    svg.append('path')
      .attr('class', 'crow-foot')
      .attr('d', `M${endX},${endY - crowLength} L${endX - crowLength},${endY} L${endX},${endY + crowLength}`)
      .attr('fill', 'none');
    svg.append('line')
      .attr('class', 'crow-foot')
      .attr('x1', startX)
      .attr('y1', startY - crowLength)
      .attr('x2', startX)
      .attr('y2', startY + crowLength);
  }
});

// Save SVG
fs.writeFileSync('erd.svg', dom.window.document.documentElement.outerHTML);
console.log('ERD generated as erd.svg');
