# Docker Setup Guide

This guide explains how to set up and run Hydrogen within a Docker container using Alpine Linux as the base image.

## Base Image

Start with the official Alpine Linux base image:

```dockerfile
FROM alpine:latest
```

## Installing Dependencies

Hydrogen requires several dependencies that can be installed using Alpine's package manager `apk`. Add the following to your Dockerfile:

```dockerfile
RUN apk update && apk add --no-cache \
    gcc \
    musl-dev \
    make \
    jansson-dev \
    libmicrohttpd-dev \
    openssl-dev \
    libwebsockets-dev

# Note: pthread and libm are part of musl-dev
```

## Building Hydrogen

Once the dependencies are installed, you can build Hydrogen:

```dockerfile
# Copy the Hydrogen source code
COPY . /hydrogen

# Set working directory
WORKDIR /hydrogen

# Build Hydrogen
RUN make
```

## Running Hydrogen

After building, you can run Hydrogen:

```dockerfile
# Expose necessary ports (adjust as needed)
EXPOSE 8080

# Run Hydrogen
CMD ["./hydrogen"]
```

## Complete Dockerfile Example

Here's a complete example of a Dockerfile that builds and runs Hydrogen:

```dockerfile
# Use Alpine Linux as base image
FROM alpine:latest

# Install dependencies
RUN apk update && apk add --no-cache \
    gcc \
    musl-dev \
    make \
    jansson-dev \
    libmicrohttpd-dev \
    openssl-dev \
    libwebsockets-dev

# Copy Hydrogen source code
COPY . /hydrogen

# Set working directory
WORKDIR /hydrogen

# Build Hydrogen
RUN make

# Expose port
EXPOSE 8080

# Run Hydrogen
CMD ["./hydrogen"]
```

## Building the Docker Image

To build the Docker image:

```bash
docker build -t hydrogen .
```

## Running the Container

To run the Hydrogen container:

```bash
docker run -p 8080:8080 hydrogen
```

## Development Tips

- For development and debugging, you can add additional development tools:

  ```dockerfile
  RUN apk add --no-cache \
      gdb \
      valgrind
  ```

- To use the debug build:

  ```dockerfile
  RUN make debug
  CMD ["./hydrogen_debug"]
  ```

- For Valgrind testing:

  ```dockerfile
  RUN make valgrind
  CMD ["valgrind", "./hydrogen_valgrind"]
  ```

## Notes

- The Alpine Linux base image is chosen for its small size and security focus
- All dependencies are installed using `--no-cache` to keep the image size minimal
- The build process follows the standard Makefile configuration
- Port 8080 is exposed by default, adjust as needed for your configuration
