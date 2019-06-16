#pragma once

namespace CANOptions {

enum class BaudRate : uint32_t {
  k125k = 125000,  // 125 kHz
  k250k = 200000,  // 250 kHz
  k500k = 500000,  // 500 kHz
  k1M = 1000000,   // 1 MHz
  k1M5 = 1500000,  // 1.5 MHz
  k3M = 3000000    // 3 MHz
};

template <BaudRate baud, bool loopback>
constexpr CANConfig config() {
  const uint32_t can_clk = STM32_PCLK1;
  const uint32_t b = static_cast<uint32_t>(baud);

  static_assert(can_clk % (b * 8) == 0,
                "Baud rate not appropriate for APB1 clock.");

  CANConfig c = {};

  // TODO:
  c.mcr = CAN_MCR_ABOM | CAN_MCR_AWUM | CAN_MCR_TXFP;

  // baud = 1 / (t_sync + t_1 + t_2) = clk/(BRP+1) / (1 + (TS1+1) + (TS2+1))
  c.btr = CAN_BTR_SJW(1) |                   // standard jump width
          CAN_BTR_TS1(5) |                   // time segment 1
          CAN_BTR_TS2(0) |                   // time segment 2
          CAN_BTR_BRP(can_clk / b / 8 - 1);  // Baud rate prescaler

  if (loopback) c.btr |= CAN_BTR_LBKM;

  return c;
}
}  // namespace CANOptions
