# Mirage: Distributed Hydrogen Proxy Network

Mirage implements a distributed proxy network where Hydrogen servers at both ends create seamless remote access tunnels. Public "Mirage" servers act as multi-tenant proxy hubs, while remote Hydrogen instances serve device-specific content through secure tunnels, preserving complete customization and feature parity.

## Architecture Overview

**Dual-Server Architecture:**

- **Mirage Server (Public)**: Multi-tenant proxy hub handling multiple remote Hydrogen instances
- **Remote Server (Private)**: Device-specific Hydrogen server with custom configurations, branding, and enabled features
- **Transparent Proxying**: Both HTTP and WebSocket traffic forwarded bidirectionally
- **Identity Preservation**: Remote server's exact configuration and customizations served unchanged

**Core Concept:**

- Hydrogen-to-Hydrogen tunneling maintains full feature compatibility
- Remote customizations (logos, themes, enabled features) served directly from device
- Public URLs provide identical experience to local access
- Multi-device support through single proxy infrastructure

**Operational Flow:**

1. **Device Registration**: Remote Hydrogen server establishes authenticated tunnel to Mirage proxy
2. **Public Endpoint Creation**: Mirage server generates unique public URLs for the remote device
3. **Client Connection**: Users access device via public URL, unaware of proxying
4. **Transparent Forwarding**: All HTTP/WebSocket requests proxied through encrypted tunnel
5. **Content Serving**: Remote server handles requests identically to direct access

**Use Case Example (3D Printing Network):**
A manufacturer operates a public Mirage server supporting hundreds of customer printers. Each customer's Hydrogen instance connects via tunnel, serving their branded interface, custom printer profiles, and enabled features. Customers access their printer via `https://manufacturer.com/printer/customer123` and see their exact customized dashboard, not a generic interface.

## Security Considerations

- **Mutual Authentication**: Both Mirage and remote servers validate credentials
- **End-to-End Encryption**: WSS tunnels with certificate-based security
- **Tenant Isolation**: Each remote device operates in isolated context
- **Access Control**: Remote server enforces its own authorization rules
- **Audit Logging**: Comprehensive logging of tunnel activity and access patterns

## Implementation Details

**Protocol Extensions:**

- Extended WebSocket subprotocol for tunnel establishment
- HTTP CONNECT tunneling for complete web application proxying
- Payload metadata for content-type aware forwarding

**Integration Points:**

- Leverages existing Hydrogen WebSocket and HTTP server infrastructure
- Queue system manages tunnel connections and message routing
- Configuration system supports both Mirage and remote server modes
- Database synchronization for multi-server deployments

**Scalability Features:**

- Single Mirage server can support thousands of remote devices
- Load balancing across multiple Mirage instances
- Connection pooling and automatic failover
- Resource monitoring and automatic scaling
