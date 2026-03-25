#!/bin/bash
# Setup script for VictoriaLogs development environment
#
# This script sets up environment variables for testing VictoriaLogs
# integration in a local development environment.
#
# Usage:
#   source setup_victoria_logs_dev.sh
#   # or
#   . setup_victoria_logs_dev.sh

# VictoriaLogs configuration
# Point to local VictoriaLogs instance (default port 9428)
# If you don't have VictoriaLogs running locally, leave this unset to disable
export VICTORIALOGS_URL="http://10.118.0.5:9428/insert/jsonline?_stream_fields=app,kubernetes_namespace,kubernetes_pod_name,kubernetes_container_name"

# Minimum log level to send to VictoriaLogs
# TRACE=0, DEBUG=1, STATE=2, ALERT=3, ERROR=4, FATAL=5
export VICTORIALOGS_LVL="DEBUG"

# Kubernetes metadata fallbacks for dev environment
# These simulate what would be injected via Downward API in Kubernetes
hostname=$(hostname) || 'hostname'
export K8S_NAMESPACE="dev"
export K8S_POD_NAME="hydrogen-dev-${hostname}"
export K8S_NODE_NAME="${hostname}"
export K8S_CONTAINER_NAME="hydrogen-dev"

# Optional: Per-developer namespace to distinguish logs
# Uncomment and modify if multiple developers share a VictoriaLogs instance
# export K8S_NAMESPACE="dev-$(whoami)"

echo "VictoriaLogs development environment configured:"
echo "  VICTORIALOGS_URL: ${VICTORIALOGS_URL}"
echo "  VICTORIALOGS_LVL: ${VICTORIALOGS_LVL}"
echo "  K8S_NAMESPACE: ${K8S_NAMESPACE}"       
echo "  K8S_POD_NAME: ${K8S_POD_NAME}"   
echo "  K8S_NODE_NAME: ${K8S_NODE_NAME}"
echo "  K8S_CONTAINER_NAME: ${K8S_CONTAINER_NAME}"
echo ""
echo "To disable VictoriaLogs logging, unset VICTORIALOGS_URL:"
echo "  unset VICTORIALOGS_URL"
echo ""
echo "Test with curl using current environment variables:"
echo "  TIMESTAMP=\$(date -u +%Y-%m-%dT%H:%M:%S.%NZ)"
echo "  curl -X POST \"\$VICTORIALOGS_URL\" \\"
echo "    -H 'Content-Type: application/stream+json' \\"
echo "    -d \"{\\\"_time\\\":\\\"\$TIMESTAMP\\\",\\\"_msg\\\":\\\"Developer test log\\\",\\\"level\\\":\\\"DEBUG\\\",\\\"subsystem\\\":\\\"Test\\\",\\\"app\\\":\\\"hydrogen\\\",\\\"kubernetes_namespace\\\":\\\"\$K8S_NAMESPACE\\\",\\\"kubernetes_pod_name\\\":\\\"\$K8S_POD_NAME\\\",\\\"kubernetes_container_name\\\":\\\"\$K8S_CONTAINER_NAME\\\",\\\"kubernetes_node_name\\\":\\\"\$K8S_NODE_NAME\\\",\\\"host\\\":\\\"\$K8S_NODE_NAME\\\"}\""
