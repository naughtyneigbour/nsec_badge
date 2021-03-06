//
//  controls.c
//  nsec16
//
//  Created by Marc-Etienne M.Léveillé on 2016-05-17.
//
//

#include "controls.h"
#include "ssd1306.h"
#include <app_error.h>

static button_handler handlers[NSEC_CONTROLS_LIMIT_MAX_HANDLERS];
static uint8_t handler_count = 0;

void nsec_controls_add_handler(button_handler handler) {
    if(handler_count >= NSEC_CONTROLS_LIMIT_MAX_HANDLERS) {
        return;
    }
    for(int i = 0; i < handler_count; i++) {
        if(handlers[i] == handler) {
            return;
        }
    }
    handlers[handler_count++] = handler;
}

void nsec_controls_trigger(button_t button) {
    for(int i = 0; i < handler_count; i++) {
        handlers[i](button);
    }
}
