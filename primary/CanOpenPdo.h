// Copyright (c) 2018 Formula Slug. All Rights Reserved.

#pragma once

#include <stdint.h>
#include <cstring>

#include "ch.h"
#include "hal.h"
#include "mcuconfFs.h"


//SIDs From ECU
//constexpr uint32_t ; 


//SIDs From Accumulator 
constexpr uint32_t kFuncIdCellTempAdc[4] = {0x002, 0x003, 0x004, 0x005};
constexpr uint32_t kFuncIdCellVoltage[4] = {0x012, 0x013, 0x014, 0x015};
constexpr uint32_t kFuncIdFaultStatuses = 0x006;
constexpr uint32_t kFuncIdPackVoltage = 0x201; 
constexpr uint32_t kFuncIdPackCurrent = 0x202; 
constexpr uint32_t kFuncIdEnergy = 0x203; 
constexpr uint32_t kFuncIdPackResistance = 0x204; 






// Payload constants
constexpr uint32_t kPayloadHeartbeat = 0x1;

struct HeartbeatMessage : public CANTxFrame {
  explicit HeartbeatMessage(uint16_t data);
};

struct ThrottleMessage : public CANTxFrame {
  explicit ThrottleMessage(uint16_t throttleVoltage);
};

struct DigitalMessage : public CANTxFrame{
	explicit DigitalMessage(uint8_t digitalState);
};


