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

/*TODO List
 *
 * HV Throttle Commands
 * BMS Error Handling
 *
 */

static THD_WORKING_AREA(canTxLvThreadFuncWa, 128 * 2);
static THD_FUNCTION(canTxLvThreadFunc, canChSubsys) {
  chRegSetThreadName("CAN TX LV");
  static_cast<CanChSubsys*>(canChSubsys)->runTxThread();
}

static THD_WORKING_AREA(canRxLvThreadFuncWa, 128 * 2);
static THD_FUNCTION(canRxLvThreadFunc, canChSubsys) {
  chRegSetThreadName("CAN RX LV");
  static_cast<CanChSubsys*>(canChSubsys)->runRxThread();
}

static THD_WORKING_AREA(canTxHvThreadFuncWa, 128*2);
static THD_FUNCTION(canTxHvThreadFunc, canChSubsys) {
  chRegSetThreadName("CAN TX HV");
  static_cast<CanChSubsys*>(canChSubsys)->runTxThread();
}

static THD_WORKING_AREA(canRxHvThreadFuncWa, 128*2);
static THD_FUNCTION(canRxHvThreadFunc, canChSubsys) {
  chRegSetThreadName("CAN RX HV");
  static_cast<CanChSubsys*>(canChSubsys)->runRxThread();
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

bool toggle = 0;

int main() {
  halInit();
  chSysInit();

  // palSetPadMode(DRIVE_BUTTON_PORT, DRIVE_BUTTON_PIN, PAL_MODE_INPUT_PULLUP);
  // // AMS
  palSetPadMode(DRIVE_MODE_PORT, DRIVE_MODE_PIN, PAL_MODE_INPUT_PULLUP);  // BSPD
  palSetPadMode(BSPD_FAULT_PORT, BSPD_FAULT_PIN, PAL_MODE_INPUT_PULLUP);  // RTDS signal
                                                                          // Digital outputs
  palSetPadMode(IMD_FAULT_INDICATOR_PORT, IMD_FAULT_INDICATOR_PIN,
                PAL_MODE_OUTPUT_PUSHPULL);  // IMD
  palSetPadMode(AMS_FAULT_INDICATOR_PORT, AMS_FAULT_INDICATOR_PIN,
                PAL_MODE_OUTPUT_PUSHPULL);  // AMS
  palSetPadMode(BSPD_FAULT_INDICATOR_PORT, BSPD_FAULT_INDICATOR_PIN,
                PAL_MODE_OUTPUT_PUSHPULL);  // BSPD
  palSetPadMode(STARTUP_SOUND_PORT, STARTUP_SOUND_PIN,
                PAL_MODE_OUTPUT_PUSHPULL);  // RTDS signal
  palSetPadMode(BRAKE_LIGHT_PORT, BRAKE_LIGHT_PIN, PAL_MODE_OUTPUT_PUSHPULL);  // Brake light signal
  palSetPadMode(STARTUP_LED_PORT, STARTUP_LED_PIN, PAL_MODE_OUTPUT_PUSHPULL);  // Brake light signal
  palSetPadMode(CAN1_RX_PORT, CAN1_RX_PIN, PAL_MODE_ALTERNATE(9));  // CAN RX
  palSetPadMode(CAN1_TX_PORT, CAN1_TX_PIN, PAL_MODE_ALTERNATE(9));  // CAN TX
  palSetPadMode(CAN1_STATUS_LED_PORT, CAN1_STATUS_LED_PIN,
                PAL_MODE_OUTPUT_PUSHPULL);
  palSetPadMode(CAN2_RX_PORT, CAN2_RX_PIN, PAL_MODE_ALTERNATE(9));  // CAN RX
  palSetPadMode(CAN2_TX_PORT, CAN2_TX_PIN, PAL_MODE_ALTERNATE(9));  // CAN TX
  palSetPadMode(CAN2_STATUS_LED_PORT, CAN2_STATUS_LED_PIN,
                PAL_MODE_OUTPUT_PUSHPULL);

  // Init LED states to LOW (including faults)
  palClearPad(IMD_FAULT_INDICATOR_PORT, IMD_FAULT_INDICATOR_PIN);
  palClearPad(AMS_FAULT_INDICATOR_PORT, AMS_FAULT_INDICATOR_PIN);
  palClearPad(BSPD_FAULT_INDICATOR_PORT, BSPD_FAULT_INDICATOR_PIN);
  palClearPad(STARTUP_SOUND_PORT, STARTUP_SOUND_PIN);
  palClearPad(BRAKE_LIGHT_PORT, BRAKE_LIGHT_PIN);
  palClearPad(STARTUP_LED_PORT, STARTUP_LED_PIN);
  palClearPad(CAN1_STATUS_LED_PORT, CAN1_STATUS_LED_PIN);
  palClearPad(CAN2_STATUS_LED_PORT, CAN2_STATUS_LED_PIN);

  palSetPad(STARTUP_LED_PORT, STARTUP_LED_PIN);

  chThdSleepMilliseconds(200);
  palClearPad(STARTUP_LED_PORT, STARTUP_LED_PIN);
  chThdSleepMilliseconds(200);

  Vehicle vehicle;

  CanBus canBusLv(&CAND1, CanBusBaudRate::k500k, false);
  chibios_rt::Mutex canBusLvMut;

  CanBus canBusHv(&CAND2, CanBusBaudRate::k500k, false);
  chibios_rt::Mutex canBusHvMut;

  palSetPad(STARTUP_LED_PORT, STARTUP_LED_PIN);

  EventQueue fsmEventQueue = EventQueue();

  CanChSubsys canLvChSubsys = CanChSubsys(canBusLv, canBusLvMut, fsmEventQueue);
  CanChSubsys canHvChSubsys = CanChSubsys(canBusHv, canBusHvMut, fsmEventQueue);
  AdcChSubsys adcChSubsys = AdcChSubsys(fsmEventQueue);
  DigInChSubsys digInChSubsys = DigInChSubsys(fsmEventQueue);
  TimerChSubsys timerChSubsys = TimerChSubsys(fsmEventQueue);

  /*
   * Create threads (many of which are driving subsystems)
   */
  chThdCreateStatic(canRxLvThreadFuncWa, sizeof(canRxLvThreadFuncWa),
                    NORMALPRIO, canRxLvThreadFunc, &canLvChSubsys);
  chThdCreateStatic(canTxLvThreadFuncWa, sizeof(canTxLvThreadFuncWa),
                    NORMALPRIO + 1, canTxLvThreadFunc, &canLvChSubsys);
  chThdCreateStatic(canRxHvThreadFuncWa, sizeof(canRxHvThreadFuncWa),
                    NORMALPRIO, canRxHvThreadFunc, &canHvChSubsys);
  chThdCreateStatic(canTxHvThreadFuncWa, sizeof(canTxHvThreadFuncWa),
                    NORMALPRIO + 1, canTxHvThreadFunc, &canHvChSubsys);
  chThdCreateStatic(adcThreadFuncWa, sizeof(adcThreadFuncWa), NORMALPRIO,
                    adcThreadFunc, &adcChSubsys);
  chThdCreateStatic(digInThreadFuncWa, sizeof(digInThreadFuncWa), NORMALPRIO,
                    digInThreadFunc, &digInChSubsys);
  chThdCreateStatic(timerThreadFuncWa, sizeof(timerThreadFuncWa), NORMALPRIO,
                    timerThreadFunc, &timerChSubsys);

  adcChSubsys.addPin(Gpio::kA1);  // add brake input
  adcChSubsys.addPin(Gpio::kA2);  // add throttle input A
  adcChSubsys.addPin(Gpio::kA3);  // add throttle input B
  adcChSubsys.addPin(Gpio::kA6);  // add steering input

  digInChSubsys.addPin(DigitalInput::kToggleUp);
  digInChSubsys.addPin(DigitalInput::kToggleDown);
  digInChSubsys.addPin(DigitalInput::kDriveMode);
  digInChSubsys.addPin(DigitalInput::kBSPDFault);

  static const SerialConfig sConfig = { 115200, 0, USART_CR2_STOP1_BITS, 0 };
  palSetLineMode(UART_RX_LINE, PAL_MODE_ALTERNATE(7));  // UART RX
  palSetLineMode(UART_TX_LINE, PAL_MODE_ALTERNATE(7));  // UART TX
  sdStart(&SD3, &sConfig);

  printf3("STARTING UP\n");

  for (uint8_t i = 0; i < 3; i++) {
    palWritePad(IMD_FAULT_INDICATOR_PORT, IMD_FAULT_INDICATOR_PIN, PAL_HIGH);  // IMD
    palWritePad(AMS_FAULT_INDICATOR_PORT, AMS_FAULT_INDICATOR_PIN, PAL_HIGH);  // AMS
    palWritePad(BSPD_FAULT_INDICATOR_PORT, BSPD_FAULT_INDICATOR_PIN, PAL_HIGH);  // BSPD
    //palWritePad(STARTUP_SOUND_PORT, STARTUP_SOUND_PIN, PAL_HIGH);  // RTDS
    palWritePad(BRAKE_LIGHT_PORT, BRAKE_LIGHT_PIN, PAL_HIGH);  // Brake Light
    chThdSleepMilliseconds(200);
    palWritePad(IMD_FAULT_INDICATOR_PORT, IMD_FAULT_INDICATOR_PIN, PAL_LOW);  // IMD
    palWritePad(AMS_FAULT_INDICATOR_PORT, AMS_FAULT_INDICATOR_PIN, PAL_LOW);  // AMS
    palWritePad(BSPD_FAULT_INDICATOR_PORT, BSPD_FAULT_INDICATOR_PIN, PAL_LOW);  // Temp
    //palWritePad(STARTUP_SOUND_PORT, STARTUP_SOUND_PIN, PAL_LOW);  // RTDS
    palWritePad(BRAKE_LIGHT_PORT, BRAKE_LIGHT_PIN, PAL_LOW);      // Brake Light

    chThdSleepMilliseconds(200);
  }
  palWritePad(BRAKE_LIGHT_PORT, BRAKE_LIGHT_PIN, PAL_HIGH);  // Brake Light

  while (1) {
    uint16_t test = vehicle.driveProfile;
    HeartbeatMessage heartbeat(test);

    canLvChSubsys.startSend(heartbeat);
    canHvChSubsys.startSend(heartbeat);

    while (fsmEventQueue.size() > 0) {
      Event e = fsmEventQueue.pop();

      if (e.type() == Event::Type::kDigInTransition) {
        DigitalInput digInPin = e.digInPin();
        bool digInState = e.digInState();
        switch (digInPin) {
          case DigitalInput::kToggleUp:
            if (digInState) {
              vehicle.dashInputs |= toggleUp;
              // printf3("toggleUp\n");
              // printf3("T:%d\n",vehicle.throttleVal);
                  printf3("brakes:%d  T1: %d   T2: %d\n",vehicle.brakeVoltage,vehicle.throttleA,vehicle.throttleB);

//              printf3("Brakes:%d\n", vehicle.brakeVoltage);
            } else {
              vehicle.dashInputs &= ~toggleUp;
              // printf3("toggleUp Released\n");
            }
            break;
          case DigitalInput::kToggleDown:
            if (digInState) {
              vehicle.dashInputs |= toggleDown;
//              printf3("ThrottleA:%d\n", vehicle.throttleA);
              // printf3("toggleDown\n");
              // printf3("B:%d\n",vehicle.brakeVoltage);
              // printf3("B:%d\n",vehicle.brakeVal);
            } else {
              vehicle.dashInputs &= ~toggleDown;
              // printf3("toggleDown Released\n");
            }
            break;
          case DigitalInput::kDriveMode:
            if (digInState) {
              vehicle.dashInputs |= revButton;
//              printf3("ThrottleB:%d\n", vehicle.throttleB);
              // printf3("Reverse\n");
              // printf3("S:%d\n",vehicle.steeringAngle);
              printf3("B:%d   T1:%d   T2:%d   S:%d\n",vehicle.brakeVoltage,vehicle.throttleA,vehicle.throttleB,vehicle.steeringIn);
            } else {
              vehicle.dashInputs &= ~revButton;
              // printf3("Reverse Released\n");
            }
            break;
          case DigitalInput::kBSPDFault:
            if (digInState) {
              printf3("BSPD Dicked\n");
              palWritePad(BSPD_FAULT_INDICATOR_PORT, BSPD_FAULT_INDICATOR_PIN,
                          PAL_HIGH);        // BSPD
              vehicle.faults |= BSPDFault;  // BSPD Fucked
            } else {
              printf3("BSPD Undicked\n");
              palWritePad(BSPD_FAULT_INDICATOR_PORT, BSPD_FAULT_INDICATOR_PIN,
                          PAL_HIGH);         // BSPD
              vehicle.faults &= ~BSPDFault;  // BSPD Reset
            }
            break;
          default:
            break;
        }
      } else if (e.type() == Event::Type::kCanRx) {
        std::array < uint16_t, 8 > canData = e.canFrame();
        uint32_t canEid = e.canEid();

        if(e.canSource() == Event::CanSource::kHv) {
          CANTxFrame msg;
          msg.EID = canEid;
          msg.DLC = 8;
          msg.IDE = CAN_IDE_STD;
          msg.RTR = CAN_RTR_DATA;
          for(int i = 0; i < 8; i++) {
            msg.data8[i] = canData[i];
          }
          canLvChSubsys.startSend(msg);
        }
        
        switch (canEid) {
          case kFuncIdHeartBeatECU:  // HEARTBEAT
            //do nothing 
           // printf3("Beat\n");
            break;
          case kFuncIdCellTempAdc[0]:  // Replace with Cell Temp ID Row 0
            for (int i = 0; i < 7; i++) {
              vehicle.cellTemps[i] = canData[i];
            }
            break;
          case kFuncIdCellTempAdc[1]:  // Replace with Cell Temp ID Row 1
            for (int i = 0; i < 7; i++) {
              vehicle.cellTemps[i + 7] = canData[i];
            }
            break;
          case kFuncIdCellTempAdc[2]:  // Replace with Cell Temp ID Row 2
            for (int i = 0; i < 7; i++) {
              vehicle.cellTemps[i + 14] = canData[i];
            }
            break;
          case kFuncIdCellTempAdc[3]:  // Replace with Cell Temp ID Row 3
            for (int i = 0; i < 7; i++) {
              vehicle.cellTemps[i + 21] = canData[i];
            }
            break;
          case kFuncIdCellVoltage[0]:  // Replace with Cell Voltage ID Row 0
            for (int i = 0; i < 7; i++) {
              vehicle.cellVoltages[i] = canData[i];
            }
            break;
          case kFuncIdCellVoltage[1]:  // Replace with Cell Voltage ID Row 1
            for (int i = 0; i < 7; i++) {
              vehicle.cellVoltages[i + 7] = canData[i];
            }
            break;
          case kFuncIdCellVoltage[2]:  // Replace with Cell Voltage ID Row 2
            for (int i = 0; i < 7; i++) {
              vehicle.cellVoltages[i + 14] = canData[i];
            }
            break;
          case kFuncIdCellVoltage[3]:  // Replace with Cell Voltage ID Row 3
            for (int i = 0; i < 7; i++) {
              vehicle.cellVoltages[i + 21] = canData[i];
            }
            break;
          case kFuncIdCellVoltage[4]:  // Replace with Cell Voltage ID Row 3
            for (int i = 0; i < 7; i++) {
              vehicle.cellVoltages[i + 21] = canData[i];
            }
            break;
          case kFuncIdCellVoltage[5]:  // Replace with Cell Voltage ID Row 3
            for (int i = 0; i < 7; i++) {
              vehicle.cellVoltages[i + 21] = canData[i];
            }
            break;
          case kFuncIdCellVoltage[6]:  // Replace with Cell Voltage ID Row 3
            for (int i = 0; i < 7; i++) {
              vehicle.cellVoltages[i + 21] = canData[i];
            }
            break;
          case kFuncIdCellStartup:  // Replace with State ID

            break;
          case kFuncIdFaultStatus:  // Replace with Pack Voltage ID

            break;
          case kFuncIdBmsStat:  // Replace with Current ID

            break;
          default:
            printf3("Unknown CAN ID 0x%x: 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n", canEid, canData[0], canData[1], canData[2], canData[3], canData[4], canData[5], canData[6], canData[7]);
            break;
        }
      } else if (e.type() == Event::Type::kAdcConversion) {
        Gpio adcPin = e.adcPin();
        uint16_t adcIn = e.adcValue();
        if (adcPin == Gpio::kA1) {  // Brakes
          vehicle.brakeVoltage = adcIn;
          //printf3("Brakes:%d\n", vehicle.brakeVoltage);
        } else if (adcPin == Gpio::kA2) {  // Throttle A
          vehicle.throttleA = adcIn;
          //printf3("ThrottleA:%d\n", vehicle.throttleA);
        } else if (adcPin == Gpio::kA3) {  // Throttle B
          vehicle.throttleB = adcIn;
          //printf3("ThrottleB:%d\n", vehicle.throttleB);
        } else if (adcPin == Gpio::kA6) {  // Steering
          vehicle.steeringIn = adcIn;
          // printf3("Steering:%d\n", adcIn);
        }
        vehicle.HandleADCs();
        ThrottleMessageLV throttleMessageLV(vehicle.throttleValDash, vehicle.driveProfile);
        canLvChSubsys.startSend(throttleMessageLV);
        ThrottleMessageHV throttleMessageHV(vehicle.throttleVal, vehicle.forwardSwitch, vehicle.reverseSwitch);
        canHvChSubsys.startSend(throttleMessageHV);
        SteeringMessage steeringMessage(vehicle.steeringAngle);
        canLvChSubsys.startSend(steeringMessage);
        BrakeMessage brakeMessage(vehicle.brakeVal);
        canLvChSubsys.startSend(brakeMessage);

      } else if (e.type() == Event::Type::kTimerTimeout) {
        if (e.timer() == vt_SM_D) {
          // printf3("SM_F Timer\n");
          vehicle.timerDoneFlag += VT_SM_D;
        } else if (e.timer() == vt_SM_R) {
          // printf3("SM_R Timer\n");
          vehicle.timerDoneFlag += VT_SM_R;
        } else if (e.timer() == vt_F_Throttle) {
          // printf3("Throttle Fault");
          vehicle.timerDoneFlag += VT_F_Throttle;
        } else {
          printf3("What the timer shit\n");
        }
      }
      // printf3("ECU Undicked\n");
    }  // END EVENT QUEUE

    if (vehicle.timerStartFlag > 0) {  // Real burnt timer shit
      if (vehicle.timerStartFlag & VT_SM_D) {
        vehicle.timerStartFlag = 0;
        timerChSubsys.startTimer(vt_SM_D, 2000);
      }
      if (vehicle.timerStartFlag & VT_SM_R) {
        vehicle.timerStartFlag = 0;
        timerChSubsys.startTimer(vt_SM_R, 2000);
      }
      if (vehicle.timerStartFlag & VT_F_Throttle) {
        vehicle.timerStartFlag = 0;
        timerChSubsys.startTimer(vt_F_Throttle, 100);
      }
    }

//    printf3("brakes:%d  T1: %d   T2: %d\n",vehicle.brakeVoltage,vehicle.throttleA,vehicle.throttleB);
    
    vehicle.FSM();  // update vehicle state

    // TODO: use condition var to signal that events are present in the queue
    // NOTE: must be fast enough to deplete event queue quickly enough
    chThdSleepMilliseconds(10);
  }
}
