#!/usr/bin/env bash

# Test script for combined features of title, footer, and summary rows
# Creates tables with different combinations of title and footer positions, and includes summary rows for CPU and MEM

# Create layout JSON files with different combinations of title and footer positions
cat > layout_center_title_center_footer.json << EOF
{
  "title": "Kubernetes Pod Resources",
  "title_position": "center",
  "footer": "Kubernetes Pod Resources Summary",
  "footer_position": "center",
  "theme": "Red",
  "columns": [
    { "header": "Pod Name", "width": 35 },
    { "header": "CPU", "width": 8, "justification": "right", "datatype": "kcpu", "summary": "sum" },
    { "header": "MEM", "width": 8, "justification": "right", "datatype": "kmem", "summary": "sum" }
  ]
}
EOF

cat > layout_left_title_right_footer.json << EOF
{
  "title": "Kubernetes Pod Resources",
  "title_position": "left",
  "footer": "Kubernetes Pod Resources Summary",
  "footer_position": "right",
  "theme": "Blue",
  "columns": [
    { "header": "Pod Name", "width": 35 },
    { "header": "CPU", "width": 8, "justification": "right", "datatype": "kcpu", "summary": "sum" },
    { "header": "MEM", "width": 8, "justification": "right", "datatype": "kmem", "total": "sum" }
  ]
}
EOF

cat > layout_right_title_left_footer.json << EOF
{
  "title": "Kubernetes Pod Resources",
  "title_position": "right",
  "footer": "Kubernetes Pod Resources Summary",
  "footer_position": "left",
  "theme": "Red",
  "columns": [
    { "header": "Pod Name", "width": 35 },
    { "header": "CPU", "width": 8, "justification": "right", "datatype": "kcpu", "total": "sum" },
    { "header": "MEM", "width": 8, "justification": "right", "datatype": "kmem", "total": "sum" }
  ]
}
EOF

cat > layout_long_title_long_footer.json << EOF
{
  "title": "Kubernetes Pod Resource Usage Metrics Dashboard - Production Cluster Overview",
  "title_position": "center",
  "footer": "Kubernetes Pod Resource Usage Metrics Dashboard - Production Cluster Overview Summary Report",
  "footer_position": "center",
  "theme": "Blue",
  "columns": [
    { "header": "Pod Name", "width": 35 },
    { "header": "CPU", "width": 8, "justification": "right", "datatype": "kcpu", "total": "sum" },
    { "header": "MEM", "width": 8, "justification": "right", "datatype": "kmem", "total": "sum" }
  ]
}
EOF

# Create data JSON
cat > data.json << EOF
[
  { "pod_name": "coredns-787d4b4b6c-abcd1", "cpu": "100m", "mem": "70M" },
  { "pod_name": "coredns-787d4b4b6c-abcd2", "cpu": "100m", "mem": "70M" },
  { "pod_name": "nginx-deployment-66b6c48dd5-efgh1", "cpu": "750m", "mem": "768M" },
  { "pod_name": "nginx-deployment-66b6c48dd5-efgh2", "cpu": "750m", "mem": "768M" }
]
EOF

# Run tests with each combination of title and footer positions
echo "Test 1: Center Title and Center Footer with Summary Rows"
echo "----------------------------------------"
./support_tables.sh layout_center_title_center_footer.json data.json

echo "Test 2: Left Title and Right Footer with Summary Rows"
echo "----------------------------------------"
./support_tables.sh layout_left_title_right_footer.json data.json

echo "Test 3: Right Title and Left Footer with Summary Rows"
echo "----------------------------------------"
./support_tables.sh layout_right_title_left_footer.json data.json

echo "Test 4: Long Title and Long Footer with Summary Rows (both center-aligned)"
echo "----------------------------------------"
./support_tables.sh layout_long_title_long_footer.json data.json

# Clean up temporary files
rm -f layout_*.json data.json
