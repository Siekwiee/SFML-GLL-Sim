#pragma once
#include <modbus.h>
#include <string>
#include <vector>
#include <mutex>
#include <thread>
#include <atomic>
#include "Sim.hpp"

class ModbusManager {
public:
    ModbusManager();
    ~ModbusManager();

    // Connection settings
    void setIp(const std::string& ip) { ip_ = ip; }
    void setPort(int port) { port_ = port; }
    void setSlaveId(int id) { slaveId_ = id; }

    const std::string& getIp() const { return ip_; }
    int getPort() const { return port_; }
    int getSlaveId() const { return slaveId_; }

    bool connect();
    void disconnect();
    bool isConnected() const { return connected_; }
    const std::string& getLastError() const { return lastError_; }

    // Synchronize simulator with Modbus
    // Reads inputs from Modbus and sets them in Simulator
    // Reads outputs from Simulator and writes them to Modbus Coils
    void sync(Simulator& sim);

private:
    std::string ip_ = "127.0.0.1";
    int port_ = 502;
    int slaveId_ = 1;

    modbus_t* ctx_ = nullptr;
    std::atomic<bool> connected_{false};
    std::string lastError_;
    
    // Buffers for I/O
    uint8_t inputBits_[16];       // Discrete Inputs (read-only)
    uint8_t coilBits_[16];        // Coils (read-write)
};


