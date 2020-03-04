
#include "hal.h"

//Arjun board output COBs
constexpr uint32_t TestCOB_TL = 0x101;
constexpr uint32_t TestCOB_TR = 0x102;
constexpr uint32_t TestCOB_Psi = 0x201;
constexpr uint32_t TestCOB_Stable = 0x202;

//Sensor Board output COBs
constexpr uint32_t TestCOB_9250_AX = 0x111;
constexpr uint32_t TestCOB_9250_AY = 0x112;
constexpr uint32_t TestCOB_9250_AZ = 0x113;

constexpr uint32_t TestCOB_9250_GX = 0x114;
constexpr uint32_t TestCOB_9250_GY = 0x115;
constexpr uint32_t TestCOB_9250_GZ = 0x116;



//407 output COBs
constexpr uint32_t TestCOB_Steering = 0x121;
constexpr uint32_t TestCOB_Throttle = 0x122;

constexpr uint32_t TestCOB_BMS_Voltage = 0x131;
constexpr uint32_t TestCOB_BMS_Temp = 0x132;

namespace CANOptions {
	enum class BaudRate : uint32_t {
		k125k = 125000, // 125 kHz
		k250k = 200000, // 250 kHz
		k500k = 500000, // 500 kHz
		k1M  = 1000000, // 1 MHz
		k1M5 = 1500000, // 1.5 MHz
		k3M  = 3000000  // 3 MHz
	};

    template <BaudRate baud, bool loopback>
    constexpr CANConfig config() {
        const uint32_t can_clk = STM32_PCLK1;
        const uint32_t b = static_cast<uint32_t>(baud);

        static_assert(can_clk % (b * 8) == 0, "Baud rate not appropriate for APB1 clock.");

        CANConfig c = {};

        // TODO: 
        c.mcr = CAN_MCR_ABOM | CAN_MCR_AWUM | CAN_MCR_TXFP;

        // baud = 1 / (t_sync + t_1 + t_2) = clk/(BRP+1) / (1 + (TS1+1) + (TS2+1))
        c.btr = CAN_BTR_SJW(1) | // standard jump width
                CAN_BTR_TS1(5) | // time segment 1
                CAN_BTR_TS2(0) | // time segment 2
                CAN_BTR_BRP(can_clk / b / 8 - 1); // Baud rate prescaler

        if (loopback) c.btr |= CAN_BTR_LBKM;

        return c;
    }
}

// vim: ts=4 sts=4 sw=4 et
