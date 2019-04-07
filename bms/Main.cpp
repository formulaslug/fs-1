#include <initializer_list>
#include "LTC6811.h"
#include "ch.hpp"
#include "chprintf.h"
#include "hal.h"

using namespace chibios_rt;

// Thread to run SPI IO and report back over serial
// NOTE: SpiThread is primarily for testing
class SpiThread : public BaseStaticThread<256> {
 public:
  SpiThread() {}

 protected:
  virtual void main(void) {
    setName("SPI thread");

    uint16_t cr1 = SPI_CR1_BR_2 | SPI_CR1_BR_1;                 // prescalar 128
    uint16_t cr2 = SPI_CR2_DS_2 | SPI_CR2_DS_1 | SPI_CR2_DS_0;  // Data width 8
    SPIConfig spiConf = SPIConfig{false, NULL, GPIOA, 4, cr1, cr2};

    // Set pin modes for SPI
    // MOSI
    palSetPadMode(GPIOA, 7, PAL_MODE_ALTERNATE(5) | PAL_STM32_OSPEED_HIGHEST);
    // MISO
    palSetPadMode(GPIOA, 6, PAL_MODE_ALTERNATE(5));
    // SCK
    palSetPadMode(GPIOA, 5, PAL_MODE_ALTERNATE(5) | PAL_STM32_OSPEED_HIGHEST);
    // NSS
    palSetPadMode(GPIOA, 4, PAL_MODE_ALTERNATE(5) | PAL_STM32_OSPEED_HIGHEST);

    LTC6811 chip0 = LTC6811(&SPID1, &spiConf, 0);

    // Initialize serial for logging
    const SerialConfig serialConf = {
        115200, /* baud rate */
        0,      /* cr1 */
        0,      /* cr2 */
        0       /* cr3 */
    };
    sdStart(&SD2, &serialConf);
    sdWrite(&SD2, (const uint8_t*)"Init Serial\r\n", 13);

    // TODO: Change to nicer constructing
    LTC6811::Command readVoltageA =
        LTC6811::buildCommand(LTC6811::AddressingMode::kAddress, 0,
                              LTC6811::CommandCode::kReadVoltageA);
    LTC6811::Command readVoltageB = LTC6811::Command{.value = 0x8006};
    LTC6811::Command readVoltageC = LTC6811::Command{.value = 0x8008};
    LTC6811::Command readVoltageD = LTC6811::Command{.value = 0x800A};

    LTC6811::Command startAdc = LTC6811::Command{.value = 0x0360};

    LTC6811::Command writeConfA = LTC6811::Command{.value = 0x0001};

    // Config tables to disable and enable GPIO pins
    uint8_t data0[] = {0x04, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t data1[] = {0xFC, 0x00, 0x00, 0x00, 0x00, 0x00};

    uint8_t rxbuf[32];

    while (1) {
      // Read and print voltage values
      chip0.wakeupSpi();
      chip0.sendCommand(startAdc);
      sleep(2);
      chip0.readCommand(readVoltageA, rxbuf);
      chip0.readCommand(readVoltageB, rxbuf + 8);
      chip0.readCommand(readVoltageC, rxbuf + 16);
      chip0.readCommand(readVoltageD, rxbuf + 24);

      chprintf((BaseSequentialStream*)&SD2, "Response: \r\n");
      for (int i = 0; i < 32; i++) {
        chprintf((BaseSequentialStream*)&SD2, "0x%02x ", rxbuf[i]);
        if ((i + 1) % 8 == 0) chprintf((BaseSequentialStream*)&SD2, "\r\n");
      }

      // Set GPIO pins to open drain
      chip0.wakeupSpi();
      chip0.sendCommandWithData(writeConfA, data0);

      sleep(1500);

      // Set GPIO pins to off
      chip0.wakeupSpi();
      chip0.sendCommandWithData(writeConfA, data1);

      sleep(1500);
    }
  }
};

// Static thread objects
static SpiThread spiThread;

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

  // Start main spi thread
  spiThread.start(NORMALPRIO + 1);

  // Flash LEDs to indicate startup
  for (int i = 0; i < 4; i++) {
    palWriteLine(LINE_LED_GREEN, PAL_HIGH);
    chThdSleepMilliseconds(50);
    palWriteLine(LINE_LED_GREEN, PAL_LOW);
    chThdSleepMilliseconds(50);
  }

  while (1) {
    // Sleep 24 hours
    // NOTE: This could be a much smaller value
    chThdSleepMilliseconds(1000 * 60 * 60 * 24);
  }
}
