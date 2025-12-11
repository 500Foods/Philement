# Hydrogen Subsystems Documentation

## API

The API subsystem is the programmable interface that allows other software and systems to communicate with and control the Hydrogen server. It provides a standardized way for external applications to access Hydrogen's capabilities, enabling automation, integration, and remote management.

- [README.md](/docs/H/core/subsystems/api/README.md)
- [api.md](/docs/H/core/subsystems/api/api.md)

## Database

The Database subsystem is a key component of the Hydrogen project that handles all data storage and retrieval operations. Think of it as a sophisticated digital filing cabinet that organizes, stores, and manages information in a way that makes it easy for the system to access and use data when needed.

- [README.md](/docs/H/core/subsystems/database/README.md)
- [database.md](/docs/H/core/subsystems/database/database.md)

## Mail Relay

The MailRelay subsystem is the email communication system that handles sending notifications, reports, and alerts via email to keep users and administrators informed about system activities. It acts as a reliable email relay service that receives messages from the system and forwards them through configured SMTP servers to ensure timely delivery.

- [README.md](/docs/H/core/subsystems/mailrelay/README.md)

## Logging

The Logging subsystem is the centralized logging system that manages all log messages from Hydrogen's subsystems. It provides structured logging with configurable levels, multiple output destinations, and efficient asynchronous processing.

- [README.md](/docs/H/core/subsystems/logging/README.md)

## mDNS Server

The mDNS Server is the network announcer that makes the Hydrogen server visible to other devices on the local network. It enables automatic discovery and connection without manual setup, eliminating the need for users to remember or configure IP addresses.

- [README.md](/docs/H/core/subsystems/mdnsserver/README.md)
- [mdnsserver.md](/docs/H/core/subsystems/mdnsserver/mdnsserver.md)

## mDNS Client

The mDNS Client is the network explorer that searches for and connects to available services on the local network. It enables automatic discovery and integration with other devices and systems, allowing Hydrogen to find and communicate with nearby services without manual configuration.

- [README.md](/docs/H/core/subsystems/mdnsclient/README.md)

## Mirage

The Mirage subsystem is the distributed proxy network that enables secure and seamless remote access to Hydrogen servers. It acts as a bridge between public access points and private Hydrogen instances, allowing users to connect to their devices from anywhere in the world while preserving all customizations, branding, and features.

- [README.md](/docs/H/core/subsystems/mirage/README.md)
- [mirage.md](/docs/H/core/subsystems/mirage/mirage.md)

## Network

The Network subsystem is a core component of the Hydrogen system that manages all aspects of network communication. It acts as the bridge between the system and the outside world, handling everything related to connecting to and communicating over networks, whether that's the internet or local networks within your home or office.

- [README.md](/docs/H/core/subsystems/network/README.md)

## Notify

The Notify subsystem is the notification manager that handles alerts and messages, keeping users and administrators informed about important events and system status changes. It acts as a centralized notification service that receives alerts from various system components and delivers them through configured notification channels.

- [README.md](/docs/H/core/subsystems/notify/README.md)

## OIDC

The OIDC subsystem is Hydrogen's implementation of OpenID Connect (OIDC), a standard protocol for secure authentication and authorization. This subsystem enables Hydrogen to function as an Identity Provider (IdP), managing user identities and controlling access to applications and services.

- [README.md](/docs/H/core/subsystems/oidc/README.md)
- [oidc.md](/docs/H/core/subsystems/oidc/oidc.md)

## Payload

The Payload subsystem is a critical component of the Hydrogen project that manages embedded resources within the executable. It serves as a built-in storage system that holds static files and resources needed by the server to deliver web pages and APIs.

- [README.md](/docs/H/core/subsystems/payload/README.md)

## Print

The Print subsystem is the component of Hydrogen responsible for managing 3D print jobs. It handles the entire lifecycle of print jobs, from submission and queuing to execution and completion. This subsystem communicates with 3D printers, monitors print progress, and provides status updates to ensure reliable and efficient 3D printing operations.

- [README.md](/docs/H/core/subsystems/print/README.md)
- [print.md](/docs/H/core/subsystems/print/print.md)

## Registry

The Registry subsystem serves as the central directory for all components of the Hydrogen system. It maintains a comprehensive record of every subsystem, tracking their current status and ensuring they interact correctly with each other.

- [README.md](/docs/H/core/subsystems/registry/README.md)

## Resources

The Resources subsystem is a core component of the Hydrogen project that manages shared system resources and ensures efficient allocation across all subsystems. Think of it as a central coordinator that handles the distribution and monitoring of critical shared assets like message queues, configuration data, memory buffers, and other components that multiple parts of the system need to access simultaneously.

- [README.md](/docs/H/core/subsystems/resources/README.md)

## Swagger

The Swagger subsystem is the interactive documentation system that provides a user-friendly web interface for exploring and testing the system's API capabilities, making complex technical features accessible to everyone.

- [README.md](/docs/H/core/subsystems/swagger/README.md)

## Terminal

The Terminal subsystem is Hydrogen's web-based command-line interface that provides secure, remote access to the system's shell through your web browser. It allows users to execute commands, manage files, and perform system administration tasks without requiring direct physical access to the server.

- [README.md](/docs/H/core/subsystems/terminal/README.md)

## Threads

The Threads subsystem provides comprehensive monitoring capabilities for the Hydrogen server's threading, memory usage, and file descriptors, offering detailed insights into system performance and resource utilization.

- [threads.md](/docs/H/core/subsystems/threads/threads.md)

## Webserver

The WebServer subsystem is a core component of Hydrogen that provides HTTP-based communication capabilities. It handles REST API endpoints, serves static content, and provides the foundation for other subsystems like WebSocket and Swagger UI.

- [README.md](/docs/H/core/subsystems/webserver/README.md)

## Websocket

The WebSocket subsystem is a core component of the Hydrogen project that enables real-time, two-way communication between the server and connected clients. Unlike traditional web requests that require the client to repeatedly ask the server for updates, WebSockets maintain an open connection that allows the server to push information instantly to clients as events occur.

- [README.md](/docs/H/core/subsystems/websocket/README.md)
- [websocket.md](/docs/H/core/subsystems/websocket/websocket.md)
