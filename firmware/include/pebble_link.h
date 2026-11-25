#ifndef PEBBLE_LINK_H
#define PEBBLE_LINK_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define PB_LINK_MAX 20
typedef struct { char kind; char name[17]; int64_t value; } pb_command;
typedef struct { char text[32]; size_t length; bool discard; } pb_usb_line;
bool pb_usb_line_feed(pb_usb_line *line, uint8_t byte);
bool pb_command_parse(const uint8_t *data, size_t len, pb_command *out);
bool pb_ntp_parse(const uint8_t *data, size_t len, const uint8_t nonce[8], uint64_t *unix_time);
bool pb_wifi_credentials_valid(const char *ssid, const char *password);
void pb_clock_text(uint64_t epoch, int timezone_minutes, char time_text[9], char date_text[24]);
#endif
