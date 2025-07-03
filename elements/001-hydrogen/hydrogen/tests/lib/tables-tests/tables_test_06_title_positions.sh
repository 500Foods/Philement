#!/usr/bin/env bash

# Test Suite 6: Title Positions - Validation of table header positions
# This test suite focuses on validating the rendering of table headers in different positions
# (not supplied, left, center, right) with headers of varying lengths relative to table width
# (less than, equal to, greater than table width).

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

# Setup test data with consistent server information
cat > "$data_file" << 'EOF'
[
  {
    "id": 1,
    "server_name": "web-server-01",
    "cpu_cores": 4,
    "load_avg": 2.45
  },
  {
    "id": 2,
    "server_name": "db-server-01",
    "cpu_cores": 8,
    "load_avg": 5.12
  },
  {
    "id": 3,
    "server_name": "cache-server",
    "cpu_cores": 2,
    "load_avg": 0.85
  },
  {
    "id": 4,
    "server_name": "api-gateway",
    "cpu_cores": 6,
    "load_avg": 3.21
  }
]
EOF

# Test 6-A: Not Supplied - Header less than table width
cat > "$layout_file" << 'EOF'
{
  "theme": "Red",
  "title": "Server Report",
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
      "header": "CPU Cores",
      "key": "cpu_cores",
      "datatype": "num",
      "justification": "right"
    },
    {
      "header": "Load Avg",
      "key": "load_avg",
      "datatype": "float",
      "justification": "right"
    }
  ]
}
EOF

echo "Test 6-A: Not Supplied - Header less than table width"
echo "----------------------------------------------------"
"$tables_script" "$layout_file" "$data_file" $DEBUG_FLAG

# Test 6-B: Not Supplied - Header equal to table width
cat > "$layout_file" << 'EOF'
{
  "theme": "Red",
  "title": "Server Performance Metrics Report Data 23",
  "columns": [
    {
      "header": "ID",
      "key": "id",
      "datatype": "int",
      "justification": "right",
      "width": 5
    },
    {
      "header": "Server Name",
      "key": "server_name",
      "datatype": "text",
      "justification": "left",
      "width": 15
    },
    {
      "header": "CPU Cores",
      "key": "cpu_cores",
      "datatype": "num",
      "justification": "right",
      "width": 10
    },
    {
      "header": "Load Avg",
      "key": "load_avg",
      "datatype": "float",
      "justification": "right",
      "width": 10
    }
  ]
}
EOF

echo -e "\nTest 6-B: Not Supplied - Header equal to table width"
echo "---------------------------------------------------"
"$tables_script" "$layout_file" "$data_file" $DEBUG_FLAG

# Test 6-C: Not Supplied - Header greater than table width
cat > "$layout_file" << 'EOF'
{
  "theme": "Red",
  "title": "Detailed Server Performance and Configuration Report for Q2 2023",
  "columns": [
    {
      "header": "ID Number",
      "key": "id",
      "datatype": "int",
      "justification": "right",
      "width": 3
    },
    {
      "header": "Server Name Identifier",
      "key": "server_name",
      "datatype": "text",
      "justification": "left",
      "width": 10
    },
    {
      "header": "CPU Cores Count",
      "key": "cpu_cores",
      "datatype": "num",
      "justification": "right",
      "width": 5
    },
    {
      "header": "Load Average Value",
      "key": "load_avg",
      "datatype": "float",
      "justification": "right",
      "width": 5
    }
  ]
}
EOF

echo -e "\nTest 6-C: Not Supplied - Header greater than table width"
echo "-------------------------------------------------------"
"$tables_script" "$layout_file" "$data_file" $DEBUG_FLAG

# Test 6-D: Left - Header less than table width
cat > "$layout_file" << 'EOF'
{
  "theme": "Blue",
  "title": "Server Report",
  "title_position": "left",
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
      "header": "CPU Cores",
      "key": "cpu_cores",
      "datatype": "num",
      "justification": "right"
    },
    {
      "header": "Load Avg",
      "key": "load_avg",
      "datatype": "float",
      "justification": "right"
    }
  ]
}
EOF

echo -e "\nTest 6-D: Left - Header less than table width"
echo "--------------------------------------------"
"$tables_script" "$layout_file" "$data_file" $DEBUG_FLAG

# Test 6-E: Left - Header equal to table width
cat > "$layout_file" << 'EOF'
{
  "theme": "Blue",
  "title": "Server Performance Metrics Report Data 23",
  "title_position": "left",
  "columns": [
    {
      "header": "ID",
      "key": "id",
      "datatype": "int",
      "justification": "right",
      "width": 5
    },
    {
      "header": "Server Name",
      "key": "server_name",
      "datatype": "text",
      "justification": "left",
      "width": 15
    },
    {
      "header": "CPU Cores",
      "key": "cpu_cores",
      "datatype": "num",
      "justification": "right",
      "width": 10
    },
    {
      "header": "Load Avg",
      "key": "load_avg",
      "datatype": "float",
      "justification": "right",
      "width": 10
    }
  ]
}
EOF

echo -e "\nTest 6-E: Left - Header equal to table width"
echo "-------------------------------------------"
"$tables_script" "$layout_file" "$data_file" $DEBUG_FLAG

# Test 6-F: Left - Header greater than table width
cat > "$layout_file" << 'EOF'
{
  "theme": "Blue",
  "title": "Detailed Server Performance and Configuration Analysis Report for Q2 2023",
  "title_position": "left",
  "columns": [
    {
      "header": "ID",
      "key": "id",
      "datatype": "int",
      "justification": "right",
      "width": 3
    },
    {
      "header": "Server Name",
      "key": "server_name",
      "datatype": "text",
      "justification": "left",
      "width": 10
    },
    {
      "header": "CPU Cores",
      "key": "cpu_cores",
      "datatype": "num",
      "justification": "right",
      "width": 5
    },
    {
      "header": "Load Avg",
      "key": "load_avg",
      "datatype": "float",
      "justification": "right",
      "width": 5
    }
  ]
}
EOF

