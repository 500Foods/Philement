# Quick Start Guide

This guide will help you get Hydrogen up and running quickly with a basic configuration.

## Prerequisites

- Linux system (Ubuntu 20.04+ or similar)
- Root or sudo access
- Network connectivity
- Connected 3D printer

## Installation

1. **Download Hydrogen**

   ```bash
   # Commands for downloading and extracting Hydrogen
   # To be completed with actual download instructions
   ```

2. **Basic Configuration**
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

3. **Start the Server**

   ```bash
   # Command to start Hydrogen
   ./hydrogen /path/to/hydrogen.json
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

1. [Configure for Your Environment](./configuration-basics.md)
2. [Set Up as a Service](../reference/service.md)
3. [Secure Your Installation](../reference/security.md)
4. [Explore Advanced Features](./basic-operations.md)

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

1. Check the [Troubleshooting Guide](../reference/troubleshooting.md)
2. Review [Known Issues](../reference/known_issues.md)
3. Search existing issues
4. Create a new issue if needed

## Use Case Examples

- [Home Workshop Setup](./use-cases/home_workshop.md)
- [Print Farm Configuration](./use-cases/print_farm.md)
- [Educational Lab Setup](./use-cases/educational_lab.md)

Note: This guide will be expanded with more detailed instructions and examples.
