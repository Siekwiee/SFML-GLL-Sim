#include "ModbusManager.hpp"
#include <iostream>

ModbusManager::ModbusManager() {
    for (int i = 0; i < 16; ++i) {
        inputBits_[i] = 0;
        coilBits_[i] = 0;
    }
}

ModbusManager::~ModbusManager() {
    disconnect();
}

bool ModbusManager::connect() {
    disconnect();

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
    int rc = modbus_read_input_bits(ctx_, 0, 8, inputBits_);
    if (rc != -1) {
        for (int i = 0; i < 8; ++i) {
            std::string signalName = "INPUT_" + std::to_string(i);
            sim.setSignal(signalName, inputBits_[i] != 0);
        }
    } else {
        lastError_ = std::string("Read error: ") + modbus_strerror(errno);
        // Maybe auto-disconnect on fatal error?
    }

    // Read from Simulator and write to Coils (Actuators) - Address 0, Write 8 bits
    bool changed = false;
    for (int i = 0; i < 8; ++i) {
        std::string signalName = "OUTPUT_" + std::to_string(i);
        uint8_t val = sim.getSignalValue(signalName) ? 1 : 0;
        if (coilBits_[i] != val) {
            coilBits_[i] = val;
            changed = true;
        }
    }

    if (changed) {
        rc = modbus_write_bits(ctx_, 0, 8, coilBits_);
        if (rc == -1) {
            lastError_ = std::string("Write error: ") + modbus_strerror(errno);
        }
    }
}


