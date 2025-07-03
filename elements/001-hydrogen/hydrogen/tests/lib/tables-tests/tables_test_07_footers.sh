#!/usr/bin/env bash

# Test Suite 7: Footers - Advanced footer mechanisms with flair
# This test suite focuses on demonstrating the footer mechanism with enhanced visual appeal,
# incorporating various formatting options, summaries, and wrapping for a more dynamic display.

# Create temporary files for our JSON
layout_file=$(mktemp)
data_file=$(mktemp)
tables_script="$(dirname "$0")/../tables.sh"

# Cleanup function
cleanup() {
    rm -f "$layout_file" "$data_file"
}
trap cleanup EXIT

# Check for --debug flag
DEBUG_FLAG=""
if [[ "$1" == "--debug" ]]; then
    DEBUG_FLAG="--debug"
    echo "Debug mode enabled"
fi

# Setup comprehensive test data showcasing various datatypes and long text for wrapping
cat > "$data_file" << 'EOF'
[
  {
    "id": 1,
    "server_name": "web-server-01",
    "category": "Web",
    "cpu_cores": 4,
    "load_avg": 2.45,
    "cpu_usage": "1250m",
    "memory_usage": "2048Mi",
    "status": "Running",
    "description": "Primary web server for frontend applications with a detailed setup.",
    "tags": "frontend,app,ui,primary,loadbalancer"
  },
  {
    "id": 2,
    "server_name": "db-server-01",
    "category": "Database",
    "cpu_cores": 8,
    "load_avg": 5.12,
    "cpu_usage": "3200m",
    "memory_usage": "8192Mi", 
    "status": "Running",
    "description": "Main database server handling critical data operations.",
    "tags": "db,sql,storage,primary,backend"
  },
  {
    "id": 3,
    "server_name": "cache-server",
    "category": "Cache",
    "cpu_cores": 2,
    "load_avg": 0.85,
    "cpu_usage": "500m",
    "memory_usage": "1024Mi",
    "status": "Starting",
    "description": "In-memory cache for speeding up data access.",
    "tags": "cache,redis,fast,memory"
  },
  {
    "id": 4,
    "server_name": "api-gateway",
    "category": "Web",
    "cpu_cores": 6,
    "load_avg": 3.21,
    "cpu_usage": "2100m",
    "memory_usage": "4096Mi",
    "status": "Running",
    "description": "API gateway managing incoming requests and routing.",
    "tags": "api,gateway,routing,web,interface"
  }
]
EOF

# Test 7-A: Basic footer with footer and summaries
cat > "$layout_file" << 'EOF'
{
  "theme": "Red",
  "footer": "Server Overview Report",
  "footer_position": "center",
  "columns": [
    {
      "header": "ID",
      "key": "id",
      "datatype": "int",
      "justification": "right",
      "summary": "count"
    },
    {
      "header": "Server Name",
      "key": "server_name",
      "datatype": "text",
      "justification": "left",
      "summary": "count"
    },
    {
      "header": "Category",
      "key": "category",
      "datatype": "text",
      "justification": "center",
      "summary": "unique"
    },
    {
      "header": "CPU Cores",
      "key": "cpu_cores",
      "datatype": "num",
      "justification": "right",
      "summary": "sum"
    }
  ]
}
EOF

echo "Test 7-A: Basic footer with footer and summaries"
echo "-----------------------------------------------"
"$tables_script" "$layout_file" "$data_file" $DEBUG_FLAG

# Test 7-B: Footer with long footer and wrapping description
cat > "$layout_file" << 'EOF'
{
  "theme": "Blue",
  "footer": "Detailed Server Performance and Configuration Analysis Report for Q2 2023",
  "columns": [
    {
      "header": "ID",
      "key": "id",
      "datatype": "int",
      "justification": "right"
    },
    {
      "header": "Server Name",
      "key": "server_name",
      "datatype": "text",
      "justification": "left"
    },
    {
      "header": "Description",
      "key": "description",
      "datatype": "text",
      "justification": "left",
      "width": 30,
      "wrap_mode": "wrap"
    },
    {
      "header": "Load Avg",
      "key": "load_avg",
      "datatype": "float",
      "justification": "right",
      "summary": "avg"
    }
  ]
}
EOF

