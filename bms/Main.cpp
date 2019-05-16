#include <functional>
#include <initializer_list>
#include <vector>

#include "ch.hpp"
#include "hal.h"

#include "chprintf.h"

#include "LTC6811.h"
#include "LTC6811Bus.h"

#include "LTC6811Commands.h"
#include "common.h"

using namespace chibios_rt;

class BMSThread : public BaseStaticThread<1024> {
 public:
  BMSThread(LTC6811Bus* bus, unsigned int frequency) : m_bus(bus) {
    m_delay = 1000 / frequency;
    for (int i = 0; i < NUM_CHIPS; i++) {
      m_chips.push_back(LTC6811(*bus, i));
    }
    for (int i = 0; i < NUM_CHIPS; i++) {
      m_chips[i].getConfig().gpio5 = LTC6811::GPIOOutputState::kLow;
      m_chips[i].getConfig().gpio4 = LTC6811::GPIOOutputState::kPassive;

      // NOTE: following line causes crash because spi driver has not been
      // initialized yet

      // m_chips[i].updateConfig();
    }
  }

  void getValues() {}

 private:
  unsigned int m_delay;
  LTC6811Bus* m_bus;
  std::vector<LTC6811> m_chips;

  static constexpr unsigned int NUM_CHIPS = 1;  // TODO: change to 4
  static constexpr unsigned int NUM_TEMP = 7;

 protected:
  void main() {
    while (!shouldTerminate()) {
      for (int i = 0; i < NUM_CHIPS; i++) {
        LTC6811::Configuration& conf = m_chips[i].getConfig();
        conf.gpio5 = LTC6811::GPIOOutputState::kLow;
        m_chips[i].updateConfig();

        uint16_t* voltages = m_chips[i].getVoltages();

        // Process voltages
        chprintf((BaseSequentialStream*)&SD2, "Voltages: \r\n");
        for (int i = 0; i < 12; i++) {
          chprintf((BaseSequentialStream*)&SD2, "0x%02x ", voltages[i]);
        }
        chprintf((BaseSequentialStream*)&SD2, "\r\n");
        delete voltages;

        // Measure all temp sensors
        for (int j = 0; j < NUM_TEMP; j++) {
          conf.gpio1 = (j & 0x01) != 0 ? LTC6811::GPIOOutputState::kHigh
                                       : LTC6811::GPIOOutputState::kLow;
          conf.gpio2 = (j & 0x02) != 0 ? LTC6811::GPIOOutputState::kHigh
                                       : LTC6811::GPIOOutputState::kLow;
          conf.gpio3 = (j & 0x04) != 0 ? LTC6811::GPIOOutputState::kHigh
                                       : LTC6811::GPIOOutputState::kLow;
          m_chips[i].updateConfig();

          uint16_t* temps = m_chips[i].getGpio();
          int temp = temps[3] / 10;

          chprintf((BaseSequentialStream*)&SD2, "Temp %d: %dmV\r\n", j, temp);

          delete temps;
        }

        conf.gpio5 = LTC6811::GPIOOutputState::kHigh;
        m_chips[i].updateConfig();
      }
      chprintf((BaseSequentialStream*)&SD2, "Sleeping...\r\n");
      chThdSleepMilliseconds(m_delay);
    }
  }
};

class KeepAliveThread : public BaseStaticThread<256> {
 public:
  // Frequency in Hz
  KeepAliveThread(LTC6811Bus* bus, unsigned int frequency) : m_bus(bus) {
    m_delay = 1000 / frequency;
  }

 private:
  unsigned int m_delay;
  LTC6811Bus* m_bus;

 protected:
  void main() {
    while (!shouldTerminate()) {
      m_bus->wakeupSpi();
      chprintf((BaseSequentialStream*)&SD2, "Sleeping...\r\n");
      chThdSleepMilliseconds(m_delay);
    }
  }
};

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

  KeepAliveThread keepAliveThd(&ltcBus, 10);
  // keepAliveThd.start(NORMALPRIO + 1);

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
    // Sleep 24 hours
    // NOTE: This could be a much smaller value
    // ltcBus.sendData(txbuf, databuf);
    chThdSleepMilliseconds(100);
  }
}
