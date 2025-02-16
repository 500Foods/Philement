# Hydrogen Documentation

Welcome to the Hydrogen Project server documentation. This documentation will help you understand how to interact with and configure the Hydrogen server.

## Contents

1. [API Documentation](./API.md)
   - REST API endpoints
   - Response formats
   - Usage examples

2. [WebSocket Interface](./WebSocket.md) (Coming Soon)
   - Real-time status updates
   - Event notifications
   - WebSocket protocol details

3. [Configuration](./Configuration.md) (Coming Soon)
   - Server configuration
   - Network settings
   - Upload directories
   - Logging options

4. [Print Queue](./PrintQueue.md) (Coming Soon)
   - Queue management
   - Job priorities
   - Status monitoring

## Quick Start

For the most common operations, refer to these quick examples:

1. Get system status:

   ```bash
   curl http://localhost:5000/api/system/info
   ```

2. View print queue (HTML interface):

   ```bash
   curl http://localhost:5000/print/queue
   ```

## Contributing

When adding new features to Hydrogen, please remember to:

1. Document any new API endpoints in [API.md](./API.md)
2. Include curl examples for REST endpoints
3. Add configuration details to [Configuration.md](./Configuration.md)
4. Document WebSocket events in [WebSocket.md](./WebSocket.md)

## Support

If you find any issues with the documentation or need clarification, please:

1. Check the existing documentation thoroughly
2. Look for related code comments in the source
3. Create an issue in the project repository
