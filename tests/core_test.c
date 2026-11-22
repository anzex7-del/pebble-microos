#include "pebble.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static unsigned branches;
static void explore(pb_ttt g) {
    if(g.result) { assert(g.result!=1); branches++; return; }
    if(g.turn==-1) { int m=pb_ttt_best_move(&g); assert(m>=0); assert(pb_ttt_move(&g,(unsigned)m)); explore(g); }
    else for(unsigned i=0;i<9;i++) if(!g.cells[i]) { pb_ttt c=g; assert(pb_ttt_move(&c,i)); explore(c); }
}
int main(void) {
    pb_state s; pb_init(&s,42); assert(s.pet.food==80);
    pb_care(&s,0); pb_care(&s,0); assert(s.pet.food==100);
    pb_tick(&s,120000); assert(s.pet.food==98&&s.pet.energy==88);
    pb_care(&s,2); pb_tick(&s,600000); assert(s.pet.energy==100);
    pb_timer_start(&s,300); pb_tick(&s,1000); assert(s.timer.remaining==299000);
    s.timer.running=false; pb_tick(&s,1000); assert(s.timer.remaining==299000);
    s.timer.running=true; pb_tick(&s,300000); assert(s.timer.complete&&s.pet.focuses==1&&s.pet.xp==25);
    pb_tick(&s,300000); assert(s.pet.focuses==1);
    pb_timer_start(&s,0); assert(s.timer.complete);
    pb_timer_start(&s,10); pb_tick(&s,10000); assert(s.pet.focuses==1);
    pb_ttt_reset(&s.ttt); assert(!pb_ttt_move(&s.ttt,9)); explore(s.ttt);
    pb_memory_reset(&s); for(unsigned i=0;i<8;i++) for(unsigned j=i+1;j<8;j++) if(s.memory.cards[i]==s.memory.cards[j]) { assert(pb_memory_flip(&s,i)); assert(!pb_memory_flip(&s,i)); assert(pb_memory_flip(&s,j)); }
    assert(s.memory.won&&s.pet.games==1); assert(!pb_memory_flip(&s,0));
    pb_catch_start(&s); unsigned old=s.catch_game.target; assert(pb_catch_hit(&s,old)); assert(!pb_catch_hit(&s,old)); pb_tick(&s,20001); assert(!s.catch_game.running&&s.pet.games==2);
    uint8_t saved[PB_SAVE_SIZE]; pb_save(&s,saved); pb_state copy; pb_init(&copy,8); assert(pb_load(&copy,saved,sizeof(saved))); assert(copy.pet.xp==s.pet.xp&&copy.pet.games==2); assert(!pb_load(&copy,saved,4)); saved[8]^=1; assert(!pb_load(&copy,saved,sizeof(saved))); assert(copy.pet.games==2);
    pb_tick(&s,UINT32_MAX); assert(s.pet.food<=100&&s.pet.energy<=100);
    printf("PASS: pet, monotonic timers, once-only rewards, %u complete tic-tac-toe paths, memory, catch, save validation\n",branches);
}
