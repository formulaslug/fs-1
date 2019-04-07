// Copyright (c) 2018 Formula Slug. All Rights Reserved.

#pragma once

#include <stdint.h>

#include "ch.h"
#include "hal.h"
#include "Gpio.h"

// COB-IDs: MAX LENGTH of 12 bits, only the LSB 12 should be used
// IDs specified in format 0x<system-id><node-id><function-id>
// NOTE: The constexprs below are now bit-shifted, there OR'd as-is so
//       the values must be in the correct hex digit.
// System IDs: 6 = FS System
// Node IDs:
//   1 = Primary Controller, (node 3)
//   2 = Secondary Controller, (node 4) (NOTE: Unused...)
//   3 = Cell Temp Monitor
//     // Function IDs
//     2 = ADC Chip 1 Reading
//     3 = ADC Chip 2 Reading
//     4 = ADC Chip 3 Reading
//     5 = ADC Chip 4 Reading
//     6 = Fault Statuses for Temp, BMS, and IMD
// Universal Function IDs:
//   1 = Heartbeat

// TODO: Reverse order of these (sys, node, func to func, node, sys) to
//       use correct frame priority at physical layer

// COB-ID:
// Function Code |      Node ID      | RTR
//    X X X X        X X X X X X X      X
// Ex/ ECU Heartbeat Packet
//    0 0 0 1        0 0 0 0 0 1 0      0
// Ex/ ECU Throttle Packet
//    0 0 1 0        0 0 0 0 0 1 0      0


// Function IDs

constexpr uint32_t kFuncIdCellTempAdc[4] = {0x002, 0x003, 0x004, 0x005};
constexpr uint32_t kFuncIdCellVoltage[4] = {0x012, 0x013, 0x014, 0x015};
constexpr uint32_t kFuncIdFaultStatuses = 0x006;
constexpr uint32_t kFuncIdPackVoltage = 0x201; 
constexpr uint32_t kFuncIdPackCurrent = 0x202; 
constexpr uint32_t kFuncIdEnergy = 0x203; 
constexpr uint32_t kFuncIdPackResistance = 0x204; 
constexpr uint32_t kFuncIdDigitalIn = 0x205; 
//constexpr uint32_t kNodeIdPackResistance = 0x204; 
//constexpr uint32_t kNodeIdPackResistance = 0x204; 







constexpr uint32_t kFuncIdThrottleValue = 0x200;
constexpr uint32_t kFuncIdBreakValue = 0x008;
constexpr uint32_t kFuncIdSteeringValue = 0x009;
// Full COB-IDs
constexpr uint32_t kCobIdTPDO5 = 0x242;



// Payload constants
constexpr uint32_t kPayloadHeartbeat = 0x1;

struct HeartbeatMessage : public CANTxFrame {
  // explicit HeartbeatMessage(uint32_t id);
  explicit HeartbeatMessage(uint16_t data);
};

struct ThrottleMessage : public CANTxFrame {
  /**
   * @param throttleVoltage The current, cleaned throttle voltage to be sent to
   *                        Master
   * @param forwardSwitch If true, enables forward drive
   */
  explicit ThrottleMessage(uint16_t throttleVoltage);
};

struct DigitalMessage : public CANTxFrame{
	
	explicit DigitalMessage(DigitalInput pin, bool digitalState);
};


/**
 * TPDO sent from Teensy to master motor controller
 */
struct[[gnu::packed]] TPDO5 {
  uint16_t throttleInputVoltage;
  uint16_t maxBatteryDischargeCurrent;
  uint16_t maxBatteryRechargeCurrent;
  uint8_t forwardSwitch : 1;
  uint8_t driveSelect1Switch : 1;
  uint8_t driveSelect2Switch : 1;
  uint8_t reverseSwitch : 1;
  uint8_t padding : 4;
};
