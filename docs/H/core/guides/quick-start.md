# Quick Start Guide

This guide will help you get Hydrogen up and running quickly with a basic configuration.

## Prerequisites

- Linux system (Ubuntu 20.04+ or similar)
- Root or sudo access
- Network connectivity
- Connected 3D printer

## Installation

1. **Install Dependencies**

   ```bash
   # Ubuntu/Debian
   sudo apt-get update
   sudo apt-get install build-essential git libjansson-dev libmicrohttpd-dev \
                        libwebsockets-dev libssl-dev

   # Fedora/RHEL
   sudo dnf install gcc git jansson-devel libmicrohttpd-devel \
                    libwebsockets-devel openssl-devel
   ```

2. **Download and Build Hydrogen**

   ```bash
   # Clone the repository
   git clone https://github.com/philement/hydrogen.git
   cd hydrogen

   # Build the server
   make

   # Optional: Build debug version
   make debug
   ```

3. **Basic Configuration**
   Create a file named `hydrogen.json`:

   ```json
   {
       "ServerName": "my-3d-printer",
       "LogFile": "/var/log/hydrogen.log",
       "WebServer": {
           "Enabled": true,
           "Port": 5000,
           "WebRoot": "/var/www/hydrogen",
           "UploadDir": "/var/lib/hydrogen/uploads"
       },
       "PrintQueue": {
           "Enabled": true
       }
   }
   ```

4. **Start the Server**

   ```bash
   # Start with your configuration
   ./hydrogen hydrogen.json

   # Or for debug version
   ./hydrogen_debug hydrogen.json
   ```

## Verify Installation

1. **Check Server Status**

   ```bash
   curl http://localhost:5000/api/system/health
   ```

   Expected response:

   ```json
   {
       "status": "Yes, I'm alive, thanks!"
   }
   ```

2. **View System Information**

   ```bash
   curl http://localhost:5000/api/system/info
   ```

   Example response (truncated):

   ```json
   {
     "status": {
       "server_running": true,
       "server_starting": false,
       "server_started": "2025-02-19T09:03:42.000Z",
       "server_runtime": 22,
       "server_runtime_formatted": "0d 0h 0m 22s",
       "totalThreads": 7
     }
   }
   ```

   The response includes server uptime and timing information useful for monitoring.

## Basic Operations

### Upload a Print Job

1. Open web interface at `http://localhost:5000`
2. Click "Upload File"
3. Select your G-code file
4. Choose print options
5. Submit job

### Monitor Print Progress

1. View queue status at `http://localhost:5000/print/queue`
2. Check real-time updates via WebSocket
3. Monitor system status

## Next Steps

1. [Configure Your Environment](/docs/H/core/configuration.md)
2. [Set Up as a Service](/docs/H/core/service.md)
3. [Explore Advanced Features](/docs/H/core/reference/api.md)

## Common Issues

### Server Won't Start

- Check port availability
- Verify file permissions
- Review log file

### Can't Connect

- Verify server is running
- Check firewall settings
- Confirm network configuration

### Upload Problems

- Check directory permissions
- Verify disk space
- Review file size limits

## Getting Help

1. Review the [System Information](/docs/H/core/reference/system_info.md)
2. Check the [Configuration Guide](/docs/H/core/configuration.md)
3. Search existing issues
4. Create a new issue if needed

## Use Case Examples

- [Home Workshop Setup](/docs/H/core/guides/use-cases/home-workshop.md)
- [Print Farm Configuration](/docs/H/core/guides/use-cases/print-farm.md)
