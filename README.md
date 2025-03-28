# 5G Network Simulation

## 📌 Overview

This C++ project simulates a 5G network environment with multiple base stations, network slices, and user equipment (UE) instances. The simulation models key aspects of 5G networks including radio signal propagation, network slicing, dynamic resource allocation, and UE mobility.

## 🚀 Features
| Feature | Description |
|---------|-------------|
| **Base Station (gNB) modeling** | with configurable parameters |
| **Network slicing** | for eMBB, URLLC, and mMTC services |
| **Realistic signal propagation** | with path loss and shadowing |
| **Dynamic UE movement** | and connection management |
| **Resource allocation** | based on slice priorities |
| **Multi-threaded simulation** | for concurrent connection attempts |

## Prerequisites
- C++17 compatible compiler (GCC, Clang, or MSVC)
- CMake 3.12+ (recommended)
- Basic understanding of 5G network concepts

## Build Instructions

### Using CMake (recommended)
```bash
mkdir build
cd build
cmake ..
make
```

## 🏗️ System Architecture

## System Architecture

```mermaid
classDiagram
    direction TB

    class FiveGNetwork {
        -vector<BaseStation> baseStations
        -vector<shared_ptr<NetworkSlice>> slices
        -vector<UserEquipment> ues
        +initialize()
        +runSimulation(steps)
    }

    class BaseStation {
        -int id
        -double x, y, frequency
        -vector<shared_ptr<NetworkSlice>> slices
        +calculateSignalMetrics() SignalMetrics
        +calculateUrbanMacroPathLoss() double
    }

    class NetworkSlice {
        <<enumeration>>
        SliceType
        eMBB
        URLLC
        mMTC
        -int id
        -SliceType type
        -double priority
        -double bandwidth
        +allocateResources() double
    }

    class UserEquipment {
        -int id
        -double x, y, speed
        -SliceType requiredSlice
        +move()
        +connect()
        +disconnect()
    }

    FiveGNetwork "1" *-- "1..*" BaseStation
    FiveGNetwork "1" *-- "1..*" NetworkSlice
    FiveGNetwork "1" *-- "1..*" UserEquipment
    BaseStation "1" *-- "0..*" NetworkSlice
    UserEquipment "1" -- "0..1" BaseStation
    UserEquipment "1" -- "0..1" NetworkSlice
```

## Key Components
### BaseStation Class
-Represents a 5G base station (gNB) with specific location and transmission characteristics
-Calculates signal metrics (SINR, RSRP, RSSI) for UEs
-Implements urban macro path loss model (3GPP TR 38.901

### NetworkSlice
-Represents logical network slices
-Handles resource allocation based on priority
-Supports three slice types: eMBB, URLLC, mMTC

### UserEquipment
-Simulates mobile devices with movement
-Implements connection logic and requirements
-Manages slice-specific QoS needs

### FiveGNetwork
-Orchestrates the overall simulation
-Coordinates interactions between components
-Provides simulation status reporting

 ### Technical Details
-Signal Propagation: Uses 3GPP Urban Macro path loss model
-Resource Allocation: Priority-based weighted fair queuing
-Concurrency: Thread-safe connection attempts
-Randomness: Mersenne Twister PRNG for realistic variations

## Key Relationships
### FiveGNetwork orchestrates all components

### BaseStations contain multiple NetworkSlices

### UserEquipment connects to:
One **BaseStation** (serving cell)    
One **NetworkSlice** (service type)

## Connection Signal Flow

```mermaid
sequenceDiagram
    participant UE as UserEquipment
    participant gNB as BaseStation
    participant Slice as NetworkSlice

    UE->>gNB: calculateSignalMetrics(x,y)
    activate gNB
        Note right of gNB: Calls calculateUrbanMacroPathLoss() + shadowing (8dB stddev)
        gNB-->>UE: SignalMetrics{sinr, rsrp}
    deactivate gNB

    UE->>Slice: allocateResources(requiredBandwidth)
    activate Slice
    alt Resources available
        Slice->>Slice: bandwidth -= allocated
        Slice-->>UE: allocatedBandwidth
        UE->>gNB: servingStation = this
        UE->>Slice: allocatedSlice = this
    else Insufficient resources
        Slice-->>UE: 0.0 (rejection)
        UE->>UE: connectionAttempts++
    end
    deactivate Slice

    loop Movement & Reconnection
        UE->>UE: move() with random displacement
        opt Connection drops
            UE->>Slice: releaseResources()
            UE->>gNB: servingStation = nullptr
        end
    end
```
