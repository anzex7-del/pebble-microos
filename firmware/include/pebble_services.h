#ifndef PEBBLE_SERVICES_H
#define PEBBLE_SERVICES_H
#include "pebble.h"
#include "pebble_link.h"
#include "pebble_controls.h"

#define PB_AP_MAX 8
typedef struct {
    uint32_t version;
    char name[17];
    uint8_t eyes, focus_minutes;
    int16_t timezone_minutes;
} pb_preferences;
typedef struct { char ssid[33]; int rssi; } pb_ap;
typedef struct {
    bool ready, wifi_on, wifi_connected, scanning, ble_enabled, ble_connected;
    bool clock_valid, clock_dated, save_ok;
    uint64_t epoch;
    uint32_t generation, ble_seconds;
    unsigned ap_count;
    pb_ap aps[PB_AP_MAX];
    char wifi_status[64], ip[20], ble_status[48], time_status[48];
} pb_services_status;

void pb_services_init(pb_state *state, pb_preferences *preferences, pb_controls_settings *controls);
void pb_services_snapshot(pb_services_status *out);
void pb_services_save(const pb_state *state, const pb_preferences *preferences, const pb_controls_settings *controls);
void pb_services_scan(void);
bool pb_services_connect(const char *ssid, const char *password, bool remember);
void pb_services_wifi_off(void);
void pb_services_ble(bool enabled);
bool pb_services_next_command(pb_command *out);
void pb_services_reply(const pb_command *command);
void pb_services_sync_time(void);
void pb_services_set_time(uint64_t epoch, bool dated);
void pb_services_forget_wifi(void);
#endif
