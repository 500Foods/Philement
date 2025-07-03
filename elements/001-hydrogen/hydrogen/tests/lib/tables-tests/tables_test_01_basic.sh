#!/usr/bin/env bash

# Test Suite 1: Basic - Various datatypes and justifications without headers, footers, or summaries
# This test suite focuses on demonstrating the core table drawing functionality
# with different datatypes (text, int, num, float, kcpu, kmem) and justifications (left, right, center)

# Create temporary files for our JSON
layout_file=$(mktemp)
data_file=$(mktemp)
tables_script="$(dirname "$0")/../tables.sh"

# Cleanup function
cleanup() {
    rm -f "$layout_file" "$data_file"
}
trap cleanup EXIT

# Setup comprehensive test data showcasing all datatypes
cat > "$data_file" << 'EOF'
[
  {
    "id": 1,
    "name": "web-server-01",
    "cpu_cores": 4,
    "load_avg": 2.45,
    "cpu_usage": "1250m",
    "memory_usage": "2048Mi",
    "status": "Running"
  },
  {
    "id": 2,
    "name": "db-server-01",
    "cpu_cores": 8,
    "load_avg": 5.12,
    "cpu_usage": "3200m",
    "memory_usage": "8192Mi", 
    "status": "Running"
  },
  {
    "id": 3,
    "name": "cache-server",
    "cpu_cores": 2,
    "load_avg": 0.85,
    "cpu_usage": "500m",
    "memory_usage": "1024Mi",
    "status": "Starting"
  },
  {
    "id": 4,
    "name": "api-gateway",
    "cpu_cores": 6,
    "load_avg": 3.21,
    "cpu_usage": "2100m",
    "memory_usage": "4096Mi",
    "status": "Running"
  }
]
EOF

# Test 1-A: Integer and Text datatypes with different justifications (Theme: Red)
cat > "$layout_file" << 'EOF'
{
  "theme": "Red",
  "columns": [
    {
      "header": "ID",
      "key": "id",
      "datatype": "int",
      "justification": "right"
    },
    {
      "header": "ID2",
      "key": "id",
      "datatype": "int",
      "justification": "right",
      "visible": false
    },
    {
      "header": "Server Name", 
      "key": "name",
      "datatype": "text",
      "justification": "left"
    },
    {
      "header": "Status",
      "key": "status", 
      "datatype": "text",
      "justification": "center"
    }
  ]
}
EOF

echo "Test 1-A: Integer and Text datatypes with different justifications"
echo "----------------------------------------------------------------"
"$tables_script" "$layout_file" "$data_file" 


# Test 1-B: Numeric datatypes - int, num, float (Theme: Blue)
cat > "$layout_file" << 'EOF'
{
  "theme": "Blue",
  "columns": [
    {
      "header": "ID",
      "key": "id", 
      "datatype": "int",
      "justification": "right"
    },
    {
      "header": "CPU Cores",
      "key": "cpu_cores",
      "datatype": "num", 
      "justification": "right"
    },
    {
      "header": "Load Average",
      "key": "load_avg",
      "datatype": "float",
      "justification": "right"
    }
  ]
}
EOF

echo -e "\nTest 1-B: Numeric datatypes - int, num, float"
echo "---------------------------------------------"
"$tables_script" "$layout_file" "$data_file"

# Test 1-C: Kubernetes resource datatypes - kcpu and kmem (Theme: Red)
cat > "$layout_file" << 'EOF'
{
  "theme": "Red", 
  "columns": [
    {
      "header": "Server",
      "key": "name",
      "datatype": "text",
      "justification": "left"
    },
    {
      "header": "CPU Usage",
      "key": "cpu_usage", 
      "datatype": "kcpu",
      "justification": "right"
    },
    {
      "header": "Memory Usage", 
      "key": "memory_usage",
      "datatype": "kmem",
      "justification": "right"
    }
  ]
}
EOF

echo -e "\nTest 1-C: Kubernetes resource datatypes - kcpu and kmem"
echo "-------------------------------------------------------"
"$tables_script" "$layout_file" "$data_file"

# Test 1-D: Mixed datatypes with center justification focus (Theme: Blue)
cat > "$layout_file" << 'EOF'
{
  "theme": "Blue",
  "columns": [
    {
      "header": "ID",
      "key": "id",
      "datatype": "int", 
      "justification": "center"
    },
    {
      "header": "Status",
      "key": "status",
      "datatype": "text",
      "justification": "center"
    },
    {
      "header": "Load",
      "key": "load_avg", 
      "datatype": "float",
      "justification": "center"
    }
  ]
}
EOF

echo -e "\nTest 1-D: Mixed datatypes with center justification focus"
echo "---------------------------------------------------------"
"$tables_script" "$layout_file" "$data_file"

# Test 1-E: All datatypes in single table (Theme: Red)
cat > "$layout_file" << 'EOF'
{
  "theme": "Red",
  "columns": [
    {
      "header": "ID",
      "key": "id",
      "datatype": "int",
      "justification": "right"
    },
    {
      "header": "Name",
      "key": "name", 
      "datatype": "text",
      "justification": "left"
    },
    {
      "header": "Cores",
      "key": "cpu_cores",
      "datatype": "num",
      "justification": "right"
    },
    {
      "header": "Load",
      "key": "load_avg",
      "datatype": "float", 
      "justification": "right"
    },
    {
      "header": "CPU",
      "key": "cpu_usage",
      "datatype": "kcpu",
      "justification": "right"
    },
    {
      "header": "Memory", 
      "key": "memory_usage",
      "datatype": "kmem",
      "justification": "right"
    }
  ]
}
EOF

echo -e "\nTest 1-E: All datatypes in single table"
echo "---------------------------------------"
"$tables_script" "$layout_file" "$data_file"

# Test 1-F: Text datatype with different justifications (Theme: Blue)
cat > "$layout_file" << 'EOF'
{
  "theme": "Blue",
  "columns": [
    {
      "header": "Left Text",
      "key": "name",
      "datatype": "text",
      "justification": "left"
    },
    {
      "header": "Center Text",
      "key": "status", 
      "datatype": "text",
      "justification": "center"
    },
    {
      "header": "Right Text",
      "key": "name",
      "datatype": "text",
      "justification": "right"
    }
  ]
}
EOF

echo -e "\nTest 1-F: Text datatype with different justifications"
echo "-----------------------------------------------------"
"$tables_script" "$layout_file" "$data_file"
