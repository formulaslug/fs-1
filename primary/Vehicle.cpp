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
			profileChange(driveProfile);  //load the ksafe params on init
			state = kProfileSelect;
			break;
		case kProfileSelect:
			if (dashInputs == (toggleDown | revButton)) {	//Profile selections
				if (driveProfile == kLudicrous) {
					driveProfile = 0;
					profileChange (driveProfile);
				} else {
					driveProfile += 1;
					profileChange (driveProfile);
				}
				state = kProfileSelect;
			} else if ((dashInputs & toggleDown)
					&& (brakeVoltage > kBrakeThreshold)
					&& (throttleVoltage < kThrottleThreshold)) { //Move to Drive Mode if toggle up with foot on brake and no throttle
				state = kForward;
			} else {
				state = kProfileSelect;
			}
			break;
		case kForward:
			if (dashInputs & revButton) {
				secTimerDone = 0;        //make sure timer flag is low if delay wasnt completed 
				state = kDelay;
				chVTReset (&vtSec);
				chVTSet(&vtSec, TIME_MS2I(1000), NULL, NULL); //ADD CALLBACK THAT POSTS EVENT 
			} else {
				state = kForward;
			}
			break;
		case kDelay:

			if ((dashInputs & revButton) && secTimerDone) {
				secTimerDone = 0;
				state = kReverse;
			} else if (dashInputs & revButton) {
				state = kDelay;
			} else {
				state = kForward;
			}
			break;
		case kReverse:
			if (dashInputs & revButton) {
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