echo -e "\nTest 6-F: Left - Header greater than table width"
echo "-----------------------------------------------"
"$tables_script" "$layout_file" "$data_file" $DEBUG_FLAG

# Test 6-G: Center - Header less than table width
cat > "$layout_file" << 'EOF'
{
  "theme": "Red",
  "title": "Server Report",
  "title_position": "center",
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
      "header": "CPU Cores",
      "key": "cpu_cores",
      "datatype": "num",
      "justification": "right"
    },
    {
      "header": "Load Avg",
      "key": "load_avg",
      "datatype": "float",
      "justification": "right"
    }
  ]
}
EOF

echo -e "\nTest 6-G: Center - Header less than table width"
echo "----------------------------------------------"
"$tables_script" "$layout_file" "$data_file" $DEBUG_FLAG

# Test 6-H: Center - Header equal to table width
cat > "$layout_file" << 'EOF'
{
  "theme": "Red",
  "title": "Server Performance Metrics Report Data 23",
  "title_position": "center",
  "columns": [
    {
      "header": "ID",
      "key": "id",
      "datatype": "int",
      "justification": "right",
      "width": 5
    },
    {
      "header": "Server Name",
      "key": "server_name",
      "datatype": "text",
      "justification": "left",
      "width": 15
    },
    {
      "header": "CPU Cores",
      "key": "cpu_cores",
      "datatype": "num",
      "justification": "right",
      "width": 10
    },
    {
      "header": "Load Avg",
      "key": "load_avg",
      "datatype": "float",
      "justification": "right",
      "width": 10
    }
  ]
}
EOF

echo -e "\nTest 6-H: Center - Header equal to table width"
echo "---------------------------------------------"
"$tables_script" "$layout_file" "$data_file" $DEBUG_FLAG

# Test 6-I: Center - Header greater than table width
cat > "$layout_file" << 'EOF'
{
  "theme": "Red",
  "title": "Detailed Server Performance and Configuration Analysis Report for Q2 2023",
  "title_position": "center",
  "columns": [
    {
      "header": "ID",
      "key": "id",
      "datatype": "int",
      "justification": "right",
      "width": 3
    },
    {
      "header": "Server Name",
      "key": "server_name",
      "datatype": "text",
      "justification": "left",
      "width": 10
    },
    {
      "header": "CPU Cores",
      "key": "cpu_cores",
      "datatype": "num",
      "justification": "right",
      "width": 5
    },
    {
      "header": "Load Avg",
      "key": "load_avg",
      "datatype": "float",
      "justification": "right",
      "width": 5
    }
  ]
}
EOF

echo -e "\nTest 6-I: Center - Header greater than table width"
echo "-------------------------------------------------"
"$tables_script" "$layout_file" "$data_file" $DEBUG_FLAG

# Test 6-J: Right - Header less than table width
cat > "$layout_file" << 'EOF'
{
  "theme": "Blue",
  "title": "Server Report",
  "title_position": "right",
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
      "header": "CPU Cores",
      "key": "cpu_cores",
      "datatype": "num",
      "justification": "right"
    },
    {
      "header": "Load Avg",
      "key": "load_avg",
      "datatype": "float",
      "justification": "right"
    }
  ]
}
EOF

echo -e "\nTest 6-J: Right - Header less than table width"
echo "---------------------------------------------"
"$tables_script" "$layout_file" "$data_file" $DEBUG_FLAG

# Test 6-K: Right - Header equal to table width
cat > "$layout_file" << 'EOF'
{
  "theme": "Blue",
  "title": "Server Performance Metrics Report Data 23",
  "title_position": "right",
  "columns": [
    {
      "header": "ID",
      "key": "id",
      "datatype": "int",
      "justification": "right",
      "width": 5
    },
    {
      "header": "Server Name",
      "key": "server_name",
      "datatype": "text",
      "justification": "left",
      "width": 15
    },
    {
      "header": "CPU Cores",
      "key": "cpu_cores",
      "datatype": "num",
      "justification": "right",
      "width": 10
    },
    {
      "header": "Load Avg",
      "key": "load_avg",
      "datatype": "float",
      "justification": "right",
      "width": 10
    }
  ]
}
EOF

echo -e "\nTest 6-K: Right - Header equal to table width"
echo "--------------------------------------------"
"$tables_script" "$layout_file" "$data_file" $DEBUG_FLAG

# Test 6-L: Right - Header greater than table width
cat > "$layout_file" << 'EOF'
{
  "theme": "Blue",
  "title": "Detailed Server Performance and Configuration Analysis Report for Q2 2023",
  "title_position": "right",
  "columns": [
    {
      "header": "ID",
      "key": "id",
      "datatype": "int",
      "justification": "right",
      "width": 3
    },
    {
      "header": "Server Name",
      "key": "server_name",
      "datatype": "text",
      "justification": "left",
      "width": 10
    },
    {
      "header": "CPU Cores",
      "key": "cpu_cores",
      "datatype": "num",
      "justification": "right",
      "width": 5
    },
    {
      "header": "Load Avg",
      "key": "load_avg",
      "datatype": "float",
      "justification": "right"
    }
  ]
}
EOF

echo -e "\nTest 6-L: Right - Header greater than table width"
echo "------------------------------------------------"
"$tables_script" "$layout_file" "$data_file" $DEBUG_FLAG
