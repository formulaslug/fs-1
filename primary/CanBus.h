// Copyright (c) 2018 Formula Slug. All Rights Reserved.

#pragma once

#include <stdint.h>
#include <cmath>
#include <cstdio>
#include <initializer_list>
#include <vector>

#include "CircularBuffer.h"
#include "Event.h"
#include "EventQueue.h"
#include "ch.h"
#include "hal.h"
#include "mcuconfFs.h"

enum class CanBusBaudRate : uint8_t {
  k125k,  // 125 kHz
  k250k,  // 250 kHz
  k500k,  // 500 kHz
  k1M,    // 1 MHz
  k1M5,   // 1.5 MHz
  k3M     // 3 MHz
};

class CanBus {
 public:
  CanBus(CANDriver* canp, CanBusBaudRate baud, bool loopback);
  virtual ~CanBus();
  bool send(const CANTxFrame& msg);
  bool recv(CANRxFrame& msg);

  // queues a message to be transmitted
  void queueTxMessage(CANTxFrame msg);
  uint8_t txQueueSize();
  // dequeues a message received
  CANRxFrame dequeueRxMessage();
  uint8_t rxQueueSize();

  // listen to CanBus bus and Enqueue/Dequeue messages accordingly
  void processTxMessages();
  void processRxMessages();

  // print out all messages currently in txLogsQueue and rxLogsQueue, dequeueing
  // them from each
  void printTxAll();
  void printRxAll();

  // NOTE: Made this public, until this abstraction can be merged
  //       with the CanChSubsys one
  CANDriver* m_canp;

 private:
  // circular buffers to hold CanBus_message_t instances
  CircularBuffer<CANTxFrame> m_txQueue{10};
  CircularBuffer<CANTxFrame> m_txLogsQueue{10};
  CircularBuffer<CANRxFrame> m_rxQueue{10};
  CircularBuffer<CANRxFrame> m_rxLogsQueue{10};

  uint32_t m_id = 0;

  // print CanBus message to serial console
  void printTxMessage(const CANTxFrame& msg) const;
  void printRxMessage(const CANRxFrame& msg) const;
};