echo -e "\nTest 7-B: Footer with long footer and wrapping description"
echo "--------------------------------------------------------"
"$tables_script" "$layout_file" "$data_file" $DEBUG_FLAG

# Test 7-C: Footer with multiple summary types and centered tags
cat > "$layout_file" << 'EOF'
{
  "theme": "Red",
  "footer": "Comprehensive Resource Utilization Dashboard",
  "footer_position": "center",
  "columns": [
    {
      "header": "Server",
      "key": "server_name",
      "datatype": "text",
      "justification": "left",
      "summary": "count"
    },
    {
      "header": "CPU Cores",
      "key": "cpu_cores",
      "datatype": "num",
      "justification": "right",
      "summary": "sum"
    },
    {
      "header": "Load Avg",
      "key": "load_avg",
      "datatype": "float",
      "justification": "right",
      "summary": "avg"
    },
    {
      "header": "CPU Usage",
      "key": "cpu_usage",
      "datatype": "kcpu",
      "justification": "right",
      "summary": "max"
    },
    {
      "header": "Tags",
      "key": "tags",
      "datatype": "text",
      "justification": "center",
      "width": 20,
      "wrap_mode": "wrap",
      "wrap_char": ","
    }
  ]
}
EOF

echo -e "\nTest 7-C: Footer with multiple summary types and centered tags"
echo "------------------------------------------------------------"
"$tables_script" "$layout_file" "$data_file" $DEBUG_FLAG

# Test 7-D: Footer with breaking by category and right-aligned footer
cat > "$layout_file" << 'EOF'
{
  "theme": "Blue",
  "footer": "Server Inventory by Category",
  "footer_position": "right",
  "columns": [
    {
      "header": "ID",
      "key": "id",
      "datatype": "int",
      "justification": "right",
      "summary": "count"
    },
    {
      "header": "Category",
      "key": "category",
      "datatype": "text",
      "justification": "left",
      "break": true,
      "summary": "unique"
    },
    {
      "header": "Server Name",
      "key": "server_name",
      "datatype": "text",
      "justification": "left"
    },
    {
      "header": "Status",
      "key": "status",
      "datatype": "text",
      "justification": "center"
    },
    {
      "header": "Memory Usage",
      "key": "memory_usage",
      "datatype": "kmem",
      "justification": "right",
      "summary": "sum"
    }
  ]
}
EOF

echo -e "\nTest 7-D: Footer with breaking by category and right-aligned footer"
echo "----------------------------------------------------------------"
"$tables_script" "$layout_file" "$data_file" $DEBUG_FLAG

# Test 7-E: Elaborate footer with full-width footer, summaries, and mixed justifications
cat > "$layout_file" << 'EOF'
{
  "theme": "Red",
  "footer": "Enterprise Server Management System - Detailed Analytics and Performance Metrics for Strategic Planning",
  "footer_position": "full",
  "columns": [
    {
      "header": "ID",
      "key": "id",
      "datatype": "int",
      "justification": "right",
      "summary": "min"
    },
    {
      "header": "Server Name",
      "key": "server_name",
      "datatype": "text",
      "justification": "left",
      "summary": "count"
    },
    {
      "header": "Category",
      "key": "category",
      "datatype": "text",
      "justification": "center",
      "summary": "unique"
    },
    {
      "header": "Description",
      "key": "description",
      "datatype": "text",
      "justification": "right",
      "width": 25,
      "wrap_mode": "wrap"
    },
    {
      "header": "CPU Cores",
      "key": "cpu_cores",
      "datatype": "num",
      "justification": "right",
      "summary": "avg"
    },
    {
      "header": "Load Avg",
      "key": "load_avg",
      "datatype": "float",
      "justification": "center",
      "summary": "max"
    }
  ]
}
EOF

echo -e "\nTest 7-E: Elaborate footer with full-width footer, summaries, and mixed justifications"
echo "----------------------------------------------------------------------------------"
"$tables_script" "$layout_file" "$data_file" $DEBUG_FLAG
