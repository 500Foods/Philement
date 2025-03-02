#!/bin/bash
# Simplified OpenAPI JSON generator for troubleshooting
# This script creates a basic valid OpenAPI 3.1.0 JSON structure

# Set script to exit on any error
set -e

# Path configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
OUTPUT_FILE="${SCRIPT_DIR}/swagger.json"

# Create a minimal valid OpenAPI 3.1.0 JSON
cat > "$OUTPUT_FILE" << EOF
{
  "openapi": "3.1.0",
  "info": {
    "title": "Hydrogen API",
    "description": "REST API for the Hydrogen Project",
    "version": "1.0.0"
  },
  "servers": [
    {
      "url": "http://localhost:8080/api",
      "description": "Development server"
    }
  ],
  "tags": [
    {
      "name": "system",
      "description": "System operations"
    },
    {
      "name": "monitoring",
      "description": "Health monitoring endpoints"
    }
  ],
  "paths": {
    "/system/health": {
      "get": {
        "tags": ["system", "monitoring"],
        "summary": "Health check endpoint",
        "description": "Returns a simple health check response indicating the service is alive.",
        "operationId": "getSystemHealth",
        "responses": {
          "200": {
            "description": "Successful operation",
            "content": {
              "application/json": {
                "schema": {
                  "type": "object",
                  "properties": {
                    "status": {
                      "type": "string",
                      "example": "Yes, I'm alive, thanks!"
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  },
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
}
EOF

echo "Generated simplified OpenAPI 3.1.0 JSON at: $OUTPUT_FILE"
chmod +x "$SCRIPT_DIR/swagger-generate-simple.sh"