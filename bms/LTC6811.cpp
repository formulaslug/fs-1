#include "LTC6811.h"
#include "chprintf.h"

constexpr uint16_t LTC6811::crc15Table[256];

LTC6811::Command LTC6811::buildCommand(AddressingMode addrMode, uint8_t addr,
                                       CommandCode cmd) {
  Command out;

  out.addrMode = (uint8_t)addrMode;
  out.address = (uint8_t)(addrMode == AddressingMode::kAddress ? addr : 0);
  out.command = (uint16_t)cmd;

  return out;
}

LTC6811::Command LTC6811::buildBroadcastCommand(CommandCode cmd) {
  return Command{.command = (uint16_t)cmd};
}

LTC6811::Command LTC6811::toCommand(uint8_t val[2]) {
  return Command{.value = (uint16_t)((uint8_t)val[0] << 8 | (uint8_t)val[1])};
}

LTC6811::Command LTC6811::toCommand(uint16_t val) {
  return Command{.value = val};
}

LTC6811::LTC6811(SPIDriver *spiDriver, SPIConfig *spiConfig, uint8_t id)
    : m_spiDriver(spiDriver), m_spiConfig(spiConfig), m_id(id) {}

void LTC6811::acquireSpi() {
  spiAcquireBus(m_spiDriver);
  spiStart(m_spiDriver, m_spiConfig);
  spiSelect(m_spiDriver);
}

void LTC6811::releaseSpi() {
  spiUnselect(m_spiDriver);
  spiReleaseBus(m_spiDriver);
}

void LTC6811::wakeupSpi() {
  acquireSpi();
  releaseSpi();
}

void LTC6811::sendCommand(Command txCmd) {
  uint16_t cmdPec = calculatePec(2, txCmd.valueArr);
  uint8_t cmd[4] = {txCmd.valueArr[0], txCmd.valueArr[1],
                    (uint8_t)(cmdPec >> 8), (uint8_t)(cmdPec)};

  acquireSpi();
  spiSend(m_spiDriver, 4, cmd);
  releaseSpi();
}

void LTC6811::sendCommandWithData(Command txCmd, uint8_t txData[6]) {
  uint16_t cmdPec = calculatePec(2, txCmd.valueArr);
  uint8_t cmd[4] = {txCmd.valueArr[0], txCmd.valueArr[1],
                    (uint8_t)(cmdPec >> 8), (uint8_t)(cmdPec)};

  uint8_t data[8];
  for (int i = 0; i < 6; i++) {
    data[i] = txData[i];
  }
  uint16_t dataPec = calculatePec(6, txData);
  data[6] = (uint8_t)(dataPec >> 8);
  data[7] = (uint8_t)(dataPec);

  acquireSpi();
  spiSend(m_spiDriver, 4, cmd);
  spiSend(m_spiDriver, 8, data);
  releaseSpi();
}

void LTC6811::readCommand(Command txCmd, uint8_t *rxbuf) {
  uint16_t cmdPec = calculatePec(2, txCmd.valueArr);
  uint8_t cmd[4] = {txCmd.valueArr[0], txCmd.valueArr[1],
                    (uint8_t)(cmdPec >> 8), (uint8_t)(cmdPec)};

  acquireSpi();
  spiSend(m_spiDriver, 4, cmd);
  spiReceive(m_spiDriver, 8, rxbuf);
  releaseSpi();

  uint16_t dataPec = calculatePec(6, rxbuf);
  bool goodPec =
      ((uint8_t)(dataPec >> 8)) == rxbuf[6] && ((uint8_t)dataPec) == rxbuf[7];
  if (!goodPec) {
    // TODO: return error or throw out read result
    chprintf((BaseSequentialStream *)&SD2,
             "ERR: Bad PEC on read. Computed: 0x%x. Actual: 0x%x\r\n", dataPec,
             (uint16_t)(rxbuf[6] << 8 | rxbuf[7]));
  }
}

uint16_t LTC6811::calculatePec(uint8_t length, uint8_t *data) {
  uint16_t remainder = 16;
  uint16_t addr;

  for (uint8_t i = 0; i < length; i++) {
    // Calculate pec table address
    addr = ((remainder >> 7) ^ data[i]) & 0xff;

    remainder = (remainder << 8) ^ crc15Table[addr];
  }
  return (remainder << 1);
}
