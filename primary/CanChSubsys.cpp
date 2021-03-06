// Copyright (c) 2018 Formula Slug. All Rights Reserved.

#include "CanChSubsys.h"

CanChSubsys::CanChSubsys(CanBus& cb, chibios_rt::Mutex& cbMut, EventQueue& eq)
    : m_canBus(cb), m_canBusMut(cbMut), m_eventQueue(eq) {}

void CanChSubsys::startSend(CANTxFrame& msg) {
  std::lock_guard<chibios_rt::Mutex> lock(m_canBusMut);
  m_canBus.queueTxMessage(msg);
}

/*
 * @brief subsystem run function for CAN TX called from within a
 *        ChibiOS static thread
 */
void CanChSubsys::runTxThread() {
  while (true) {
    {
      // Lock from simultaneous thread access
      std::lock_guard<chibios_rt::Mutex> lock(m_canBusMut);
      // Process all messages to transmit from the message transmission queue
      m_canBus.processTxMessages();
    }
    // throttle back thread runloop to prevent overconsumption of resources
    chThdSleepMilliseconds(2);
  }
}

/*
 * @brief subsystem run function for CAN TX called from within a
 *        ChibiOS static thread
 */
void CanChSubsys::runRxThread() {
  event_listener_t el;
  chEvtRegister(&(m_canBus.m_canp->rxfull_event), &el, 0);

  while (true) {
    if (chEvtWaitAnyTimeout(ALL_EVENTS, TIME_MS2I(100)) == 0) {
      continue;
    }
    // receive any present messages from hardware
    {
      std::lock_guard<chibios_rt::Mutex> lock(m_canBusMut);
      m_canBus.processRxMessages();
    }

    // generate events from any received messages
    while (m_canBus.rxQueueSize() > 0) {
      // get CAN message
      CANRxFrame msg = m_canBus.dequeueRxMessage();
      // create event
      std::array<uint16_t, 8> frame = {0, 0, 0, 0, 0, 0, 0, 0};
      for (int i = 0; i < 8; i++) {
        // pushing data bytes to event param vector in reverse order,
        // such that popping off stack will result in correct order
        frame[i] = msg.data8[i];
      }
      // write COBID to event params
      Event e = Event(Event::Type::kCanRx, msg.EID,
          m_canBus.m_canp == &CAND1 ? Event::CanSource::kLv
                                    : Event::CanSource::kHv,
          frame);
      // push event
      m_eventQueue.push(e);
    }
    chThdSleepMilliseconds(2);
  }

  chEvtUnregister(&(m_canBus.m_canp->rxfull_event), &el);
}
