#!/usr/bin/env bash

# Create temporary files for our JSON
layout_file=$(mktemp)
data_file=$(mktemp)

# Setup the test data we'll use across all tests
cat > "$data_file" << 'EOF'
[
  {
    "namespace": "kube-system",
    "pod_name": "coredns-787d4b4b6c-abcd1",
    "cpu": "100m",
    "memory": "70Mi"
  },
  {
    "namespace": "kube-system",
    "pod_name": "coredns-787d4b4b6c-abcd2",
    "cpu": "100m",
    "memory": "70Mi"
  },
  {
    "namespace": "default",
    "pod_name": "nginx-deployment-66b6c48dd5-efgh1",
    "cpu": "750m",
    "memory": "768Mi"
  },
  {
    "namespace": "default",
    "pod_name": "nginx-deployment-66b6c48dd5-efgh2",
    "cpu": "750m",
    "memory": "768Mi"
  }
]
EOF

# Test 1: Basic - no title, no footer, no totals, two short columns MEM and CPU
cat > "$layout_file" << 'EOF'
{
  "theme": "Red",
  "columns": [
    {
      "header": "CPU",
      "key": "cpu",
      "datatype": "kcpu",
      "justification": "right"
    },
    {
      "header": "MEM",
      "key": "memory",
      "datatype": "kmem",
      "justification": "right"
    }
  ]
}
EOF

echo -e "\nTest 1: Basic - no title, no footer, no totals, two short columns"
echo "----------------------------------------"
./support_tables.sh "$layout_file" "$data_file"

# Test 2: Basic Totals - same as test 1 but with sum totals for the two columns
cat > "$layout_file" << 'EOF'
{
  "theme": "Red",
  "columns": [
    {
      "header": "CPU",
      "key": "cpu",
      "datatype": "kcpu",
      "justification": "right",
      "total": "sum"
    },
    {
      "header": "MEM",
      "key": "memory",
      "datatype": "kmem",
      "justification": "right",
      "total": "sum"
    }
  ]
}
EOF

echo -e "\nTest 2: Basic Totals - same as test 1 but with sum totals"
echo "----------------------------------------"
./support_tables.sh "$layout_file" "$data_file"

# Test 3: Basic Wide - same as Test 1 but with Pod Name as the first column
cat > "$layout_file" << 'EOF'
{
  "theme": "Red",
  "columns": [
    {
      "header": "Pod Name",
      "key": "pod_name",
      "datatype": "text"
    },
    {
      "header": "CPU",
      "key": "cpu",
      "datatype": "kcpu",
      "justification": "right"
    },
    {
      "header": "MEM",
      "key": "memory",
      "datatype": "kmem",
      "justification": "right"
    }
  ]
}
EOF

echo -e "\nTest 3: Basic Wide - same as Test 1 but with Pod Name as the first column"
echo "----------------------------------------"
./support_tables.sh "$layout_file" "$data_file"

# Test 4: Basic Wide Totals - Same as Test 2 but with count as the total for Pod Name
cat > "$layout_file" << 'EOF'
{
  "theme": "Red",
  "columns": [
    {
      "header": "Pod Name",
      "key": "pod_name",
      "datatype": "text",
      "total": "count"
    },
    {
      "header": "CPU",
      "key": "cpu",
      "datatype": "kcpu",
      "justification": "right",
      "total": "sum"
    },
    {
      "header": "MEM",
      "key": "memory",
      "datatype": "kmem",
      "justification": "right",
      "total": "sum"
    }
  ]
}
EOF

echo -e "\nTest 4: Basic Wide Totals - Same as Test 2 but with count as the total for Pod Name"
echo "----------------------------------------"
./support_tables.sh "$layout_file" "$data_file"

# Cleanup
rm -f "$layout_file" "$data_file"
