// Copyright (c) 2018 Formula Slug. All Rights Reserved.

#include <array>
#include <mutex>

#include "AdcChSubsys.h"
#include "AnalogFilter.h"
#include "CanChSubsys.h"
#include "CanOpenPdo.h"
#include "DigInChSubsys.h"
#include "EventQueue.h"
#include "timerChSubsys.h"
#include "Vehicle.h"
#include "ch.hpp"
#include "hal.h"

static THD_WORKING_AREA(canLvThreadFuncWa, 128);
static THD_FUNCTION(canLvThreadFunc, canChSubsys) {
	chRegSetThreadName("CAN LV");
	static_cast<CanChSubsys*>(canChSubsys)->runThread(); //UNSURE IF BOTH CAN BE RUN IN THE SAME THREAD BUT PROB NOT
//	static_cast<CanChSubsys*>(canChSubsys)->runRxThread();//REWRITE AS ONE THREAD OR SPLIT INTO TWO STATIC THREADS
}

static THD_WORKING_AREA(canHvThreadFuncWa, 128);
static THD_FUNCTION(canHvThreadFunc, canChSubsys) {
	chRegSetThreadName("CAN HV");
	static_cast<CanChSubsys*>(canChSubsys)->runThread();
}

static THD_WORKING_AREA(adcThreadFuncWa, 128);
static THD_FUNCTION(adcThreadFunc, adcChSubsys) {
	chRegSetThreadName("ADC");
	static_cast<AdcChSubsys*>(adcChSubsys)->runThread();
}

static THD_WORKING_AREA(digInThreadFuncWa, 128);
static THD_FUNCTION(digInThreadFunc, digInChSubsys) {
	chRegSetThreadName("Digital In");
	static_cast<DigInChSubsys*>(digInChSubsys)->runThread();
}

static THD_WORKING_AREA(timerThreadFuncWa, 128);
static THD_FUNCTION(timerThreadFunc, timerChSubsys) {
	chRegSetThreadName("Timers");
	static_cast<TimerChSubsys*>(timerChSubsys)->runThread();
}

