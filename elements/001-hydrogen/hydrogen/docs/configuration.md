# Hydrogen Configuration Guide

## Intended Audience

This guide is written for users setting up and managing a Hydrogen 3D printer control server. While some technical concepts are covered, we aim to explain everything in clear, accessible terms. No programming or system administration experience is required to understand and use this guide.

## Introduction

The Hydrogen server provides network-based control and monitoring for 3D printers. It uses a configuration file to control various aspects of its operation, from basic settings like network ports to advanced features like printer discovery on your network. This guide will help you understand and customize these settings for your needs.

For instructions on running Hydrogen as a system service that starts automatically with your computer, see the [Service Setup Guide](service.md).

## Using a Custom Configuration File

By default, Hydrogen looks for its settings in a file named `hydrogen.json` in the same folder as the program. You can use a different configuration file by providing its path as the first argument when starting Hydrogen:

```bash
./hydrogen /path/to/your/config.json
```

## Understanding the Configuration Sections

The configuration file is divided into several sections, each controlling a different aspect of the Hydrogen server. Let's explore what each section does in practical terms.

### Basic Server Settings (Root Level)

These are the fundamental settings that identify your Hydrogen server:

```json
{
    "ServerName": "Philement/hydrogen",
    "LogFile": "/var/log/hydrogen.log"
}
```

| Setting | What It Does |
|---------|-------------|
| `ServerName` | Gives your printer a unique name, useful if you have multiple printers |
| `LogFile` | Specifies where to save information about what the server is doing |

### Web Interface Settings (WebServer)

This section controls the web page you use to interact with your printer:

```json
{
    "WebServer": {
        "Enabled": true,
        "EnableIPv6": false,
        "Port": 5000,
        "WebRoot": "/var/www/html",
        "UploadPath": "/api/upload",
        "UploadDir": "/tmp/hydrogen_uploads",
        "MaxUploadSize": 2147483648,
        "LogLevel": "WARN"
    }
}
```

| Setting | What It Does |
|---------|-------------|
| `Enabled` | Turns the web interface on or off |
| `EnableIPv6` | Allows connections over IPv6 networks |
| `Port` | The network port number for accessing the web interface |
| `WebRoot` | Where the web interface files are stored |
| `UploadPath` | The web address for uploading files to your printer |
| `UploadDir` | Where uploaded files are stored on your computer |
| `MaxUploadSize` | The largest file size you can upload (default 2GB) |
| `LogLevel` | How much information to record about web interface activity |

### Real-Time Updates (WebSocket)

WebSockets allow your web browser to receive instant updates about your printer's status:

```json
{
    "WebSocket": {
        "Enabled": true,
        "EnableIPv6": false,
        "Port": 5001,
        "LogLevel": "NONE"
    }
}
```

| Setting | What It Does |
|---------|-------------|
| `Enabled` | Turns real-time updates on or off |
| `EnableIPv6` | Allows real-time updates over IPv6 networks |
| `Port` | The network port used for real-time communications |
| `LogLevel` | How much information to record about real-time updates |

### Print Job Management (PrintQueue)

This section controls how Hydrogen manages your print jobs:

```json
{
    "PrintQueue": {
        "Enabled": true,
        "LogLevel": "WARN"
    }
}
```

| Setting | What It Does |
|---------|-------------|
| `Enabled` | Turns print job management on or off |
| `LogLevel` | How much information to record about print jobs |

### Network Discovery (mDNS)

mDNS helps your printer be automatically discovered by other devices on your network:

```json
{
    "mDNS": {
        "Enabled": true,
        "EnableIPv6": true,
        "DeviceId": "hydrogen-printer",
        "LogLevel": "ALL",
        "FriendlyName": "Hydrogen 3D Printer",
        "Model": "Hydrogen",
        "Manufacturer": "Philement",
        "Version": "0.1.0",
        "Services": [
            {
                "Name": "Hydrogen Web Interface",
                "Type": "_http._tcp",
                "Port": 5000,
                "TxtRecords": "path=/api/upload"
            },
            {
                "Name": "Hydrogen OctoPrint Emulation",
                "Type": "_octoprint._tcp",
                "Port": 5000,
                "TxtRecords": "path=/api,version=1.1.0"
            },
            {
                "Name": "Hydrogen WebSocket",
                "Type": "_websocket._tcp",
                "Port": 5001,
                "TxtRecords": "path=/websocket"
            }
        ]
    }
}
```

| Setting | What It Does |
|---------|-------------|
| `Enabled` | Turns network discovery on or off |
| `EnableIPv6` | Allows discovery over newer IPv6 networks |
| `DeviceId` | A unique name for your printer on the network |
| `FriendlyName` | The name that appears when your printer is discovered |
| `Model` | Your printer's model name |
| `Manufacturer` | Your printer's manufacturer |
| `Version` | The software version |
| `Services` | List of services your printer advertises on the network |

