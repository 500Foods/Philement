#!/bin/bash
# ──────────────────────────────────────────────────────────────────────────────
# OpenAPI JSON Generator for Hydrogen REST API
# ──────────────────────────────────────────────────────────────────────────────
# This script scans the API code and extracts OpenAPI annotations
# to generate an OpenAPI 3.1.0 specification file for API documentation

# Terminal formatting codes
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
MAGENTA='\033[0;35m'
BOLD='\033[1m'
NC='\033[0m'  # No Color

# Status symbols
PASS="✅"
FAIL="❌"
WARN="⚠️"
INFO="🛈 "

# Function to convert absolute path to path relative to hydrogen project root
convert_to_relative_path() {
    local absolute_path="$1"
    
    # Extract the part starting from "hydrogen" and keep everything after
    local relative_path=$(echo "$absolute_path" | sed -n 's|.*/hydrogen/|hydrogen/|p')
    
    # If the path contains elements/001-hydrogen/hydrogen but not starting with hydrogen/
    if [ -z "$relative_path" ]; then
        relative_path=$(echo "$absolute_path" | sed -n 's|.*/elements/001-hydrogen/hydrogen|hydrogen|p')
    fi
    
    # If we still couldn't find a match, return the original
    if [ -z "$relative_path" ]; then
        echo "$absolute_path"
    else
        echo "$relative_path"
    fi
}

# Set script to exit on any error
set -e

# Path configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
API_DIR="${PROJECT_ROOT}/src/api"
OUTPUT_FILE="${SCRIPT_DIR}/swagger.json"

# Print header function
print_header() {
    echo -e "\n${BLUE}────────────────────────────────────────────────────────────────${NC}"
    echo -e "${BLUE}${BOLD} $1 ${NC}"
    echo -e "${BLUE}────────────────────────────────────────────────────────────────${NC}"
}

print_header "OpenAPI Generator for Hydrogen API"
echo -e "${CYAN}${INFO} API directory: ${NC}$(convert_to_relative_path "${API_DIR}")"
echo -e "${CYAN}${INFO} Output file:   ${NC}$(convert_to_relative_path "${OUTPUT_FILE}")"

# Create a temporary directory for intermediate files
TEMP_DIR=$(mktemp -d)
PATHS_FILE="${TEMP_DIR}/paths.json"
TAGS_FILE="${TEMP_DIR}/tags.json"

echo -e "${CYAN}${INFO} Working directory: ${NC}${TEMP_DIR}"

# Initialize basic files
echo "[]" > "$TAGS_FILE"
echo "{}" > "$PATHS_FILE"

