// Copyright (c) 2018 Formula Slug. All Rights Reserved.

#include "TimerChSubsys.h"

#include "Event.h"
#include "ch.hpp"
#include "hal.h"
#include "mcuconfFs.h"

TimerChSubsys::TimerChSubsys(EventQueue& eq) :
		m_eventQueue(eq) {
}

bool TimerChSubsys::addTimer() {

	stop();
	m_timers[m_numTimers] = true;
	m_timerStates[m_numTimers] = false;
	m_numTimers++;

	// start the subsystem again with new configuration
	start();

	// return success
	return true;
}

void TimerChSubsys::runThread() {
	while (true) {
		if (m_subsysActive) {
			std::vector < Event > events;

			for (uint8_t i = 0; i < m_numTimers; i++) {
				// check for transition in pin state

				// if pin changed states
				bool currentState = getState(i);
				if (currentState != getSavedState(i)) {
					// save change
					m_timerStates[i] = currentState;
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
		chThdSleepMilliseconds (m_sampleClkMs);
	}
}

/**
 * @note Public interface only permits access to subsystem's
 *       internally saved pin state, rather than pin value, in order
 *       to ensure consistent in user's implementation
 */
bool TimerChSubsys::getState(uint8_t timerNum) {
	return m_timerStates[timerNum];
}

bool TimerChSubsys::getSavedState(uint8_t timerNum) {
	return m_timerStates[timerNum];
}

// TODO: implement full subsystem, then implement pin adding
void TimerChSubsys::start() {
	m_subsysActive = true;
}

/**
 * @note Subsystem should be stopped before any configurations are
 *       changed. Otherwise, partial configuration will temporarily
 *       corrupt conversion events send to the user's event queue
 */
void TimerChSubsys::stop() {
	m_subsysActive = false;
}

