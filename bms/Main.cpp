#include "Main.h"

#include "BmsConfig.h"
#include "CanOpenPdo.h"
#include "common.h"

#include "ch.hpp"
#include "chprintf.h"
#include "hal.h"

#include "BmsThread.h"
#include "CANOptions.h"
#include "LTC6811Bus.h"

using namespace chibios_rt;

// When car is off maybe one reading every 10 sec
// when car is on 10 readings per second

// TODO: map 12 cells to number of cells
// TODO: change m_chips to array rather than vector
// TODO: publish fault states
// TODO: SoC tracking using Ah counting and voltage measuring

// Fault states:
// - AMS
// - IMD
//
// * Over temp
// - Under temp
// * Over voltage
// * Under voltage
//
// * Failed init
// * Comm fail

static constexpr CANConfig cancfg =
    CANOptions::config<CANOptions::BaudRate::k500k, false>();

static void adcerrorcallback(ADCDriver *adcp, adcerror_t err) {
  (void)adcp;
  (void)err;
}

static const uint32_t kCurrentDepth = 512;
static adcsample_t current_data[kCurrentDepth];

int32_t gCurrent = 0;

// This might run on main thread right now, but who cares.
static void adccallback(ADCDriver *adcp) {
  // only adcp == &ADCD1 currently
  (void)adcp;
  int32_t V_sum;
  for (uint16_t i = 0; i < kCurrentDepth; ++i) {
    V_sum += current_data[i];
  }
  // where n = 256, resolution = 2**12
  // Ip = (V_sum / n * 5 V / resolution - 2.5 V) * 300 A / 0.625 V 

  // offset = * 2.5 V * resolution / 5 V * n = 524288
  // V->I multiplier = 30 deciA * 5 V / 0.625 V = 240
  // lastly divide by 2**12 * 256 == 2**20
  gCurrent = (240 * (V_sum - 524288)) >> 20; // pray for arithemetic shift
}

// if STM32_ADCV3_OVERSAMPLING  == TRUE
// Set ccr=0 after TR1 // ADC12_CCR = :shrug:
// endif
// Probably ignore ADC_TR, may need enable to matter? 0, 4095 is the reset
// register value anyway
static const ADCConversionGroup adcgrpcfg1 = { 
  true, // circular
  1, // 1 channel
  &adccallback,
  &adcerrorcallback,
  ADC_CFGR_CONT, // CFGR = continuous
  ADC_TR(0, 4095), // TR1 (watchdog thresholds?)
  { // SMPR[2] = sample times
    ADC_SMPR1_SMP_AN1(05), // 0b101 => 61.5 cycles Sample & Hold time
    0   
  },  
  { // SQR[4] = regular sequence channels
    // ADC 1, ch 1 = pin A0 = current sensor
    ADC_SQR1_SQ1_N(ADC_CHANNEL_IN1), // | ADC_SQR1_SQ2_N(ADC_CHANNEL_IN2),
    0,  
    0,  
    0   
  }
};

int main() {
  /*
   * System initializations.
   * - HAL initialization, this also initializes the configured device drivers
   *   and performs the board-specific initializations.
   * - Kernel initialization, the main() function becomes a thread and the
   *   RTOS is active.
   */
  halInit();
  System::init();

  // Init all io pins
  initIO();

  // Initialize serial for logging
  const SerialConfig serialConf = {
      115200, /* baud rate */
      0,      /* cr1 */
      0,      /* cr2 */
      0       /* cr3 */
  };
  sdStart(&SD2, &serialConf);
  sdWrite(&SD2, (const uint8_t*)"Serial Init\r\n", 13);

  canStart(&BMS_CAN_DRIVER, &cancfg);
  CANTxFrame txmsg;
  txmsg.IDE = CAN_IDE_STD;
  txmsg.RTR = CAN_RTR_DATA;
  txmsg.SID = kFuncIdCellStartup;
  txmsg.DLC = 8;
  uint8_t msg[8] = {'S', 'P', 'I', 'C', 'Y', 'B', 'O', 'I'};
  for (size_t i = 0; i < 8; i++) {
    txmsg.data8[i] = msg[i];
  }
  if (canTransmit(&BMS_CAN_DRIVER, CAN_ANY_MAILBOX, &txmsg, TIME_MS2I(100)) !=
      MSG_OK) {
    sdWrite(&SD2, (const uint8_t*)"Can startup failed\r\n", 20);
  }

  chThdSleepMilliseconds(1000);

  // Init spi config
  SPIConfig* spiConf = new SPIConfig{false /* Circular buffer */,
                                     NULL /* Callback */,
                                     PAL_PORT(LINE_SPI_SSEL) /* SSEL port */,
                                     PAL_PAD(LINE_SPI_SSEL) /* SSEL pin */,
                                     BMS_SPI_CR1 /* CR1 (prescalar) */,
                                     BMS_SPI_CR2 /* CR2 (data width) */};

  LTC6811Bus ltcBus = LTC6811Bus(&BMS_SPI_DRIVER, spiConf);

  BMSThread bmsThread(&ltcBus, 1);
  bmsThread.start(NORMALPRIO + 1);

  // Flash LEDs to indicate startup
  for (int i = 0; i < 4; i++) {
    palWriteLine(LINE_LED_GREEN, PAL_HIGH);
    chThdSleepMilliseconds(50);
    palWriteLine(LINE_LED_GREEN, PAL_LOW);
    chThdSleepMilliseconds(50);
  }

  // Enable current sensor ADC
  adcStart(&ADCD1, nullptr);
  adcStartConversion(&ADCD1, &adcgrpcfg1, current_data, kCurrentDepth);

  while (1) {
    // Sleep 100 secs
    chThdSleepMilliseconds(100 * 1000);
  }
}

void initIO() {
  // Set modes for IO
  palSetLineMode(LINE_BMS_FLT, PAL_MODE_OUTPUT_PUSHPULL);
  palSetLineMode(LINE_BMS_FLT_LAT, PAL_MODE_INPUT);
  palSetLineMode(LINE_IMD_STATUS, PAL_MODE_INPUT);
  palSetLineMode(LINE_IMD_FLT_LAT, PAL_MODE_INPUT);
  palSetLineMode(LINE_CHARGER_CONTROL, PAL_MODE_OUTPUT_PUSHPULL);
  palSetLineMode(LINE_SIG_CURRENT, PAL_MODE_INPUT_ANALOG);

  // Reset BMS fault line
  palSetLine(LINE_BMS_FLT);
  // palClearLine(LINE_BMS_FLT);

  // Enable charging
  palClearLine(LINE_CHARGER_CONTROL);

  // Set modes for SPI
  palSetLineMode(LINE_SPI_MISO,
                 PAL_MODE_ALTERNATE(5) | PAL_STM32_OSPEED_HIGHEST);
  palSetLineMode(LINE_SPI_MOSI,
                 PAL_MODE_ALTERNATE(5) | PAL_STM32_OSPEED_HIGHEST);
  palSetLineMode(LINE_SPI_SCLK,
                 PAL_MODE_ALTERNATE(5) | PAL_STM32_OSPEED_HIGHEST);
  palSetLineMode(LINE_SPI_SSEL,
                 PAL_MODE_OUTPUT_PUSHPULL | PAL_STM32_OSPEED_HIGHEST);

  // Set modes for CAN
  palSetLineMode(LINE_CAN_TX, PAL_MODE_ALTERNATE(9) | PAL_STM32_OSPEED_HIGHEST);
  palSetLineMode(LINE_CAN_RX, PAL_MODE_ALTERNATE(9));
}