int main() {

	halInit();
	chSysInit();

	Vehicle vehicle;

	CanBus canBusLv(&CAND1, CanBusBaudRate::k1M, false);
	chibios_rt::Mutex canBusLvMut;

	CanBus canBusHv(&CAND2, CanBusBaudRate::k1M, false);
	chibios_rt::Mutex canBusHvMut;

	EventQueue fsmEventQueue = EventQueue();

	CanChSubsys canLvChSubsys = CanChSubsys(canBusLv, canBusLvMut,
			fsmEventQueue);
	CanChSubsys canHvChSubsys = CanChSubsys(canBusHv, canBusHvMut,
			fsmEventQueue);
	AdcChSubsys adcChSubsys = AdcChSubsys(fsmEventQueue);
	DigInChSubsys digInChSubsys = DigInChSubsys(fsmEventQueue);
	TimerChSubsys timerChSubsys = TimerChSubsys(fsmEventQueue);

	/*
	 * Create threads (many of which are driving subsystems)
	 */
	chThdCreateStatic(canLvThreadFuncWa, sizeof(canLvThreadFuncWa), NORMALPRIO,
			canLvThreadFunc, &canLvChSubsys);
	chThdCreateStatic(canLvThreadFuncWa, sizeof(canHvThreadFuncWa),
			NORMALPRIO + 1, canHvThreadFunc, &canHvChSubsys);
	chThdCreateStatic(adcThreadFuncWa, sizeof(adcThreadFuncWa), NORMALPRIO,
			adcThreadFunc, &adcChSubsys);
	chThdCreateStatic(digInThreadFuncWa, sizeof(digInThreadFuncWa), NORMALPRIO,
			digInThreadFunc, &digInChSubsys);
	chThdCreateStatic(timerThreadFuncWa, sizeof(timerThreadFuncWa), NORMALPRIO,
			timerThreadFunc, &timerChSubsys);

	AnalogFilter throttleFilter = AnalogFilter();

	while (1) {
		// always deplete the queue to help ensure that events are
		// processed faster than they're generated
		while (fsmEventQueue.size() > 0) {
			Event e = fsmEventQueue.pop();

			if (e.type() == Event::Type::kDigInTransition) {
				palSetPad(BSPD_FAULT_INDICATOR_PORT, BSPD_FAULT_INDICATOR_PIN); // IMD
				DigitalInput digInPin = e.digInPin();
				bool digInState = e.digInState();
				DigitalMessage digitalMsg(digInPin, digInState);
				canLvChSubsys.startSend(digitalMsg);

				switch (digInPin) {

					case DigitalInput::kToggleUp:
						if (digInState) {
							vehicle.dashInputs |= toggleUp;
						} else {
							vehicle.dashInputs &= ~toggleUp;
						}
						break;
					case DigitalInput::kToggleDown:
						if (digInState) {
							vehicle.dashInputs |= toggleDown;
						} else {
							vehicle.dashInputs &= ~toggleDown;
						}
						break;
					case DigitalInput::kReverseButton:
						if (digInState) {
							vehicle.dashInputs |= revButton;
						} else {
							vehicle.dashInputs &= ~revButton;
						}
						break;
					default:
						break;
				}
			} else if (e.type() == Event::Type::kCanRx) {
				std::array < uint16_t, 8 > canFrame = e.canFrame();
				uint32_t canEid = e.canEid();
				// handle packet types
				switch (canEid) {
					case kFuncIdCellTempAdc[0]: // Replace with Cell Temp ID 0
						for (int i = 0; i < 7; i++) {
							vehicle.cellTemps[i] = canFrame[i];
						}
						break;
					case kFuncIdCellTempAdc[1]: // Replace with Cell Temp ID 1
						for (int i = 0; i < 7; i++) {
							vehicle.cellTemps[i + 7] = canFrame[i];
						}
						break;
					case kFuncIdCellTempAdc[2]: // Replace with Cell Temp ID 2
						for (int i = 0; i < 7; i++) {
							vehicle.cellTemps[i + 14] = canFrame[i];
						}
						break;
					case kFuncIdCellTempAdc[3]: // Replace with Cell Temp ID 3
						for (int i = 0; i < 7; i++) {
							vehicle.cellTemps[i + 21] = canFrame[i];
						}
						break;
					case kFuncIdCellVoltage[0]: // Replace with Cell Voltage ID 0
						for (int i = 0; i < 7; i++) {
							vehicle.cellVoltages[i] = canFrame[i];
						}
						break;
					case kFuncIdCellVoltage[1]: // Replace with Cell Voltage ID 1
						for (int i = 0; i < 7; i++) {
							vehicle.cellVoltages[i + 7] = canFrame[i];
						}
						break;
					case kFuncIdCellVoltage[2]: // Replace with Cell Voltage ID 2
						for (int i = 0; i < 7; i++) {
							vehicle.cellVoltages[i + 14] = canFrame[i];
						}
						break;
					case kFuncIdCellVoltage[3]: // Replace with Cell Voltage ID 3
						for (int i = 0; i < 7; i++) {
							vehicle.cellVoltages[i + 21] = canFrame[i];
						}
						break;

					case kFuncIdFaultStatuses: // Replace with State ID 

						break;
					case kFuncIdPackVoltage: // Replace with Pack Voltage ID 

						break;
					case kFuncIdPackCurrent: // Replace with Current ID

						break;
					case kFuncIdEnergy: // Replace with Energy In/Out

						break;
					case kFuncIdPackResistance: // Replace with Pack Resistance ID

						break;
					default:
						break;
				}
			} else if (e.type() == Event::Type::kAdcConversion) {
				if (e.adcPin() == Gpio::kA2) {
					uint16_t throttle = throttleFilter.filterLms(
							static_cast<uint16_t>(e.adcValue()));

					// output non-zero if passed sensitivity margin
					if (throttle < 130) {
						ThrottleMessage throttleMessage(0);
						canLvChSubsys.startSend(throttleMessage);
					} else {
						ThrottleMessage throttleMessage(throttle);
						canLvChSubsys.startSend(throttleMessage);
//						canHvChSubsys.startSend(throttleMessageL);
//						canHvChSubsys.startSend(throttleMessageR);
					}
				} else if (e.adcPin() == Gpio::kA1) {
					uint16_t throttle = throttleFilter.filterLms(
							static_cast<uint16_t>(e.adcValue()));

					// output non-zero if passed sensitivity margin
					if (throttle < 130) {
						ThrottleMessage throttleMessage(0);
						canLvChSubsys.startSend(throttleMessage);
					} else {
						ThrottleMessage throttleMessage(throttle);
						canLvChSubsys.startSend(throttleMessage);
					}
				}
			} else if (e.type() == Event::Type::kTimerTimeout) { //Create Timer Subsys?

				vehicle.secTimerDone = 1;

			}
		}
		vehicle.FSM(); //update vehicle state
		// TODO: use condition var to signal that events are present in the queue
		chThdSleepMilliseconds(1); // must be fast enough to deplete event queue quickly enough
	}
}