# Collect all endpoints
function find_endpoints() {
    print_header "Scanning API Endpoints"
    
    # Process each service directory
    for service_dir in "${API_DIR}"/*/ ; do
        if [ -d "$service_dir" ]; then
            service_name=$(basename "$service_dir")
            echo -e "${CYAN}${BOLD}Processing service: ${NC}${service_name}"
            
            # Process service tags
            if [ -f "${service_dir}/${service_name}_service.h" ]; then
                grep -E "//@ swagger:tag" "${service_dir}/${service_name}_service.h" | while read -r line; do
                    # Extract tag name and description, handling quoted strings
                    if echo "$line" | grep -q "swagger:tag[[:space:]]*\""; then
                        # Handle quoted tag names
                        tag_name=$(echo "$line" | sed -E 's/.*swagger:tag[[:space:]]*"([^"]+)".*/\1/')
                        tag_desc=$(echo "$line" | sed -E 's/.*swagger:tag[[:space:]]*"[^"]+"[[:space:]]+(.*)/\1/')
                    else
                        # Handle unquoted tag names
                        tag_name=$(echo "$line" | sed -E 's/.*swagger:tag[[:space:]]+([^[:space:]]+)[[:space:]](.*)/\1/')
                        tag_desc=$(echo "$line" | sed -E 's/.*swagger:tag[[:space:]]+([^[:space:]]+)[[:space:]](.*)/\2/')
                    fi
                    
                    # Add to tags file if not already present
                    # Remove quotes from tag name for jq processing
                    clean_tag_name=$(echo "$tag_name" | sed 's/"//g')
                    tag_exists=$(jq --arg name "$clean_tag_name" '.[] | select(.name == $name) | .name' "$TAGS_FILE")
                    if [ -z "$tag_exists" ]; then
                        jq --arg name "$clean_tag_name" --arg desc "$tag_desc" \
                            '. += [{"name": $name, "description": $desc}]' "$TAGS_FILE" > "${TEMP_DIR}/tags_new.json"
                        mv "${TEMP_DIR}/tags_new.json" "$TAGS_FILE"
                        echo -e "  ${GREEN}${INFO} Added tag: ${NC}${tag_name}"
                    fi
                done
            fi
            
            # Process each endpoint directory
            for endpoint_dir in "$service_dir"/*/ ; do
                if [ -d "$endpoint_dir" ]; then
                    endpoint_name=$(basename "$endpoint_dir")
                    
                    # Skip if this is not an endpoint directory
                    if [[ "$endpoint_name" == *_service* ]]; then
                        continue
                    fi
                    
                    echo -e "  ${YELLOW}→ Processing endpoint: ${NC}${endpoint_name}"
                    
                    # Process header files
                    for header_file in "$endpoint_dir"/*.h; do
                        if [ -f "$header_file" ]; then
                            # Find handler function
                            handler_func=$(grep -E "handle_.*_${endpoint_name}" "$header_file" | grep -v "^//" | head -1 || true)
                            
                            if [ -n "$handler_func" ]; then
                                echo -e "    ${GREEN}✓ Found handler: ${NC}${handler_func}"
                                
                                # Extract path
                                path=$(grep -E "//@ swagger:path" "$header_file" | sed -E 's/.*swagger:path[[:space:]]+([^[:space:]]+).*/\1/' || echo "/$service_name/$endpoint_name")
                                
                                # Remove leading /api/ if present
                                path=$(echo "$path" | sed 's|^/api||')
                                
                                # Extract method
                                method=$(grep -E "//@ swagger:method" "$header_file" | sed -E 's/.*swagger:method[[:space:]]+([^[:space:]]+).*/\1/' || echo "GET")
                                method=${method,,}  # Convert to lowercase
                                
                                # Extract the service tag first
                                service_tag_file="${TEMP_DIR}/service_tag.txt"
                                grep -E "//@ swagger:tag" "${service_dir}/${service_name}_service.h" | head -1 | sed -E 's/.*swagger:tag[[:space:]]*"([^"]+)".*/\1/' > "$service_tag_file"
                                service_tag=$(cat "$service_tag_file")
                                tag_array="[\"$service_tag\"]"

                                # Only include operationId if explicitly defined
                                operation_id=$(grep -E "//@ swagger:operationId" "$header_file" | sed -E 's/.*swagger:operationId[[:space:]]+([^[:space:]]+).*/\1/' || echo "")
                                
                                # Extract summary
                                summary=$(grep -E "//@ swagger:summary" "$header_file" | sed -E 's/.*swagger:summary[[:space:]]+([^$]*)/\1/' || echo "$endpoint_name endpoint")
                                
                                # Extract description
                                description=$(grep -E "//@ swagger:description" "$header_file" | sed -E 's/.*swagger:description[[:space:]]+([^$]*)/\1/' || echo "")
                                
                                # Build responses object
                                responses_file="${TEMP_DIR}/responses.json"
                                echo "{}" > "$responses_file"
                                
                                # Clear responses file before processing
                                echo "{}" > "$responses_file"
                                
                                # Process each swagger:response annotation
                                grep -E "//@ swagger:response" "$header_file" | while read -r line; do
                                    # Extract components from the line
                                    line_without_prefix=${line#*//@ swagger:response}
                                    
                                    # Extract status code (first component)
                                    status=$(echo "$line_without_prefix" | awk '{print $1}')
                                    
                                    # Extract content type (second component)
                                    content_type=$(echo "$line_without_prefix" | awk '{print $2}')
                                    
                                    # Extract schema - get everything after status and content_type
                                    # Use awk instead of sed to avoid issues with special characters
                                    schema_part=$(echo "$line_without_prefix" | awk -v s="$status" -v c="$content_type" '{
                                        for(i=1; i<=NF; i++) {
                                            if (i>2) {
                                                printf "%s", (i>3 ? " " : "") $i
                                            }
                                        }
                                    }')
                                    
                                    # If schema_part is empty or invalid JSON, provide a default
                                    if [ -z "$schema_part" ] || ! echo "$schema_part" | jq empty &>/dev/null; then
                                        schema_part='{"type": "object"}'
                                    fi
                                    
                                    # Select appropriate description based on status code
                                    if [ "$status" -ge 200 ] && [ "$status" -lt 300 ]; then
                                        description="Successful operation"
                                    elif [ "$status" -ge 400 ] && [ "$status" -lt 500 ]; then
                                        description="Client error"
                                    elif [ "$status" -ge 500 ]; then
                                        description="Server error"
                                    else
                                        description="Response"
                                    fi
                                    
                                    # Create a temporary file for the response
                                    resp_temp_file="${TEMP_DIR}/resp_temp.json"
                                    
                                    # Create the response object structure
                                    echo "{" > "$resp_temp_file"
                                    echo "  \"description\": \"$description\"," >> "$resp_temp_file"
                                    echo "  \"content\": {" >> "$resp_temp_file"
                                    echo "    \"$content_type\": {" >> "$resp_temp_file"
                                    echo "      \"schema\": $schema_part" >> "$resp_temp_file"
                                    echo "    }" >> "$resp_temp_file"
                                    echo "  }" >> "$resp_temp_file"
                                    echo "}" >> "$resp_temp_file"
                                    
                                    # Add this response to the responses file by status code
                                    jq --arg status "$status" \
                                       --slurpfile resp "$resp_temp_file" \
                                       '. + {($status): $resp[0]}' \
                                       "$responses_file" > "${responses_file}.tmp" && mv "${responses_file}.tmp" "$responses_file"
                                done
                                
                                # If no responses were found, add a default 200 response
                                if [ "$(jq 'length' "$responses_file")" -eq 0 ]; then
                                    jq '. + {"200": {"description": "Successful operation", "content": {"application/json": {"schema": {"type": "object"}}}}}' \
                                       "$responses_file" > "${responses_file}.tmp" && mv "${responses_file}.tmp" "$responses_file"
                                fi
                                
                                # Build the operation object based on whether operationId exists
                                operation_file="${TEMP_DIR}/operation.json"
                                if [ -n "$operation_id" ]; then
                                    jq -n --arg summary "$summary" \
                                          --arg desc "$description" \
                                          --arg opid "$operation_id" \
                                          --argjson tags "$tag_array" \
                                          --slurpfile responses "$responses_file" \
                                          '{
                                             "summary": $summary,
                                             "description": $desc,
                                             "operationId": $opid,
                                             "tags": $tags,
                                             "responses": $responses[0]
                                           }' > "$operation_file"
                                else
                                    jq -n --arg summary "$summary" \
                                          --arg desc "$description" \
                                          --argjson tags "$tag_array" \
                                          --slurpfile responses "$responses_file" \
                                          '{
                                             "summary": $summary,
                                             "description": $desc,
                                             "tags": $tags,
                                             "responses": $responses[0]
                                           }' > "$operation_file"
                                fi
                                
                                # Add the operation to the path
                                path_exists=$(jq --arg path "$path" 'has($path)' "$PATHS_FILE")
                                if [ "$path_exists" == "true" ]; then
                                    jq --arg path "$path" \
                                       --arg method "$method" \
                                       --slurpfile operation "$operation_file" \
                                       '.[$path] += {($method): $operation[0]}' "$PATHS_FILE" > "${PATHS_FILE}.tmp" && \
                                       mv "${PATHS_FILE}.tmp" "$PATHS_FILE"
                                else
                                    jq --arg path "$path" \
                                       --arg method "$method" \
                                       --slurpfile operation "$operation_file" \
                                       '.[$path] = {($method): $operation[0]}' "$PATHS_FILE" > "${PATHS_FILE}.tmp" && \
                                       mv "${PATHS_FILE}.tmp" "$PATHS_FILE"
                                fi
                                
                                echo -e "    ${GREEN}✓ Added ${NC}${BOLD}${method} ${path}${NC}"
                            fi
                        fi
                    done
                fi
            done
        fi
    done
}

