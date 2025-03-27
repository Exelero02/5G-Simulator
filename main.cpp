#include <iostream>
#include <vector>
#include <map>
#include <cmath>
#include <random>
#include <memory>
#include <algorithm>
#include <thread>
#include <limits>

constexpr double FREQUENCY_5G_LOW = 600e6;    // 600 MHz (Sub-6 GHz)
constexpr double FREQUENCY_5G_HIGH = 28e9;    // 28 GHz (mmWave)
constexpr double SPEED_OF_LIGHT = 3e8;        // m/s
constexpr double BOLTZMANN_CONST = 1.380649e-23; // Boltzmann constant
constexpr double TEMPERATURE = 290;           // Temperature in Kelvin
constexpr double NOISE_FIGURE = 5;            // Receiver noise figure in dB
constexpr int MAX_CONNECTION_ATTEMPTS = 5;    // Max connection attempts

class UserEquipment;
class NetworkSlice;

class BaseStation {
public:
    struct SignalMetrics {
        double sinr;
        double rsrp;
        double rssi;
    };

    BaseStation(int id, double x, double y, double frequency, double power, double height = 25.0)
            : id(id), x(x), y(y), frequency(frequency), transmitPower(power), height(height),
              antennaGain(10.0) {}

    SignalMetrics calculateSignalMetrics(double ueX, double ueY, double ueHeight = 1.5) const {
        SignalMetrics metrics;
        double distance = std::sqrt(std::pow(x - ueX, 2) + std::pow(y - ueY, 2));

        if (distance == 0) {
            metrics.rsrp = transmitPower;
            metrics.sinr = transmitPower - calculateNoisePower();
            return metrics;
        }

        double pathLoss = calculateUrbanMacroPathLoss(distance, ueHeight);

        // Add log-normal shadowing (8 dB standard deviation)
        static std::default_random_engine generator;
        static std::normal_distribution<double> shadowing(0.0, 8.0);
        double shadowingLoss = shadowing(generator);

        metrics.rsrp = transmitPower - pathLoss + antennaGain - shadowingLoss;
        double interference = calculateInterference(ueX, ueY);
        double noise = calculateNoisePower();
        metrics.sinr = metrics.rsrp - (10 * log10(pow(10, interference/10) + pow(10, noise/10)));

        return metrics;
    }

    double calculateUrbanMacroPathLoss(double distance, double ueHeight) const {
        double dBP = 4 * (height - 1) * (ueHeight - 1) * frequency / SPEED_OF_LIGHT;

        if (distance < dBP) {
            return 28.0 + 22*log10(distance) + 20*log10(frequency/1e9);
        } else {
            return 28.0 + 40*log10(distance) + 20*log10(frequency/1e9) - 9*log10(pow(dBP,2) + pow(distance,2));
        }
    }

    double calculateInterference(double ueX, double ueY) const {
        return -90.0; // Constant value for basic simulation
    }

    double calculateNoisePower() const {
        double bandwidth = 10e6; // 10 MHz bandwidth
        double noisePowerLinear = BOLTZMANN_CONST * TEMPERATURE * bandwidth;
        double noisePowerDb = 10 * log10(noisePowerLinear / 0.001); // Convert to dBm
        return noisePowerDb + NOISE_FIGURE;
    }

    void addSlice(std::shared_ptr<NetworkSlice> slice) {
        slices.push_back(slice);
    }

    // Getters
    int getId() const { return id; }
    double getX() const { return x; }
    double getY() const { return y; }
    double getFrequency() const { return frequency; }

private:
    int id;
    double x, y;
    double frequency;
    double transmitPower;
    double height;
    double antennaGain;
    std::vector<std::shared_ptr<NetworkSlice>> slices;
};

class NetworkSlice {
public:
    enum class SliceType {
        eMBB, URLLC, mMTC
    };

    NetworkSlice(int id, SliceType type, double priority, double bandwidth)
            : id(id), type(type), priority(priority), bandwidth(bandwidth) {}

    double allocateResources(double requestedResources) {
        if (requestedResources < 0.1) return 0;
        double allocated = std::min(requestedResources, bandwidth * priority);
        bandwidth -= allocated;
        return allocated;
    }

    double checkAvailableResources() const {
        return bandwidth * priority;
    }

    void releaseResources(double resources) {
        bandwidth += resources;
    }

    int getId() const { return id; }
    SliceType getType() const { return type; }

    std::string getTypeName() const {
        switch (type) {
            case SliceType::eMBB: return "eMBB";
            case SliceType::URLLC: return "URLLC";
            case SliceType::mMTC: return "mMTC";
            default: return "Unknown";
        }
    }

private:
    int id;
    SliceType type;
    double priority;
    double bandwidth;
};

class UserEquipment {
public:
    struct SliceRequirements {
        double minSinr;
        double minRsrp;
        double bandwidthPriority;
    };

