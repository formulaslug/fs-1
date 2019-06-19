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

 private:
  unsigned int m_delay;
  LTC6811Bus* m_bus;
  std::vector<LTC6811> m_chips;
  bool m_discharging = false;

  void throwBmsFault() {
    m_discharging = false;
    palClearLine(LINE_BMS_FLT);
    palClearLine(LINE_CHARGER_CONTROL);
  }

 protected:
  void main() {
    uint16_t* allVoltages = new uint16_t[BMS_BANK_COUNT * BMS_BANK_CELL_COUNT];
    uint8_t* allTemps = new uint8_t[BMS_BANK_COUNT * BMS_BANK_CELL_COUNT];
    uint16_t averageVoltage = -1;
    while (!shouldTerminate()) {
      systime_t timeStart = chVTGetSystemTime();

      uint32_t allBanksVoltage = 0;
      uint16_t minVoltage = 0xFFFF;
      uint16_t maxVoltage = 0x0000;
      uint8_t minTemp = 0xFF;
      uint8_t maxTemp = 0x00;

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
        for (int j = 0; j < 12; j++) {
          uint16_t voltage = voltages[j] / 10;


          int index = BMS_CELL_MAP[j];
          if (index != -1) {
            allVoltages[(BMS_BANK_CELL_COUNT * i) + index] = voltage;

            if (voltage < minVoltage && voltage != 0) minVoltage = voltage;
            if (voltage > maxVoltage) maxVoltage = voltage;

            totalVoltage += voltage;
            chprintf((BaseSequentialStream*)&SD2, "%dmV ", voltage);

            if (voltage >= BMS_FAULT_VOLTAGE_THRESHOLD_HIGH) {
              // Set fault line
              chprintf((BaseSequentialStream*)&SD2,
                       "***** BMS LOW VOLTAGE FAULT *****\r\nVoltage at %d\r\n\r\n",
                       voltage);
              throwBmsFault();
            }
            if (voltage <= BMS_FAULT_VOLTAGE_THRESHOLD_LOW) {
              // Set fault line
              chprintf((BaseSequentialStream*)&SD2,
                       "***** BMS HIGH VOLTAGE FAULT *****\r\nVoltage at %d\r\n\r\n",
                       voltage);
              throwBmsFault();
            }

            // Discharge cells if enabled
            if(m_discharging) {
              if((voltage > averageVoltage) && (voltage - averageVoltage > BMS_DISCHARGE_THRESHOLD)) {
                // Discharge

                chprintf((BaseSequentialStream*)&SD2, "DISCHARGE CELL %d: %dmV (%dmV)\r\n", index, voltage, (voltage - averageVoltage));

                // Enable discharging
                conf.dischargeState.value |= (1 << j);
              } else {
                // Disable discharging
                conf.dischargeState.value &= ~(1 << j);
              }
            } else {
              // Disable discharging
              conf.dischargeState.value &= ~(1 << j);
            }
          }
        }
        chprintf((BaseSequentialStream*)&SD2, "\r\n");

        chprintf((BaseSequentialStream*)&SD2, "Total Voltage: %dmV",
                 totalVoltage);
        chprintf((BaseSequentialStream*)&SD2, "\r\n");
        delete voltages;

        chprintf((BaseSequentialStream*)&SD2, "Temperatures: ");
        for (unsigned int j = 0; j < BMS_BANK_TEMP_COUNT; j++) {
          uint8_t temp = convertTemp(temperatures[j] / 10);

          allTemps[(BMS_BANK_TEMP_COUNT * i) + j] = temp;
          if (temp < minTemp) minTemp = temp;
          if (temp > maxTemp) maxTemp = temp;

          if (temp >= BMS_FAULT_TEMP_THRESHOLD_HIGH) {
            // Set fault line
            chprintf((BaseSequentialStream*)&SD2,
                     "***** BMS TEMP FAULT *****\r\nTemp at %d\r\n\r\n", temp);
            throwBmsFault();
          }
          if (temp <= BMS_FAULT_TEMP_THRESHOLD_LOW) {
            // Set fault line
            chprintf((BaseSequentialStream*)&SD2,
                     "***** BMS TEMP FAULT *****\r\nTemp at %d\r\n\r\n", temp);
            throwBmsFault();
          }

          chprintf((BaseSequentialStream*)&SD2, "%dC ", temp);
        }
        chprintf((BaseSequentialStream*)&SD2, "\r\n");

        allBanksVoltage += totalVoltage;

        chprintf((BaseSequentialStream*)&SD2, "\r\n");
      }

      averageVoltage = allBanksVoltage / (BMS_BANK_COUNT * BMS_BANK_CELL_COUNT);

      auto txmsg = BMSStatMessage(allBanksVoltage / 10, 0, maxVoltage / 100,
                                  minVoltage / 100, maxTemp, minTemp);
      canTransmit(&BMS_CAN_DRIVER, CAN_ANY_MAILBOX, &txmsg, TIME_MS2I(100));

      // Send CAN
      for (size_t i = 0; i < BMS_BANK_COUNT; i++) {
        auto txmsg = BMSTempMessage(i, allTemps + (BMS_BANK_TEMP_COUNT * i));
        canTransmit(&BMS_CAN_DRIVER, CAN_ANY_MAILBOX, &txmsg, TIME_MS2I(100));
      }

      for (size_t i = 0; i < 7; i++) {
        auto txmsg = BMSVoltageMessage(i, allVoltages + (4 * i));
        canTransmit(&BMS_CAN_DRIVER, CAN_ANY_MAILBOX, &txmsg, TIME_MS2I(100));
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
