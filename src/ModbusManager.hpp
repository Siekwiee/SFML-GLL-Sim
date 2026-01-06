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

    // Helpers for digital I/O
    void setNumInputs(int n);
    void setNumOutputs(int n);
    int getNumInputs() const { return numInputs_; }
    int getNumOutputs() const { return numOutputs_; }
    
    // Helpers for analog I/O (registers)
    void setNumAnalogInputs(int n);
    void setNumAnalogOutputs(int n);
    int getNumAnalogInputs() const { return numAnalogInputs_; }
    int getNumAnalogOutputs() const { return numAnalogOutputs_; }

    // Persistent settings management
    void loadConfig();
    void saveConfig();

private:
    std::string ip_ = "127.0.0.1";
    int port_ = 502;
    int slaveId_ = 1;

    modbus_t* ctx_ = nullptr;
    std::atomic<bool> connected_{false};
    std::string lastError_;
    
    // Buffers for digital I/O (bits)
    int numInputs_ = 8;
    int numOutputs_ = 8;
    std::vector<uint8_t> inputBits_;
    std::vector<uint8_t> coilBits_;
    
    // Buffers for analog I/O (16-bit registers, we use lower 8 bits)
    int numAnalogInputs_ = 0;
    int numAnalogOutputs_ = 0;
    std::vector<uint16_t> inputRegisters_;
    std::vector<uint16_t> holdingRegisters_;
};


