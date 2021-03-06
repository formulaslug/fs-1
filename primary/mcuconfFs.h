// Copyright (c) 2018 Formula Slug. All Rights Reserved.

#ifndef MCUCONF_FS_H
#define MCUCONF_FS_H

// Current MCU
#define STM32F405

enum class Gpio { kA1 = 0, kA2, kA3, kA6 };

enum class DigitalInput { kToggleUp = 0, kToggleDown, kDriveMode, kBSPDFault };

/*********************************************************************
 * @brief Bit Timing Register - Baud Rate Prescalar (BTR_BRP) vals
 * @note Only 250k is confirmed working, others were originally
 *       recovered from docs for STM32F103xx reference manual
 ********************************************************************/
#ifdef STM32F3
#define CAN_BTR_BRP_125k 239
#define CAN_BTR_BRP_250k 7
#define CAN_BTR_BRP_500k 5
#define CAN_BTR_BRP_1M 2
#define CAN_BTR_BRP_1M5 1
#define CAN_BTR_BRP_3M 0
#endif /* STM32F3 */

#ifdef STM32F405
#define CAN_BTR_BRP_125k 23
#define CAN_BTR_BRP_250k 11
#define CAN_BTR_BRP_500k 5
#define CAN_BTR_BRP_1M 2
#define CAN_BTR_BRP_1M5 1
#define CAN_BTR_BRP_3M 0
#endif /* STM32F4 */

/*********************************************************************
 * @brief Pin mappings
 ********************************************************************/

#ifdef STM32F3
#define ARBITRARY_PORT_1 GPIOA
#define ARBITRARY_PIN_1 GPIOA_ARD_A1

#define RIGHT_THROTTLE_PORT GPIOA
#define RIGHT_THROTTLE_PIN GPIOA_ARD_A3

#define LEFT_THROTTLE_PORT GPIOA
#define LEFT_THROTTLE_PIN GPIOA_ARD_A0

#define ARBITRARY_PORT_2 GPIOA
#define ARBITRARY_PIN_2 GPIOA_ARD_D2

#define ARBITRARY_PORT_3 GPIOB
#define ARBITRARY_PIN_3 GPIOB_ARD_D3

#define STARTUP_SOUND_PORT GPIOB
#define STARTUP_SOUND_PIN GPIOB_ARD_D4

#define BRAKE_LIGHT_PORT GPIOB
#define BRAKE_LIGHT_PIN GPIOB_ARD_D5
#endif /* STM32F3 */

#ifdef STM32F405

// Analog inputs
#define STEERING_VALUE_PORT GPIOA
#define STEERING_VALUE_PIN GPIOA_SDO  // ADC12_IN6 (pin A6)
// TODO: ^ Change to GPIOA_PIN0 (discovery board has routed to button)

#define BRAKE_VALUE_PORT GPIOA
#define BRAKE_VALUE_PIN GPIOA_PIN1  // ADC123_IN1

#define RIGHT_THROTTLE_PORT GPIOA
#define RIGHT_THROTTLE_PIN GPIOA_PIN2  // ADC123_IN2

#define LEFT_THROTTLE_PORT GPIOA
#define LEFT_THROTTLE_PIN GPIOA_PIN3  // ADC123_IN3

// Digital inputs
#define TOGGLE_UP_PORT GPIOB  // Button 1
#define TOGGLE_UP_PIN GPIOB_PIN12

#define TOGGLE_DOWN_PORT GPIOB  // Button 2
#define TOGGLE_DOWN_PIN GPIOB_PIN11

#define DRIVE_MODE_PORT GPIOB  // Button 3
#define DRIVE_MODE_PIN GPIOB_PIN13

#define BSPD_FAULT_PORT GPIOB
#define BSPD_FAULT_PIN GPIOB_PIN14

// Digital outputs
#define IMD_FAULT_INDICATOR_PORT GPIOA
#define IMD_FAULT_INDICATOR_PIN GPIOA_PIN15

#define AMS_FAULT_INDICATOR_PORT GPIOA
#define AMS_FAULT_INDICATOR_PIN GPIOA_PIN8

#define BSPD_FAULT_INDICATOR_PORT GPIOC
#define BSPD_FAULT_INDICATOR_PIN GPIOC_PIN6

#define STARTUP_SOUND_PORT GPIOC
#define STARTUP_SOUND_PIN GPIOC_PIN8

#define BRAKE_LIGHT_PORT GPIOC
#define BRAKE_LIGHT_PIN GPIOC_PIN9

#define STARTUP_LED_PORT GPIOB
#define STARTUP_LED_PIN GPIOB_PIN0

#define CAN1_STATUS_LED_PORT GPIOB
#define CAN1_STATUS_LED_PIN GPIOB_PIN1

#define CAN2_STATUS_LED_PORT GPIOB
#define CAN2_STATUS_LED_PIN GPIOB_PIN2

// CAN IO
#define CAN1_RX_PORT GPIOB
#define CAN1_RX_PIN GPIOB_PIN8

#define CAN1_TX_PORT GPIOB
#define CAN1_TX_PIN GPIOB_SDA  // B9

#define CAN2_TX_PORT GPIOB
#define CAN2_TX_PIN GPIOB_SCL  // B6

#define CAN2_RX_PORT GPIOB
#define CAN2_RX_PIN GPIOB_PIN5

// UART IO
#define UART_TX_PORT GPIOC
#define UART_TX_PIN 10

#define UART_RX_PORT GPIOC
#define UART_RX_PIN 11

#define UART_RX_LINE PAL_LINE(UART_RX_PORT, UART_RX_PIN)
#define UART_TX_LINE PAL_LINE(UART_TX_PORT, UART_TX_PIN)

/*
 * @brief Pin/Port aliasing
 */

#endif /* STM32F405 */

#endif /* MCUCONF_FS_H */