    UserEquipment(int id, double x, double y, double speed,
                  NetworkSlice::SliceType requiredSlice, double requiredBandwidth)
            : id(id), x(x), y(y), speed(speed),
              requiredSlice(requiredSlice), requiredBandwidth(requiredBandwidth),
              connected(false), servingStation(nullptr), allocatedSlice(nullptr),
              connectionAttempts(0) {}

    void move(double timeStep) {
        x += speed * timeStep * (rand() % 3 - 1);
        y += speed * timeStep * (rand() % 3 - 1);
    }

    void connect(std::vector<BaseStation>& stations,
                 std::vector<std::shared_ptr<NetworkSlice>>& slices) {
        connectionAttempts++;
        ConnectionCandidate bestCandidate = evaluatePotentialConnections(stations, slices);

        if (bestCandidate.isViable()) {
            establishConnection(bestCandidate);
        } else {
            handleConnectionFailure(bestCandidate);
        }

        if (!connected && connectionAttempts < MAX_CONNECTION_ATTEMPTS) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100 * connectionAttempts));
        }
    }

    void disconnect() {
        if (connected && allocatedSlice) {
            allocatedSlice->releaseResources(allocatedBandwidth);
            connected = false;
            servingStation = nullptr;
            allocatedSlice = nullptr;
            std::cout << "UE " << id << " disconnected\n";
        }
    }

    // Getters
    int getId() const { return id; }
    bool isConnected() const { return connected; }
    double getX() const { return x; }
    double getY() const { return y; }
    NetworkSlice::SliceType getRequiredSlice() const { return requiredSlice; }

private:
    struct ConnectionCandidate {
        BaseStation* station = nullptr;
        std::shared_ptr<NetworkSlice> slice = nullptr;
        double sinr = -std::numeric_limits<double>::infinity();
        double rsrp = -std::numeric_limits<double>::infinity();
        double availableBandwidth = 0;

        bool isViable() const {
            return station != nullptr && slice != nullptr;
        }
    };

    const std::map<NetworkSlice::SliceType, SliceRequirements> sliceRequirements = {
            {NetworkSlice::SliceType::eMBB, {5.0, -110.0, 0.7}},
            {NetworkSlice::SliceType::URLLC, {10.0, -105.0, 0.9}},
            {NetworkSlice::SliceType::mMTC, {0.0, -120.0, 0.3}}
    };

    ConnectionCandidate evaluatePotentialConnections(
            std::vector<BaseStation>& stations,
            std::vector<std::shared_ptr<NetworkSlice>>& slices) {

        ConnectionCandidate best;
        std::vector<ConnectionCandidate> viableCandidates;

        for (auto& station : stations) {
            BaseStation::SignalMetrics metrics = station.calculateSignalMetrics(x, y);

            if (metrics.sinr < sliceRequirements.at(requiredSlice).minSinr ||
                metrics.rsrp < sliceRequirements.at(requiredSlice).minRsrp) {
                continue;
            }

            for (auto& slice : slices) {
                if (slice->getType() == requiredSlice) {
                    double availableBW = slice->checkAvailableResources();
                    if (availableBW >= requiredBandwidth * 0.5) {
                        viableCandidates.emplace_back(
                                ConnectionCandidate{&station, slice,
                                                    metrics.sinr, metrics.rsrp, availableBW});
                    }
                }
            }
        }

        if (!viableCandidates.empty()) {
            std::sort(viableCandidates.begin(), viableCandidates.end(),
                      [](const ConnectionCandidate& a, const ConnectionCandidate& b) {
                          return (0.7 * a.sinr + 0.2 * a.rsrp + 0.1 * a.availableBandwidth) >
                                 (0.7 * b.sinr + 0.2 * b.rsrp + 0.1 * b.availableBandwidth);
                      });

            best = viableCandidates.front();
        }

        return best;
    }

    void establishConnection(const ConnectionCandidate& candidate) {
        double allocated = candidate.slice->allocateResources(requiredBandwidth);
        if (allocated > 0) {
            connected = true;
            servingStation = candidate.station;
            allocatedSlice = candidate.slice;
            currentSignal = candidate.sinr;
            allocatedBandwidth = allocated;
            connectionAttempts = 0;

            std::cout << "UE " << id << " connected to gNB " << servingStation->getId()
                      << " on " << allocatedSlice->getTypeName() << " slice\n"
                      << "  - Allocated BW: " << allocated << "/" << requiredBandwidth << " MHz\n"
                      << "  - SINR: " << currentSignal << " dB, RSRP: " << candidate.rsrp << " dBm\n";
        } else {
            std::cout << "UE " << id << " failed to allocate resources on "
                      << candidate.slice->getTypeName() << " slice\n";
        }
    }

    void handleConnectionFailure(const ConnectionCandidate& bestCandidate) {
        if (bestCandidate.station) {
            std::cout << "UE " << id << " could not connect (Best Candidate: "
                      << "SINR " << bestCandidate.sinr << " dB, "
                      << "RSRP " << bestCandidate.rsrp << " dBm, "
                      << "BW " << bestCandidate.availableBandwidth << " MHz)\n";
        } else {
            std::cout << "UE " << id << " found no viable stations (Attempt "
                      << connectionAttempts << ")\n";
        }
    }

    int id;
    double x, y;
    double speed;
    NetworkSlice::SliceType requiredSlice;
    double requiredBandwidth;
    bool connected;
    BaseStation* servingStation;
    std::shared_ptr<NetworkSlice> allocatedSlice;
    double currentSignal;
    double allocatedBandwidth;
    int connectionAttempts;
};

