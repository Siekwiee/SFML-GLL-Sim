#include "ModbusManager.hpp"
#include <iostream>
#include <fstream>
#include <sstream>

ModbusManager::ModbusManager() {
    loadConfig();
    inputBits_.resize(numInputs_, 0);
    coilBits_.resize(numOutputs_, 0);
}

ModbusManager::~ModbusManager() {
    saveConfig();
    disconnect();
}

void ModbusManager::setNumInputs(int n) {
    if (n < 1) n = 1;
    if (n > 512) n = 512;
    numInputs_ = n;
    inputBits_.resize(numInputs_, 0);
}

void ModbusManager::setNumOutputs(int n) {
    if (n < 1) n = 1;
    if (n > 512) n = 512;
    numOutputs_ = n;
    coilBits_.resize(numOutputs_, 0);
}

void ModbusManager::loadConfig() {
    std::ifstream f("modbus_config.txt");
    if (!f.is_open()) return;

    std::string line;
    while (std::getline(f, line)) {
        std::istringstream iss(line);
        std::string key, value;
        if (std::getline(iss, key, '=') && std::getline(iss, value)) {
            try {
                if (key == "ip") ip_ = value;
                else if (key == "port") port_ = std::stoi(value);
                else if (key == "slave_id") slaveId_ = std::stoi(value);
                else if (key == "num_inputs") numInputs_ = std::stoi(value);
                else if (key == "num_outputs") numOutputs_ = std::stoi(value);
            } catch (...) {}
        }
    }
}

void ModbusManager::saveConfig() {
    std::ofstream f("modbus_config.txt");
    if (!f.is_open()) return;

    f << "ip=" << ip_ << "\n";
    f << "port=" << port_ << "\n";
    f << "slave_id=" << slaveId_ << "\n";
    f << "num_inputs=" << numInputs_ << "\n";
    f << "num_outputs=" << numOutputs_ << "\n";
}

bool ModbusManager::connect() {
    disconnect();
    saveConfig();

    ctx_ = modbus_new_tcp(ip_.c_str(), port_);
    if (!ctx_) {
        lastError_ = "Failed to create modbus context";
        return false;
    }

    modbus_set_slave(ctx_, slaveId_);

    if (modbus_connect(ctx_) == -1) {
        lastError_ = std::string("Connection failed: ") + modbus_strerror(errno);
        modbus_free(ctx_);
        ctx_ = nullptr;
        return false;
    }

    connected_ = true;
    lastError_ = "";
    return true;
}

void ModbusManager::disconnect() {
    if (ctx_) {
        modbus_close(ctx_);
        modbus_free(ctx_);
        ctx_ = nullptr;
    }
    connected_ = false;
}

void ModbusManager::sync(Simulator& sim) {
    if (!connected_ || !ctx_) return;

    // Read Discrete Inputs (Sensors) - Address 0, Read 8 bits
    int rc = modbus_read_input_bits(ctx_, 0, numInputs_, inputBits_.data());
    if (rc != -1) {
        for (int i = 0; i < numInputs_; ++i) {
            std::string signalName = "INPUT_" + std::to_string(i);
            sim.setSignal(signalName, inputBits_[i] != 0);
        }
    } else {
        lastError_ = std::string("Read error: ") + modbus_strerror(errno);
        // Maybe auto-disconnect on fatal error?
    }

    // Read from Simulator and write to Coils (Actuators) - Address 0, Write 8 bits
    bool changed = false;
    for (int i = 0; i < numOutputs_; ++i) {
        std::string signalName = "OUTPUT_" + std::to_string(i);
        uint8_t val = sim.getSignalValue(signalName) ? 1 : 0;
        if (coilBits_[i] != val) {
            coilBits_[i] = val;
            changed = true;
        }
    }

    if (changed) {
        rc = modbus_write_bits(ctx_, 0, numOutputs_, coilBits_.data());
        if (rc == -1) {
            lastError_ = std::string("Write error: ") + modbus_strerror(errno);
        }
    }
}


