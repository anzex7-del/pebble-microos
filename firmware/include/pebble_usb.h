#ifndef PEBBLE_USB_H
#define PEBBLE_USB_H
#include "pebble_link.h"

void pb_usb_start(void);
void pb_usb_apply_pending(void (*apply)(const pb_command *));

#endif
