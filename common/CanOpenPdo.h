// Copyright (c) 2018 Formula Slug. All Rights Reserved.

#pragma once

#include <stdint.h>
#include <cstring>

#include "ch.h"
#include "hal.h"

// SIDs From ECU
constexpr uint32_t kFuncIdHeartBeatECU = 0x700;
// ADC
constexpr uint32_t kFuncIdBrake = 0x020;
constexpr uint32_t kFuncIdThrottleLV = 0x021;
constexpr uint32_t kFuncIdThrottleHV = 0x022;
constexpr uint32_t kFuncIdSteering = 0x023;
// Digital
constexpr uint32_t kFuncIdDigital = 0x022;

// SIDs From Accumulator
constexpr uint32_t kFuncIdHeartBeatAcc = 0x701;
constexpr uint32_t kFuncIdCellStartup = 0x420;
constexpr uint32_t kFuncIdFaultStatus = 0x421;
constexpr uint32_t kFuncIdBmsStat = 0x422;
constexpr uint32_t kFuncIdCellVoltage[7] = {0x423, 0x424, 0x425, 0x426,
                                            0x427, 0x428, 0x429};
constexpr uint32_t kFuncIdCellTempAdc[4] = {0x42a, 0x42b, 0x42c, 0x42d};

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
struct ThrottleMessageHV : public CANTxFrame {
  explicit ThrottleMessageHV(uint16_t throttleVoltage, uint8_t forwardSwitch, uint8_t reverseSwitch) {
    IDE = CAN_IDE_STD;
    RTR = CAN_RTR_DATA;
    SID = kFuncIdThrottleHV;
    DLC = 4;
    data8[0] = throttleVoltage;  // LSB (32's 1st byte) (left most byte in DVT)
    data8[1] = throttleVoltage >> 8;
    data8[2] = reverseSwitch << 1 | forwardSwitch;
  }
};

struct ThrottleMessageLV : public CANTxFrame {
  explicit ThrottleMessageLV(uint16_t throttleVoltage, uint8_t driveMode) {
    IDE = CAN_IDE_STD;
    RTR = CAN_RTR_DATA;
    SID = kFuncIdThrottleLV;
    DLC = 5;
    data8[0] = throttleVoltage >> 8;  // MSB (32's 3rd byte) (left most byte in DVT)
    data8[1] = throttleVoltage;
    data8[4] = driveMode;
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

struct BMSFaultMessage : public CANTxFrame {
  explicit BMSFaultMessage(uint16_t current) {
    IDE = CAN_IDE_STD;
    RTR = CAN_RTR_DATA;
    SID = kFuncIdFaultStatus;
    DLC = 8;
    // TODO: remaining fault data bytes 0-5
    data8[6] = (current >> 8) & 0xff;
    data8[7] = current & 0xff;
  }
};

struct BMSVoltageMessage : public CANTxFrame {
  explicit BMSVoltageMessage(uint8_t row, uint16_t *voltages) {
    IDE = CAN_IDE_STD;
    RTR = CAN_RTR_DATA;
    SID = kFuncIdCellVoltage[row];
    DLC = 8;
    for (int i = 0; i < 7; i += 2) {
      data8[i] = voltages[i / 2] >> 8;
      data8[i + 1] = voltages[i / 2] & 0xFF;
    }
  }
};

struct BMSTempMessage : public CANTxFrame {
  explicit BMSTempMessage(uint8_t row, int8_t *temps) {
    IDE = CAN_IDE_STD;
    RTR = CAN_RTR_DATA;
    SID = kFuncIdCellTempAdc[row];
    DLC = 7;
    for (uint8_t i = 0; i < 7; i++) {
      data8[i] = temps[i];
    }
  }
};

struct BMSStatMessage : public CANTxFrame {
  explicit BMSStatMessage(uint16_t totalVoltage,
                          int16_t maxVoltage, int16_t minVoltage,
                          int8_t maxTemp, int8_t minTemp) {
    IDE = CAN_IDE_STD;
    RTR = CAN_RTR_DATA;
    SID = kFuncIdBmsStat;
    DLC = 8;
    data8[0] = (totalVoltage >> 8) & 0xff;
    data8[1] = totalVoltage & 0xff;
    data8[2] = (maxVoltage >> 8) & 0xff;
    data8[3] = maxVoltage & 0xff;
    data8[4] = (minVoltage >> 8) & 0xff;
    data8[5] = minVoltage & 0xff;
    data8[6] = maxTemp;
    data8[7] = minTemp;
  }
};
