// Copyright (c) 2018 Formula Slug. All Rights Reserved.

#pragma once

#include <stdint.h>

#include <vector>

#include "Event.h"
#include "EventQueue.h"
#include "ch.h"
#include "hal.h"


#define TIMER_INIT_NUM 4

//#include "Pins.h"

/**
 *
 * TODO: Rewrite subsystem using the EXT external interrupt driver
 *
 */

/**
 *
 * Singleton class for an ADC subsystem, sitting on top of ChibiOS's
 * low-level ADC driver
 *
 * @note Requires that the runThread() public member function is called
 *       within the context of a chibios static thread
 */
class TimerChSubsys {
	

public:
	
	//static virtual_timer_t vt_timers[10]; //STRUCT OF TIMER OBJECTS

	explicit TimerChSubsys(EventQueue& eq);

	/**
	 * Add a pin to the subsystem and start generating transition events
	 * immediately
	 * @param pin Gpio pin (port and pin number)
	 */
	bool addTimer();
	void startTimer(uint8_t timerNum, uint16_t ms);
	static void timerDone(void *timerState);
	bool getState(uint8_t timerNum);
	bool getSavedState(uint8_t timerNum);

	void runThread();

private:

	static constexpr uint16_t kMaxNumTimers = 10;
	uint16_t m_numTimers = 0;

	// @note true if timer is running, false otherwise
	std::array<virtual_timer_t, kMaxNumTimers> m_timers = { };
	std::array<bool, kMaxNumTimers> m_timerStates = { };

	bool m_subsysActive = false;

	EventQueue& m_eventQueue;

	// TODO: Implement sampling frequency per-pin. Simple solution is
	//       just a subsystem tick that meets sampling requirements of
	//       the fastest sampled pin
	uint32_t m_sampleClkMs = 25;

	void start();
	void stop();

};
