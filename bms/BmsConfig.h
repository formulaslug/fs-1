#pragma once

#include "hal.h"

// BMS Master Configuration

#define BMS_BANK_COUNT 4
#define BMS_BANK_CELL_COUNT 7
#define BMS_BANK_TEMP_COUNT BMS_BANK_CELL_COUNT

#define BMS_FAULT_TEMP_THRESHOLD_HIGH 55
#define BMS_FAULT_TEMP_THRESHOLD_LOW 0

#define BMS_FAULT_VOLTAGE_THRESHOLD_HIGH 0xFFFF
#define BMS_FAULT_VOLTAGE_THRESHOLD_LOW 0x0000

// IO Configuration

#define PIN_BMS_FLT 6
#define PIN_BMS_FLT_LAT 11
#define PIN_IMD_STATUS 10
#define PIN_IMD_FLT_LAT 12

#define LINE_BMS_FLT PAL_LINE(GPIOD, PIN_BMS_FLT)
#define LINE_BMS_FLT_LAT PAL_LINE(GPIOD, PIN_BMS_FLT_LAT)
#define LINE_IMD_STATUS PAL_LINE(GPIOD, PIN_IMD_STATUS)
#define LINE_IMD_FLT_LAT PAL_LINE(GPIOD, PIN_IMD_FLT_LAT)

// SPI Configuration

#define BMS_SPI_DRIVER SPID1
#define BMS_SPI_CR1 (SPI_CR1_BR_2 | SPI_CR1_BR_1)  // Prescalar of 128
#define BMS_SPI_CR2 (SPI_CR2_DS_2 | SPI_CR2_DS_1 | SPI_CR2_DS_0)  // 8 bits data

#define PIN_SPI_MOSI 7
#define PIN_SPI_MISO 6
#define PIN_SPI_SCLK 5
#define PIN_SPI_SSEL 4

#define LINE_SPI_MOSI PAL_LINE(GPIOA, PIN_SPI_MOSI)
#define LINE_SPI_MISO PAL_LINE(GPIOA, PIN_SPI_MISO)
#define LINE_SPI_SCLK PAL_LINE(GPIOA, PIN_SPI_SCLK)
#define LINE_SPI_SSEL PAL_LINE(GPIOA, PIN_SPI_SSEL)
