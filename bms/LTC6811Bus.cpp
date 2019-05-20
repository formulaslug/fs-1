#include "LTC6811Bus.h"
#include "chprintf.h"

constexpr uint16_t LTC6811Bus::crc15Table[256];

LTC6811Bus::Command LTC6811Bus::buildAddressedCommand(
    uint8_t addr, LTC6811Command const &cmd) {
  Command out;

  out.addrMode = (uint8_t)0x01;
  out.address = addr;
  out.command = cmd.toValue();

  return out;
}

LTC6811Bus::Command LTC6811Bus::buildBroadcastCommand(
    LTC6811Command const &cmd) {
  Command out;

  out.addrMode = 0x00;
  out.address = 0x00;
  out.command = cmd.toValue();

  return out;
}

LTC6811Bus::LTC6811Bus(SPIDriver *spiDriver, SPIConfig *spiConfig)
    : m_spiDriver(spiDriver), m_spiConfig(spiConfig) {}

void LTC6811Bus::acquireSpi() {
  spiAcquireBus(m_spiDriver);
  spiStart(m_spiDriver, m_spiConfig);
  spiSelect(m_spiDriver);
}

void LTC6811Bus::releaseSpi() {
  spiUnselect(m_spiDriver);
  spiReleaseBus(m_spiDriver);
}

void LTC6811Bus::wakeupSpi() {
  acquireSpi();
  releaseSpi();
}

void LTC6811Bus::send(uint8_t txCmd[2]) {
  uint16_t cmdPec = calculatePec(2, txCmd);
  uint8_t cmd[4] = {txCmd[1], txCmd[0], (uint8_t)(cmdPec >> 8),
                    (uint8_t)(cmdPec)};

#ifdef DEBUG
  for (int i = 0; i < 4; i++) {
    chprintf((BaseSequentialStream *)&SD2, "CMD: %d: 0x%x\r\n", i, cmd[i]);
  }
#endif

  acquireSpi();
  spiSend(m_spiDriver, 4, cmd);
  releaseSpi();
}

void LTC6811Bus::sendData(uint8_t txCmd[2], uint8_t txData[6]) {
  uint16_t cmdPec = calculatePec(2, txCmd);
  uint8_t cmd[4] = {txCmd[1], txCmd[0], (uint8_t)(cmdPec >> 8),
                    (uint8_t)(cmdPec)};

  uint8_t data[8];
  for (int i = 0; i < 6; i++) {
    data[i] = txData[i];
  }
  uint16_t dataPec = calculatePec(6, txData);
  data[6] = (uint8_t)(dataPec >> 8);
  data[7] = (uint8_t)(dataPec);

#ifdef DEBUG
  for (int i = 0; i < 4; i++) {
    chprintf((BaseSequentialStream *)&SD2, "CMD: %d: 0x%x\r\n", i, cmd[i]);
  }
  for (int i = 0; i < 8; i++) {
    chprintf((BaseSequentialStream *)&SD2, "Byte: %d: 0x%x\r\n", i, data[i]);
  }
#endif

  acquireSpi();
  spiSend(m_spiDriver, 4, cmd);
  spiSend(m_spiDriver, 8, data);
  releaseSpi();
}

void LTC6811Bus::sendCommand(Command txCmd) {
  uint8_t cmdCode[2] = {(uint8_t)(txCmd.value >> 8), (uint8_t)(txCmd.value)};
  uint16_t cmdPec = calculatePec(2, cmdCode);
  uint8_t cmd[4] = {cmdCode[0], cmdCode[1], (uint8_t)(cmdPec >> 8),
                    (uint8_t)(cmdPec)};

  wakeupSpi();
  acquireSpi();
  spiSend(m_spiDriver, 4, cmd);
  releaseSpi();
}

void LTC6811Bus::sendCommandWithData(Command txCmd, uint8_t txData[6]) {
  uint8_t cmdCode[2] = {(uint8_t)(txCmd.value >> 8), (uint8_t)(txCmd.value)};
  uint16_t cmdPec = calculatePec(2, cmdCode);
  uint8_t cmd[4] = {cmdCode[0], cmdCode[1], (uint8_t)(cmdPec >> 8),
                    (uint8_t)(cmdPec)};

  uint8_t data[8];
  for (int i = 0; i < 6; i++) {
    data[i] = txData[i];
  }
  uint16_t dataPec = calculatePec(6, txData);
  data[6] = (uint8_t)(dataPec >> 8);
  data[7] = (uint8_t)(dataPec);

#ifdef DEBUG
  for (int i = 0; i < 4; i++) {
    chprintf((BaseSequentialStream *)&SD2, "CMD: %d: 0x%x\r\n", i, cmd[i]);
  }
  for (int i = 0; i < 8; i++) {
    chprintf((BaseSequentialStream *)&SD2, "Byte: %d: 0x%x\r\n", i, data[i]);
  }
  chprintf((BaseSequentialStream *)&SD2, "pec: 0x%x\r\n", dataPec);
#endif

  wakeupSpi();
  acquireSpi();
  spiSend(m_spiDriver, 4, cmd);
  spiSend(m_spiDriver, 8, data);
  releaseSpi();
}

void LTC6811Bus::readCommand(Command txCmd, uint8_t *rxbuf) {
  uint8_t cmdCode[2] = {(uint8_t)(txCmd.value >> 8), (uint8_t)(txCmd.value)};
  uint16_t cmdPec = calculatePec(2, cmdCode);
  uint8_t cmd[4] = {cmdCode[0], cmdCode[1], (uint8_t)(cmdPec >> 8),
                    (uint8_t)(cmdPec)};
#ifdef DEBUG
  for (int i = 0; i < 4; i++) {
    chprintf((BaseSequentialStream *)&SD2, "CMD: %d: 0x%x\r\n", i, cmd[i]);
  }
#endif

  wakeupSpi();
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

uint16_t LTC6811Bus::calculatePec(uint8_t length, uint8_t *data) {
  uint16_t remainder = 16;
  uint16_t addr;

  for (uint8_t i = 0; i < length; i++) {
    // Calculate pec table address
    addr = ((remainder >> 7) ^ data[i]) & 0xff;

    remainder = (remainder << 8) ^ crc15Table[addr];
  }
  return (remainder << 1);
}
