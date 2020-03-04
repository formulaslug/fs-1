/*
 ChibiOS - Copyright (C) 2006..2018 Giovanni Di Sirio

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
 */

/*
 *_____          ____  
 |  __ \   /\   / __ \ 
 | |  | | /  \ | |  | |
 | |  | |/ /\ \| |  | |
 | |__| / ____ \ |__| |
 |_____/_/    \_\___\_\
 
 
 */

// chibiOS includes
#include "ch.h"
#include "hal.h"

// these are unnecessary for building
// commented out because the project would not compile in this directory
#include "rt_test_root.h"
#include "oslib_test_root.h"

// voodoo includes
#include "DaqConfig.h"
#include "fscore/fsCAN.h"
#include "fscore/fsprintf.h"

// configures CAN listener settings
// according to vehicle CAN configuration
static constexpr CANConfig cancfg = CANOptions::config<
  		CANOptions::BaudRate::k500k, false>();

// printf() overloading???
static mutex_t print_mutex;
static auto printf = SDPrinter<&SD2>();
//#define CH_CFG_ST_RESOLUTION                64
//#define CH_CFG_ST_FREQUENCY                 10 //1000gives 100ms resolutio
//#define CH_CFG_INTERVALS_SIZE               16


// new buffer code
#include "CircularBuffer.h"
#include "common.h"
auto messages = new CircularBuffer<CANRxFrame>(10);

// creates thread to listen to CAN messages
static THD_WORKING_AREA(can_rx_wa, 256);
static THD_FUNCTION(can_rx, p) {
	event_listener_t el; // stores events?
	CANRxFrame rxmsg;    // contains CAN message data
	(void)p;             // wtf is this for

	chRegSetThreadName("receiver");
	chEvtRegister(&CAND1.rxfull_event, &el, 0); // listens for CAN events, pases them to event listener
                                              // is this even necessary?
	while (true) {
		// if (chEvtWaitAnyTimeout(ALL_EVENTS, TIME_MS2I(100)) == 0) {  // for blinking lights?
		// 	//palClearLine(LINE_LED_GREEN);
		// 	continue;
		// }
		while (canReceive(&CAND1, CAN_ANY_MAILBOX, &rxmsg, TIME_IMMEDIATE) == MSG_OK) {
			/* Process message.*/
            uint32_t id;
			if (rxmsg.IDE == CAN_IDE_EXT) {
                id = rxmsg.EID;
//				printf("T%d I%X D%d\n", chVTGetSystemTimeX() ,rxmsg.EID,
//						rxmsg.data16[0]);
//				printf("T%d I%X D%d\n", rxmsg.TIME ,rxmsg.EID,
//										rxmsg.data16[0]);
			}
			else {
                id = rxmsg.SID;
			}

//				printf("T%d I%X D%d\n", chVTGetSystemTimeX() ,rxmsg.EID,
      if (messages->Capacity() == messages->Size()) {
        // buffer is at capacity
        printf("buffer is full, ya printf function sucks dick\n");
      } else {
        messages->PushBack(rxmsg);
      }

			palToggleLine(LINE_LED_GREEN);
		}
	}
	chEvtUnregister(&CAND1.rxfull_event, &el);
}


static THD_WORKING_AREA(serial_out_wa, 256);
static THD_FUNCTION(serial_out, p) {
  (void)p;

  if (messages->Capacity() > 0) {
    CANRxFrame rxmsg = messages->PopFront();

    uint32_t id;
    if (rxmsg.IDE == CAN_IDE_EXT) {
      id = rxmsg.EID;
    } else {
      id = rxmsg.SID;
    }

    // prints can message to serial
    // this is too slow, replace with writing whole message to sim card somehow
    printf("ID=%X DATA=%02X%02X%02X%02X%02X%02X%02X%02X\n", id,
            rxmsg.data8[0], rxmsg.data8[1], rxmsg.data8[2], rxmsg.data8[3],
            rxmsg.data8[4], rxmsg.data8[5], rxmsg.data8[6], rxmsg.data8[7]);
  }
}


/*
 * Application entry point.
 */
int main(void) {

	/*
	 * System initializations.
	 * - HAL initialization, this also initializes the configured device drivers
	 *   and performs the board-specific initializations.
	 * - Kernel initialization, the main() function becomes a thread and the
	 *   RTOS is active.
	 */
	halInit();
	chSysInit();

	static const SerialConfig config = { 230400, 0, USART_CR2_STOP1_BITS, USART_CR3_CTSE };
	chMtxObjectInit(&print_mutex);
	sdStart(&SD2, &config);

	/*
	 * Activates the CAN driver 1.
	 */
	palSetLineMode(LINE_ARD_D10, PAL_MODE_ALTERNATE(9));  // CAN RX
	palSetLineMode(LINE_ARD_D2, PAL_MODE_ALTERNATE(9));  // CAN TX
	canStart(&CAND1, &cancfg);

	/*
	 * Starting the transmitter and receiver threads.
	 */
	chThdCreateStatic(can_rx_wa, sizeof(can_rx_wa), NORMALPRIO + 7, can_rx,
			NULL);
	//chThdCreateStatic(can_tx_wa, sizeof(can_tx_wa), NORMALPRIO + 7, can_tx,
	//		NULL);
	chThdCreateStatic(serial_out_wa, sizeof(serial_out_wa), NORMALPRIO + 7, serial_out,
			NULL);
	/*
	 * Normal main() thread activity, in this demo it does nothing except
	 * sleeping in a loop and check the button state.
	 */
	while (true) {
		chThdSleepMilliseconds(10000);
	}
}

// vim: sts=4 sw=4 ts=4 et
