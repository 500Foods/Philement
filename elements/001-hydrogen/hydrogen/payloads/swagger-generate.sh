#!/usr/bin/env bash

# swagger-generate.sh
# OpenAPI JSON Generator for Hydrogen REST API
# This script scans the API code and extracts OpenAPI annotations
# to generate an OpenAPI 3.1.0 specification file for API documentation

# CHANGELOG
# 1.3.0 - 2025-10-20 - Added parameter processing support for query, path, header, and body parameters
# 1.2.0 - 2024-01-01 - Improved modularity, fixed shellcheck warnings, enhanced error handling
# 1.1.0 - 2024-01-01 - Added support for multiple HTTP methods and improved tag handling
# 1.0.0 - 2024-01-01 - Initial release with basic OpenAPI generation

# Display script information
echo "swagger-generate.sh version 1.3.0"
echo "OpenAPI JSON Generator for Hydrogen REST API"

# Common utilities - use GNU versions if available (eg: homebrew on macOS)
GREP=$(command -v ggrep 2>/dev/null || command -v grep)
SED=$(command -v gsed 2>/dev/null || command -v sed)
AWK=$(command -v gawk 2>/dev/null || command -v awk)

# Terminal formatting codes
readonly GREEN='\033[0;32m'
readonly RED='\033[0;31m'
readonly YELLOW='\033[0;33m'
readonly BLUE='\033[0;34m'
readonly CYAN='\033[0;36m'
readonly BOLD='\033[1m'
readonly NC='\033[0m'  # No Color

# Status symbols
readonly PASS="✅"
readonly FAIL="❌"
readonly WARN="⚠️"
readonly INFO="🛈 "

# Function to convert absolute path to path relative to hydrogen project root
convert_to_relative_path() {
    local absolute_path="$1"
    local relative_path
    
    # Extract the part starting from "hydrogen" and keep everything after
    relative_path=$(echo "${absolute_path}" | "${SED}" -n 's|.*/hydrogen/|hydrogen/|p')
    
    # If the path contains elements/001-hydrogen/hydrogen but not starting with hydrogen/
    if [[ -z "${relative_path}" ]]; then
        relative_path=$(echo "${absolute_path}" | "${SED}" -n 's|.*/elements/001-hydrogen/hydrogen|hydrogen|p')
    fi
    
    # If we still couldn't find a match, return the original
    if [[ -z "${relative_path}" ]]; then
        echo "${absolute_path}"
    else
        echo "${relative_path}"
    fi
}

# Set script to exit on any error
set -e

# Path configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
readonly SCRIPT_DIR

PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
readonly PROJECT_ROOT

readonly API_DIR="${PROJECT_ROOT}/src/api"
readonly OUTPUT_FILE="${SCRIPT_DIR}/swagger.json"

# Create temporary directory
TEMP_DIR=$(mktemp -d)
readonly TEMP_DIR

readonly PATHS_FILE="${TEMP_DIR}/paths.json"
readonly TAGS_FILE="${TEMP_DIR}/tags.json"

# Function to print formatted header
print_header() {
    local title="$1"
    echo -e "\n${BLUE}────────────────────────────────────────────────────────────────${NC}"
    echo -e "${BLUE}${BOLD} ${title} ${NC}"
    echo -e "${BLUE}────────────────────────────────────────────────────────────────${NC}"
}

# Function to display script information
display_script_info() {
    print_header "OpenAPI Generator for Hydrogen API"
    PATH1=$(set -e;convert_to_relative_path "${API_DIR}")
    PATH2=$(set -e;convert_to_relative_path "${OUTPUT_FILE}")
    echo -e "${CYAN}${INFO} API directory: ${NC}${PATH1}"
    echo -e "${CYAN}${INFO} Output file:   ${NC}${PATH2}"
    echo -e "${CYAN}${INFO} Working directory: ${NC}${TEMP_DIR}"
}

# Function to initialize basic files
initialize_files() {
    echo "[]" > "${TAGS_FILE}"
    echo "{}" > "${PATHS_FILE}"
}

# Function to check for required dependencies
check_dependencies() {
    if ! command -v jq &> /dev/null; then
        echo -e "${RED}${FAIL} Error: jq is required for this script to work properly.${NC}"
        echo -e "${YELLOW}${INFO} Please install jq using your package manager.${NC}"
        exit 1
    fi
}