# Extract API info from api_utils.h
function extract_api_info() {
    print_header "Extracting API Information"
    
    API_INFO_FILE="${TEMP_DIR}/api_info.json"
    SERVERS_FILE="${TEMP_DIR}/servers.json"
    
    # Initialize with defaults
    echo '{
        "title": "Hydrogen API",
        "description": "REST API for the Hydrogen Project",
        "version": "1.0.0"
    }' > "$API_INFO_FILE"
    
    echo '[
        {
            "url": "http://localhost:8080/api",
            "description": "Development server"
        }
    ]' > "$SERVERS_FILE"
    
    API_UTILS_FILE="${API_DIR}/api_utils.h"
    if [ -f "$API_UTILS_FILE" ]; then
        echo -e "${CYAN}${INFO} Analyzing ${NC}$(convert_to_relative_path "${API_UTILS_FILE}")"
        
        # Extract title
        title=$(grep -E "//@ swagger:title" "$API_UTILS_FILE" | sed -E 's/.*swagger:title[[:space:]]+([^$]*)/\1/' || echo "")
        if [ -n "$title" ]; then
            jq --arg title "$title" '.title = $title' "$API_INFO_FILE" > "${API_INFO_FILE}.tmp" && \
                mv "${API_INFO_FILE}.tmp" "$API_INFO_FILE"
            echo -e "  ${GREEN}✓ Found title: ${NC}${title}"
        fi
        
        # Extract description
        desc=$(grep -E "//@ swagger:description" "$API_UTILS_FILE" | sed -E 's/.*swagger:description[[:space:]]+([^$]*)/\1/' || echo "")
        if [ -n "$desc" ]; then
            jq --arg desc "$desc" '.description = $desc' "$API_INFO_FILE" > "${API_INFO_FILE}.tmp" && \
                mv "${API_INFO_FILE}.tmp" "$API_INFO_FILE"
            echo -e "  ${GREEN}✓ Found description"
        fi
        
        # Extract version
        version=$(grep -E "//@ swagger:version" "$API_UTILS_FILE" | sed -E 's/.*swagger:version[[:space:]]+([^$]*)/\1/' || echo "")
        if [ -n "$version" ]; then
            jq --arg version "$version" '.version = $version' "$API_INFO_FILE" > "${API_INFO_FILE}.tmp" && \
                mv "${API_INFO_FILE}.tmp" "$API_INFO_FILE"
            echo -e "  ${GREEN}✓ Found version: ${NC}${version}"
        fi
        
        # Extract contact email
        contact=$(grep -E "//@ swagger:contact.email" "$API_UTILS_FILE" | sed -E 's/.*swagger:contact.email[[:space:]]+([^$]*)/\1/' || echo "")
        if [ -n "$contact" ]; then
            jq --arg email "$contact" '.contact = {"email": $email}' "$API_INFO_FILE" > "${API_INFO_FILE}.tmp" && \
                mv "${API_INFO_FILE}.tmp" "$API_INFO_FILE"
            echo -e "  ${GREEN}✓ Found contact email: ${NC}${contact}"
        fi
        
        # Extract license
        license=$(grep -E "//@ swagger:license.name" "$API_UTILS_FILE" | sed -E 's/.*swagger:license.name[[:space:]]+([^$]*)/\1/' || echo "")
        if [ -n "$license" ]; then
            jq --arg name "$license" '.license = {"name": $name}' "$API_INFO_FILE" > "${API_INFO_FILE}.tmp" && \
                mv "${API_INFO_FILE}.tmp" "$API_INFO_FILE"
            echo -e "  ${GREEN}✓ Found license: ${NC}${license}"
        fi
        
        # Clear the default servers first to prevent duplicates
        echo "[]" > "$SERVERS_FILE"
        
        # Extract servers
        grep -E "//@ swagger:server" "$API_UTILS_FILE" | while read -r line; do
            url=$(echo "$line" | sed -E 's/.*swagger:server[[:space:]]+([^[:space:]]+)[[:space:]]+.*/\1/')
            desc=$(echo "$line" | sed -E 's/.*swagger:server[[:space:]]+[^[:space:]]+[[:space:]]+([^$]*)/\1/')
            
            # Check if this server already exists to avoid duplicates
            exists=$(jq --arg url "$url" 'map(select(.url == $url)) | length' "$SERVERS_FILE")
            if [ "$exists" -eq 0 ]; then
                jq --arg url "$url" --arg desc "$desc" '. += [{"url": $url, "description": $desc}]' \
                    "$SERVERS_FILE" > "${SERVERS_FILE}.tmp" && mv "${SERVERS_FILE}.tmp" "$SERVERS_FILE"
                echo -e "  ${GREEN}✓ Added server: ${NC}${url}"
            fi
        done
        
        # Add default server if none specified
        servers_count=$(jq 'length' "$SERVERS_FILE")
        if [ "$servers_count" -eq 0 ]; then
            jq '. += [{"url": "http://localhost:8080/api", "description": "Development server"}]' \
                "$SERVERS_FILE" > "${SERVERS_FILE}.tmp" && mv "${SERVERS_FILE}.tmp" "$SERVERS_FILE"
            echo -e "  ${YELLOW}${WARN} No servers found, adding default: ${NC}http://localhost:8080/api"
        fi
    fi
}

