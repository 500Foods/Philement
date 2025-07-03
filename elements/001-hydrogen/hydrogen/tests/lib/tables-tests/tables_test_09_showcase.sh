#!/usr/bin/env bash

# Test Suite 9: Showcase - Ultimate demonstration of tables.sh capabilities
# Refactored version with reduced line count while maintaining full functionality

# Setup
layout_file=$(mktemp)
data_file=$(mktemp)
tables_script="$(dirname "$0")/../tables.sh"
cleanup() { rm -f "$layout_file" "$data_file"; }
trap cleanup EXIT

DEBUG_FLAG=""
[[ "$1" == "--debug" ]] && DEBUG_FLAG="--debug" && echo "Debug mode enabled"

# Create test data once
cat > "$data_file" << 'EOF'
[
  {"id": 1, "server_name": "web-server-01", "category": "Web", "cpu_cores": 4, "load_avg": 2.45, "cpu_usage": "1250m", "memory_usage": "2048Mi", "status": "Running", "description": "Primary web server for frontend applications with a detailed setup for high availability.", "tags": "frontend,app,ui,primary,loadbalancer", "location": "US-East", "uptime_days": 120, "bandwidth_mbps": 1000.5},
  {"id": 2, "server_name": "db-server-01", "category": "Database", "cpu_cores": 8, "load_avg": 5.12, "cpu_usage": "3200m", "memory_usage": "8192Mi", "status": "Running", "description": "Main database server handling critical data operations with replication enabled.", "tags": "db,sql,storage,primary,backend", "location": "US-West", "uptime_days": 180, "bandwidth_mbps": 500.25},
  {"id": 3, "server_name": "cache-server", "category": "Cache", "cpu_cores": 2, "load_avg": 0.85, "cpu_usage": "500m", "memory_usage": "1024Mi", "status": "Starting", "description": "In-memory cache for speeding up data access across multiple services.", "tags": "cache,redis,fast,memory", "location": "EU-Central", "uptime_days": 10, "bandwidth_mbps": 200.75},
  {"id": 4, "server_name": "api-gateway", "category": "Web", "cpu_cores": 6, "load_avg": 3.21, "cpu_usage": "2100m", "memory_usage": "4096Mi", "status": "Running", "description": "API gateway managing incoming requests and routing to appropriate services.", "tags": "api,gateway,routing,web,interface", "location": "US-East", "uptime_days": 90, "bandwidth_mbps": 750.0},
  {"id": 5, "server_name": "backup-server", "category": "Storage", "cpu_cores": 4, "load_avg": 1.15, "cpu_usage": "800m", "memory_usage": "4096Mi", "status": "Running", "description": "Dedicated backup server for periodic data snapshots and recovery.", "tags": "backup,storage,recovery,secondary", "location": "US-West", "uptime_days": 200, "bandwidth_mbps": 300.0}
]
EOF

# Function to create layout and run test
run_test() {
    local test_id="$1" theme="$2" title="$3" title_pos="$4" footer="$5" footer_pos="$6"
    shift 6
    local columns="$*"
    
    cat > "$layout_file" << EOF
{"theme": "$theme", "title": "$title", "title_position": "$title_pos", "footer": "$footer", "footer_position": "$footer_pos", "columns": [$columns]}
EOF
    
    echo -e "\nTest 9-$test_id: $(echo "$title" | cut -c1-50)..."
    printf '%.0s-' {1..60}
    echo
    "$tables_script" "$layout_file" "$data_file" $DEBUG_FLAG
}

# Column definitions (reusable)
col_id='{"header": "ID", "key": "id", "datatype": "int", "justification": "right"}'
col_id_sum='{"header": "ID", "key": "id", "datatype": "int", "justification": "right", "summary": "count"}'
col_id_break='{"header": "ID", "key": "id", "datatype": "int", "justification": "right", "summary": "count", "break": true}'
col_server='{"header": "Server Name", "key": "server_name", "datatype": "text", "justification": "left"}'
col_server_sum='{"header": "Server Name", "key": "server_name", "datatype": "text", "justification": "left", "summary": "count"}'
col_category_sum='{"header": "Category", "key": "category", "datatype": "text", "justification": "center", "summary": "unique"}'
col_category_break='{"header": "Category", "key": "category", "datatype": "text", "justification": "left", "break": true, "summary": "unique"}'
col_cpu='{"header": "CPU Cores", "key": "cpu_cores", "datatype": "num", "justification": "right", "summary": "sum"}'
col_load='{"header": "Load Avg", "key": "load_avg", "datatype": "float", "justification": "right", "summary": "avg"}'
col_cpu_usage='{"header": "CPU Usage", "key": "cpu_usage", "datatype": "kcpu", "justification": "right", "summary": "max"}'
col_memory='{"header": "Memory Usage", "key": "memory_usage", "datatype": "kmem", "justification": "right", "summary": "sum"}'
col_status='{"header": "Status", "key": "status", "datatype": "text", "justification": "center"}'
col_desc_wrap='{"header": "Description", "key": "description", "datatype": "text", "justification": "left", "width": 30, "wrap_mode": "wrap"}'
col_tags_wrap='{"header": "Tags", "key": "tags", "datatype": "text", "justification": "center", "width": 20, "wrap_mode": "wrap", "wrap_char": ","}'
col_location='{"header": "Location", "key": "location", "datatype": "text", "justification": "center", "summary": "unique"}'
col_location_break='{"header": "Location", "key": "location", "datatype": "text", "justification": "left", "break": true, "summary": "unique"}'
col_uptime='{"header": "Uptime (Days)", "key": "uptime_days", "datatype": "int", "justification": "right", "summary": "avg"}'
col_bandwidth='{"header": "Bandwidth (Mbps)", "key": "bandwidth_mbps", "datatype": "float", "justification": "right", "summary": "sum"}'