# Function to process service tags
process_service_tags() {
    local service_dir="$1"
    local service_name="$2"
    
    if [[ -f "${service_dir}/${service_name}_service.h" ]]; then
        "${GREP}" -E "//@ swagger:tag" "${service_dir}/${service_name}_service.h" | while read -r line; do
            local tag_name tag_desc clean_tag_name tag_exists
            
            # Extract tag name and description, handling quoted strings
            if echo "${line}" | "${GREP}" -q "swagger:tag[[:space:]]*\""; then
                # Handle quoted tag names
                tag_name=$(echo "${line}" | "${SED}" -E 's/.*swagger:tag[[:space:]]*"([^"]+)".*/\1/')
                tag_desc=$(echo "${line}" | "${SED}" -E 's/.*swagger:tag[[:space:]]*"[^"]+"[[:space:]]+(.*)/\1/')
            else
                # Handle unquoted tag names
                tag_name=$(echo "${line}" | "${SED}" -E 's/.*swagger:tag[[:space:]]+([^[:space:]]+)[[:space:]](.*)/\1/')
                tag_desc=$(echo "${line}" | "${SED}" -E 's/.*swagger:tag[[:space:]]+([^[:space:]]+)[[:space:]](.*)/\2/')
            fi
            
            # Add to tags file if not already present
            # Remove quotes from tag name for jq processing
            clean_tag_name=${tag_name//\"/}
            tag_exists=$(jq --arg name "${clean_tag_name}" '.[] | select(.name == $name) | .name' "${TAGS_FILE}")
            if [[ -z "${tag_exists}" ]]; then
                jq --arg name "${clean_tag_name}" --arg desc "${tag_desc}" \
                    '. += [{"name": $name, "description": $desc}]' "${TAGS_FILE}" > "${TEMP_DIR}/tags_new.json"
                mv "${TEMP_DIR}/tags_new.json" "${TAGS_FILE}"
                echo -e "  ${GREEN}${INFO} Added tag: ${NC}${tag_name}"
            fi
        done || true
    fi
}

# Function to process swagger responses
process_swagger_responses() {
    local header_file="$1"
    local method="$2"
    local method_responses_file="${TEMP_DIR}/responses_${method}.json"
    
    echo "{}" > "${method_responses_file}"
    
    # Process each swagger:response annotation
    "${GREP}" -E "//@ swagger:response" "${header_file}" | while read -r line; do
        local line_without_prefix status content_type schema_part response_description
        local resp_temp_file="${TEMP_DIR}/resp_temp_${method}.json"
        
        # Extract components from the line
        line_without_prefix=${line#*//@ swagger:response}
        
        # Extract status code (first component)
        # shellcheck disable=SC2016 # We want to use single quotes to avoid variable expansion
        status=$(echo "${line_without_prefix}" | "${AWK}" '{print $1}')
        
        # Extract content type (second component)
        # shellcheck disable=SC2016 # We want to use single quotes to avoid variable expansion
        content_type=$(echo "${line_without_prefix}" | "${AWK}" '{print $2}')
        
        # Extract schema - get everything after status and content_type
        # Use awk instead of sed to avoid issues with special characters
        # shellcheck disable=SC2016 # We want to use single quotes to avoid variable expansion
        schema_part=$(echo "${line_without_prefix}" | "${AWK}" -v s="${status}" -v c="${content_type}" '{
            for(i=1; i<=NF; i++) {
                if (i>2) {
                    printf "%s", (i>3 ? " " : "") $i
                }
            }
        }')
        
        # If schema_part is empty or invalid JSON, provide a default
        if [[ -z "${schema_part}" ]] || ! echo "${schema_part}" | jq empty &>/dev/null; then
            schema_part='{"type": "object"}'
        fi
        
        # Select appropriate description based on status code
        if [[ "${status}" -ge 200 ]] && [[ "${status}" -lt 300 ]]; then
            response_description="Successful operation"
        elif [[ "${status}" -ge 400 ]] && [[ "${status}" -lt 500 ]]; then
            response_description="Client error"
        elif [[ "${status}" -ge 500 ]]; then
            response_description="Server error"
        else
            response_description="Response"
        fi
        
        # Create the response object structure using grouped redirects
        {
            echo "{"
            echo "  \"description\": \"${response_description}\","
            echo "  \"content\": {"
            echo "    \"${content_type}\": {"
            echo "      \"schema\": ${schema_part}"
            echo "    }"
            echo "  }"
            echo "}"
        } > "${resp_temp_file}"
        
        # Add this response to the method responses file by status code
        jq --arg status "${status}" \
            --slurpfile resp "${resp_temp_file}" \
            '. + {($status): $resp[0]}' \
            "${method_responses_file}" > "${method_responses_file}.tmp" && mv "${method_responses_file}.tmp" "${method_responses_file}" || true
    done || true # End of response loop
    
    # If no responses were found, add a default 200 response for this method
    if [[ "$(jq 'length' "${method_responses_file}" || true)" -eq 0 ]]; then
        jq '. + {"200": {"description": "Successful operation", "content": {"application/json": {"schema": {"type": "object"}}}}}' \
           "${method_responses_file}" > "${method_responses_file}.tmp" && mv "${method_responses_file}.tmp" "${method_responses_file}"
    fi
# Function to process swagger request body
process_swagger_request_body() {
    local header_file="$1"
    local method="$2"
    local method_request_body_file="${TEMP_DIR}/request_body_${method}.json"
    local method_formdata_file="${TEMP_DIR}/formdata_${method}.json"

    # Initialize with null
    echo "null" > "${method_request_body_file}"

    # Process swagger:request annotation if present
    local request_line
    request_line=$("${GREP}" -E "//@ swagger:request" "${header_file}" | head -1 || echo "" || true)

    if [[ -n "${request_line}" ]]; then
        local line_without_prefix content_type schema_part

        # Extract components from the line
        line_without_prefix=${request_line#*//@ swagger:request}

        # Parse: body content_type {json schema}
        # shellcheck disable=SC2016 # We want to use single quotes to avoid variable expansion
        content_type=$(echo "${line_without_prefix}" | "${AWK}" '{print $2}')

        # Extract schema - everything after content_type
        schema_part=$(echo "${line_without_prefix}" | "${SED}" -E 's/^[[:space:]]*body[[:space:]]+[^[:space:]]+[[:space:]]+//')

        # Validate schema is valid JSON
        if echo "${schema_part}" | jq empty &>/dev/null 2>&1; then
            # Create request body object
            echo "${schema_part}" | jq --arg ct "${content_type}" \
                '{
                   "required": true,
                   "content": {
                     ($ct): {
                       "schema": .
                     }
                   }
                 }' > "${method_request_body_file}"
        fi
    elif [[ -f "${method_formdata_file}" ]] && [[ "$(cat "${method_formdata_file}" || true)" != "{}" ]]; then
        # If we have formData parameters but no explicit request body, create one
        local formdata_schema
        formdata_schema=$(cat "${method_formdata_file}")

        # Create request body for form data
        echo "${formdata_schema}" | jq \
            '{
               "required": true,
               "content": {
                 "application/x-www-form-urlencoded": {
                   "schema": .
                 }
               }
             }' > "${method_request_body_file}"
    fi
}

}

# Function to process swagger parameters
process_swagger_parameters() {
    local header_file="$1"
    local method="$2"
    local method_parameters_file="${TEMP_DIR}/parameters_${method}.json"
    local method_formdata_file="${TEMP_DIR}/formdata_${method}.json"

    echo "[]" > "${method_parameters_file}"
    echo "{}" > "${method_formdata_file}"

    # Process each swagger:parameter annotation
    "${GREP}" -E "//@ swagger:parameter" "${header_file}" | while read -r line; do
        local line_without_prefix param_name param_in param_type param_required param_desc param_example
        local required_bool actual_in

        # Extract components from the line
        line_without_prefix=${line#*//@ swagger:parameter}

        # Parse: name in type required "description" example
        # shellcheck disable=SC2016 # We want to use single quotes to avoid variable expansion
        param_name=$(echo "${line_without_prefix}" | "${AWK}" '{print $1}')
        # shellcheck disable=SC2016 # We want to use single quotes to avoid variable expansion
        param_in=$(echo "${line_without_prefix}" | "${AWK}" '{print $2}')
        # shellcheck disable=SC2016 # We want to use single quotes to avoid variable expansion
        param_type=$(echo "${line_without_prefix}" | "${AWK}" '{print $3}')
        # shellcheck disable=SC2016 # We want to use single quotes to avoid variable expansion
        param_required=$(echo "${line_without_prefix}" | "${AWK}" '{print $4}')

        # Convert required to proper boolean for jq
        if [[ "${param_required}" == "true" ]]; then
            required_bool="true"
        else
            required_bool="false"
        fi

        # Extract description (between quotes)
        param_desc=$(echo "${line_without_prefix}" | "${SED}" -E 's/.*"([^"]*)".*/\1/')

        # Extract example (after the closing quote) - strip whitespace
        # shellcheck disable=SC2016 # We want to use single quotes to avoid variable expansion
        param_example=$(echo "${line_without_prefix}" | "${AWK}" -F'"' '{print $NF}' | "${AWK}" '{print $NF}' | tr -d ' ')

        # Convert OpenAPI 2.0 "formData" to OpenAPI 3.0+ compatible format
        if [[ "${param_in}" == "formData" ]]; then
            if [[ "${method}" == "get" ]]; then
                # For GET requests, convert formData to query parameters
                actual_in="query"
            else
                # For POST/PUT requests, collect formData parameters for requestBody
                # Build form data property for requestBody schema
                if [[ -n "${param_example}" && "${param_example}" != "${param_required}" && "${param_example}" != "true" && "${param_example}" != "false" ]]; then
                    if [[ "${param_type}" == "integer" || "${param_type}" == "number" ]]; then
                        jq -n --arg name "${param_name}" \
                              --arg desc "${param_desc}" \
                              --arg type "${param_type}" \
                              --argjson example "${param_example}" \
                              --argjson required "${required_bool}" \
                              '{"type": $type, "description": $desc, "example": $example}' | \
                        jq --arg name "${param_name}" --argjson required "${required_bool}" \
                           '{"properties": {($name): .}, "required": (if $required then [$name] else [] end)}' >> "${method_formdata_file}.tmp"
                    else
                        jq -n --arg name "${param_name}" \
                              --arg desc "${param_desc}" \
                              --arg type "${param_type}" \
                              --arg example "${param_example}" \
                              --argjson required "${required_bool}" \
                              '{"type": $type, "description": $desc, "example": $example}' | \
                        jq --arg name "${param_name}" --argjson required "${required_bool}" \
                           '{"properties": {($name): .}, "required": (if $required then [$name] else [] end)}' >> "${method_formdata_file}.tmp"
                    fi
                else
                    jq -n --arg name "${param_name}" \
                          --arg desc "${param_desc}" \
                          --arg type "${param_type}" \
                          --argjson required "${required_bool}" \
                          '{"type": $type, "description": $desc}' | \
                    jq --arg name "${param_name}" --argjson required "${required_bool}" \
                       '{"properties": {($name): .}, "required": (if $required then [$name] else [] end)}' >> "${method_formdata_file}.tmp"
                fi
                continue # Skip adding to parameters array for formData in POST/PUT
            fi
        else
            actual_in="${param_in}"
        fi

        # Build parameter JSON using jq for proper escaping (for non-formData params)
        if [[ -n "${param_example}" && "${param_example}" != "${param_required}" && "${param_example}" != "true" && "${param_example}" != "false" ]]; then
            # Has example value
            if [[ "${param_type}" == "integer" || "${param_type}" == "number" ]]; then
                jq -n --arg name "${param_name}" \
                      --arg in "${actual_in}" \
                      --arg desc "${param_desc}" \
                      --arg type "${param_type}" \
                      --argjson example "${param_example}" \
                      --argjson required "${required_bool}" \
                      '{
                         "name": $name,
                         "in": $in,
                         "required": $required,
                         "description": $desc,
                         "schema": {
                           "type": $type,
                           "example": $example
                         }
                       }' >> "${method_parameters_file}.tmp"
            else
                jq -n --arg name "${param_name}" \
                      --arg in "${actual_in}" \
                      --arg desc "${param_desc}" \
                      --arg type "${param_type}" \
                      --arg example "${param_example}" \
                      --argjson required "${required_bool}" \
                      '{
                         "name": $name,
                         "in": $in,
                         "required": $required,
                         "description": $desc,
                         "schema": {
                           "type": $type,
                           "example": $example
                         }
                       }' >> "${method_parameters_file}.tmp"
            fi
        else
            # No example value
            jq -n --arg name "${param_name}" \
                  --arg in "${actual_in}" \
                  --arg desc "${param_desc}" \
                  --arg type "${param_type}" \
                  --argjson required "${required_bool}" \
                  '{
                     "name": $name,
                     "in": $in,
                     "required": $required,
                     "description": $desc,
                     "schema": {
                       "type": $type
                     }
                   }' >> "${method_parameters_file}.tmp"
        fi
    done || true

    # Combine all individual parameter objects into an array
    if [[ -f "${method_parameters_file}.tmp" ]]; then
        jq -s '.' "${method_parameters_file}.tmp" > "${method_parameters_file}"
        rm -f "${method_parameters_file}.tmp"
    fi

    # Merge formData properties into a single schema
    if [[ -f "${method_formdata_file}.tmp" ]]; then
        jq -s 'reduce .[] as $item ({"properties": {}, "required": []}; .properties += $item.properties | .required += $item.required) | .type = "object"' "${method_formdata_file}.tmp" > "${method_formdata_file}"
        rm -f "${method_formdata_file}.tmp"
    fi
}

