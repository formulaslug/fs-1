// Copyright (c) 2018 Formula Slug. All Rights Reserved.

#pragma once

#include <stdint.h>
#include <cstring>

#include "ch.h"
#include "hal.h"
#include "mcuconfFs.h"

// SIDs From ECU
constexpr uint32_t kFuncIdHeartBeatECU = 0x700;
// ADC
constexpr uint32_t kFuncIdBrake = 0x020;
constexpr uint32_t kFuncIdThrottle = 0x021;
constexpr uint32_t kFuncIdSteering = 0x022;
// Digital
constexpr uint32_t kFuncIdDigital = 0x022;

// SIDs From Accumulator
constexpr uint32_t kFuncIdHeartBeatAcc = 0x701;
constexpr uint32_t kFuncIdCellTempAdc[4] = {0x002, 0x003, 0x004, 0x005};
constexpr uint32_t kFuncIdCellVoltage[4] = {0x012, 0x013, 0x014, 0x015};
constexpr uint32_t kFuncIdFaultStatuses = 0x006;
constexpr uint32_t kFuncIdPackVoltage = 0x201;
constexpr uint32_t kFuncIdPackCurrent = 0x202;
constexpr uint32_t kFuncIdEnergy = 0x203;
constexpr uint32_t kFuncIdPackResistance = 0x204;

// Payload constants
constexpr uint32_t kPayloadHeartbeat = 0x1;

// TODO: Make COB-ID naming match CANOpen convention
constexpr uint32_t kFuncIdHeartbeat = 0x100;

struct HeartbeatMessage : public CANTxFrame {
  explicit HeartbeatMessage(uint16_t data) {
    IDE = CAN_IDE_STD;
    RTR = CAN_RTR_DATA;
    SID = kFuncIdHeartBeatECU;
    DLC = 2;
    data8[0] = (data >> 8) & 0xFF;  // MSB (32's 3rd byte)
    data8[1] = data & 0xFF;         // LSB (32's 4th byte)
  }
};

// ADC MESSAGES
struct ThrottleMessage : public CANTxFrame {
  explicit ThrottleMessage(uint16_t throttleVoltage, uint8_t forwardSwitch, uint8_t reverseSwitch) {
    IDE = CAN_IDE_EXT;
    RTR = CAN_RTR_DATA;
    SID = kFuncIdThrottle;
    DLC = 4;
    data8[0] = throttleVoltage >> 8;  // MSB (32's 3rd byte) (left most byte in DVT)
    data8[1] = throttleVoltage;
    data8[2] = forwardSwitch;
    data8[3] = reverseSwitch; 
  }
};

struct SteeringMessage : public CANTxFrame {
  explicit SteeringMessage(uint8_t steeringAngle) {
    IDE = CAN_IDE_STD;
    RTR = CAN_RTR_DATA;
    SID = kFuncIdSteering;
    DLC = 1;
    data8[0] = steeringAngle;
  }
};

struct BrakeMessage : public CANTxFrame {
  explicit BrakeMessage(uint8_t brakeVal) {
    IDE = CAN_IDE_STD;
    RTR = CAN_RTR_DATA;
    SID = kFuncIdBrake;
    DLC = 1;
    data8[0] = brakeVal;  // MSB (32's 3rd byte) (left most byte in DVT)
  }
};

struct DigitalMessage : public CANTxFrame {
  explicit DigitalMessage(uint8_t digitalState) {
    IDE = CAN_IDE_STD;
    RTR = CAN_RTR_DATA;
    SID = kFuncIdDigital;
    DLC = 1;
    data8[0] = digitalState;  // MSB (32's 3rd byte) (left most byte in DVT)
  }
};

struct BMSVoltageMessage : public CANTxFrame {
  explicit BMSVoltageMessage(uint8_t row, uint8_t *voltages) {
    IDE = CAN_IDE_STD;
    RTR = CAN_RTR_DATA;
    SID = kFuncIdCellVoltage[row];
    DLC = 7;
    for (int i = 0; i < 7; i++) {
      data8[i] = voltages[i];
    }
  }
};

struct BMSTempMessage : public CANTxFrame {
  explicit BMSTempMessage(uint8_t row, uint8_t *temps) {
    IDE = CAN_IDE_STD;
    RTR = CAN_RTR_DATA;
    SID = kFuncIdCellTempAdc[row];
    DLC = 7;
    for (int i = 0; i < 7; i++) {
      data8[i] = temps[i];
    }
  }
};
