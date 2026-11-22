/* Pebble microOS: LVGL 9 application. Native board build is not yet validated. */
#include "pebble.h"
#include "pebble_profile.h"
#include "lvgl.h"
#include "tal_api.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
enum { HOME, APPS, FOCUS, GAMES, TTT, MEMORY, CATCH, STOPWATCH, BREATHE, DICE, SETTINGS, REST };
static pb_state s;
static int page=HOME;
static bool dirty=true, storage_ok=false, stopwatch_running=false;
static uint32_t last_tick, last_save, stopwatch_ms, breath_ms;
static lv_obj_t *root, *live, *sub;
static uint8_t previous_save[PB_SAVE_SIZE];
static uint32_t accent(void) { static const uint32_t c[]={0xc7f568,0x8fcfff,0xffb7a4,0xc4b5ff}; return c[s.pet.theme%4]; }
static lv_obj_t *label(const char *text,int x,int y,int w) {
    lv_obj_t *o=lv_label_create(root); lv_label_set_text(o,text); lv_obj_set_pos(o,x,y); lv_obj_set_width(o,w);
    lv_obj_set_style_text_color(o,lv_color_hex(0xf3f4ee),0); lv_obj_set_style_text_align(o,LV_TEXT_ALIGN_CENTER,0); lv_obj_set_style_text_font(o,&lv_font_montserrat_20,0); return o;
}
static void nav(lv_event_t *e) { page=(int)(intptr_t)lv_event_get_user_data(e); dirty=true; }
static lv_obj_t *button(const char *text,int x,int y,int w,int h,lv_event_cb_t cb,int data) {
    lv_obj_t *o=lv_button_create(root); lv_obj_set_pos(o,x,y); lv_obj_set_size(o,w,h);
    lv_obj_set_style_bg_color(o,lv_color_hex(0x242824),0); lv_obj_set_style_radius(o,18,0); lv_obj_set_style_shadow_width(o,0,0);
    lv_obj_t *l=lv_label_create(o); lv_label_set_text(l,text); lv_obj_set_style_text_font(l,&lv_font_montserrat_20,0); lv_obj_center(l);
    lv_obj_add_event_cb(o,cb,LV_EVENT_CLICKED,(void *)(intptr_t)data); return o;
}
static void head(const char *name) { label(name,100,53,266); }
static void back(int target) { button("Back",173,363,120,48,nav,target); }
static void care(lv_event_t *e) { pb_care(&s,(unsigned)(uintptr_t)lv_event_get_user_data(e)); dirty=true; }
static void timer_set(lv_event_t *e) { pb_timer_start(&s,(uint32_t)(uintptr_t)lv_event_get_user_data(e)); dirty=true; }
static void timer_toggle(lv_event_t *e) { (void)e; if(s.timer.remaining) s.timer.running=!s.timer.running; dirty=true; }
static void timer_cancel(lv_event_t *e) { (void)e; memset(&s.timer,0,sizeof(s.timer)); dirty=true; }
static void ttt_move(lv_event_t *e) {
    if(!pb_ttt_move(&s.ttt,(unsigned)(uintptr_t)lv_event_get_user_data(e))) return;
    if(!s.ttt.result) { int m=pb_ttt_best_move(&s.ttt); if(m>=0) pb_ttt_move(&s.ttt,(unsigned)m); }
    if(s.ttt.result) { s.pet.games++; pb_reward(&s,5); } dirty=true;
}
static void ttt_reset(lv_event_t *e) { (void)e; pb_ttt_reset(&s.ttt); dirty=true; }
static void memory_flip(lv_event_t *e) { pb_memory_flip(&s,(unsigned)(uintptr_t)lv_event_get_user_data(e)); dirty=true; }
static void memory_reset(lv_event_t *e) { (void)e; pb_memory_reset(&s); dirty=true; }
static void catch_hit(lv_event_t *e) { pb_catch_hit(&s,(unsigned)(uintptr_t)lv_event_get_user_data(e)); dirty=true; }
static void catch_start(lv_event_t *e) { (void)e; pb_catch_start(&s); dirty=true; }
static void sw_toggle(lv_event_t *e) { (void)e; stopwatch_running=!stopwatch_running; dirty=true; }
static void sw_reset(lv_event_t *e) { (void)e; stopwatch_running=false; stopwatch_ms=0; dirty=true; }
static void dice_roll(lv_event_t *e) { (void)e; char t[24]; snprintf(t,sizeof(t),"%u",(unsigned)pb_random(&s,6)+1); if(live) lv_label_set_text(live,t); }
static void theme(lv_event_t *e) { s.pet.theme=(uint8_t)(uintptr_t)lv_event_get_user_data(e); dirty=true; }
static void save_now(void) {
    uint8_t data[PB_SAVE_SIZE]; pb_save(&s,data);
    if(storage_ok&&memcmp(data,previous_save,sizeof(data))) {
        if(tal_kv_set("pebble.v1",data,sizeof(data))==OPRT_OK) memcpy(previous_save,data,sizeof(data));
        else PR_ERR("Pebble: save failed");
    }
}
static void save_click(lv_event_t *e) { (void)e; save_now(); }
static void face(void) {
    for(int i=0;i<2;i++) { lv_obj_t *eye=lv_obj_create(root); lv_obj_set_pos(eye,156+i*100,135); lv_obj_set_size(eye,54,s.pet.sleeping?10:PB_EYE_STYLE==1?38:76); lv_obj_set_style_radius(eye,PB_EYE_STYLE==2?4:25,0); lv_obj_set_style_bg_color(eye,lv_color_hex(accent()),0); lv_obj_set_style_border_width(eye,0,0); lv_obj_remove_flag(eye,LV_OBJ_FLAG_SCROLLABLE); }
    label(s.pet.sleeping?"z z z":"  u  ",170,215,126);
}
static void render(void) {
    live=sub=NULL; lv_obj_clean(root); lv_obj_set_style_bg_color(root,lv_color_hex(0x080b09),0);
    char buf[100];
    switch(page) {
    case HOME:
        snprintf(buf,sizeof(buf),"%s / LEVEL %u",PB_PET_NAME,(unsigned)(1+s.pet.xp/100)); head(buf); face();
        snprintf(buf,sizeof(buf),"Food %u  Joy %u  Energy %u",s.pet.food,s.pet.joy,s.pet.energy); label(buf,75,258,316);
        button("Feed",83,295,94,52,care,0); button("Pat",186,295,94,52,care,1); button(s.pet.sleeping?"Wake":"Nap",289,295,94,52,care,2);
        button("Apps",173,363,120,48,nav,APPS); break;
    case APPS: {
        head("YOUR LITTLE WORLD"); const char *names[]={"Pet","Focus","Games","Stopwatch","Breathe","Dice","Settings","Screen rest"}; const int pages[]={HOME,FOCUS,GAMES,STOPWATCH,BREATHE,DICE,SETTINGS,REST};
        for(int i=0;i<8;i++) {
            button(names[i],85+(i%2)*154,100+(i/2)*63,144,54,nav,pages[i]);
        }
        back(HOME); break;
    }
    case FOCUS:
        head("FOCUS / TIMER"); live=label("25:00",110,124,246); lv_obj_set_style_text_font(live,&lv_font_montserrat_40,0); sub=label("Grow a little, together.",80,185,306);
        if(s.timer.remaining) { button(s.timer.running?"Pause":"Resume",103,237,124,55,timer_toggle,0); button("Cancel",239,237,124,55,timer_cancel,0); }
        else { snprintf(buf,sizeof(buf),"%d min",PB_FOCUS_MINUTES); button("5 min",83,235,94,55,timer_set,300); button(buf,186,235,94,55,timer_set,PB_FOCUS_MINUTES*60); button("50 min",289,235,94,55,timer_set,3000); button("1 min timer",143,300,180,48,timer_set,60); } back(APPS); break;
    case GAMES:
        head("PLAY A LITTLE"); button("Tic-tac-toe",108,117,250,59,nav,TTT); button("Memory garden",108,193,250,59,nav,MEMORY); button("Catch a spark",108,269,250,59,nav,CATCH); back(APPS); break;
    case TTT:
        head(s.ttt.result?(s.ttt.result==2?"A FRIENDLY DRAW":s.ttt.result==1?"YOU WIN":"PEBBLE WINS"):"YOUR TURN / X");
        for(int i=0;i<9;i++) button(s.ttt.cells[i]==1?"X":s.ttt.cells[i]==-1?"O":".",116+(i%3)*80,96+(i/3)*75,74,68,ttt_move,i);
        button("Again",173,323,120,36,ttt_reset,0); back(GAMES); break;
    case MEMORY:
        snprintf(buf,sizeof(buf),"MEMORY / %u OF 4 PAIRS",s.memory.pairs); head(buf);
        for(int i=0;i<8;i++) { bool shown=(s.memory.matched&(1<<i))||s.memory.first==i||s.memory.second==i; char c[2]={(char)('A'+s.memory.cards[i]),0}; button(shown?c:"?",85+(i%4)*76,136+(i/4)*83,68,74,memory_flip,i); }
        button(s.memory.won?"You did it! Again":"New game",143,313,180,42,memory_reset,0); back(GAMES); break;
    case CATCH:
        head("CATCH A SPARK"); live=label("20 seconds",90,92,286);
        for(int i=0;i<6;i++) button(s.catch_game.running&&s.catch_game.target==i?"*":".",116+(i%3)*80,132+(i/3)*80,72,70,catch_hit,i);
        if(!s.catch_game.running) {
            button("Start",153,309,160,43,catch_start,0);
        }
        back(GAMES); break;
    case STOPWATCH:
        head("STOPWATCH"); live=label("00:00",100,165,266); lv_obj_set_style_text_font(live,&lv_font_montserrat_40,0);
        button(stopwatch_running?"Pause":"Start",103,249,124,58,sw_toggle,0); button("Reset",239,249,124,58,sw_reset,0); back(APPS); break;
    case BREATHE:
        head("ONE QUIET MOMENT"); live=label("Breathe in",100,181,266); sub=label("4 in / 4 hold / 6 out",90,232,286); back(APPS); break;
    case DICE:
        head("LET IT ROLL"); live=label("?",100,158,266); button("Roll a die",133,258,200,60,dice_roll,0); back(APPS); break;
    case SETTINGS:
        head("MAKE IT YOURS"); for(int i=0;i<4;i++) { const char *c[]={"Moss","Sky","Peach","Lilac"}; button(c[i],85+(i%2)*154,115+(i/2)*66,144,55,theme,i); }
        button("Save pet now",123,255,220,49,save_click,0); label(storage_ok?"Offline / pet saved every 30 s":"Storage unavailable / RAM only",60,322,346); back(APPS); break;
    case REST:
        /* This is a black screen, NOT a verified hardware low-power mode. */
        button("Wake",173,209,120,48,nav,HOME); break;
    }
    dirty=false;
}
static void tick(lv_timer_t *t) {
    (void)t; uint32_t now=lv_tick_get(), dt=now-last_tick; last_tick=now;
    uint8_t old_target=s.catch_game.target, old_first=s.memory.first; bool was_timer=s.timer.running, was_catch=s.catch_game.running;
    pb_tick(&s,dt); breath_ms+=dt; if(stopwatch_running) stopwatch_ms+=dt;
    if((page==MEMORY&&old_first!=s.memory.first)||(page==CATCH&&(old_target!=s.catch_game.target||was_catch!=s.catch_game.running))||(page==FOCUS&&was_timer!=s.timer.running)) dirty=true;
    if(dirty) render();
    char b[100];
    if(page==FOCUS&&live) { unsigned seconds=(s.timer.remaining+999)/1000; if(!s.timer.duration) seconds=PB_FOCUS_MINUTES*60; snprintf(b,sizeof(b),"%02u:%02u",seconds/60,seconds%60); lv_label_set_text(live,b); lv_label_set_text(sub,s.timer.complete?"Done. A little win for you both.":s.timer.running?"Pebble is keeping you company.":"Ready when you are."); }
    if(page==STOPWATCH&&live) { unsigned sec=stopwatch_ms/1000; snprintf(b,sizeof(b),"%02u:%02u.%u",sec/60,sec%60,(unsigned)(stopwatch_ms/100)%10); lv_label_set_text(live,b); }
    if(page==CATCH&&live) { snprintf(b,sizeof(b),"%u s / %u sparks",(unsigned)(s.catch_game.left_ms+999)/1000,s.catch_game.hits); lv_label_set_text(live,b); }
    if(page==BREATHE&&live) { uint32_t p=breath_ms%14000; lv_label_set_text(live,p<4000?"Breathe in":p<8000?"Hold gently":"Breathe out"); }
    if(now-last_save>=30000) { last_save=now; save_now(); }
}
void pebble_ui_start(void) {
    pb_init(&s,lv_tick_get()+37);
    s.pet.theme=PB_DEFAULT_THEME;
    /* Local pet data only. These public demo parameters provide no secrecy. */
    tal_kv_cfg_t cfg={.seed="pebblelocal2026",.key="pebblekvlocalv1"};
    storage_ok=(tal_kv_init(&cfg)==OPRT_OK);
    if(storage_ok) { uint8_t *p=NULL; size_t n=0; if(tal_kv_get("pebble.v1",&p,&n)==OPRT_OK) { if(!pb_load(&s,p,n)) PR_NOTICE("Pebble: invalid save, using defaults"); tal_kv_free(p); } }
    root=lv_screen_active(); lv_obj_remove_flag(root,LV_OBJ_FLAG_SCROLLABLE); last_tick=last_save=lv_tick_get(); render(); lv_timer_create(tick,50,NULL);
}
