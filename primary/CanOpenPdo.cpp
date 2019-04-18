// Copyright (c) 2018 Formula Slug. All Rights Reserved.

#include "CanOpenPdo.h"

constexpr uint32_t kFuncIdHeartbeat = 0x100;

// TODO: Make COB-ID naming match CANOpen convention

HeartbeatMessage::HeartbeatMessage(uint16_t data) {
  RTR = CAN_RTR_DATA;
  DLC = 2;
  data8[0] = (data >> 8) & 0xFF;  // MSB (32's 3rd byte)
  data8[1] = data & 0xFF;         // LSB (32's 4th byte)
}

DigitalMessage::DigitalMessage(uint8_t digitalStates) {

	RTR = CAN_RTR_DATA;
	DLC = 2;

//	data8[0] = pin; // MSB (32's 3rd byte) (left most byte in DVT)
	data8[1] = digitalStates & 0xFF; // MSB (32's 3rd byte) (left most byte in DVT)
}

ThrottleMessage::ThrottleMessage(uint16_t throttleVoltage) {
	IDE = CAN_IDE_EXT;
	RTR = CAN_RTR_DATA;
	DLC = 2;

	data8[0] = throttleVoltage & 0xFF; // MSB (32's 3rd byte) (left most byte in DVT)
	data8[1] = (throttleVoltage >> 8) & 0xFF; // LSB (32's 4th byte) (right most byte in DVT)
}
