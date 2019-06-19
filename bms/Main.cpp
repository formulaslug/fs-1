#include "Main.h"

#include "BmsConfig.h"
#include "CanOpenPdo.h"
#include "common.h"

#include "ch.hpp"
#include "chprintf.h"
#include "hal.h"

#include "BmsThread.h"
#include "CANOptions.h"
#include "LTC6811Bus.h"

using namespace chibios_rt;

// When car is off maybe one reading every 10 sec
// when car is on 10 readings per second

// TODO: map 12 cells to number of cells
// TODO: change m_chips to array rather than vector
// TODO: publish fault states
// TODO: SoC tracking using Ah counting and voltage measuring

// Fault states:
// - AMS
// - IMD
//
// * Over temp
// - Under temp
// * Over voltage
// * Under voltage
//
// * Failed init
// * Comm fail

static constexpr CANConfig cancfg =
    CANOptions::config<CANOptions::BaudRate::k500k, false>();

int main() {
  /*
   * System initializations.
   * - HAL initialization, this also initializes the configured device drivers
   *   and performs the board-specific initializations.
   * - Kernel initialization, the main() function becomes a thread and the
   *   RTOS is active.
   */
  halInit();
  System::init();

  // Init all io pins
  initIO();

  // Initialize serial for logging
  const SerialConfig serialConf = {
      115200, /* baud rate */
      0,      /* cr1 */
      0,      /* cr2 */
      0       /* cr3 */
  };
  sdStart(&SD2, &serialConf);
  sdWrite(&SD2, (const uint8_t*)"Serial Init\r\n", 13);

  canStart(&BMS_CAN_DRIVER, &cancfg);
  CANTxFrame txmsg;
  txmsg.IDE = CAN_IDE_STD;
  txmsg.RTR = CAN_RTR_DATA;
  txmsg.SID = kFuncIdCellStartup;
  txmsg.DLC = 8;
  uint8_t msg[8] = {'S', 'P', 'I', 'C', 'Y', 'B', 'O', 'I'};
  for (size_t i = 0; i < 8; i++) {
    txmsg.data8[i] = msg[i];
  }
  if (canTransmit(&BMS_CAN_DRIVER, CAN_ANY_MAILBOX, &txmsg, TIME_MS2I(100)) !=
      MSG_OK) {
    sdWrite(&SD2, (const uint8_t*)"Can startup failed\r\n", 20);
  }
  chThdSleepMilliseconds(1000);

  // Init spi config
  SPIConfig* spiConf = new SPIConfig{false /* Circular buffer */,
                                     NULL /* Callback */,
                                     PAL_PORT(LINE_SPI_SSEL) /* SSEL port */,
                                     PAL_PAD(LINE_SPI_SSEL) /* SSEL pin */,
                                     BMS_SPI_CR1 /* CR1 (prescalar) */,
                                     BMS_SPI_CR2 /* CR2 (data width) */};

  LTC6811Bus ltcBus = LTC6811Bus(&BMS_SPI_DRIVER, spiConf);

  BMSThread bmsThread(&ltcBus, 1);
  bmsThread.start(NORMALPRIO + 1);

  // Flash LEDs to indicate startup
  for (int i = 0; i < 4; i++) {
    palWriteLine(LINE_LED_GREEN, PAL_HIGH);
    chThdSleepMilliseconds(50);
    palWriteLine(LINE_LED_GREEN, PAL_LOW);
    chThdSleepMilliseconds(50);
  }

  while (1) {
    // Sleep 100 secs
    chThdSleepMilliseconds(100 * 1000);
  }
}

void initIO() {
  // Set modes for IO
  palSetLineMode(LINE_BMS_FLT, PAL_MODE_OUTPUT_PUSHPULL);
  palSetLineMode(LINE_BMS_FLT_LAT, PAL_MODE_INPUT);
  palSetLineMode(LINE_IMD_STATUS, PAL_MODE_INPUT);
  palSetLineMode(LINE_IMD_FLT_LAT, PAL_MODE_INPUT);
  palSetLineMode(LINE_CHARGER_CONTROL, PAL_MODE_OUTPUT_PUSHPULL);

  // Reset BMS fault line
  palSetLine(LINE_BMS_FLT);
  palClearLine(LINE_BMS_FLT);

  // Set modes for SPI
  palSetLineMode(LINE_SPI_MISO,
                 PAL_MODE_ALTERNATE(5) | PAL_STM32_OSPEED_HIGHEST);
  palSetLineMode(LINE_SPI_MOSI,
                 PAL_MODE_ALTERNATE(5) | PAL_STM32_OSPEED_HIGHEST);
  palSetLineMode(LINE_SPI_SCLK,
                 PAL_MODE_ALTERNATE(5) | PAL_STM32_OSPEED_HIGHEST);
  palSetLineMode(LINE_SPI_SSEL,
                 PAL_MODE_OUTPUT_PUSHPULL | PAL_STM32_OSPEED_HIGHEST);

  // Set modes for CAN
  palSetLineMode(LINE_CAN_TX, PAL_MODE_ALTERNATE(9) | PAL_STM32_OSPEED_HIGHEST);
  palSetLineMode(LINE_CAN_RX, PAL_MODE_ALTERNATE(9));
}
