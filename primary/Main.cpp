// Copyright (c) 2018 Formula Slug. All Rights Reserved.

#include <array>
#include <mutex>

#include "AdcChSubsys.h"
#include "AnalogFilter.h"
#include "CanChSubsys.h"
#include "CanOpenPdo.h"
#include "DigInChSubsys.h"
#include "EventQueue.h"
#include "TimerChSubsys.h"
#include "Vehicle.h"
#include "ch.hpp"
#include "hal.h"
#include "fsprintf.h"


static auto printf3 = SDPrinter<&SD3>();


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

	//palSetPadMode(DRIVE_BUTTON_PORT, DRIVE_BUTTON_PIN, PAL_MODE_INPUT_PULLUP); // AMS
	palSetPadMode(DRIVE_MODE_PORT, DRIVE_MODE_PIN, PAL_MODE_INPUT_PULLUP); // BSPD
	palSetPadMode(BSPD_FAULT_PORT, BSPD_FAULT_PIN, PAL_MODE_INPUT_PULLUP); // RTDS signal
	// Digital outputs
	palSetPadMode(IMD_FAULT_INDICATOR_PORT, IMD_FAULT_INDICATOR_PIN,
			PAL_MODE_OUTPUT_PUSHPULL);  // IMD
	palSetPadMode(AMS_FAULT_INDICATOR_PORT, AMS_FAULT_INDICATOR_PIN,
			PAL_MODE_OUTPUT_PUSHPULL);  // AMS
	palSetPadMode(BSPD_FAULT_INDICATOR_PORT, BSPD_FAULT_INDICATOR_PIN,
			PAL_MODE_OUTPUT_PUSHPULL);  // BSPD
	palSetPadMode(STARTUP_SOUND_PORT, STARTUP_SOUND_PIN,
			PAL_MODE_OUTPUT_PUSHPULL);  // RTDS signal
	palSetPadMode(BRAKE_LIGHT_PORT, BRAKE_LIGHT_PIN, PAL_MODE_OUTPUT_PUSHPULL); // Brake light signal
	palSetPadMode(STARTUP_LED_PORT, STARTUP_LED_PIN, PAL_MODE_OUTPUT_PUSHPULL); // Brake light signal

	// Init LED states to LOW (including faults)
	palClearPad(IMD_FAULT_INDICATOR_PORT, IMD_FAULT_INDICATOR_PIN);
	palClearPad(AMS_FAULT_INDICATOR_PORT, AMS_FAULT_INDICATOR_PIN);
	palClearPad(BSPD_FAULT_INDICATOR_PORT, BSPD_FAULT_INDICATOR_PIN);
	palClearPad(STARTUP_SOUND_PORT, STARTUP_SOUND_PIN);
	palClearPad(BRAKE_LIGHT_PORT, BRAKE_LIGHT_PIN);
	palClearPad(STARTUP_LED_PORT, STARTUP_LED_PIN);

	Vehicle vehicle;

	CanBus canBusLv(&CAND1, CanBusBaudRate::k1M, true);
	chibios_rt::Mutex canBusLvMut;

	CanBus canBusHv(&CAND2, CanBusBaudRate::k1M, true);
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

	adcChSubsys.addPin(Gpio::kA1);  // add brake input
	adcChSubsys.addPin(Gpio::kA2);  // add throttle input
	digInChSubsys.addPin(DigitalInput::kToggleUp);
	digInChSubsys.addPin(DigitalInput::kToggleDown);
	digInChSubsys.addPin(DigitalInput::kDriveMode);
	digInChSubsys.addPin(DigitalInput::kBSPDFault);

	static const SerialConfig sConfig = { 115200, 0, USART_CR2_STOP1_BITS, 0 };
	palSetLineMode(UART_RX_LINE, PAL_MODE_ALTERNATE(7));  // UART RX
	palSetLineMode(UART_TX_LINE, PAL_MODE_ALTERNATE(7));  // UART TX
	sdStart(&SD3, &sConfig);


	printf3("STARTING UP\n");



	for (uint8_t i = 0; i < 4; i++) {
		palWritePad(IMD_FAULT_INDICATOR_PORT, IMD_FAULT_INDICATOR_PIN,
				PAL_HIGH);  // IMD
		palWritePad(AMS_FAULT_INDICATOR_PORT, AMS_FAULT_INDICATOR_PIN,
				PAL_HIGH);  // AMS
		palWritePad(BSPD_FAULT_INDICATOR_PORT, BSPD_FAULT_INDICATOR_PIN,
				PAL_HIGH);  // BSPD
		palWritePad(STARTUP_SOUND_PORT, STARTUP_SOUND_PIN, PAL_HIGH);  // RTDS
		palWritePad(BRAKE_LIGHT_PORT, BRAKE_LIGHT_PIN, PAL_HIGH); // Brake Light
		chThdSleepMilliseconds(200);
		palWritePad(IMD_FAULT_INDICATOR_PORT, IMD_FAULT_INDICATOR_PIN, PAL_LOW); // IMD
		palWritePad(AMS_FAULT_INDICATOR_PORT, AMS_FAULT_INDICATOR_PIN, PAL_LOW); // AMS
		palWritePad(BSPD_FAULT_INDICATOR_PORT, BSPD_FAULT_INDICATOR_PIN,
				PAL_LOW);  // Temp
		palWritePad(STARTUP_SOUND_PORT, STARTUP_SOUND_PIN, PAL_LOW);  // RTDS
		palWritePad(BRAKE_LIGHT_PORT, BRAKE_LIGHT_PIN, PAL_LOW);  // Brake Light
		chThdSleepMilliseconds(200);
	}

	while (1) {
//		palWritePad(IMD_FAULT_INDICATOR_PORT, IMD_FAULT_INDICATOR_PIN, PAL_LOW); // IMD
//		chThdSleepMilliseconds(100);
//		palWritePad(IMD_FAULT_INDICATOR_PORT, IMD_FAULT_INDICATOR_PIN,
//									PAL_HIGH);  // IMD

		// always deplete the queue to help ensure that events are
		// processed faster than they're generated

		while (fsmEventQueue.size() > 0) {

			Event e = fsmEventQueue.pop();

			if (e.type() == Event::Type::kDigInTransition) {
				DigitalInput digInPin = e.digInPin();
				bool digInState = e.digInState();
				DigitalMessage digitalMsg(digInPin, digInState);
				canLvChSubsys.startSend(digitalMsg);

				switch (digInPin) {

					case DigitalInput::kToggleUp:
						if (digInState) {
							vehicle.dashInputs |= toggleUp;
							//printfs("toggleUp\n");
//							palWritePad(BSPD_FAULT_INDICATOR_PORT,
//									BSPD_FAULT_INDICATOR_PIN, PAL_HIGH);  // BSPD

						} else {
							vehicle.dashInputs &= ~toggleUp;
							//printfs("toggleUp Released\n");
//							palWritePad(BSPD_FAULT_INDICATOR_PORT,
//									BSPD_FAULT_INDICATOR_PIN, PAL_LOW); // BSPD

						}
						break;
					case DigitalInput::kToggleDown:
						if (digInState) {
							vehicle.dashInputs |= toggleDown;
							//printfs("toggleDown\n");
//							palWritePad(AMS_FAULT_INDICATOR_PORT, AMS_FAULT_INDICATOR_PIN,
//											PAL_HIGH);  // AMS
						} else {
							vehicle.dashInputs &= ~toggleDown;
							//printfs("toggleDown Released\n");
//							palWritePad(AMS_FAULT_INDICATOR_PORT, AMS_FAULT_INDICATOR_PIN,
//											PAL_LOW);  // AMS
						}
						break;
					case DigitalInput::kDriveMode:
						if (digInState) {
							vehicle.dashInputs |= revButton;
							//printfs("Reverse\n");
//							palWritePad(IMD_FAULT_INDICATOR_PORT,
//									IMD_FAULT_INDICATOR_PIN, PAL_HIGH);  // IMD
						} else {
							vehicle.dashInputs &= ~revButton;
							//printfs("Reverse Released\n");
//							palWritePad(IMD_FAULT_INDICATOR_PORT,
//									IMD_FAULT_INDICATOR_PIN, PAL_LOW);  // IMD
						}
						break;
					case DigitalInput::kBSPDFault:
						if (digInState) {
//							printfs("BSPD Dicked\n");
						} else {
//							printfs("BSPD Undicked\n");
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
				uint16_t throttle = e.adcValue();
//				uint16_t throttle = throttleFilter.filterLms(
//						static_cast<uint16_t>(e.adcValue()));
				if (e.adcPin() == Gpio::kA2) {
					vehicle.throttleA = throttle;
				} else if (e.adcPin() == Gpio::kA1) {
					vehicle.throttleB = throttle;
				}
				if ((vehicle.throttleA < vehicle.throttleB + maxReading * 10)
						&& (vehicle.throttleA
								> vehicle.throttleB - maxReading * 10)) {
					vehicle.throttleAvg =
							(vehicle.throttleA + vehicle.throttleB) / 2;
					if (vehicle.throttleAvg > kThrottleThreshold) {

//						printfs("Throttle Pressed");
						palWritePad(IMD_FAULT_INDICATOR_PORT,
								IMD_FAULT_INDICATOR_PIN, PAL_HIGH);  // IMD

					}
					palWritePad(IMD_FAULT_INDICATOR_PORT,
							IMD_FAULT_INDICATOR_PIN, PAL_LOW);  // IMD
				} else {

					palWritePad(IMD_FAULT_INDICATOR_PORT,
							IMD_FAULT_INDICATOR_PIN, PAL_HIGH);  // IMD
					//START 100MS FAULT TIMER FOR AN ENCODER DISAGREEMENT 

				}

			} else if (e.type() == Event::Type::kTimerTimeout) { //Create Timer Subsys?

				if (e.timer() == 0) {
//					printfs("Timer0 Done");

				} else if (e.timer() == 1) {
//					printfs("Timer1 Done");

				} else if (e.timer() == 2) {
//					printfs("Timer2 Done");

				} else if (e.timer() == 3) {
//					printfs("Timer3 Done");

				} else {
//					printfs("What the timer shit");
				}

			}
		}
//		vehicle.FSM(); //update vehicle state
		// TODO: use condition var to signal that events are present in the queue
		chThdSleepMilliseconds(1); // must be fast enough to deplete event queue quickly enough
	}
}