# Function to create method operation
create_method_operation() {
    local method="$1"
    local summary="$2"
    local description="$3"
    local operation_id="$4"
    local tag_array="$5"
    local method_responses_file="${TEMP_DIR}/responses_${method}.json"
    local method_parameters_file="${TEMP_DIR}/parameters_${method}.json"
    local method_request_body_file="${TEMP_DIR}/request_body_${method}.json"
    local method_operation_file="${TEMP_DIR}/operation_${method}.json"
    
    # Check if parameters file exists and has content
    local has_parameters=false
    if [[ -f "${method_parameters_file}" ]] && [[ "$(jq 'length' "${method_parameters_file}" 2>/dev/null || echo 0 || true)" -gt 0 ]]; then
        has_parameters=true
    fi
    
    # Check if request body exists and is not null
    local has_request_body=false
    if [[ -f "${method_request_body_file}" ]] && [[ "$(cat "${method_request_body_file}" 2>/dev/null || echo "null" || true)" != "null" ]]; then
        has_request_body=true
    fi
    
    # Build operation JSON based on what's available
    local base_operation
    if [[ -n "${operation_id}" ]]; then
        base_operation=$(jq -n --arg summary "${summary}" \
                  --arg desc "${description}" \
                  --arg opid "${operation_id}" \
                  --argjson tags "${tag_array}" \
                  --slurpfile responses "${method_responses_file}" \
                  '{
                     "summary": $summary,
                     "description": $desc,
                     "operationId": $opid,
                     "tags": $tags,
                     "responses": $responses[0]
                   }')
    else
        base_operation=$(jq -n --arg summary "${summary}" \
                  --arg desc "${description}" \
                  --argjson tags "${tag_array}" \
                  --slurpfile responses "${method_responses_file}" \
                  '{
                     "summary": $summary,
                     "description": $desc,
                     "tags": $tags,
                     "responses": $responses[0]
                   }')
    fi
    
    # Add parameters if present
    if [[ "${has_parameters}" == "true" ]]; then
        base_operation=$(echo "${base_operation}" | jq --slurpfile params "${method_parameters_file}" \
            '. + {"parameters": $params[0]}')
    fi
    
    # Add request body if present
    if [[ "${has_request_body}" == "true" ]]; then
        base_operation=$(echo "${base_operation}" | jq --slurpfile reqbody "${method_request_body_file}" \
            '. + {"requestBody": $reqbody[0]}')
    fi
    
    # Write final operation
    echo "${base_operation}" > "${method_operation_file}"
}

