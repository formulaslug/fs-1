// Copyright (c) 2018 Formula Slug. All Rights Reserved.

#pragma once

#include <stdint.h>

#include <array>
#include "hal.h"

constexpr uint8_t kNumLEDs = 3;
constexpr uint8_t kNumButtons = 2;
static constexpr uint8_t kOn = 1;
static constexpr uint8_t kOff = 0;
static constexpr uint16_t kBrakeThreshold = 512;
static constexpr uint16_t kThrottleThreshold = 512;

// Operating on all 8 bits so that can be notted "~"
static constexpr uint8_t kLEDOn = 0xff;
static constexpr uint8_t kLEDOff = 0x00;

static constexpr uint8_t toggleUp  = 4;
static constexpr uint8_t toggleDown = 2;
static constexpr uint8_t revButton = 1;

enum States {
	kInit, kProfileSelect, kForward, kDelay, kReverse
};

enum DriveProfiles {
	kSafe,       		// Parade/Pits/sloooo-moooo
	kEndurance,  		// competition ready: Endurance
	kAutoX,       		// competition ready: AutoX
	kLudicrous 			// PLAID
};

enum Leds {
	kLedBMS, kLedIMD, kLedBSPD,
};

enum Buttons {
	kToggleUp, kToggleDown, kReverseButton
};

enum AnalogInputs {
	kThrottleVoltage, kBrakeVoltage
};


class Vehicle {

public:
	
	
	Vehicle();
	virtual_timer_t vtSec;
	void profileChange(uint8_t prof);
	void FSM();
    void setTimerFlag();
	void secTimer();

	
	uint8_t state = kInit;
	uint8_t driveProfile = kSafe;
	
	uint8_t secTimerDone = 0;
	
	uint16_t throttleVoltage = 1;
	uint16_t brakeVoltage = 1;
	uint8_t maxSpeed = 5; // 5 speed units
	
	uint8_t dashInputs = 0; 

	uint8_t cellVoltages[28];
	uint8_t cellTemps[28];

	std::array<uint8_t, kNumLEDs> ledStates; // LED values are 0x00 or 0xff to allow for bitwise not

};
