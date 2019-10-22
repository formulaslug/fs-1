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
 |  __ \   /\   / _  \
 | |  | | /  \ | |  | |
 | |  | |/ /\ \| |  | |
 | |__| / ____ \ |__| |
 |_____/_/    \_\___\_\
 
 
 */

#include "ch.h"
#include "hal.h"

#include "CANOptions.h"
#include "fsprintf.h"

static mutex_t print_mutex;
static auto printf = SDPrinter<&SD2>();
//#define CH_CFG_ST_RESOLUTION                64
//#define CH_CFG_ST_FREQUENCY                 10 //1000gives 100ms resolutio
//#define CH_CFG_INTERVALS_SIZE               16


static THD_WORKING_AREA(can_rx_wa, 256);
static THD_FUNCTION(can_rx, p) {
	event_listener_t el;
	CANRxFrame rxmsg;
	(void)p;

	chRegSetThreadName("receiver");
	chEvtRegister(&CAND1.rxfull_event, &el, 0);
	while (true) {
		if (chEvtWaitAnyTimeout(ALL_EVENTS, TIME_MS2I(100)) == 0) {
			//palClearLine(LINE_LED_GREEN);
			continue;
		}
		while (canReceive(&CAND1, CAN_ANY_MAILBOX, &rxmsg, TIME_IMMEDIATE) == MSG_OK) {
			/* Process message.*/
            uint32_t id;
			if (rxmsg.IDE == CAN_IDE_EXT) {
                id = rxmsg.EID;
			}
			else {
                id = rxmsg.SID;
			}

            printf("%X %02X%02X%02X%02X%02X%02X%02X%02X %d\n", id,
                    rxmsg.data8[0], rxmsg.data8[1], rxmsg.data8[2], rxmsg.data8[3],
                    rxmsg.data8[4], rxmsg.data8[5], rxmsg.data8[6], rxmsg.data8[7],
                    chVTGetSystemTimeX());

			palToggleLine(LINE_LED_GREEN);
		}
	}
	chEvtUnregister(&CAND1.rxfull_event, &el);
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
    static constexpr CANConfig cancfg = CANOptions::config<CANOptions::BaudRate::k500k, false>();
	palSetLineMode(LINE_ARD_D10, PAL_MODE_ALTERNATE(9));  // CAN RX
	palSetLineMode(LINE_ARD_D2, PAL_MODE_ALTERNATE(9));  // CAN TX
	canStart(&CAND1, &cancfg);

	/*
	 * Starting the transmitter and receiver threads.
	 */
	chThdCreateStatic(can_rx_wa, sizeof(can_rx_wa), NORMALPRIO + 7, can_rx,
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