class FiveGNetwork {
public:
    FiveGNetwork() {
        rng.seed(std::random_device()());
    }

    void initialize() {
        createBaseStations();
        createNetworkSlices();
        createUserEquipment();
    }

    void runSimulation(int steps) {
        for (int i = 0; i < steps; ++i) {
            auto start = std::chrono::steady_clock::now();
            std::cout << "\n=== Simulation Step " << i + 1 << " ===\n";

            for (auto &ue: ues) {
                ue.move(1.0);

                if (ue.isConnected() && (rand() % 10 == 0)) {
                    ue.disconnect();
                }

                if (!ue.isConnected()) {
                    ue.connect(baseStations, slices);
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
            }

            displayStatus();
        }
    }

private:
    void createBaseStations() {
        baseStations.emplace_back(1, 0, 0, FREQUENCY_5G_LOW, 40);
        baseStations.emplace_back(2, 1000, 1000, FREQUENCY_5G_HIGH, 30);
        baseStations.emplace_back(3, 0, 1000, FREQUENCY_5G_LOW, 40);
        baseStations.emplace_back(4, 1000, 0, FREQUENCY_5G_HIGH, 30);
        std::cout << "Created " << baseStations.size() << " base stations\n";
    }

    void createNetworkSlices() {
        slices.push_back(std::make_shared<NetworkSlice>(1, NetworkSlice::SliceType::eMBB, 0.7, 100));
        slices.push_back(std::make_shared<NetworkSlice>(2, NetworkSlice::SliceType::URLLC, 0.9, 50));
        slices.push_back(std::make_shared<NetworkSlice>(3, NetworkSlice::SliceType::mMTC, 0.3, 200));

        for (auto &station: baseStations) {
            for (auto &slice: slices) {
                station.addSlice(slice);
            }
        }
        std::cout << "Created " << slices.size() << " network slices\n";
    }

    void createUserEquipment() {
        std::uniform_real_distribution<double> dist(0, 1000);
        std::discrete_distribution<int> sliceDist({70, 20, 10});

        for (int i = 1; i <= 50; ++i) {
            NetworkSlice::SliceType type;
            switch (sliceDist(rng)) {
                case 0: type = NetworkSlice::SliceType::eMBB; break;
                case 1: type = NetworkSlice::SliceType::URLLC; break;
                case 2: type = NetworkSlice::SliceType::mMTC; break;
            }

            double bandwidth = 5 + (rand() % 20);
            ues.emplace_back(i, dist(rng), dist(rng), 1 + (rand() % 5), type, bandwidth);
        }
        std::cout << "Created " << ues.size() << " user equipment instances\n";
    }

    void displayStatus() {
        int connected = std::count_if(ues.begin(), ues.end(),
                                      [](const UserEquipment &ue) { return ue.isConnected(); });

        std::cout << "Network Status: " << connected << "/" << ues.size()
                  << " UEs connected (" << (100.0 * connected / ues.size()) << "%)\n";

        std::map<NetworkSlice::SliceType, int> sliceCounts;
        for (const auto &ue: ues) {
            if (ue.isConnected()) {
                sliceCounts[ue.getRequiredSlice()]++;
            }
        }

        std::cout << "Slice Distribution:\n";
        for (const auto &[type, count]: sliceCounts) {
            std::string name;
            switch (type) {
                case NetworkSlice::SliceType::eMBB: name = "eMBB"; break;
                case NetworkSlice::SliceType::URLLC: name = "URLLC"; break;
                case NetworkSlice::SliceType::mMTC: name = "mMTC"; break;
            }
            std::cout << "  " << name << ": " << count << " UEs\n";
        }
    }

    std::vector<BaseStation> baseStations;
    std::vector<std::shared_ptr<NetworkSlice>> slices;
    std::vector<UserEquipment> ues;
    std::mt19937 rng;
};

int main() {
    FiveGNetwork network;
    network.initialize();
    network.runSimulation(10);
    return 0;
}