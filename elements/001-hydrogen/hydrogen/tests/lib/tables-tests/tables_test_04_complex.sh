#!/usr/bin/env bash

# Test Suite 4: Complex - Combining summaries, wrapping, breaking, and advanced formatting
# This test suite focuses on demonstrating the full power of the table rendering system
# by combining elements of summaries, wrapping, breaking, and various justifications
# to create complex and visually appealing tables.

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
    "description": "Primary web server for frontend applications with a very long description to test wrapping functionality. This description is extended to ensure that it will wrap across multiple lines when a width constraint is applied in the test configuration. Additional text is added here for thorough testing.",
    "tags": "frontend,app,ui,primary,loadbalancer,highavailability"
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
    "description": "Main database server handling critical data storage and retrieval operations.",
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
    "description": "In-memory cache for speeding up data access in applications.",
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
    "description": "API gateway managing incoming requests and routing to appropriate services with detailed logging and monitoring capabilities enabled for performance tracking.",
    "tags": "api,gateway,routing,web,interface,management"
  }
]
EOF

# Test 4-A: Complex table with summaries, wrapping, and breaking by category
cat > "$layout_file" << 'EOF'
{
  "theme": "Red",
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
      "break": true,
      "summary": "unique"
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
      "header": "Description",
      "key": "description",
      "datatype": "text",
      "justification": "left",
      "width": 50,
      "wrap_mode": "wrap"
    }
  ]
}
EOF

echo "Test 4-A: Complex table with summaries, wrapping, and breaking by category"
echo "------------------------------------------------------------------------"
"$tables_script" "$layout_file" "$data_file" $DEBUG_FLAG

# Test 4-B: Resource-focused table with Kubernetes summaries and delimiter wrapping
cat > "$layout_file" << 'EOF'
{
  "theme": "Blue",
  "columns": [
    {
      "header": "Server",
      "key": "server_name",
      "datatype": "text",
      "justification": "left",
      "summary": "count"
    },
    {
      "header": "Status",
      "key": "status",
      "datatype": "text",
      "justification": "center",
      "summary": "unique"
    },
    {
      "header": "CPU Usage",
      "key": "cpu_usage",
      "datatype": "kcpu",
      "justification": "right",
      "summary": "sum"
    },
    {
      "header": "Memory Usage",
      "key": "memory_usage",
      "datatype": "kmem",
      "justification": "right",
      "summary": "sum"
    },
    {
      "header": "Tags",
      "key": "tags",
      "datatype": "text",
      "justification": "left",
      "width": 25,
      "wrap_mode": "wrap",
      "wrap_char": ","
    }
  ]
}
EOF

echo -e "\nTest 4-B: Resource-focused table with Kubernetes summaries and delimiter wrapping"
echo "------------------------------------------------------------------------------"
"$tables_script" "$layout_file" "$data_file" $DEBUG_FLAG

# Test 4-C: Mixed justification with summaries and centered wrapping
cat > "$layout_file" << 'EOF'
{
  "theme": "Red",
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
      "header": "Load Average",
      "key": "load_avg",
      "datatype": "float",
      "justification": "right",
      "summary": "max"
    },
    {
      "header": "Description",
      "key": "description",
      "datatype": "text",
      "justification": "center",
      "width": 40,
      "wrap_mode": "wrap"
    },
    {
      "header": "CPU Cores",
      "key": "cpu_cores",
      "datatype": "num",
      "justification": "right",
      "summary": "avg"
    }
  ]
}
EOF

echo -e "\nTest 4-C: Mixed justification with summaries and centered wrapping"
echo "----------------------------------------------------------------"
"$tables_script" "$layout_file" "$data_file" $DEBUG_FLAG

# Test 4-D: Full-featured table with all summary types and right-aligned tags wrapping
cat > "$layout_file" << 'EOF'
{
  "theme": "Blue",
  "columns": [
    {
      "header": "Sum CPU",
      "key": "cpu_cores",
      "datatype": "int",
      "justification": "right",
      "summary": "sum"
    },
    {
      "header": "Min Load",
      "key": "load_avg",
      "datatype": "float",
      "justification": "right",
      "summary": "min"
    },
    {
      "header": "Max Load",
      "key": "load_avg",
      "datatype": "float",
      "justification": "right",
      "summary": "max"
    },
    {
      "header": "Avg CPU",
      "key": "cpu_cores",
      "datatype": "int",
      "justification": "right",
      "summary": "avg"
    },
    {
      "header": "Count",
      "key": "id",
      "datatype": "int",
      "justification": "right",
      "summary": "count"
    },
    {
      "header": "Unique Cat",
      "key": "category",
      "datatype": "text",
      "justification": "center",
      "summary": "unique"
    },
    {
      "header": "Tags",
      "key": "tags",
      "datatype": "text",
      "justification": "right",
      "width": 20,
      "wrap_mode": "wrap",
      "wrap_char": ","
    }
  ]
}
EOF

echo -e "\nTest 4-D: Full-featured table with all summary types and right-aligned tags wrapping"
echo "---------------------------------------------------------------------------------"
"$tables_script" "$layout_file" "$data_file" $DEBUG_FLAG

# Test 4-E: Elegant table with breaking, wrapping, and minimal summaries
cat > "$layout_file" << 'EOF'
{
  "theme": "Red",
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
      "break": true
    },
    {
      "header": "Server",
      "key": "server_name",
      "datatype": "text",
      "justification": "left"
    },
    {
      "header": "Status",
      "key": "status",
      "datatype": "text",
      "justification": "center",
      "summary": "unique"
    },
    {
      "header": "Description",
      "key": "description",
      "datatype": "text",
      "justification": "left",
      "width": 60,
      "wrap_mode": "wrap"
    },
    {
      "header": "CPU",
      "key": "cpu_cores",
      "datatype": "num",
      "justification": "right",
      "summary": "sum"
    }
  ]
}
EOF

echo -e "\nTest 4-E: Elegant table with breaking, wrapping, and minimal summaries"
echo "--------------------------------------------------------------------"
"$tables_script" "$layout_file" "$data_file" $DEBUG_FLAG
