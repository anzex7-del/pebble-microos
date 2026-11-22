#ifndef PEBBLE_H
#define PEBBLE_H
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#define PB_SAVE_SIZE 32
typedef struct { uint8_t food, joy, energy, theme; uint32_t xp, focuses, games; bool sleeping; uint32_t decay_ms; } pb_pet;
typedef struct { uint32_t remaining, duration; bool running, complete, rewarded; } pb_timer;
typedef struct { int8_t cells[9]; int8_t turn, result; } pb_ttt;
typedef struct { uint8_t cards[8], matched, first, second, pairs, moves; uint32_t hide_ms; bool won; } pb_memory;
typedef struct { uint32_t left_ms, spawn_ms; uint8_t target, hits, misses; bool running; } pb_catch;
typedef struct { pb_pet pet; pb_timer timer; pb_ttt ttt; pb_memory memory; pb_catch catch_game; uint32_t rng; } pb_state;
void pb_init(pb_state *s, uint32_t seed);
void pb_tick(pb_state *s, uint32_t elapsed_ms);
void pb_care(pb_state *s, unsigned action); /* 0 feed, 1 pat, 2 sleep/wake */
void pb_reward(pb_state *s, uint32_t xp);
uint32_t pb_random(pb_state *s, uint32_t limit);
void pb_timer_start(pb_state *s, uint32_t seconds);
void pb_ttt_reset(pb_ttt *g);
bool pb_ttt_move(pb_ttt *g, unsigned cell);
int pb_ttt_best_move(pb_ttt *g);
void pb_memory_reset(pb_state *s);
bool pb_memory_flip(pb_state *s, unsigned cell);
void pb_catch_start(pb_state *s);
bool pb_catch_hit(pb_state *s, unsigned target);
void pb_save(const pb_state *s, uint8_t out[PB_SAVE_SIZE]);
bool pb_load(pb_state *s, const uint8_t *data, size_t length);
#endif
