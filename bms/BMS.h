#pragma once

#include <functional>

#include "ch.hpp"
#include "hal.h"

#include "chprintf.h"

#include "BMSConfig.h"
#include "LTC6811.h"

// Temp @ 10Hz
// Voltage @ hella Hz

using namespace chibios_rt;

class BMS : public BaseStaticThread<1024> {
 private:
  // @TODO: change callback types
  int *(m_voltagecallback)(void);
  int *(m_tempCallback)(void);

  int m_pollVoltageRate;
  int m_pollTemperatureRate;

 public:
  BMS(int pollVoltageRate, int pollTemperatureRate);

  void getTemps();
  void getVoltages();
  void setDischargeAllowed(bool allowed);

  void runTempThread();
  void runVoltageThread();

 protected:
  virtual void main(void) {
    setName("bms");

    // Initialize serial for logging
    const SerialConfig serialConf = {
        115200, /* baud rate */
        0,      /* cr1 */
        0,      /* cr2 */
        0       /* cr3 */
    };
    sdStart(&SD2, &serialConf);
    sdWrite(&SD2, (const uint8_t *)"Init Serial\r\n", 13);

    // m_vThread.start(NORMALPRIO + 1);

    while (!shouldTerminate()) {
      // sdWrite(&SD2, (const uint8_t *)"YEEET\r\n", 7);
    }
  }
};

// 2 threads: voltage and temp
