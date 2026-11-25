#ifndef PEBBLE_SOUND_H
#define PEBBLE_SOUND_H
#include <stdbool.h>
/* Called only from the background service worker. */
bool pb_sound_play(unsigned reaction);
#endif
