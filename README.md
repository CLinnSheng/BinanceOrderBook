# Local Order Book Implementation Documentation

## Build Instructions

### Prerequisites

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

### Build Options

#### Option 1: Native Build with CMake

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

#### Option 2: Docker Build

```bash
# Build Docker image
docker build -t binance-orderbook .

# Run the container
docker run -it --rm binance-orderbook

```

### Requirements

- **C++17 compatible compiler** (GCC 7+, Clang 5+)
- **CMake 3.12+**
- **Internet connection** for Binance API access

## Overview

The orderbook implementation is based on the official Binance documentation

## Program Architecture & Flow

### High-Level Architecture

```
┌─────────────────┐    ┌──────────────────┐    ┌─────────────────┐
│   OrderBook     │───▶│ OrderBookManager │───▶│ OrderBookUI     │
│   (Main)        │    │   (Coordinator)  │    │   (Display)     │
└─────────────────┘    └──────────────────┘    └─────────────────┘
         │                       │                       ▲
         ▼                       ▼                       │
┌─────────────────┐    ┌──────────────────┐             │
│   WebSocket     │    │OrderBookSync     │─────────────┘
│   (Network)     │    │  (Protocol)      │
└─────────────────┘    └──────────────────┘
         │                       ▲
         └───────────────────────┘
```

### Program Flow

#### 1. Initialization Phase

```cpp
// main.cpp or OrderBook::run()
OrderBook orderBook(symbol);
├── OrderBookSynchronizer synchronizer(symbol)
├── OrderBookManager orderBookManager
├── WebSocket ws(avgPrice, orderBookManager, synchronizer, symbol)
└── OrderBookUI ui(avgPrice, orderBookManager, symbol)
```

#### 2. Startup Sequence

```cpp
void OrderBook::run() {
    // 1. Start synchronizer (begins protocol)
    synchronizer.start();

    // 2. Start WebSocket connection
    ws.start();

    // 3. Wait for synchronization
    while (!synchronizer.isSynchronized()) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    // 4. Start UI
    ui.start();
}
```

#### 3. Data Flow During Operation

```
WebSocket Events → OrderBookSynchronizer → OrderBookManager → OrderBookUI
                                       ↘
                                    AveragePrice → OrderBookUI
```

## Threading Architecture

### Thread Overview

The application uses **3 main threads** for optimal performance:

```
Main Thread                WebSocket Thread       Sync Thread
    │                          │                    │
    ├─ Coordination            ├─ Network I/O       ├─ Protocol
    ├─ Startup/Shutdown        ├─ Event Reception   ├─ Validation
    ├─ UI Rendering            ├─ Message Parsing   ├─ State Mgmt
    ├─ User Input              └─ Event Forwarding  └─ Order Book Updates
    └─ Signal Handling
```

### 1. Main Thread

**File**: `src/OrderBook.cpp:27-62`

```cpp
void OrderBook::run() {
    // Coordinates startup sequence
    synchronizer.start();    // Creates Sync Thread
    ws.start();             // Creates WebSocket Thread

    // Wait for synchronization
    while (!synchronizer.isSynchronized()) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    ui.start();             // Blocks on UI event loop (main thread)
}
```

**Responsibilities**:
- System initialization and coordination
- Signal handling (Ctrl+C)
- Startup sequence orchestration
- **UI rendering and event loop** (blocks on `screen.Loop()`)
- User input handling
- Graceful shutdown coordination

### 2. WebSocket Thread

**File**: `src/WebSocket.cpp:245`

```cpp
void WebSocket::start() {
    running.store(true);

    // Connect to WebSocket
    client::connection_ptr con = ws_client.get_connection(depth_uri, ec);
    ws_client.connect(con);

    // Start dedicated thread for WebSocket I/O
    ws_thread = std::thread([this]() {
        ws_client.run();  // Blocks on WebSocket events
    });
}
```

**Responsibilities**:
- WebSocket connection management
- Network I/O operations
- Message reception and parsing
- Event forwarding to synchronizer

**Thread Safety**:
- Uses atomic `running` flag for shutdown coordination
- Thread-safe message forwarding through synchronizer

### 3. Synchronization Thread

**File**: `src/OrderBookSynchronizer.cpp:27`

```cpp
void OrderBookSynchronizer::start() {
    running.store(true);

    // Start background processor
    processingThread = std::thread(&OrderBookSynchronizer::backgroundProcessor, this);

    // Request initial snapshot (async)
    requestSnapshot();
}
```

**Background Processor Loop**:

```cpp
void OrderBookSynchronizer::backgroundProcessor() {
    while (running.load()) {
        switch (state.load()) {
            case SyncState::BUFFERING:
                // Check for snapshot completion
                if (snapshotFuture.valid() &&
                    snapshotFuture.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
                    DepthSnapshot snapshot = snapshotFuture.get();
                    handleSnapshotReceived(snapshot);
                }
                break;

            case SyncState::SNAPSHOT_RECEIVED:
                // Process buffered events
                processEventBuffer();
                break;

            case SyncState::ERROR_STATE:
                // Attempt recovery
                reset();
                break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}
```

**Responsibilities**:

- Binance protocol state management
- Event buffering and validation
- Snapshot processing and application
- Error detection and recovery
- Order book state updates

**Thread Safety**:

- Mutex-protected order book (`orderBookMutex`)
- Mutex-protected event buffer (`bufferMutex`)
- Atomic state variables
- Lock-free where possible for performance


### Thread Lifecycle Management

#### Startup Order

1. **Main Thread** creates all components
2. **Sync Thread** starts background processor
3. **WebSocket Thread** connects and starts I/O
4. **Main Thread** waits for synchronization, then starts UI event loop

### Data Flow Diagram

```
┌─────────────────┐
│ Binance WebSocket│
│ (Network Layer) │
└─────────┬───────┘
          │ Raw JSON Events
          ▼
┌─────────────────┐
│   WebSocket     │◄─── Thread 2: WebSocket I/O
│   Message       │
│   Handler       │
└─────────┬───────┘
          │ Parsed Events
          ▼
┌─────────────────┐
│OrderBookSync    │◄─── Thread 3: Protocol Management
│ • Event Buffer  │
│ • State Machine │
│ • Validation    │
└─────────┬───────┘
          │ Validated Updates
          ▼
┌─────────────────┐
│ OrderBookMgr    │◄─── Delegation Layer
│ • Coordination  │
│ • Callbacks     │
└─────────┬───────┘
          │ UI Updates
          ▼
┌─────────────────┐
│ OrderBookUI     │◄─── Main Thread: Display
│ • Rendering     │
│ • User Input    │
└─────────────────┘
```

### Callback Chain

```cpp
// 1. Synchronizer triggers update
synchronizer.applyDepthEvent() → updateCallback()

// 2. OrderBookManager forwards to UI
orderBookManager.updateCallback() → screen.PostEvent()

// 3. UI redraws with fresh data
ui.Render() → orderBookManager.getTopLevels()
```

