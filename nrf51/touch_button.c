//
//  touch_button.c
//  nsec16
//
//  Created by Marc-Etienne M.Léveillé on 2016-05-18.
//
//

#include "touch_button.h"
#include <stdint.h>
#include <stdio.h>

#include <nrf51.h>
#include <nrf_gpio.h>
#include <spi_slave.h>
#include <app_error.h>
#include <app_timer.h>

#include "ssd1306.h"
#include "boards.h"
#include "controls.h"


#define TX_BUF_SIZE   2
#define RX_BUF_SIZE   2
// SPI default character. Character clocked out in case of an ignored transaction
#define DEF_CHARACTER 0xAAu
// SPI over-read character. Character clocked out after an    over-read of the transmit buffer
#define ORC_CHARACTER 0x55u

static uint8_t m_tx_buf[TX_BUF_SIZE];
static uint8_t m_rx_buf[RX_BUF_SIZE];
static uint32_t last_event_received_time;

void touch_on_event(enum touch_event event, enum touch_button button) {
    uint32_t event_received_time, event_received_diff;
    app_timer_cnt_get(&event_received_time);
    app_timer_cnt_diff_compute(event_received_time, last_event_received_time, &event_received_diff);
    last_event_received_time = event_received_time;
    if(event_received_diff < 3000) {
        return;
    }

    if(event == TOUCH_EVENT_DOWN) {
        button_t button_pressed;
        switch (button) {
            case TOUCH_BUTTON_UP:
                button_pressed = BUTTON_UP;
                break;
            case TOUCH_BUTTON_DOWN:
                button_pressed = BUTTON_DOWN;
                break;
            case TOUCH_BUTTON_LEFT:
                button_pressed = BUTTON_LEFT;
                break;
            case TOUCH_BUTTON_RIGHT:
                button_pressed = BUTTON_RIGHT;
                break;
            case TOUCH_BUTTON_BACK:
                button_pressed = BUTTON_BACK;
                break;
            case TOUCH_BUTTON_ENTER:
                button_pressed = BUTTON_ENTER;
                break;
            default:
                break;
        }
        nsec_controls_trigger(button_pressed);
    }
}

static void spi_slave_buffers_init(uint8_t * const p_tx_buf,
                                   uint8_t * const p_rx_buf,
                                   const uint16_t len) {
    uint16_t i;

    for (i = 0; i < len; i++) {
        p_tx_buf[i] = (uint8_t)('a' + i);
        p_rx_buf[i] = 0;
    }
}

static void spi_slave_event_handle(spi_slave_evt_t event) {
    uint32_t err_code;

    if (event.evt_type == SPI_SLAVE_XFER_DONE) {
        // Check if buffer size is the same as amount of received data
        APP_ERROR_CHECK_BOOL(event.rx_amount == RX_BUF_SIZE);

        // Check if received data is valid
        //bool success = spi_slave_buffer_check(m_rx_buf, event.rx_amount);
        //APP_ERROR_CHECK_BOOL(success);

        touch_on_event(m_rx_buf[0], m_rx_buf[1]);

        // Reset buffers
        err_code = spi_slave_buffers_set(m_tx_buf, m_rx_buf, sizeof(m_tx_buf), sizeof(m_rx_buf));
        APP_ERROR_CHECK(err_code);
    }
}

uint32_t touch_init(void) {
    uint32_t err_code;
    spi_slave_config_t spi_slave_config;

    err_code = spi_slave_evt_handler_register(spi_slave_event_handle);
    APP_ERROR_CHECK(err_code);

    spi_slave_config.pin_miso         = SPIS_MISO_PIN;
    spi_slave_config.pin_mosi         = SPIS_MOSI_PIN;
    spi_slave_config.pin_sck          = SPIS_SCK_PIN;
    spi_slave_config.pin_csn          = SPIS_CSN_PIN;
    spi_slave_config.mode             = SPI_MODE_0;
    spi_slave_config.bit_order        = SPIM_MSB_FIRST;
    spi_slave_config.def_tx_character = DEF_CHARACTER;
    spi_slave_config.orc_tx_character = ORC_CHARACTER;

    err_code = spi_slave_init(&spi_slave_config);
    APP_ERROR_CHECK(err_code);

    spi_slave_buffers_init(m_tx_buf, m_rx_buf, (uint16_t)TX_BUF_SIZE);

    err_code = spi_slave_buffers_set(m_tx_buf, m_rx_buf, sizeof(m_tx_buf), sizeof(m_rx_buf));
    APP_ERROR_CHECK(err_code);
    app_timer_cnt_get(&last_event_received_time);

    return NRF_SUCCESS;
}
