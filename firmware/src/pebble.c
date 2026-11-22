#include "pebble.h"
#include <string.h>
#include <limits.h>
static uint8_t clamp(int n) { return (uint8_t)(n<0?0:n>100?100:n); }
static uint32_t add_sat(uint32_t a, uint32_t b) { return UINT32_MAX-a<b?UINT32_MAX:a+b; }
uint32_t pb_random(pb_state *s, uint32_t limit) {
    uint32_t x=s->rng?s->rng:0x9e3779b9u;
    x^=x<<13; x^=x>>17; x^=x<<5; s->rng=x;
    return limit?x%limit:0;
}
void pb_ttt_reset(pb_ttt *g) { memset(g,0,sizeof(*g)); g->turn=1; }
void pb_init(pb_state *s, uint32_t seed) {
    memset(s,0,sizeof(*s)); s->rng=seed?seed:1;
    s->pet.food=80; s->pet.joy=80; s->pet.energy=90;
    pb_ttt_reset(&s->ttt); pb_memory_reset(s);
}
void pb_reward(pb_state *s, uint32_t xp) { s->pet.xp=add_sat(s->pet.xp,xp); s->pet.joy=clamp(s->pet.joy+8); }
void pb_care(pb_state *s, unsigned action) {
    if(action==0) s->pet.food=clamp(s->pet.food+15);
    else if(action==1) s->pet.joy=clamp(s->pet.joy+10);
    else if(action==2) s->pet.sleeping=!s->pet.sleeping;
}
void pb_timer_start(pb_state *s, uint32_t sec) {
    if(sec==0||sec>86400) return;
    s->timer=(pb_timer){.remaining=sec*1000,.duration=sec*1000,.running=true};
}
void pb_tick(pb_state *s, uint32_t dt) {
    /* Caller supplies elapsed monotonic time; no dependence on UI frame rate. */
    uint64_t decay=(uint64_t)s->pet.decay_ms+dt;
    uint32_t mins=(uint32_t)(decay/60000); s->pet.decay_ms=(uint32_t)(decay%60000);
    if(mins) { int m=mins>100?100:(int)mins; s->pet.food=clamp(s->pet.food-m); s->pet.joy=clamp(s->pet.joy-m); s->pet.energy=clamp(s->pet.energy+(s->pet.sleeping?m*3:-m)); }
    if(s->timer.running) {
        if(dt>=s->timer.remaining) {
            s->timer.remaining=0; s->timer.running=false; s->timer.complete=true;
            if(!s->timer.rewarded) { s->timer.rewarded=true; if(s->timer.duration>=5*60000u) { pb_reward(s,25); s->pet.focuses=add_sat(s->pet.focuses,1); } }
        } else s->timer.remaining-=dt;
    }
    if(s->memory.hide_ms) {
        if(dt>=s->memory.hide_ms) { s->memory.hide_ms=0; s->memory.first=s->memory.second=255; }
        else s->memory.hide_ms-=dt;
    }
    pb_catch *g=&s->catch_game;
    if(g->running) {
        if(dt>=g->left_ms) { g->left_ms=0; g->running=false; s->pet.games=add_sat(s->pet.games,1); pb_reward(s,g->hits); }
        else { g->left_ms-=dt; if(dt>=g->spawn_ms) { g->target=(uint8_t)pb_random(s,6); g->spawn_ms=1100; if(g->misses<255) g->misses++; } else g->spawn_ms-=dt; }
    }
}
static int result(const pb_ttt *g) {
    static const uint8_t rows[8][3]={{0,1,2},{3,4,5},{6,7,8},{0,3,6},{1,4,7},{2,5,8},{0,4,8},{2,4,6}};
    for(unsigned i=0;i<8;i++) { int a=g->cells[rows[i][0]]; if(a&&a==g->cells[rows[i][1]]&&a==g->cells[rows[i][2]]) return a; }
    for(unsigned i=0;i<9;i++) if(!g->cells[i]) return 0;
    return 2; /* draw */
}
bool pb_ttt_move(pb_ttt *g, unsigned cell) {
    if(cell>=9||g->result||g->cells[cell]) return false;
    g->cells[cell]=g->turn; g->result=(int8_t)result(g); g->turn=(int8_t)-g->turn; return true;
}
static int solve(pb_ttt *g, int player) {
    int r=result(g); if(r) return r==2?0:r==player?10:-10;
    int best=-99;
    for(unsigned i=0;i<9;i++) if(!g->cells[i]) { g->cells[i]=(int8_t)player; int score=-solve(g,-player); g->cells[i]=0; if(score>best) best=score; }
    return best;
}
int pb_ttt_best_move(pb_ttt *g) {
    if(result(g)) return -1;
    static const uint8_t order[9]={4,0,2,6,8,1,3,5,7}; int best=-99, move=-1;
    for(unsigned j=0;j<9;j++) { unsigned i=order[j]; if(!g->cells[i]) { g->cells[i]=g->turn; int score=-solve(g,-g->turn); g->cells[i]=0; if(score>best) { best=score; move=(int)i; } } }
    return move;
}
void pb_memory_reset(pb_state *s) {
    pb_memory *g=&s->memory; memset(g,0,sizeof(*g)); g->first=g->second=255;
    for(unsigned i=0;i<8;i++) g->cards[i]=(uint8_t)(i/2);
    for(unsigned i=7;i>0;i--) { unsigned j=pb_random(s,i+1); uint8_t t=g->cards[i]; g->cards[i]=g->cards[j]; g->cards[j]=t; }
}
bool pb_memory_flip(pb_state *s, unsigned cell) {
    pb_memory *g=&s->memory;
    if(cell>=8||g->won||g->hide_ms||(g->matched&(1u<<cell))||g->first==cell) return false;
    if(g->first==255) { g->first=(uint8_t)cell; return true; }
    g->second=(uint8_t)cell; if(g->moves<255) g->moves++;
    if(g->cards[g->first]==g->cards[cell]) {
        g->matched|=(uint8_t)((1u<<g->first)|(1u<<cell)); g->pairs++; g->first=g->second=255;
        if(g->pairs==4) { g->won=true; s->pet.games=add_sat(s->pet.games,1); pb_reward(s,15); }
    } else g->hide_ms=900;
    return true;
}
void pb_catch_start(pb_state *s) { s->catch_game=(pb_catch){.left_ms=20000,.spawn_ms=1100,.target=(uint8_t)pb_random(s,6),.running=true}; }
bool pb_catch_hit(pb_state *s, unsigned target) {
    pb_catch *g=&s->catch_game; if(!g->running||target!=g->target) return false;
    if(g->hits<255) g->hits++;
    /* Move to a different location so a double tap cannot score twice. */
    g->target=(uint8_t)((g->target+1+pb_random(s,5))%6); g->spawn_ms=1100; return true;
}
static void put32(uint8_t *p,uint32_t n) { for(unsigned i=0;i<4;i++) p[i]=(uint8_t)(n>>(8*i)); }
static uint32_t get32(const uint8_t *p) { uint32_t n=0; for(unsigned i=0;i<4;i++) n|=(uint32_t)p[i]<<(8*i); return n; }
static uint32_t crc(const uint8_t *p,size_t n) {
    uint32_t c=0xffffffffu; for(size_t i=0;i<n;i++) { c^=p[i]; for(unsigned b=0;b<8;b++) c=(c>>1)^(0xedb88320u & (uint32_t)-(int32_t)(c&1)); } return ~c;
}
void pb_save(const pb_state *s,uint8_t out[PB_SAVE_SIZE]) {
    memset(out,0,PB_SAVE_SIZE); memcpy(out,"PB01",4);
    out[4]=s->pet.food; out[5]=s->pet.joy; out[6]=s->pet.energy; out[7]=s->pet.theme;
    put32(out+8,s->pet.xp); put32(out+12,s->pet.focuses); put32(out+16,s->pet.games);
    out[20]=s->pet.sleeping?1:0; put32(out+24,s->rng); put32(out+28,crc(out,28));
}
bool pb_load(pb_state *s,const uint8_t *p,size_t n) {
    if(n!=PB_SAVE_SIZE||memcmp(p,"PB01",4)||get32(p+28)!=crc(p,28)||p[4]>100||p[5]>100||p[6]>100||p[7]>3||p[20]>1) return false;
    pb_state next; pb_init(&next,get32(p+24)); next.pet.food=p[4]; next.pet.joy=p[5]; next.pet.energy=p[6]; next.pet.theme=p[7]; next.pet.xp=get32(p+8); next.pet.focuses=get32(p+12); next.pet.games=get32(p+16); next.pet.sleeping=p[20]!=0; *s=next; return true;
}
