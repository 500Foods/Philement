# Hydrogen Configuration Guide

## Intended Audience

This guide is written for users setting up and managing a Hydrogen 3D printer control server. While some technical concepts are covered, we aim to explain everything in clear, accessible terms. No programming or system administration experience is required to understand and use this guide.

## Introduction

The Hydrogen server provides network-based control and monitoring for 3D printers. It uses a configuration file to control various aspects of its operation, from basic settings like network ports to advanced features like printer discovery on your network. This guide will help you understand and customize these settings for your needs.

For instructions on running Hydrogen as a system service that starts automatically with your computer, see the [Service Setup Guide](/docs/H/core/reference/service.md).

## Using a Custom Configuration File

By default, Hydrogen looks for its settings in a file named `hydrogen.json` in the same folder as the program. You can use a different configuration file by providing its path as the first argument when starting Hydrogen:

```bash
./hydrogen /path/to/your/config.json
```

[Rest of the content remains the same...]