# Assemble the final OpenAPI JSON
function assemble_openapi_json() {
    print_header "Assembling OpenAPI Specification"
    
    # Create the final OpenAPI JSON file - filter out empty paths
    jq -n --slurpfile info "${TEMP_DIR}/api_info.json" \
          --slurpfile servers "${TEMP_DIR}/servers.json" \
          --slurpfile tags "${TEMP_DIR}/tags.json" \
          --slurpfile paths "${TEMP_DIR}/paths.json" \
          '{
            "openapi": "3.1.0",
            "info": $info[0],
            "servers": $servers[0],
            "tags": $tags[0],
            "paths": ($paths[0] | with_entries(select(.key != ""))),
            "components": {
              "schemas": {},
              "securitySchemes": {
                "bearerAuth": {
                  "type": "http",
                  "scheme": "bearer",
                  "bearerFormat": "JWT"
                }
              }
            }
          }' > "$OUTPUT_FILE"
          
    # Get the file size
    FILE_SIZE=$(du -h "$OUTPUT_FILE" | cut -f1)
    ENDPOINT_COUNT=$(jq '.paths | keys | length' "$OUTPUT_FILE")
    
    echo -e "${GREEN}${INFO} Generated OpenAPI specification (${FILE_SIZE})${NC}"
    echo -e "${GREEN}${INFO} Documented endpoints: ${NC}${ENDPOINT_COUNT}"
}

# Make sure jq is available
if ! command -v jq &> /dev/null; then
    echo -e "${RED}${FAIL} Error: jq is required for this script to work properly.${NC}"
    echo -e "${YELLOW}${INFO} Please install jq using your package manager.${NC}"
    exit 1
fi

# Run the processing steps
extract_api_info
find_endpoints
assemble_openapi_json

# Clean up temporary files
echo -e "${CYAN}${INFO} Cleaning up temporary files...${NC}"
rm -rf "$TEMP_DIR"

print_header "Generation Complete"
echo -e "${GREEN}${PASS} ${BOLD}OpenAPI 3.1.0 JSON generated successfully!${NC}"
echo -e "${CYAN}${INFO} Location: ${BOLD}$(convert_to_relative_path "${OUTPUT_FILE}")${NC}"
echo -e "${BLUE}────────────────────────────────────────────────────────────────${NC}"