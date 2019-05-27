#pragma once

#include <initializer_list>
#include <vector>

#include "ch.hpp"
#include "hal.h"

#include "chprintf.h"

#include "BmsConfig.h"
#include "EnergusTempSensor.h"
#include "LTC6811.h"
#include "LTC6811Bus.h"

using namespace chibios_rt;

class BMSThread : public BaseStaticThread<1024> {
 public:
  BMSThread(LTC6811Bus* bus, unsigned int frequency) : m_bus(bus) {
    m_delay = 1000 / frequency;
    for (int i = 0; i < BMS_BANK_COUNT; i++) {
      m_chips.push_back(LTC6811(*bus, i));
    }
    for (int i = 0; i < BMS_BANK_COUNT; i++) {
      m_chips[i].getConfig().gpio5 = LTC6811::GPIOOutputState::kLow;
      m_chips[i].getConfig().gpio4 = LTC6811::GPIOOutputState::kPassive;

      m_chips[i].updateConfig();
    }
  }

  void getValues() {}

 private:
  unsigned int m_delay;
  LTC6811Bus* m_bus;
  std::vector<LTC6811> m_chips;

 protected:
  void main() {
    while (!shouldTerminate()) {
      systime_t timeStart = chVTGetSystemTime();

      for (int i = 0; i < BMS_BANK_COUNT; i++) {
        // Get a reference to the config for toggling gpio
        LTC6811::Configuration& conf = m_chips[i].getConfig();

        // Turn on status LED
        conf.gpio5 = LTC6811::GPIOOutputState::kLow;
        m_chips[i].updateConfig();

        uint16_t* voltages = m_chips[i].getVoltages();

        int temperatures[BMS_BANK_TEMP_COUNT];

        // Measure all temp sensors
        for (unsigned int j = 0; j < BMS_BANK_TEMP_COUNT; j++) {
          conf.gpio1 = (j & 0x01) ? LTC6811::GPIOOutputState::kHigh
                                  : LTC6811::GPIOOutputState::kLow;
          conf.gpio2 = (j & 0x02) ? LTC6811::GPIOOutputState::kHigh
                                  : LTC6811::GPIOOutputState::kLow;
          conf.gpio3 = (j & 0x04) ? LTC6811::GPIOOutputState::kHigh
                                  : LTC6811::GPIOOutputState::kLow;
          conf.gpio4 = LTC6811::GPIOOutputState::kPassive;
          m_chips[i].updateConfig();

          // Wait for config changes to take effect
          chThdSleepMilliseconds(2);

          uint16_t* temps = m_chips[i].getGpioPin(GpioSelection::k4);
          temperatures[j] = temps[3];

          delete temps;
        }

        // Turn off status LED
        conf.gpio5 = LTC6811::GPIOOutputState::kHigh;
        m_chips[i].updateConfig();

        // Done with communication at this point
        // Now time to crunch numbers

        chprintf((BaseSequentialStream*)&SD2, "Slave %d:\r\n", i);

        // Process voltages
        unsigned int totalVoltage = 0;
        chprintf((BaseSequentialStream*)&SD2, "Voltages: ");
        for (int i = 0; i < 12; i++) {
          int voltage = voltages[i] / 10;
          totalVoltage += voltage;
          chprintf((BaseSequentialStream*)&SD2, "%dmV ", voltage);
        }
        chprintf((BaseSequentialStream*)&SD2, "\r\n");

        chprintf((BaseSequentialStream*)&SD2, "Total Voltage: %dmV",
                 totalVoltage);
        chprintf((BaseSequentialStream*)&SD2, "\r\n");
        delete voltages;

        chprintf((BaseSequentialStream*)&SD2, "Temperatures: ");
        for (unsigned int j = 0; j < BMS_BANK_TEMP_COUNT; j++) {
          if (temperatures[j] >= BMS_FAULT_TEMP_THRESHOLD_HIGH) {
            // Set fault line
            palSetLine(LINE_BMS_FLT);
            // TODO: Do this better
          }

          chprintf((BaseSequentialStream*)&SD2, "%dC ",
                   convertTemp(temperatures[j] / 10));
        }
        chprintf((BaseSequentialStream*)&SD2, "\r\n");

        chprintf((BaseSequentialStream*)&SD2, "\r\n");
      }

      // Compute time elapsed since beginning of measurements and sleep for
      // m_delay accounting for elapsed time
      unsigned int timeElapsed = TIME_I2MS(chVTTimeElapsedSinceX(timeStart));
#ifdef DEBUG
      chprintf((BaseSequentialStream*)&SD2, "BMS Thread time elapsed: %dms\r\n",
               timeElapsed);
#endif
      chThdSleepMilliseconds(timeElapsed >= m_delay ? 0
                                                    : m_delay - timeElapsed);
    }
  }
};
