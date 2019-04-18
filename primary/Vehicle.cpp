// Copyright (c) 2018 Formula Slug. All Rights Reserved.

#include "Vehicle.h"

#include "mcuconfFs.h"

Vehicle::Vehicle() {
	for (auto& led : ledStates) {
		led = kLEDOff;
	}
	//ledStates[kStatusLED] = kOn;

}
void Vehicle::profileChange(uint8_t prof) {
	switch (prof) {
		case kSafe:
			printf3("SAFE\n");
			break;
		case kEndurance:
			printf3("ENDURANCE\n");
			break;
		case kAutoX:
			printf3("AUTOX\n");
			break;
		case kLudicrous:
			printf3("LUDICROUS\n");
			break;
		default:
			break;
	}
}

void Vehicle::HandleADCs() {
	int16_t tempA, tempB;
	if (throttleA > kThrottleAMax) {
		throttleA = kThrottleAMax;
	} else if (throttleA < kThrottleAMin) {
		throttleA = kThrottleAMin;
	}
	if (throttleB > kThrottleBMax) {
		throttleB = kThrottleBMax;
	} else if (throttleB < kThrottleBMin) {
		throttleB = kThrottleBMin;
	}

	tempA = 255 * (throttleA - kThrottleAMin) / (kThrottleAMax - kThrottleAMin);
	tempB = 255 * (throttleB - kThrottleBMin) / (kThrottleBMax - kThrottleBMin);
	if (tempA > tempB + 25 || tempA < tempB - 25) {
		//Dicked
	}
	throttleVal = 255 - ((tempA + tempB) / 2);
//	if (throttleVal > 255) {
//		throttleVal = 255;
//	} else if (throttleVal < 0) {
//		throttleVal = 0;
//	}

	if (brakeVoltage > kBrakeMax) {
		brakeVoltage = kBrakeMax;
	} else if (brakeVoltage < kBrakeMin) {
		brakeVoltage = kBrakeMin;
	}
	brakeVal = 100
			- (100 * (brakeVoltage - kBrakeMin) / (kBrakeMax - kBrakeMin));

	if (brakeVal > kBrakeThreshold) {
		palWritePad(BRAKE_LIGHT_PORT, BRAKE_LIGHT_PIN, PAL_HIGH); // Brake Light
	} else {
		palWritePad(BRAKE_LIGHT_PORT, BRAKE_LIGHT_PIN, PAL_LOW);
	}

	if (steeringIn > kSteeringMax) {
		steeringIn = kSteeringMax;
	} else if (throttleA < kThrottleAMin) {
		steeringIn = kSteeringMin;
	}

	steeringAngle = (100 * (steeringIn - kSteeringMin)
			/ (kSteeringMax - kSteeringMin));
//	arctan(total displacement (cm) /7 (cm)) = steering angle

}

void Vehicle::FSM() {
	HandleADCs();
	switch (state) {
		case kInit:

			printf3("kINIT\n");
			profileChange (driveProfile);  //load the ksafe params on init
			state = kProfileSelect;
			chThdSleepMilliseconds(10);
			printf3("kPROFSELECT\n");
			break;
		case kProfileSelect:

			if (dashInputs == (toggleDown | revButton)) {	//Profile selections
				if (driveProfile == kLudicrous) {
					driveProfile = 0;
					profileChange(driveProfile);
				} else {
					driveProfile += 1;
					profileChange(driveProfile);
				}
				state = kProfileSelectBreak;
			} else if ((dashInputs & toggleUp)
					&& (brakeVoltage > kBrakeThreshold)
					&& (throttleVal < kThrottleThreshold)) { //Move to Drive Mode Delay if toggle up with foot on brake and no throttle
				timerStartFlag += VT_SM_D;
				printf3("kDelayF\n");
				state = kDelayF;
			} else {
				state = kProfileSelect;
			}
			break;

		case kProfileSelectBreak:
			if (dashInputs == 0 || dashInputs == toggleDown) { //release rev button 
				state = kProfileSelect;
			} else {
				state = kProfileSelectBreak;
			}
			break;

		case kDelayF: //Delay to shift into forward
			if ((dashInputs & toggleUp) //Hold toggle up, foot off gas, foot on break, 1 second has elapsed
			&& (brakeVoltage > kBrakeThreshold)
					&& (throttleVal < kThrottleThreshold)
					&& (timerDoneFlag & VT_SM_D)) {
				timerDoneFlag = 0;
				printf3("kFORWARD\n");
				state = kForward;

			} else if ((dashInputs & toggleUp)
					&& (brakeVoltage > kBrakeThreshold)
					&& (throttleVal < kThrottleThreshold)) {
				state = kDelayF;
			}

			else {
				state = kProfileSelect;
				printf3("kPROFSELECT\n");
			}

			break;
		case kForward:
			if (dashInputs & toggleDown) {
				state = kProfileSelect;
				printf3("kPROFSELECT\n");
			} else if (dashInputs & revButton) {
				state = kDelayR;
				printf3("kDelayR\n");
				timerStartFlag += VT_SM_R;
			} else {
				state = kForward;
			}
			break;
		case kDelayR:
			if ((dashInputs & revButton) && (timerDoneFlag & VT_SM_R)) {
				timerDoneFlag = 0;//replace for multiple timers running at once
				state = kReverse;
				printf3("kREV\n");
			} else if (dashInputs & revButton) {
				state = kDelayR;
			} else {
				state = kForward;
				timerStartFlag = 0; //replace for multiple timers running at once
				printf3("kFORWARD\n");
			}
			break;
		case kReverse:
			if (dashInputs & revButton) {
				state = kReverse;
			} else {
				state = kForward;
				timerStartFlag = 0;//replace for multiple timers running at once
				printf3("kFORWARD\n");
			}
			break;
		default:
			printf("FSM FUCKED\n");
			state = kInit;
			break;
	}

}
