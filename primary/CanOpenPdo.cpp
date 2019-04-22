// Copyright (c) 2018 Formula Slug. All Rights Reserved.

#include "CanOpenPdo.h"

constexpr uint32_t kFuncIdHeartbeat = 0x100;

// TODO: Make COB-ID naming match CANOpen convention

HeartbeatMessage::HeartbeatMessage(uint16_t data) {
	IDE = CAN_IDE_STD;
	RTR = CAN_RTR_DATA;
	SID = kFuncIdHeartBeatECU;
	DLC = 2;
	data8[0] = (data >> 8) & 0xFF;  // MSB (32's 3rd byte)
	data8[1] = data & 0xFF;         // LSB (32's 4th byte)
}

DigitalMessage::DigitalMessage(uint8_t digitalStates) {
	IDE = CAN_IDE_STD;
	RTR = CAN_RTR_DATA;
	SID = kFuncIdDigital;
	DLC = 1;
	data8[0] = digitalStates; // MSB (32's 3rd byte) (left most byte in DVT)
}

ThrottleMessage::ThrottleMessage(uint8_t throttleVoltage) {
	IDE = CAN_IDE_STD;
	RTR = CAN_RTR_DATA;
	SID = kFuncIdThrottle;
	DLC = 1;
	data8[0] = throttleVoltage; // MSB (32's 3rd byte) (left most byte in DVT)
}

SteeringMessage::SteeringMessage(uint8_t steeringAngle) {

	IDE = CAN_IDE_STD;
	RTR = CAN_RTR_DATA;
	SID = kFuncIdSteering;
	DLC = 1;
	data8[0] = steeringAngle;
}

BrakeMessage::BrakeMessage(uint8_t brakeVal) {
	IDE = CAN_IDE_STD;
	RTR = CAN_RTR_DATA;
	SID = kFuncIdBrake;
	DLC = 1;
	data8[0] = brakeVal; // MSB (32's 3rd byte) (left most byte in DVT)
}

BMSVoltageMessage::BMSVoltageMessage(uint8_t row, uint8_t *voltages) {
	IDE = CAN_IDE_STD;
	RTR = CAN_RTR_DATA;
	SID = kFuncIdCellVoltage[row];
	DLC = 7;
	for (int i = 0; i < 7; i++) {
		data8[i] = voltages[i];
	}

}

BMSTempMessage::BMSTempMessage(uint8_t row, uint8_t *temps) {
	IDE = CAN_IDE_STD;
	RTR = CAN_RTR_DATA;
	SID = kFuncIdCellTempAdc[row];
	DLC = 7;
	for (int i = 0; i < 7; i++) {
		data8[i] = temps[i];
	}
}
