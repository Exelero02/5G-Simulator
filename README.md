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
    participant UE as User Equipment
    participant gNB as Base Station
    participant Slice as Network Slice
    participant Core as 5G Core

    UE->>gNB: Measurement Report (RSRP/RSRQ/SINR)
    activate gNB
        gNB->>gNB: Calculate Path Loss
        gNB->>gNB: Add Shadowing Effects
        gNB-->>UE: Signal Metrics (RSRP: -85dBm, SINR: 18dB)
    deactivate gNB

    UE->>Slice: Slice Request (Type: eMBB, BW: 20MHz)
    activate Slice
        alt Slice Available
            Slice->>Slice: Check Priority & Bandwidth
            Slice-->>UE: Allocation Grant (15MHz)
        else Slice Congested
            Slice-->>UE: Reject (Insufficient Resources)
        end
    deactivate Slice

    UE->>gNB: RRC Connection Request
    activate gNB
        gNB->>Core: Authentication Request
        Core-->>gNB: Authentication Response
        gNB-->>UE: RRC Connection Setup
    deactivate gNB

    UE->>gNB: Data Transmission
    loop Every TTI
        gNB->>Slice: QoS Monitoring
        Slice-->>gNB: Adjust Allocation
    end
```

