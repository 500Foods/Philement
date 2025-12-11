# WebSocket Subsystem

## What is the WebSocket Subsystem?

The WebSocket subsystem is a core component of the Hydrogen project that enables real-time, two-way communication between the server and connected clients. Unlike traditional web requests that require the client to repeatedly ask the server for updates, WebSockets maintain an open connection that allows the server to push information instantly to clients as events occur.

## Why is it Important?

WebSockets are essential for applications that need immediate updates and interactive features. In the context of Hydrogen, this subsystem powers:

- **Live Status Updates**: Clients receive real-time information about server performance, active connections, and system health without needing to refresh or poll the server.
- **Print Job Monitoring**: Users can see live progress of 3D printing jobs, temperature changes, and completion notifications as they happen.
- **Interactive Control**: Enables direct, responsive control of printers and other devices through the web interface.
- **Notifications**: Instant alerts for important events like job completions, errors, or system changes.

This real-time capability transforms the user experience from static page refreshes to dynamic, live interactions, making Hydrogen more responsive and user-friendly for managing 3D printing operations.