The `Services` section helps other software find and connect to your printer. For example, the "OctoPrint Emulation" service allows OctoPrint-compatible software to work with your Hydrogen printer.

## Using Environment Variables in Configuration

Hydrogen now supports using environment variables in your configuration file. This allows you to:

1. Keep sensitive settings like API keys secure
2. Create different configurations for development and production
3. Share configuration files without exposing system-specific values
4. Override configuration values without editing the file

### Basic Syntax

To use an environment variable in your configuration file, use the `${env.VARIABLE}` syntax:

```json
{
  "ServerName": "${env.HYDROGEN_SERVER_NAME}"
}
```

In this example, the `ServerName` will be set to the value of the `HYDROGEN_SERVER_NAME` environment variable. If this environment variable doesn't exist, Hydrogen will fall back to the default value as if the key was missing.

### Value Type Conversion

Hydrogen intelligently converts environment variable values to the appropriate JSON type:

1. **Null Values**: If an environment variable exists but has no value, it's treated as JSON `null`
2. **Boolean Values**: Values of "true" or "false" (case-insensitive) are converted to JSON boolean values
3. **Number Values**: Numeric values are converted to JSON numbers
4. **String Values**: All other values are treated as strings

### Examples

#### Boolean Configuration

```json
{
  "WebServer": {
    "Enabled": "${env.HYDROGEN_WEB_ENABLED}"
  }
}
```

If you set `HYDROGEN_WEB_ENABLED=true`, the web server will be enabled. Setting it to `false` will disable it.

#### Numeric Configuration

```json
{
  "WebServer": {
    "Port": "${env.HYDROGEN_WEB_PORT}"
  }
}
```

If you set `HYDROGEN_WEB_PORT=8080`, the port will be set to 8080 as a number.

#### Server Paths and Directories

```json
{
  "WebServer": {
    "WebRoot": "${env.HYDROGEN_WEB_ROOT}",
    "UploadDir": "${env.HYDROGEN_UPLOAD_DIR}"
  }
}
```

This allows you to configure paths based on your system's directory structure.

### Common Use Cases

1. **Development vs. Production**:

   ```bash
   # Development
   export HYDROGEN_WEB_PORT=8080
   export HYDROGEN_UPLOAD_DIR="/tmp/hydrogen_dev"
   
   # Production
   export HYDROGEN_WEB_PORT=5000
   export HYDROGEN_UPLOAD_DIR="/var/lib/hydrogen/uploads"
   ```

2. **Sensitive Information**:

   ```json
   {
     "API": {
       "JWTSecret": "${env.HYDROGEN_JWT_SECRET}"
     }
   }
   ```

3. **Cluster Configuration**:

   ```json
   {
     "ServerName": "${env.HOSTNAME}-hydrogen"
   }
   ```

### Fallback Behavior

If an environment variable referenced in your configuration doesn't exist, Hydrogen will behave as if that configuration key was missing, using the default value for that setting. This means your configuration remains robust even if some environment variables aren't defined.

## Understanding Log Levels

Log levels control how much information Hydrogen records about its operation:

| Level | When to Use It |
|-------|-------------|
| "INFO" | Normal operation - records general activity |
| "WARN" | Records potential issues (recommended for most users) |
| "DEBUG" | Detailed information for troubleshooting |
| "ERROR" | Records only error conditions |
| "CRITICAL" | Records only serious problems |
| "ALL" | Records everything (uses more disk space) |
| "NONE" | Records nothing |

## Keeping Your Printer Secure

Here are some important security tips:

1. **File Access**: Make sure only necessary users can access your configuration file
2. **Upload Space**: Ensure you have enough disk space for uploaded files
3. **Size Limits**: Set reasonable upload size limits to prevent problems
4. **Network Security**: Consider using a firewall to control access to your printer

## Common Issues and Solutions

1. **Can't Connect to Printer**:
   - Check if the web interface is enabled
   - Verify the port numbers
   - Make sure your firewall allows access to the configured ports
   - If using IPv6, verify EnableIPv6 is set to true in the WebServer section

2. **Can't Upload Files**:
   - Check if you have enough disk space
   - Verify the file isn't larger than `MaxUploadSize`
   - Ensure the upload directory exists and is writable

3. **Real-Time Updates Not Working**:
   - Verify WebSocket server is enabled
   - Check if the correct port is being used
   - If using IPv6, ensure EnableIPv6 is set to true in the WebSocket section

4. **Printer Not Being Discovered**:
   - Check if mDNS is enabled
   - Verify your network allows discovery
   - Check IPv6 settings in all relevant sections (mDNS, WebServer, WebSocket)
   - Try disabling IPv6 in all sections if your network doesn't support it

## Getting Help

If you encounter issues not covered in this guide:

1. Check the Hydrogen log file for error messages
2. Try setting the relevant component's `LogLevel` to "DEBUG" for more detailed information
3. Contact support with the log files and your configuration file

Remember to remove any sensitive information (like API keys) before sharing your configuration file with others.
