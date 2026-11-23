#include "pebble_controls.h"
#include "board_com_api.h"
#include "tdl_display_manage.h"
#include "tkl_gpio.h"

/* The board driver owns the PWR interrupt and its three-second shutdown.
 * Sample the existing input without reconfiguring its pin or power latch. */
static const TUYA_GPIO_NUM_E pins[PB_BUTTON_COUNT] = {EXAMPLE_SYS_PWR_PIN, TUYA_GPIO_NUM_12};
static bool key_ready;
static TDL_DISP_HANDLE_T display;
static struct button_state {
    bool initialized, raw, stable, armed;
    uint32_t changed, pressed;
} buttons[PB_BUTTON_COUNT];

void pb_controls_init(void)
{
    TUYA_GPIO_BASE_CFG_T cfg = {.direct = TUYA_GPIO_INPUT, .mode = TUYA_GPIO_PULLUP,
                                .level = TUYA_GPIO_LEVEL_HIGH};
    key_ready = tkl_gpio_init(pins[PB_KEY], &cfg) == OPRT_OK;
    display = tdl_disp_find_dev(DISPLAY_NAME);
}

unsigned pb_controls_clicks(uint32_t now)
{
    unsigned clicks = 0;
    for (unsigned i = 0; i < PB_BUTTON_COUNT; ++i) {
        TUYA_GPIO_LEVEL_E level;
        struct button_state *b = &buttons[i];
        if ((i == PB_KEY && !key_ready) || tkl_gpio_read(pins[i], &level) != OPRT_OK) {
            b->initialized = false;
            continue;
        }
        bool down = level == TUYA_GPIO_LEVEL_LOW;
        if (!b->initialized) {
            *b = (struct button_state){.initialized = true, .raw = down, .stable = down};
        }
        if (down != b->raw) {
            b->raw = down;
            b->changed = now;
        }
        if (down == b->stable || now - b->changed < 40) continue;
        b->stable = down;
        if (down) {
            b->pressed = b->changed;
            b->armed = true;
        } else {
            if (b->armed && b->changed - b->pressed < 800) clicks |= 1u << i;
            b->armed = false;
        }
    }
    return clicks;
}

bool pb_set_brightness(uint8_t percent)
{
    return display && tdl_disp_set_brightness(display, percent) == OPRT_OK;
}
