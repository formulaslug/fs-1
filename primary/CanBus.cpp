// Copyright (c) 2018 Formula Slug. All Rights Reserved.

#include "CanBus.h"

#include "fsprintf.h"
static auto printf3 = SDPrinter<&SD3>();

/* baudrate = 36MHz / ((1 + BRP) * (3 + TS1 + TS2))
 * See STM32F103xx reference manual, 24.7.7 for info on CAN_BTR register.
 *
 * BRP   freq (Hz)
 * ---------------
 * 239   125k
 *  11   250k
 *   5   500k
 *   2     1M
 *   1   1.5M
 *   0     3M
 */
constexpr CANConfig MakeConfig(CanBusBaudRate baud, bool loopback) {
  // uint32_t btr = CAN_BTR_SJW(0) | CAN_BTR_TS2(5) | CAN_BTR_TS1(4);
  //uint32_t btr = CAN_BTR_SJW(1) | CAN_BTR_TS2(1) | CAN_BTR_TS1(10);
  
  
  const uint32_t can_clk = STM32_PCLK1;
  const uint32_t b = static_cast<uint32_t>(baud); //42 MHz on F405
  
  uint32_t btr = CAN_BTR_SJW(0) | // standard jump width
                 CAN_BTR_TS1(10) | // time segment 1 = 12+1 = 13
                 CAN_BTR_TS2(1); //| // time segment 2 = 1+1 = 2
                 //CAN_BTR_BRP(can_clk / b / 8 - 1); // Baud rate prescaler

  if (loopback) {
    btr |= CAN_BTR_LBKM;
  }

  switch (baud) {
    case CanBusBaudRate::k125k:
      btr |= CAN_BTR_BRP(CAN_BTR_BRP_125k);
      break;
    case CanBusBaudRate::k250k:
      btr |= CAN_BTR_BRP(CAN_BTR_BRP_250k);  // was 11
      break;
    case CanBusBaudRate::k500k:
      btr |= CAN_BTR_BRP(CAN_BTR_BRP_500k);  // test hal says 6 (was 5)
      break;
    case CanBusBaudRate::k1M:
      btr |= CAN_BTR_BRP(CAN_BTR_BRP_1M);
      break;
    case CanBusBaudRate::k1M5:
      btr |= CAN_BTR_BRP(CAN_BTR_BRP_1M5);
      break;
    case CanBusBaudRate::k3M:
      btr |= CAN_BTR_BRP(CAN_BTR_BRP_3M);
      break;
  }

  return {CAN_MCR_ABOM | CAN_MCR_AWUM | CAN_MCR_TXFP | CAN_MCR_NART, btr};
}

CanBus::CanBus(CANDriver* canp, CanBusBaudRate baud, bool loopback) {
  m_canp = canp;

  CANConfig config = MakeConfig(baud, loopback);
  canObjectInit(m_canp);
  canStart(m_canp, &config);

  // config the pins
  if (canp == &CAND1) {
    palSetPadMode(CAN1_RX_PORT, CAN1_RX_PIN, PAL_MODE_ALTERNATE(9));  // CAN RX
    palSetPadMode(CAN1_TX_PORT, CAN1_TX_PIN, PAL_MODE_ALTERNATE(9));  // CAN TX
    palSetPadMode(CAN1_STATUS_LED_PORT, CAN1_STATUS_LED_PIN,
                  PAL_MODE_OUTPUT_PUSHPULL);
    palWritePad(CAN1_STATUS_LED_PORT, CAN1_STATUS_LED_PIN, PAL_HIGH);
    chThdSleepMilliseconds(500);
    palWritePad(CAN1_STATUS_LED_PORT, CAN1_STATUS_LED_PIN, PAL_LOW);
  } else if(canp == &CAND2){
    palSetPadMode(CAN2_RX_PORT, CAN2_RX_PIN, PAL_MODE_ALTERNATE(9));  // CAN RX
    palSetPadMode(CAN2_TX_PORT, CAN2_TX_PIN, PAL_MODE_ALTERNATE(9));  // CAN TX
    palSetPadMode(CAN2_STATUS_LED_PORT, CAN2_STATUS_LED_PIN,
                  PAL_MODE_OUTPUT_PUSHPULL);
    palWritePad(CAN2_STATUS_LED_PORT, CAN2_STATUS_LED_PIN, PAL_HIGH);
    chThdSleepMilliseconds(500);
    palWritePad(CAN2_STATUS_LED_PORT, CAN2_STATUS_LED_PIN, PAL_LOW);
  }else{
    //fucked
  }
}

CanBus::~CanBus() { canStop(m_canp); }

/*
 * @note A blinking LED for the CAN status LEDs indicates alterning
 *       errors and successes, while a solid HIGH indicates continuous
 *       success and a solid LOW indicates continuous failure.
 */
