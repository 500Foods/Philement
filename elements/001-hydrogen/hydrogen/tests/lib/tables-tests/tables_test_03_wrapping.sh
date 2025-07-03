#!/usr/bin/env bash

# Test Suite 3: Wrapping - String column width handling, truncation, wrapping, and breaking
# This test suite focuses on demonstrating the handling of string columns with different width settings,
# truncation, wrapping (delimiter-based and regular text), and the break feature for grouping.

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

# Setup test data with string columns for wrapping and breaking tests
cat > "$data_file" << 'EOF'
[
  {
    "id": 1,
    "category": "Web",
    "description": "Primary",
    "tags": "frontend,app,ui"
  },
  {
    "id": 2,
    "category": "Web",
    "description": "Secondary web server with a very long description to test wrapping functionality. This description is extended to be over 125 characters to ensure that it will wrap when a width of 75 is set in the test. Additional text is added here to meet the length requirement for thorough testing of wrapping behavior.",
    "tags": "frontend,backup,loadbalancer,highavailability,performance"
  },
  {
    "id": 3,
    "category": "Database",
    "description": "Main database server",
    "tags": "db,sql,storage"
  },
  {
    "id": 4,
    "category": "Database",
    "description": "Replica database",
    "tags": "db,replica,read"
  },
  {
    "id": 5,
    "category": "Cache",
    "description": "In-memory cache",
    "tags": "cache,redis,fast"
  }
]
EOF

# Test 3-A: No width or wrap, full string display
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
      "header": "Category",
      "key": "category",
      "datatype": "text",
      "justification": "left"
    },
    {
      "header": "Description",
      "key": "description",
      "datatype": "text",
      "justification": "left"
    }
  ]
}
EOF

echo "Test 3-A: No width or wrap, full string display"
echo "-----------------------------------------------"
"$tables_script" "$layout_file" "$data_file" $DEBUG_FLAG

# Test 3-B: Width set, no wrap, left justification clipping on right (width less than title)
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
      "header": "Category",
      "key": "category",
      "datatype": "text",
      "justification": "left"
    },
    {
      "header": "Description",
      "key": "description",
      "datatype": "text",
      "justification": "left",
      "width": 10
    }
  ]
}
EOF

echo -e "\nTest 3-B: Width set, no wrap, left justification clipping on right"
echo "------------------------------------------------------------------"
"$tables_script" "$layout_file" "$data_file" $DEBUG_FLAG

# Test 3-C: Width set, no wrap, right justification clipping on left (width equal to title)
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
      "header": "Category",
      "key": "category",
      "datatype": "text",
      "justification": "left"
    },
    {
      "header": "Description",
      "key": "description",
      "datatype": "text",
      "justification": "right",
      "width": 11
    }
  ]
}
EOF

echo -e "\nTest 3-C: Width set, no wrap, right justification clipping on left"
echo "------------------------------------------------------------------"
"$tables_script" "$layout_file" "$data_file" $DEBUG_FLAG

# Test 3-D: Width set, no wrap, center justification clipping on both sides (width greater than title)
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
      "header": "Category",
      "key": "category",
      "datatype": "text",
      "justification": "left"
    },
    {
      "header": "Description",
      "key": "description",
      "datatype": "text",
      "justification": "center",
      "width": 15
    }
  ]
}
EOF

echo -e "\nTest 3-D: Width set, no wrap, center justification clipping on both sides"
echo "------------------------------------------------------------------------"
"$tables_script" "$layout_file" "$data_file" $DEBUG_FLAG

# Test 3-E: Delimiter-based wrapping with comma delimiter
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
      "header": "Category",
      "key": "category",
      "datatype": "text",
      "justification": "left"
    },
    {
      "header": "Tags",
      "key": "tags",
      "datatype": "text",
      "justification": "left",
      "width": 15,
      "wrap_mode": "wrap",
      "wrap_char": ","
    }
  ]
}
EOF

