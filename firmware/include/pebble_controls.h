#ifndef PEBBLE_CONTROLS_H
#define PEBBLE_CONTROLS_H

#include <stdbool.h>
#include <stdint.h>

enum { PB_PWR, PB_KEY, PB_BUTTON_COUNT };
enum { PB_SHORTCUT_NONE, PB_SHORTCUT_HOME, PB_SHORTCUT_APPS, PB_SHORTCUT_CLOCK,
       PB_SHORTCUT_DICE, PB_SHORTCUT_COUNT };
enum { PB_BRIGHTNESS_MIN = 10, PB_BRIGHTNESS_MAX = 100 };

typedef struct {
    uint8_t brightness;
    uint8_t shortcut[PB_BUTTON_COUNT];
    uint8_t sound_muted;
} pb_controls_settings;

void pb_controls_init(void);
unsigned pb_controls_clicks(uint32_t now);
bool pb_set_brightness(uint8_t percent);

#endif