# Function to process endpoint methods
process_endpoint_methods() {
    local header_file="$1"
    local path="$2"
    local service_name="$3"
    local endpoint_name="$4"
    local service_dir="$5"
    
    # Extract all methods for this path
    local methods
    readarray -t methods < <("${GREP}" -E "//@ swagger:method" "${header_file}" | "${SED}" -E 's/.*swagger:method[[:space:]]+([^[:space:]]+).*/\1/' | tr '[:upper:]' '[:lower:]' || true)

    # If no methods found, default to GET
    if [[ ${#methods[@]} -eq 0 ]]; then
        methods=("get")
    fi

    # Extract the service tag first - this is used for all methods
    local service_tag_file="${TEMP_DIR}/service_tag.txt"
    local service_tag tag_array summary description
    "${GREP}" -E "//@ swagger:tag" "${service_dir}/${service_name}_service.h" | head -1 | "${SED}" -E 's/.*swagger:tag[[:space:]]*"([^"]+)".*/\1/' > "${service_tag_file}" 2>/dev/null || true
    service_tag=$(cat "${service_tag_file}" 2>/dev/null || echo "")
    tag_array="[\"${service_tag}\"]"
    
    # Extract common info for all methods
    summary=$("${GREP}" -E "//@ swagger:summary" "${header_file}" | "${SED}" -E 's/.*swagger:summary[[:space:]]+([^$]*)/\1/' || echo "${endpoint_name} endpoint") || true
    description=$("${GREP}" -E "//@ swagger:description" "${header_file}" | "${SED}" -E 's/.*swagger:description[[:space:]]+([^$]*)/\1/' || echo "" || true)
    
    # Create path entry if it doesn't exist yet
    local path_exists
    path_exists=$(jq --arg path "${path}" 'has($path)' "${PATHS_FILE}")
    if [[ "${path_exists}" != "true" ]]; then
        jq --arg path "${path}" \
           '.[$path] = {}' "${PATHS_FILE}" > "${PATHS_FILE}.tmp" && \
           mv "${PATHS_FILE}.tmp" "${PATHS_FILE}"
    fi
    
    # Process each method separately
    echo -e "    ${YELLOW}✓ Found ${#methods[@]} HTTP methods: ${NC}${BOLD}${methods[*]} ${NC}"
    
    for method in "${methods[@]}"; do
        echo -e "    ${GREEN}→ Processing ${NC}${BOLD}${method} ${path}${NC}"
        
        # Get method-specific operationId if defined
        local operation_id method_op_id generic_op_id
        operation_id=""
        method_op_id=$("${GREP}" -E "//@ swagger:operationId.*${method}" "${header_file}" | head -1 | "${SED}" -E 's/.*swagger:operationId[[:space:]]+([^[:space:]]+).*/\1/' 2>/dev/null || echo "" || true)
        if [[ -n "${method_op_id}" ]]; then
            operation_id="${method_op_id}"
        else
            # Fall back to generic operationId
            generic_op_id=$("${GREP}" -E "//@ swagger:operationId" "${header_file}" | head -1 | "${SED}" -E 's/.*swagger:operationId[[:space:]]+([^[:space:]]+).*/\1/' 2>/dev/null || echo "" || true)
            if [[ -n "${generic_op_id}" ]]; then
                operation_id="${generic_op_id}${method^}"  # Append capitalized method name
            fi
        fi
        
        # Process responses for this method
        process_swagger_responses "${header_file}" "${method}"
        
        # Process parameters for this method
        process_swagger_parameters "${header_file}" "${method}"
        
        # Process request body for this method
        process_swagger_request_body "${header_file}" "${method}"
        
        # Create method operation
        create_method_operation "${method}" "${summary}" "${description}" "${operation_id}" "${tag_array}"
        
        # Add this method's operation to the path
        local method_operation_file="${TEMP_DIR}/operation_${method}.json"
        jq --arg path "${path}" \
           --arg method "${method}" \
           --slurpfile operation "${method_operation_file}" \
           '.[$path] += {($method): $operation[0]}' "${PATHS_FILE}" > "${PATHS_FILE}.tmp" && \
           mv "${PATHS_FILE}.tmp" "${PATHS_FILE}"
        
        echo -e "    ${GREEN}✓ Added ${NC}${BOLD}${method} ${path}${NC}"
    done # End of method loop
}

# Function to process endpoint directory
process_endpoint_directory() {
    local service_dir="$1"
    local service_name="$2"
    local endpoint_dir="$3"
    local endpoint_name
    
    endpoint_name=$(basename "${endpoint_dir}")
    
    # Skip if this is not an endpoint directory
    if [[ "${endpoint_name}" == *_service* ]]; then
        return
    fi
    
    echo -e "  ${YELLOW}→ Processing endpoint: ${NC}${endpoint_name}"
    
    # Process header files
    for header_file in "${endpoint_dir}"/*.h; do
        if [[ -f "${header_file}" ]]; then
            # Find handler function
            local handler_func
            handler_func=$("${GREP}" -E "handle_.*_${endpoint_name}" "${header_file}" | "${GREP}" -v "^//" | head -1 || true)
            
            if [[ -n "${handler_func}" ]]; then
                echo -e "    ${GREEN}✓ Found handler: ${NC}${handler_func}"
                
                # Extract path and methods
                local path
                path=$("${GREP}" -E "//@ swagger:path" "${header_file}" | head -1 | "${SED}" -E 's/.*swagger:path[[:space:]]+([^[:space:]]+).*/\1/' || echo "/${service_name}/${endpoint_name}" || true)
                # Remove leading /api/ if present
                path=${path#/api}

                # Process endpoint methods
                process_endpoint_methods "${header_file}" "${path}" "${service_name}" "${endpoint_name}" "${service_dir}"
            fi
        fi
    done
}

# Function to collect all endpoints
find_endpoints() {
    print_header "Scanning API Endpoints"
    
    # Process each service directory
    for service_dir in "${API_DIR}"/*/ ; do
        if [[ -d "${service_dir}" ]]; then
            local service_name
            service_name=$(basename "${service_dir}")
            echo -e "${CYAN}${BOLD}Processing service: ${NC}${service_name}"
            
            # Process service tags
            process_service_tags "${service_dir}" "${service_name}"
            
            # Process each endpoint directory
            for endpoint_dir in "${service_dir}"/*/ ; do
                if [[ -d "${endpoint_dir}" ]]; then
                    process_endpoint_directory "${service_dir}" "${service_name}" "${endpoint_dir}"
                fi
            done
        fi
    done
}

# Function to extract API info from api_utils.h
extract_api_info() {
    print_header "Extracting API Information"
    
    local api_info_file="${TEMP_DIR}/api_info.json"
    local servers_file="${TEMP_DIR}/servers.json"
    
    # Initialize with defaults
    echo '{
        "title": "Hydrogen API",
        "description": "REST API for the Hydrogen Project",
        "version": "1.0.0"
    }' > "${api_info_file}"
    
    echo '[
        {
            "url": "http://localhost:8080/api",
            "description": "Development server"
        }
    ]' > "${servers_file}"
    
    local api_utils_file="${API_DIR}/api_utils.h"
    if [[ -f "${api_utils_file}" ]]; then
        PATH3=$(set -e;convert_to_relative_path "${api_utils_file}")
        echo -e "${CYAN}${INFO} Analyzing ${NC}${PATH3}"
        
        # Extract title
        local title
        title=$("${GREP}" -E "//@ swagger:title" "${api_utils_file}" | "${SED}" -E 's/.*swagger:title[[:space:]]+([^$]*)/\1/' || echo "" || true)
        if [[ -n "${title}" ]]; then
            jq --arg title "${title}" '.title = $title' "${api_info_file}" > "${api_info_file}.tmp" && \
                mv "${api_info_file}.tmp" "${api_info_file}"
            echo -e "  ${GREEN}✓ Found title: ${NC}${title}"
        fi
        
        # Extract description
        local desc
        desc=$("${GREP}" -E "//@ swagger:description" "${api_utils_file}" | "${SED}" -E 's/.*swagger:description[[:space:]]+([^$]*)/\1/' || echo "" || true)
        if [[ -n "${desc}" ]]; then
            jq --arg desc "${desc}" '.description = $desc' "${api_info_file}" > "${api_info_file}.tmp" && \
                mv "${api_info_file}.tmp" "${api_info_file}"
            echo -e "  ${GREEN}✓ Found description"
        fi
        
        # Extract version
        local version
        version=$("${GREP}" -E "//@ swagger:version" "${api_utils_file}" | "${SED}" -E 's/.*swagger:version[[:space:]]+([^$]*)/\1/' || echo "" || true)
        if [[ -n "${version}" ]]; then
            jq --arg version "${version}" '.version = $version' "${api_info_file}" > "${api_info_file}.tmp" && \
                mv "${api_info_file}.tmp" "${api_info_file}"
            echo -e "  ${GREEN}✓ Found version: ${NC}${version}"
        fi
        
        # Extract contact email
        local contact
        contact=$("${GREP}" -E "//@ swagger:contact.email" "${api_utils_file}" | "${SED}" -E 's/.*swagger:contact.email[[:space:]]+([^$]*)/\1/' || echo "" || true)
        if [[ -n "${contact}" ]]; then
            jq --arg email "${contact}" '.contact = {"email": $email}' "${api_info_file}" > "${api_info_file}.tmp" && \
                mv "${api_info_file}.tmp" "${api_info_file}"
            echo -e "  ${GREEN}✓ Found contact email: ${NC}${contact}"
        fi
        
        # Extract license - handle both SPDX identifiers and name/url pairs
        local license license_url
        license=$("${GREP}" -E "//@ swagger:license.name" "${api_utils_file}" | "${SED}" -E 's/.*swagger:license.name[[:space:]]+([^$]*)/\1/' || echo "" || true)
        license_url=$("${GREP}" -E "//@ swagger:license.url" "${api_utils_file}" | "${SED}" -E 's/.*swagger:license.url[[:space:]]+([^$]*)/\1/' || echo "" || true)
        
        if [[ -n "${license}" ]]; then
            # Check if this is a standard SPDX license identifier
            case "${license}" in
                "MIT"|"Apache-2.0"|"GPL-3.0"|"BSD-3-Clause"|"BSD-2-Clause"|"ISC"|"Unlicense"|"GPL-2.0"|"LGPL-3.0"|"LGPL-2.1")
                    # Use both name and identifier for standard licenses to ensure compatibility
                    jq --arg name "${license}" --arg identifier "${license}" '.license = {"name": $name, "identifier": $identifier}' "${api_info_file}" > "${api_info_file}.tmp" && \
                        mv "${api_info_file}.tmp" "${api_info_file}"
                    echo -e "  ${GREEN}✓ Found SPDX license: ${NC}${license}"
                    ;;
                *)
                    # For non-standard or custom licenses, use name and url
                    if [[ -n "${license_url}" ]]; then
                        jq --arg name "${license}" --arg url "${license_url}" '.license = {"name": $name, "url": $url}' "${api_info_file}" > "${api_info_file}.tmp" && \
                            mv "${api_info_file}.tmp" "${api_info_file}"
                        echo -e "  ${GREEN}✓ Found license: ${NC}${license} (${license_url})"
                    else
                        # Default to MIT with both name and identifier if no URL provided
                        jq '.license = {"name": "MIT", "identifier": "MIT"}' "${api_info_file}" > "${api_info_file}.tmp" && \
                            mv "${api_info_file}.tmp" "${api_info_file}"
                        echo -e "  ${YELLOW}${WARN} License name without URL, defaulting to MIT${NC}"
                    fi
                    ;;
            esac
        else
            # Default to MIT license if none specified
            jq '.license = {"name": "MIT", "identifier": "MIT"}' "${api_info_file}" > "${api_info_file}.tmp" && \
                mv "${api_info_file}.tmp" "${api_info_file}"
            echo -e "  ${YELLOW}${WARN} No license found, defaulting to MIT${NC}"
        fi
        
        # Clear the default servers first to prevent duplicates
        echo "[]" > "${servers_file}"
        
        # Extract servers
        "${GREP}" -E "//@ swagger:server" "${api_utils_file}" | while read -r line; do
            local url desc exists
            url=$(echo "${line}" | "${SED}" -E 's/.*swagger:server[[:space:]]+([^[:space:]]+)[[:space:]]+.*/\1/')
            desc=$(echo "${line}" | "${SED}" -E 's/.*swagger:server[[:space:]]+[^[:space:]]+[[:space:]]+([^$]*)/\1/')
            
            # Check if this server already exists to avoid duplicates
            exists=$(jq --arg url "${url}" 'map(select(.url == $url)) | length' "${servers_file}")
            if [[ "${exists}" -eq 0 ]]; then
                jq --arg url "${url}" --arg desc "${desc}" '. += [{"url": $url, "description": $desc}]' \
                    "${servers_file}" > "${servers_file}.tmp" && mv "${servers_file}.tmp" "${servers_file}"
                echo -e "  ${GREEN}✓ Added server: ${NC}${url}"
            fi
        done || true
        
        # Add default server if none specified
        local servers_count
        servers_count=$(jq 'length' "${servers_file}")
        if [[ "${servers_count}" -eq 0 ]]; then
            jq '. += [{"url": "http://localhost:8080/api", "description": "Development server"}]' \
                "${servers_file}" > "${servers_file}.tmp" && mv "${servers_file}.tmp" "${servers_file}"
            echo -e "  ${YELLOW}${WARN} No servers found, adding default: ${NC}http://localhost:8080/api"
        fi
    fi
}