echo -e "\nTest 3-E: Delimiter-based wrapping with comma"
echo "---------------------------------------------"
"$tables_script" "$layout_file" "$data_file" $DEBUG_FLAG

# Test 3-F: Wrap mode with width, no delimiter, word wrapping preferred
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
      "header": "Category",
      "key": "category",
      "datatype": "text",
      "justification": "left"
    },
    {
      "header": "Description",
      "key": "description",
      "datatype": "text",
      "justification": "left",
      "width": 20,
      "wrap_mode": "wrap"
    }
  ]
}
EOF

echo -e "\nTest 3-F: Wrap mode with width, no delimiter, word wrapping preferred"
echo "--------------------------------------------------------------------"
"$tables_script" "$layout_file" "$data_file" $DEBUG_FLAG

# Test 3-G: Break feature based on category grouping
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
      "header": "Category",
      "key": "category",
      "datatype": "text",
      "justification": "left",
      "break": true
    },
    {
      "header": "Description",
      "key": "description",
      "datatype": "text",
      "justification": "left",
      "width": 20
    }
  ]
}
EOF

echo -e "\nTest 3-G: Break feature based on category grouping"
echo "--------------------------------------------------"
"$tables_script" "$layout_file" "$data_file" $DEBUG_FLAG

# Test 3-H: Combined break, width clipping, and delimiter wrapping
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
      "header": "Category",
      "key": "category",
      "datatype": "text",
      "justification": "left",
      "break": true
    },
    {
      "header": "Description",
      "key": "description",
      "datatype": "text",
      "justification": "left",
      "width": 15
    },
    {
      "header": "Tags",
      "key": "tags",
      "datatype": "text",
      "justification": "left",
      "width": 10,
      "wrap_mode": "wrap",
      "wrap_char": ","
    }
  ]
}
EOF

echo -e "\nTest 3-H: Combined break, width clipping, and delimiter wrapping"
echo "---------------------------------------------------------------"
"$tables_script" "$layout_file" "$data_file" $DEBUG_FLAG

# Test 3-I: Delimiter-based wrapping with comma delimiter, right alignment
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
      "header": "Category",
      "key": "category",
      "datatype": "text",
      "justification": "left"
    },
    {
      "header": "Tags",
      "key": "tags",
      "datatype": "text",
      "justification": "right",
      "width": 15,
      "wrap_mode": "wrap",
      "wrap_char": ","
    }
  ]
}
EOF

echo -e "\nTest 3-I: Delimiter-based wrapping with comma, right alignment"
echo "-------------------------------------------------------------"
"$tables_script" "$layout_file" "$data_file" $DEBUG_FLAG

# Test 3-J: Delimiter-based wrapping with comma delimiter, center alignment
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
      "header": "Category",
      "key": "category",
      "datatype": "text",
      "justification": "left"
    },
    {
      "header": "Tags",
      "key": "tags",
      "datatype": "text",
      "justification": "center",
      "width": 15,
      "wrap_mode": "wrap",
      "wrap_char": ","
    }
  ]
}
EOF

echo -e "\nTest 3-J: Delimiter-based wrapping with comma, center alignment"
echo "--------------------------------------------------------------"
"$tables_script" "$layout_file" "$data_file" $DEBUG_FLAG

# Test 3-K: No width or wrap for full display, but with wrapping and width of 75 for Description
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
      "header": "Category",
      "key": "category",
      "datatype": "text",
      "justification": "left"
    },
    {
      "header": "Description",
      "key": "description",
      "datatype": "text",
      "justification": "left",
      "width": 75,
      "wrap_mode": "wrap"
    }
  ]
}
EOF

echo -e "\nTest 3-K: Full string display with wrapping, width 75 for Description"
echo "--------------------------------------------------------------------"
"$tables_script" "$layout_file" "$data_file" $DEBUG_FLAG
