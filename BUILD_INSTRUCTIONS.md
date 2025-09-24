# Build Instructions

## Prerequisites

The following packages are required:

- **C++ Compiler**: GCC 7+ or Clang 5+ with C++17 support
- **CMake**: Version 3.12 or higher
- **Git**: For version control
- **pkg-config**: Package configuration tool
- **OpenSSL**: SSL/TLS library and headers
- **libcurl**: HTTP client library and headers
- **JsonCpp**: JSON parsing library and headers
- **WebSocket++**: Header-only WebSocket library
- **Boost**: C++ libraries (optional, depends on WebSocket++ version)

## Build Options

### Option 1: Native Build with CMake

```bash
# Clone or navigate to project directory
cd BlockSys

# Create build directory
mkdir build && cd build

# Configure
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build
make -j$(nproc)

# Run
./main
```

### Option 2: Docker Build

```bash
# Build Docker image
docker build -t binance-orderbook .

# Run the container
docker run -it --rm binance-orderbook

```

## Requirements

- **C++17 compatible compiler** (GCC 7+, Clang 5+)
- **CMake 3.12+**
- **Internet connection** for Binance API access

