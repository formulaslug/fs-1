// Copyright (c) 2018 Formula Slug. All Rights Reserved.

#pragma once

#include <stdint.h>

#include <array>
#include <vector>
#include "mcuconfFs.h"

class Event {
 public:
  // Event types
  enum Type { kNone, kCanRx, kTimerTimeout, kAdcConversion, kDigInTransition };
  enum CanSource { kHv, kLv };

  Event(Type t, Gpio adcPin, uint32_t adcValue);
  Event(Type t, uint32_t canEid, CanSource canSource, std::array<uint16_t, 8> canFrame);
  Event(Type t, uint8_t timerNum);
  Event(Type t, DigitalInput pin, bool currentState);
  Event();

  Type type();

  // type-specific member functions (see note in source)
  Gpio adcPin();
  uint32_t adcValue();
  uint32_t canEid();
  CanSource canSource();
  std::array<uint16_t, 8> canFrame();
  DigitalInput digInPin();
  bool digInState();
  uint8_t timer();

 private:
  Type m_type = kNone;

  /**
   * @note Allocation of 320 bytes (10 32 bit integers), of course,
   *       overflows a 128 byte static thread workspace and breaks
   *       the kernel. Be careful with static thread workspace
   */
  std::array<uint16_t, 10> m_params;
};
