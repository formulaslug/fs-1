// Copyright (c) 2018 Formula Slug. All Rights Reserved.

#include "TimerChSubsys.h"

TimerChSubsys::TimerChSubsys(EventQueue &eq) : m_eventQueue(eq) {
  for (int i = 0; i < TIMER_INIT_NUM; i++) {
    chVTObjectInit(&m_timers[i]);
    m_timerStates[m_numTimers] = false;
    m_timerSavedStates[m_numTimers] = false;
    m_numTimers++;
  }
  start();
}

bool TimerChSubsys::addTimer() {
  stop();
  chVTObjectInit(&m_timers[m_numTimers]);
  m_timerStates[m_numTimers] = false;
  m_timerSavedStates[m_numTimers] = false;
  m_numTimers++;

  // start the subsystem again with new configuration
  start();

  // return success
  return true;
}

void TimerChSubsys::runThread() {
  while (true) {
    if (m_subsysActive) {
      std::vector<Event> events;

      for (uint8_t i = 0; i < m_numTimers; i++) {
        // check for transition in pin state

        // if pin changed states
        bool currentState = getState(i);
        if (currentState != getSavedState(i)) {
          // save change
          m_timerStates[i] = currentState;
          m_timerSavedStates[i] = currentState;

          // queue event
          events.push_back(Event(Event::Type::kTimerTimeout, i));
        }
      }

      // post events if present
      if (events.size() > 0) {
        m_eventQueue.push(events);
      }
    }
    // sleep to yield processing until next sample of channels

    chThdSleepMilliseconds(m_sampleClkMs);
  }
}

/**
 * @note Public interface only permits access to subsystem's
 *       internally saved pin state, rather than pin value, in order
 *       to ensure consistent in user's implementation
 */
bool TimerChSubsys::getState(uint8_t timerNum) {
  return m_timerStates[timerNum];  // DOESNT DO ANYTHING
}

bool TimerChSubsys::getSavedState(uint8_t timerNum) {
  return m_timerSavedStates[timerNum];
}

// TODO: implement full subsystem, then implement pin adding
void TimerChSubsys::start() { m_subsysActive = true; }

void TimerChSubsys::startTimer(uint8_t timerNum, uint16_t ms) {
  chVTReset(&m_timers[timerNum]);
  chVTSet(&m_timers[timerNum], TIME_MS2I(ms), TimerChSubsys::timerDone,
          &m_timerStates[timerNum]);  // ADD CALLBACK THAT POSTS EVENT
  m_timerStates[timerNum] = true;
  m_timerSavedStates[timerNum] = true;
}

void TimerChSubsys::timerDone(void *timerState) {
  bool *state = static_cast<bool *>(timerState);
  *state = false;
}

void TimerChSubsys::stop() { m_subsysActive = false; }
