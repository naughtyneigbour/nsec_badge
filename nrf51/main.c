/*
 * Copyright (c) 2015, Benjamin Vanheuverzwijn <bvanheu@gmail.com>
 * Copyright (c) 2016, Marc-Etienne M.Léveillé <marc.etienne.ml@gmail.com>
 * License: MIT (see LICENSE for details)
 */

#include "boards.h"

#include <app_util.h>
#include <app_scheduler.h>
#include <app_error.h>
#include <app_button.h>
#include <app_timer.h>
#include <app_gpiote.h>
#include <app_uart.h>

#include "ble/nsec_ble.h"

#include <softdevice_handler.h>

#include <nrf.h>
#include <nrf_error.h>
#include <nrf_gpio.h>
#include <nrf_delay.h>
#include <nrf51.h>
#include <nrf51_bitfields.h>
#include <nordic_common.h>

#include <stdbool.h>
#include <stdint.h>

#include "ssd1306.h"

#include "images/nsec_logo_bitmap.c"
#include "animal_care.h"
#include "status_bar.h"
#include "menu.h"
#include "nsec_conf_schedule.h"
#include "nsec_settings.h"
#include "battery.h"
#include "touch_button.h"

static char g_device_id[32];


void wdt_init(void)
{
    NRF_WDT->CONFIG = (WDT_CONFIG_HALT_Pause << WDT_CONFIG_HALT_Pos) | ( WDT_CONFIG_SLEEP_Run << WDT_CONFIG_SLEEP_Pos);   //Configure Watchdog. a) Pause watchdog while the CPU is halted by the debugger.  b) Keep the watchdog running while the CPU is sleeping.
    NRF_WDT->CRV = 3*32768;             //ca 3 sek. timout
    NRF_WDT->RREN |= WDT_RREN_RR0_Msk;  //Enable reload register 0
    NRF_WDT->TASKS_START = 1;           //Start the Watchdog timer
}

void app_error_handler(uint32_t error_code, uint32_t line_num, const uint8_t * p_file_name) {
    nrf_gpio_cfg_output(LED_RED);
    nrf_gpio_cfg_output(LED_GREEN);
    nrf_gpio_pin_set(LED_RED);
    nrf_gpio_pin_set(LED_GREEN);
    
    static int error_displayed = 0;
    
    if(!error_displayed) {
        char error_msg[128];
        snprintf(error_msg, sizeof(error_msg), "%s:%u -> error 0x%08x\r\n", p_file_name, (unsigned int)line_num, (unsigned int)error_code);
        puts(error_msg);
        gfx_setCursor(0, 0);
        gfx_puts(error_msg);
        gfx_update();
        error_displayed = 1;
    }
    uint8_t count = 10;
    while (count > 0) {
        nrf_gpio_pin_toggle(LED_RED);
        nrf_delay_ms(500);
        count--;
    }

    NVIC_SystemReset(); 
}


/**
 * Callback function for asserts in the SoftDevice.
 * This function will be called in case of an assert in the SoftDevice.
 */
void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name) {
    app_error_handler(0xdeadbeef, line_num, p_file_name);
}


/**
 * Task timers
 */
// Green hearbeat
static app_timer_id_t m_heartbeat_timer_id;
static void heartbeat_timeout_handler(void * p_context) {
    //if (flashlight_cmd_data.state == ST_INIT) {
    //    nrf_gpio_pin_set(LED_RGB_GREEN);
    //}

    NRF_WDT->RR[0] = WDT_RR_RR_Reload;
}

/**
 * Init functions
 */
static void timers_init(void) {
    uint32_t err_code;

    // Initialize timer module.
    APP_TIMER_INIT(APP_TIMER_PRESCALER, 10 /* APP_TIMER_MAX_TIMERS */, 16 /* APP_TIMER_OP_QUEUE_SIZE */, true /* USE SCHEDULER */);

    // Create timers.
    err_code = app_timer_create(&m_heartbeat_timer_id,
            APP_TIMER_MODE_REPEATED,
            heartbeat_timeout_handler);
    APP_ERROR_CHECK(err_code);
}

