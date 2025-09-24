# Docker Containerization Guide

## Overview

The Binance OrderBook application has been containerized using Docker for easy deployment and consistent runtime environments. The Docker configuration uses a simple single-stage build that's easy to understand and maintain.

## Quick Start

```bash
# Build the image
docker build -t binance-orderbook .

# Run the container with default symbol (BTCUSDT)
docker run -it --rm binance-orderbook

# Run with a different trading symbol
docker run -it --rm binance-orderbook ETHUSDT

# Run with interactive shell access
docker run -it --rm --entrypoint=/bin/bash binance-orderbook
```

## Configuration

### Trading Symbol

The application accepts a trading symbol as a command-line argument:

```bash
# Run with different symbols
docker run -it --rm binance-orderbook ETHUSDT
docker run -it --rm binance-orderbook ADAUSDT
docker run -it --rm binance-orderbook SOLUSDT
```

### Environment Variables

Available environment variables:

- `TERM=xterm-256color` - Terminal type for proper UI rendering

## Docker Image Details

### Simple Single-Stage Build

The Dockerfile uses a straightforward single-stage build:

- **Base**: `ubuntu:22.04` with C++17 support
- **Build tools**: cmake, g++, pkg-config
- **Dependencies**: libssl-dev, libcurl4-openssl-dev, libjsoncpp-dev
- **Final image size**: ~600MB (includes build tools for flexibility)

### Features

- **Root execution**: Simple and straightforward
- **All tools included**: Can rebuild or modify in container
- **No exposed ports**: Terminal application doesn't need network ports

### Dependencies

Dependencies included:
- `libssl-dev` - TLS/SSL support for WebSocket connections
- `libcurl4-openssl-dev` - HTTP client for REST API calls
- `libjsoncpp-dev` - JSON parsing library

## Production Deployment

### Docker Swarm / Kubernetes

For production deployment, consider:

```yaml
# Example Kubernetes deployment
apiVersion: apps/v1
kind: Deployment
metadata:
  name: orderbook-deployment
spec:
  replicas: 1
  selector:
    matchLabels:
      app: orderbook
  template:
    metadata:
      labels:
        app: orderbook
    spec:
      containers:
      - name: orderbook
        image: binance-orderbook:latest
        args: ["BTCUSDT"]
        resources:
          requests:
            memory: "64Mi"
            cpu: "100m"
          limits:
            memory: "128Mi"
            cpu: "200m"
```

### Resource Requirements

**Minimum Requirements:**
- CPU: 100m (0.1 core)
- Memory: 64MB
- Network: Outbound HTTPS (443) access to Binance APIs

**Recommended:**
- CPU: 200m (0.2 core)
- Memory: 128MB

### Logging

For production logging, enable Docker logging:

```yaml
services:
  orderbook:
    logging:
      driver: "json-file"
      options:
        max-size: "10m"
        max-file: "3"
```

## Development

### Development Override

Create `docker-compose.override.yml` for development:

```yaml
services:
  orderbook:
    volumes:
      - .:/app/src:ro
    build:
      target: builder
    command: ["bash"]
```

### Debugging

```bash
# Access container shell
docker-compose exec orderbook bash

# View build logs
docker-compose build --no-cache

# Check container health
docker-compose ps
docker-compose logs orderbook
```

## Networking

### Binance API Access

The application requires outbound internet access to:
- `stream.binance.com:9443` (WebSocket)
- `api.binance.com:443` (REST API)

### Firewall Rules

If running behind a firewall, allow outbound connections:
```bash
# Example iptables rules
iptables -A OUTPUT -p tcp --dport 443 -d api.binance.com -j ACCEPT
iptables -A OUTPUT -p tcp --dport 9443 -d stream.binance.com -j ACCEPT
```

## Troubleshooting

### Common Issues

1. **Build Failures**
   ```bash
   # Clear Docker cache
   docker system prune -a
   docker-compose build --no-cache
   ```

2. **UI Display Issues**
   ```bash
   # Ensure TTY allocation
   docker run -it binance-orderbook

   # Check terminal environment
   docker run -it --rm binance-orderbook bash -c 'echo $TERM'
   ```

3. **Network Connectivity**
   ```bash
   # Test connectivity from container
   docker run -it --rm binance-orderbook bash -c 'curl -I https://api.binance.com/api/v3/ping'
   ```

### Health Checks

Add health check to docker-compose.yml:

```yaml
services:
  orderbook:
    healthcheck:
      test: ["CMD", "curl", "-f", "https://api.binance.com/api/v3/ping"]
      interval: 30s
      timeout: 10s
      retries: 3
      start_period: 10s
```

## Performance Optimization

### Image Size Optimization

The current configuration already includes:
- Multi-stage build (reduces size by ~85%)
- Minimal base image
- Package cache cleanup

### Runtime Optimization

For better performance:
```bash
# Increase container resources
docker run -it --rm --memory=256m --cpus=0.5 binance-orderbook

# Use read-only filesystem for security
docker run -it --rm --read-only --tmpfs /tmp binance-orderbook
```