# mDNS Server

## What is the mDNS Server?

The mDNS Server is the network announcer that makes the Hydrogen server visible to other devices on the local network. It enables automatic discovery and connection without manual setup, eliminating the need for users to remember or configure IP addresses.

## Why is it Important?

In traditional networking, connecting to a device like a 3D printer requires knowing its exact IP address, which can be cumbersome and error-prone. The mDNS Server solves this by broadcasting the printer's availability using multicast DNS (mDNS) protocol, allowing client applications and other network devices to automatically find and connect to the Hydrogen server.

## Key Features

- **Automatic Discovery**: Printers announce themselves on the network
- **Service Advertisement**: Publishes available endpoints like web interfaces and WebSocket connections
- **Zero Configuration**: No manual network setup required
- **Local Network Focus**: Designed specifically for secure, local network environments

This subsystem is essential for creating a plug-and-play experience where users can simply connect their devices to the same network and immediately access the Hydrogen printer's services.