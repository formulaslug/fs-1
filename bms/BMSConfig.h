#pragma once

#include "ch.hpp"

struct BMSConfig {
  const SerialConfig serialConf = {
      115200, /* baud rate */
      0,      /* cr1 */
      0,      /* cr2 */
      0       /* cr3 */
  };
  SerialDriver *serialDriver = &SD2;

  int cellsPerChip = 7;  // Possibly does not matter
  int numChips = 4;
  int chipIds[4] = {0, 1, 2, 3};
};