# Pre-calculate server count for dynamic titles
server_count=$(jq length "$data_file")

# Run all tests with compact definitions
run_test "A" "Red" "Server Overview Report - Total Servers: $server_count" "center" "Generated on $(date +%Y-%m-%d)" "center" "$col_id_sum, $col_server_sum, $col_category_sum, $col_cpu"

run_test "B" "Blue" "Server Performance Report Q2 2023" "left" "Data Compiled by IT Department" "right" "$col_id, $col_server, $col_desc_wrap, $col_load"

run_test "C" "Red" "Comprehensive Resource Utilization Dashboard for Enterprise Systems Monitoring" "full" "Data Accurate as of $(date '+%Y-%m-%d %H:%M:%S')" "center" "$col_server_sum, $col_cpu, $col_load, $col_cpu_usage, $col_tags_wrap"

run_test "D" "Blue" "Server Inventory by Category" "right" "Report Generated for Internal Use Only" "right" "$col_id_sum, $col_category_break, $col_server, $col_status, $col_memory"

run_test "E" "Red" "Enterprise Server Management System" "center" "Detailed Analytics and Performance Metrics for Strategic Planning - IT Dept. $(date +%Y)" "full" '{"header": "ID", "key": "id", "datatype": "int", "justification": "right", "summary": "min"}, '"$col_server_sum"', '"$col_category_sum"', {"header": "Description", "key": "description", "datatype": "text", "justification": "right", "width": 25, "wrap_mode": "wrap"}, {"header": "CPU Cores", "key": "cpu_cores", "datatype": "num", "justification": "right", "summary": "avg"}, {"header": "Load Avg", "key": "load_avg", "datatype": "float", "justification": "center", "summary": "max"}'

run_test "F" "Blue" "Network Bandwidth Analysis" "left" "Data Source: Network Monitoring System" "left" "$col_id_sum, $col_server, $col_bandwidth, $col_location"

run_test "G" "Red" "Server Uptime Report" "center" "Uptime Data as of Today" "center" "$col_server_sum, $col_uptime, $col_status, $col_category_sum"

run_test "H" "Blue" "Detailed Server Specifications" "full" "Specifications Compiled for Review" "right" "$col_id_sum, $col_server, $col_cpu, $col_memory, $col_desc_wrap"

run_test "I" "Red" "Server Distribution by Location" "center" "Location Data for Infrastructure Planning" "center" "$col_id_sum, $col_location_break, $col_server, $col_cpu, $col_load"

run_test "J" "Blue" "Server Status Report" "center" "Status Updated Hourly" "right" "$col_id_sum, $col_server, $col_status, $col_uptime"

run_test "K" "Red" "Performance Metrics Dashboard" "left" "Performance Data for System Optimization and Capacity Planning - IT Operations Team" "full" "$col_server_sum, $col_cpu_usage, $col_load, $col_bandwidth"

run_test "L" "Blue" "Server Category Breakdown" "left" "Category Analysis for Resource Allocation" "left" "$col_id_sum, $col_category_break, $col_server, $col_cpu, $col_memory"

run_test "M" "Red" "Resource Utilization Overview" "right" "Resource Data for Capacity Planning" "center" "$col_server_sum, $col_cpu, $col_cpu_usage, $col_memory"

run_test "N" "Blue" "Network Bandwidth Utilization Report for All Data Centers" "full" "Bandwidth Metrics for Network Optimization" "left" "$col_id_sum, $col_server, $col_location, $col_bandwidth"

run_test "O" "Red" "Server Descriptions Catalog" "right" "Descriptions for Documentation" "right" "$col_id_sum, $col_server, $col_desc_wrap"

run_test "P" "Blue" "Server Tag Analysis" "center" "Tag Data for Classification" "center" "$col_server_sum, $col_category_sum, $col_tags_wrap"

run_test "Q" "Red" "Mixed Data Type Analysis" "left" "Comprehensive Server Data for All Metrics - IT Operations $(date +%Y)" "full" "$col_id_sum, $col_location_break, $col_server, $col_cpu_usage, $col_bandwidth"

run_test "R" "Blue" "Detailed Uptime Analysis" "center" "Uptime Metrics for Reliability Assessment" "right" "$col_id_sum, $col_server, $col_uptime, $col_status, $col_location"

run_test "S" "Red" "Ultimate Server Dashboard - All Metrics" "full" "Complete Server Data for Strategic Decision Making - Generated on $(date +%Y-%m-%d)" "full" "$col_id_sum, $col_category_break, $col_server, $col_desc_wrap, $col_cpu, $col_load, $col_memory"

run_test "T" "Blue" "Final Showcase of Server Analytics" "center" "End of Comprehensive Server Report - IT Team $(date +%Y)" "center" '{"header": "ID", "key": "id", "datatype": "int", "justification": "right", "summary": "min"}, '"$col_server_sum"', '"$col_category_sum"', {"header": "Status", "key": "status", "datatype": "text", "justification": "right", "summary": "unique"}, {"header": "CPU Usage", "key": "cpu_usage", "datatype": "kcpu", "justification": "center", "summary": "max"}, '"$col_bandwidth"

run_test "U" "Red" "Dynamic Dual Command Footer Test" "center" "Generated on $(date +%Y-%m-%d) at $(uptime | cut -d' ' -f2-3)" "center" "$col_id_break, $col_server, $col_desc_wrap, $col_cpu"
