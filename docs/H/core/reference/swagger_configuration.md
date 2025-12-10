# Swagger Configuration Guide

This guide explains how to configure and customize the Swagger UI documentation system in Hydrogen.

## Overview

Swagger UI provides interactive API documentation for your Hydrogen server. You can configure various aspects of the documentation interface through the `Swagger` section in your `hydrogen.json` configuration file.

## Basic Configuration

The minimal configuration to enable Swagger UI:

```json
{
    "Swagger": {
        "Enabled": true,
        "Prefix": "/swagger"
    }
}
```

| Setting | Description | Default |
|---------|-------------|---------|
| `Enabled` | Turns API documentation on or off | `true` |
| `Prefix` | The web address where documentation is available | `/swagger` |

## UI Customization

The `UIOptions` section allows you to customize how the documentation is displayed:

```json
{
    "Swagger": {
        "Enabled": true,
        "Prefix": "/swagger",
        "UIOptions": {
            "tryItEnabled": true,
            "displayOperationId": false,
            "defaultModelsExpandDepth": 1,
            "defaultModelExpandDepth": 1,
            "showExtensions": false,
            "showCommonExtensions": true,
            "docExpansion": "list",
            "syntaxHighlightTheme": "agate"
        }
    }
}
```

### Available UI Options

| Option | Description | Values | Default |
|--------|-------------|--------|---------|
| `tryItEnabled` | Allows testing API calls directly from the documentation | `true`/`false` | `true` |
| `displayOperationId` | Shows or hides technical operation IDs | `true`/`false` | `false` |
| `defaultModelsExpandDepth` | How much to expand the data models section | `0` to `5` | `1` |
| `defaultModelExpandDepth` | How much to expand individual data models | `0` to `5` | `1` |
| `showExtensions` | Shows vendor-specific OpenAPI extensions | `true`/`false` | `false` |
| `showCommonExtensions` | Shows commonly used OpenAPI extensions | `true`/`false` | `true` |
| `docExpansion` | How operations are initially displayed | `"list"`/`"full"`/`"none"` | `"list"` |
| `syntaxHighlightTheme` | Color theme for code examples | string | `"agate"` |

## Common Configurations

### Basic Developer View

```json
{
    "Swagger": {
        "UIOptions": {
            "displayOperationId": true,
            "showExtensions": true,
            "defaultModelsExpandDepth": 1,
            "docExpansion": "full"
        }
    }
}
```

Best for developers who need technical details and want to see all operations.

### Simplified User View

```json
{
    "Swagger": {
        "UIOptions": {
            "displayOperationId": false,
            "showExtensions": false,
            "defaultModelsExpandDepth": 0,
            "docExpansion": "list"
        }
    }
}
```

Better for users who just need to understand the API basics.

### Testing-Focused View

```json
{
    "Swagger": {
        "UIOptions": {
            "tryItEnabled": true,
            "displayOperationId": true,
            "defaultModelsExpandDepth": 2,
            "docExpansion": "list"
        }
    }
}
```

Optimized for API testing and integration work.

## Accessing the Documentation

Once configured, access your API documentation at:

```URL
http://your-server:port/swagger/
```

Replace `your-server` and `port` with your server's address and configured port.

## Security Considerations

1. Consider disabling Swagger UI in production environments if you don't want your API documentation publicly accessible
2. The `tryItEnabled` option allows real API calls - ensure your authentication is properly configured
3. Operation IDs can expose implementation details - consider setting `displayOperationId` to `false` in production

## Related Documentation

- [API Documentation Guide](/docs/H/core/api.md) - How to document your API endpoints
- [Web Server Configuration](/docs/H/core/reference/webserver_configuration.md) - Configure the web server hosting Swagger UI
- [Security Configuration](/docs/H/core/reference/security_configuration.md) - Secure your API and documentation
