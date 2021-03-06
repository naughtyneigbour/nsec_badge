/*
 * Copyright (c) 2015, Benjamin Vanheuverzwijn <bvanheu@gmail.com>
 * License: MIT (see LICENSE for details)
 */

#ifndef BOARDS_H
#define BOARDS_H

//////////////////////////////////////////////
//Config

// RTC prescaler
#define APP_TIMER_PRESCALER 0

#define UART_BAUDRATE 		UART_BAUDRATE_BAUDRATE_Baud115200

//////////////////////////////////////////////
// PINOUT

#define BATT_CHARGE     1
#define OLED_DATA       3
#define OLED_CLK        4
#define OLED_DC_MODE    5
#define OLED_RESET      6
#define OLED_CS         7
#define TX_PIN_NUMBER   10
#define RX_PIN_NUMBER   11
#define SPIS_MOSI_PIN   12
#define SPIS_MISO_PIN   13
#define SPIS_SCK_PIN    14
#define SPIS_CSN_PIN    15
#define LED_GREEN       28
#define LED_RED         29
#define PSU_ENABLE      30

#endif