# Function to assemble the final OpenAPI JSON
assemble_openapi_json() {
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
          }' > "${OUTPUT_FILE}"
          
    # Get the file size and endpoint count
    local file_size endpoint_count
    file_size=$(du -h "${OUTPUT_FILE}" | cut -f1 || true)
    endpoint_count=$(jq '.paths | keys | length' "${OUTPUT_FILE}")
    
    echo -e "${GREEN}${INFO} Generated OpenAPI specification (${file_size})${NC}"
    echo -e "${GREEN}${INFO} Documented endpoints: ${NC}${endpoint_count}"
}

# Function to validate the generated OpenAPI specification
validate_openapi_spec() {
    print_header "Validating OpenAPI Specification"
    
    # Check if swagger-cli is available
    if ! command -v swagger-cli &> /dev/null; then
        echo -e "${YELLOW}${WARN} swagger-cli not found, skipping validation${NC}"
        echo -e "${CYAN}${INFO} To install swagger-cli: ${NC}npm install -g swagger-cli"
        return 0
    fi
    
    echo -e "${CYAN}${INFO} Running swagger-cli validation...${NC}"
    
    # Run swagger-cli validate and capture the result
    if swagger-cli validate "swagger.json" &>/dev/null; then
        echo -e "${GREEN}${PASS} ${BOLD}OpenAPI specification validation passed!${NC}"
        return 0
    else
        echo -e "${RED}${FAIL} ${BOLD}OpenAPI specification validation failed!${NC}"
        echo -e "${YELLOW}${INFO} Please check the validation errors above and update the annotations accordingly.${NC}"
        return 1
    fi
}

# Function to cleanup temporary files
cleanup_temp_files() {
    echo -e "${CYAN}${INFO} Cleaning up temporary files...${NC}"
    rm -rf "${TEMP_DIR}"
}

# Function to display completion summary
display_completion_summary() {
    print_header "Generation Complete"
    PATH4=$(set -e;convert_to_relative_path "${OUTPUT_FILE}")
    echo -e "${GREEN}${PASS} ${BOLD}OpenAPI 3.1.0 JSON generated successfully!${NC}"
    echo -e "${CYAN}${INFO} Location: ${BOLD}${PATH4}${NC}"
    echo -e "${BLUE}────────────────────────────────────────────────────────────────${NC}"
}

# Main execution function
main() {
    # Display script information
    display_script_info
    
    # Check dependencies
    check_dependencies
    
    # Initialize files
    initialize_files
    
    # Run the processing steps
    extract_api_info
    find_endpoints
    assemble_openapi_json
    
    # Validate the generated specification
    validate_openapi_spec
    
    # Clean up temporary files
    cleanup_temp_files
    
    # Display completion summary
    display_completion_summary
}

# Execute main function
main "$@"
