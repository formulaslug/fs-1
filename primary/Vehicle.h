// Copyright (c) 2018 Formula Slug. All Rights Reserved.

#pragma once

#include <stdint.h>

#include <array>
#include "hal.h"

#include "fsprintf.h"
static auto printf3 = SDPrinter<&SD3>();

constexpr uint8_t kNumLEDs = 3;
constexpr uint8_t kNumButtons = 2;
static constexpr uint8_t kOn = 1;
static constexpr uint8_t kOff = 0;

static constexpr uint16_t kBrakeMin = 1870;
static constexpr uint16_t kBrakeMax = 1970;
static constexpr uint16_t kBrakeThreshold = 10;  // MAKE THIS REAL

static constexpr uint16_t kThrottleThreshold = 30;
static constexpr uint16_t kThrottleAMin = 1000;
static constexpr uint16_t kThrottleAMax = 2100;
static constexpr uint16_t kThrottleBMin = 620;
static constexpr uint16_t kThrottleBMax = 1380;

static constexpr uint16_t kThrottleSafeMax = 65535 * .1;
static constexpr uint16_t kThrottleEnduranceMax = 65535 * .3;
static constexpr uint16_t kThrottleAutoXMax = 65535 * .5;
static constexpr uint16_t kThrottleLudicrousMax = 65535 * .7;

static constexpr uint16_t kSteeringMin = 1050;
static constexpr uint16_t kSteeringMax = 3300;

// BMS Konstants
static constexpr uint8_t kVoltageMin = 200;
static constexpr uint8_t kTempMax = 200;

// Operating on all 8 bits so that can be notted "~"
static constexpr uint8_t kLEDOn = 0xff;
static constexpr uint8_t kLEDOff = 0x00;

static constexpr uint8_t toggleUp = 4;
static constexpr uint8_t toggleDown = 2;
static constexpr uint8_t revButton = 1;

static constexpr uint8_t AMSFault = 8;
static constexpr uint8_t BSPDFault = 4;
static constexpr uint8_t IMDFault = 2;
static constexpr uint8_t throttleFault = 1;

static constexpr uint8_t VT_SM_D = 4;
static constexpr uint8_t VT_SM_R = 2;
static constexpr uint8_t VT_F_Throttle = 1;

enum States {
	kInit,
	kProfileSelect,
	kProfileSelectBreak,
	kForward,
	kDelayF,
	kDelayR,
	kReverse,
	kFault
};

enum DriveProfiles {
	kSafe,       // Parade/Pits/sloooo-moooo
	kEndurance,  // competition ready: Endurance
	kAutoX,      // competition ready: AutoX
	kLudicrous   // PLAID
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

	void profileChange(uint8_t prof);
	void HandleADCs();
	void FSM();
	void setTimerFlag();
	void secTimer();
	void FaultCheck();

	uint8_t state = kInit;
	uint8_t driveProfile = kSafe;

	uint8_t timerStartFlag = 0;  // request timer number
	uint8_t timerDoneFlag = 0; // updated by event queue, this is fucking gross Im sorry

	uint16_t throttleA = 0;
	uint16_t throttleB = 0;
	uint16_t throttleVal = 0;
	uint8_t forwardSwitch = 0;
	uint8_t reverseSwitch = 0;

	uint16_t throttleMax = 0;

	uint16_t brakeVoltage = 1;
	uint16_t brakeVal = 1;

	uint16_t steeringIn = 1;
	uint16_t steeringAngle = 1;

	uint8_t faults = 0;
	uint8_t maxSpeed = 5;  // 5 speed units

	uint8_t dashInputs = 0;

	uint8_t cellVoltages[28];
	uint8_t cellTemps[28];

	std::array<uint8_t, kNumLEDs> ledStates; // LED values are 0x00 or 0xff to allow for bitwise not
};
