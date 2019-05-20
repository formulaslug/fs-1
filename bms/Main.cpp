#include "ch.hpp"
#include "hal.h"

#include "chprintf.h"

#include "BmsThread.h"
#include "LTC6811Bus.h"

#include "common.h"

using namespace chibios_rt;

// When car is off maybe one reading every 10 sec
// when car is on 10 readings per second

// TODO: map 12 cells to number of cells
// TODO: change m_chips to array rather than vector
// TODO: publish fault states
// TODO: SoC tracking using Ah counting and voltage measuring

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

  // Initialize serial for logging
  const SerialConfig serialConf = {
      115200, /* baud rate */
      0,      /* cr1 */
      0,      /* cr2 */
      0       /* cr3 */
  };
  sdStart(&SD2, &serialConf);
  sdWrite(&SD2, (const uint8_t*)"Serial Init\r\n", 13);

  // MOSI
  palSetPadMode(GPIOA, 7, PAL_MODE_ALTERNATE(5) | PAL_STM32_OSPEED_HIGHEST);
  // MISO
  palSetPadMode(GPIOA, 6, PAL_MODE_ALTERNATE(5) | PAL_STM32_OSPEED_HIGHEST);
  // SCLK
  palSetPadMode(GPIOA, 5, PAL_MODE_ALTERNATE(5) | PAL_STM32_OSPEED_HIGHEST);
  // SSEL
  palSetPadMode(GPIOA, 4, PAL_MODE_OUTPUT_PUSHPULL | PAL_STM32_OSPEED_HIGHEST);
  //  palSetPad(GPIOA, 4);

  uint16_t cr1 = SPI_CR1_BR_2 | SPI_CR1_BR_1;                 // prescalar 128
  uint16_t cr2 = SPI_CR2_DS_2 | SPI_CR2_DS_1 | SPI_CR2_DS_0;  // Data width 8
  SPIConfig* spiConf = new SPIConfig{false, NULL, GPIOA, 4, cr1, cr2};

  LTC6811Bus ltcBus = LTC6811Bus(&SPID1, spiConf);

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
