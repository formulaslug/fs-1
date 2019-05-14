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
      m_chips[i].getConfig().gpio5 = LTC6811::GPIOOutputState::kHigh;
      // m_chips[i].updateConfig();
    }
  }

  void getValues() {}

 private:
  unsigned int m_delay;
  LTC6811Bus* m_bus;
  std::vector<LTC6811> m_chips;

  const LTC6811Bus::Command startAdc = LTC6811Bus::buildBroadcastCommand(
      StartCellVoltageADC(AdcMode::k7k, false, CellSelection::kAll));
  const LTC6811Bus::Command readStatusGroup =
      LTC6811Bus::buildAddressedCommand(0, ReadConfigurationGroupA());

  static constexpr unsigned int NUM_CHIPS = 1;  // TODO: change to 4

  static constexpr unsigned int NUM_TEMP = 7;

 protected:
  void main() {
    while (!shouldTerminate()) {
      for (int i = 0; i < NUM_CHIPS; i++) {
        LTC6811::Configuration& conf = m_chips[i].getConfig();
        conf.gpio5 = LTC6811::GPIOOutputState::kLow;
        conf.gpio1 = LTC6811::GPIOOutputState::kPullDown;
        m_chips[i].updateConfig();

        continue;  // TODO: this skips the rest of this for testing

        uint8_t rxbuf[6];

        m_bus->readCommand(readStatusGroup, rxbuf);
        chprintf((BaseSequentialStream*)&SD2, "Voltages: \r\n");
        for (int i = 0; i < 6; i++) {
          chprintf((BaseSequentialStream*)&SD2, "0x%02x ", rxbuf[i]);
        }
        chprintf((BaseSequentialStream*)&SD2, "\r\n");

        // Skip rest
        continue;

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

          chprintf((BaseSequentialStream*)&SD2, "Temp %d: 0x%02x\r\n", j,
                   temps[3]);

          delete temps;
        }

        conf.gpio5 = LTC6811::GPIOOutputState::kPassive;
        m_chips[i].updateConfig();
      }
      chprintf((BaseSequentialStream*)&SD2, "Sleeping...\r\n");
      sleep(m_delay);
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

  const SerialConfig serialConf = {
      115200, /* baud rate */
      0,      /* cr1 */
      0,      /* cr2 */
      0       /* cr3 */
  };
  sdStart(&SD2, &serialConf);
  sdWrite(&SD2, (const uint8_t*)"Init Serial\r\n", 13);

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

  uint8_t txbuf[2] = {0x00, 0x01};
  uint8_t databuf[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  // ltcBus.sendData(txbuf, databuf);
  // Start main spi thread
  // bms.start(NORMALPRIO + 2);
  // Initialize serial for logging

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