bool CanBus::send(const CANTxFrame& msg) {
  msg_t ret = canTransmit(m_canp, CAN_ANY_MAILBOX, &msg, TIME_MS2I(100));
  if (ret == MSG_OK) {
    // success: HIGH LED
    if (m_canp == &CAND1) {
      palWritePad(CAN1_STATUS_LED_PORT, CAN1_STATUS_LED_PIN, PAL_HIGH);
    } else if (m_canp == &CAND2) {
      palWritePad(CAN2_STATUS_LED_PORT, CAN2_STATUS_LED_PIN, PAL_HIGH);
    }
    return true;
  } else {
    // error: LOW LED
    if (m_canp == &CAND1) {
      palWritePad(CAN1_STATUS_LED_PORT, CAN1_STATUS_LED_PIN, PAL_HIGH);
      chThdSleepMilliseconds(100);
      palWritePad(CAN1_STATUS_LED_PORT, CAN1_STATUS_LED_PIN, PAL_LOW);
    } else if (m_canp == &CAND2) {
      palWritePad(CAN2_STATUS_LED_PORT, CAN2_STATUS_LED_PIN, PAL_HIGH);
      chThdSleepMilliseconds(100);
      palWritePad(CAN2_STATUS_LED_PORT, CAN2_STATUS_LED_PIN, PAL_LOW);
    }
    // failure
    return false;
  }
}

bool CanBus::recv(CANRxFrame& msg) {
  return canReceive(m_canp, CAN_ANY_MAILBOX, &msg, TIME_IMMEDIATE) == MSG_OK;
}

// void CanBus::printTxMessage(const CANTxFrame& msg) const {
//	std::printf("[CAN TX] COB-ID:");
//
//	// Pad left of shorter ID with spaces
//	for (uint32_t i = 0; i < 8 - std::log(msg.EID) / std::log(16); ++i) {
//		std::printf(" ");
//	}
//	std::printf("0x");
//
//	// Print the node's ID
//	std::printf("%lx", msg.EID);
//
//	std::printf("  data:");
//	for (uint32_t i = 0; i < msg.DLC; ++i) {
//		std::printf(" 0x");
//		// Print every byte of message payload
//		std::printf("%x", msg.data8[i]);
//	}
//
//	std::printf("\n");
//}
//
// void CanBus::printRxMessage(const CANRxFrame& msg) const {
//	std::printf("[CAN RX] COB-ID:");
//
//	// Pad left of shorter ID with spaces
//	for (uint32_t i = 0; i < 8 - std::log(msg.EID) / std::log(16); ++i) {
//		std::printf(" ");
//	}
//	std::printf("0x");
//
//	// Print the node's ID
//	std::printf("%lx", msg.EID);
//
//	std::printf("  data:");
//	for (uint32_t i = 0; i < msg.DLC; ++i) {
//		std::printf(" 0x");
//		// Print every byte of message payload
//		std::printf("%x", msg.data8[i]);
//	}
//
//	std::printf("\n");
//}

/**
 * @desc Transmits all enqueued messages. Enqueue them onto the transmit logs
 *       queue after so that they can be printed
 */
void CanBus::processTxMessages() {
  while (m_txQueue.Size() > 0) {
    send(m_txQueue[0]);
    m_txQueue.PopFront();
    // m_txLogsQueue.PushBack(m_txQueue[0]);
  }
}

/**
 * @desc Enqueue any messages apearing on the CAN bus
 */
void CanBus::processRxMessages() {
  static CANRxFrame rxMessageTmp;
  while (recv(rxMessageTmp)) {
    m_rxQueue.PushBack(rxMessageTmp);
    // TODO: figure out a way to remove this duplication
    // m_rxLogsQueue.PushBack(rxMessageTmp);
  }
}

/**
 * @desc Prints over serial all messages currently in the tx logs queue
 */
// void CanBus::printTxAll() {
//	static CANTxFrame queueMessage;
//	queueMessage = m_txLogsQueue.PopFront();
//	while (queueMessage.EID) {
//		// print
//		printTxMessage(queueMessage);
//		// dequeue another message
//		queueMessage = m_txLogsQueue.PopFront();
//	}
//}
//
///**
// * @desc Prints over serial all messages currently in the rx queue
// */
// void CanBus::printRxAll() {
//	static CANRxFrame msg;
//	msg = m_rxLogsQueue.PopFront();
//	while (msg.EID) {
//		// print
//		printRxMessage(msg);
//		// dequeue another message
//		msg = m_rxLogsQueue.PopFront();
//	}
//}
/**
 * @desc Enqueues a packaged message to be transmitted over the CAN bus
 */
void CanBus::queueTxMessage(CANTxFrame msg) { m_txQueue.PushBack(msg); }

/**
 * @desc Dequeues a packaged message to be unpacked and used
 * @param msg The message at the front of the rx queue
 */
CANRxFrame CanBus::dequeueRxMessage() { return m_rxQueue.PopFront(); }

/**
 * @desc Gets the current size of the tx queue
 * @return The size
 */
uint8_t CanBus::txQueueSize() { return m_txQueue.Size(); }

/**
 * @desc Gets the current size of the rx queue
 * @return The size
 */
uint8_t CanBus::rxQueueSize() { return m_rxQueue.Size(); }
