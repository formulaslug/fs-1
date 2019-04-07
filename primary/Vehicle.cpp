// Copyright (c) 2018 Formula Slug. All Rights Reserved.

#include "Vehicle.h"

Vehicle::Vehicle() {
	for (auto& led : ledStates) {
		led = kLEDOff;
	}
	//ledStates[kStatusLED] = kOn;

}
void Vehicle::profileChange(uint8_t prof) {
	switch (prof) {
		case kSafe:

			break;
		case kEndurance:

			break;
		case kAutoX:

			break;
		case kLudicrous:

			break;
		default:
			break;
	}
}



void Vehicle::FSM() {
	switch (state) {
		case kInit:
			state = kProfileSelect;
			break;
		case kProfileSelect:
			if (kToggleDown) {			//Profile selections
				if (kReverseButton) {
					driveProfile += 1;
					profileChange (driveProfile);
				} else if (kReverseButton && (driveProfile == kLudicrous)) {
					driveProfile = 0;
					profileChange (driveProfile);
				}
				state = kProfileSelect;
			} else if (kToggleUp && (kBrakeVoltage > kBrakeThreshold)) { //Move to Drive Mode
				state = kForward;
			} else {
				state = kProfileSelect;
			}
			break;
		case kForward:
			if (kReverseButton) {
				state = kDelay;
				chVTReset(&vtSec);
				chVTSet(&vtSec, TIME_MS2I(1000),NULL, NULL); //ADD CALLBACK
			} else {
				state = kForward;
			}
			break;
		case kDelay:
			
			if (kReverseButton && secTimerDone) {
				secTimerDone = 0;
				state = kReverse;
			} else if (kReverseButton) {
				state = kDelay;
			} else {
				state = kForward;
			}
			break;
		case kReverse:
			if (kReverseButton) {
				state = kReverse;
			} else {
				state = kForward;
			}
			break;
		default:
			printf("FSM is real fucked up");
			state = kInit;
			break;
	}

}