void sys_evt_dispatch(uint32_t evt_id) {

}

static void softdevice_init(void) {
    uint32_t err_code;

    // Initialize the SoftDevice handler module.
   SOFTDEVICE_HANDLER_INIT(NRF_CLOCK_LFCLKSRC_RC_250_PPM_1000MS_CALIBRATION, false);

    // Register with the SoftDevice handler module for events.
    err_code = softdevice_sys_evt_handler_set(sys_evt_dispatch);
    APP_ERROR_CHECK(err_code);
}

static void application_timers_start(void) {
    uint32_t err_code;

    // Start application timers.
    err_code = app_timer_start(m_heartbeat_timer_id, APP_TIMER_TICKS(500 /* ms */, APP_TIMER_PRESCALER), NULL);
    APP_ERROR_CHECK(err_code);
}

static void nsec_intro(void) {
    gfx_fillScreen(BLACK);
    gfx_drawBitmap(17, 60, nsec_logo_bitmap, nsec_logo_bitmap_width, nsec_logo_bitmap_height, WHITE);
    gfx_update();
    for(int y = 60; y > 11; y--) {
        gfx_fillScreen(BLACK);
        gfx_drawBitmap(17, y, nsec_logo_bitmap, nsec_logo_bitmap_width, nsec_logo_bitmap_height, WHITE);
        gfx_update();
    }
}

void open_animal_care(uint8_t item);
void open_conference_schedule(uint8_t item);
void open_settings(uint8_t item);

static menu_item_s main_menu_items[] = {
    {
        .label = "Conference schedule",
        .handler = open_conference_schedule,
    }, {
        .label = "My Cyber Pet",
        .handler = open_animal_care,
    }, {
        .label = "Settings",
        .handler = open_settings,
    }
};

void open_animal_care(uint8_t item) {
    menu_close();
    animal_show();
}

void open_conference_schedule(uint8_t item) {
    menu_close();
    nsec_schedule_show_dates();
}

void open_settings(uint8_t item) {
    menu_close();
    nsec_setting_show();
}

void show_main_menu(void) {
    gfx_fillScreen(BLACK);
    nsec_intro();
    nsec_status_bar_ui_redraw();
    menu_init(0, 64-8, 128, 8, sizeof(main_menu_items) / sizeof(main_menu_items[0]), main_menu_items);
}

/**
 * Main
 */
int main() {
    sprintf(g_device_id, "NSEC%04X", (uint16_t)(NRF_FICR->DEVICEID[1] % 0xFFFF));
    g_device_id[9] = '\0';

    nrf_gpio_cfg_output(LED_RED);
    nrf_gpio_cfg_output(LED_GREEN);

    nrf_gpio_pin_set(LED_RED);
    nrf_gpio_pin_set(LED_GREEN);

    softdevice_init();

    APP_SCHED_INIT(APP_TIMER_SCHED_EVT_SIZE /* EVENT_SIZE */, 12 /* QUEUE SIZE */);

    timers_init();
    APP_GPIOTE_INIT(2);

    ssd1306_init();
    touch_init();
    gfx_setTextBackgroundColor(WHITE, BLACK);

    application_timers_start();
    nsec_battery_manager_init();

    nsec_ble_init(g_device_id);
    nsec_ble_add_device_information_service(g_device_id, "NSEC 2016 Badge", NULL, NULL, NULL, NULL);

    animal_init();

    nsec_status_bar_init();
    nsec_status_set_name(g_device_id);
    nsec_status_set_badge_class("");
    nsec_status_set_ble_status(STATUS_BLUETOOTH_ON);

    show_main_menu();

    while (true) {
        app_sched_execute();

        uint32_t err_code = sd_app_evt_wait();
        APP_ERROR_CHECK(err_code);
    }

    return 0;
}
