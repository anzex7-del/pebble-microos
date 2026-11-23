/* Retained LVGL controls: navigation rebuilds a page; interactions update it in place. */
#include "pebble.h"
#include "pebble_profile.h"
#include "pebble_services.h"
#include "lvgl.h"
#include "tal_api.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

enum { HOME, APPS, FOCUS, GAMES, TTT, MEMORY, CATCH, STOPWATCH, BREATHE, DICE, SETTINGS, REST, CLOCK, CLOCK_SET, TOOLS, CONNECT, WIFI, WIFI_JOIN, BLUETOOTH };
static pb_state s;
static pb_preferences preferences;
static pb_services_status services;
static int page=HOME, requested_page=HOME, mood, manual_hour=12, manual_minute;
static bool stopwatch_running, redraw=true, first_frame=true, motion=true;
static uint32_t last_tick, last_save, last_refresh, last_face, stopwatch_ms, breath_ms, mood_until;
static lv_obj_t *root, *title, *live, *sub, *status_label, *aux_label, *cells[9], *actions[5], *bars[3];
static lv_obj_t *pet_group, *pet_body, *pet_eyes[2], *pet_glints[2], *pet_blush[2], *pet_mouth, *pet_extra;
static lv_obj_t *ssid_input, *password_input, *remember_box, *keyboard;
static char selected_ssid[33];
static unsigned listed_count;
static bool listed_scanning;
static int cached_eyes=-1, cached_love=-1, cached_theme=-1, cached_target=-2;
static uint32_t accent(void) { static const uint32_t c[]={0xbfe6a0,0xa4d6f2,0xfac6b1,0xd0bbf3}; return c[s.pet.theme%4]; }
static void text_if_changed(lv_obj_t *object,const char *text) { if(object&&strcmp(lv_label_get_text(object),text)) lv_label_set_text(object,text); }
static void button_text(lv_obj_t *button,const char *text) { if(button) text_if_changed(lv_obj_get_child(button,0),text); }
static void show(lv_obj_t *object,bool visible) { if(!object) return; if(visible) lv_obj_remove_flag(object,LV_OBJ_FLAG_HIDDEN); else lv_obj_add_flag(object,LV_OBJ_FLAG_HIDDEN); }
static lv_obj_t *shape(lv_obj_t *parent,int x,int y,int w,int h,int radius,uint32_t color) {
    lv_obj_t *o=lv_obj_create(parent); lv_obj_remove_style_all(o); lv_obj_set_pos(o,x,y); lv_obj_set_size(o,w,h);
    lv_obj_set_style_radius(o,radius,0); lv_obj_set_style_bg_color(o,lv_color_hex(color),0); lv_obj_set_style_bg_opa(o,LV_OPA_COVER,0);
    lv_obj_remove_flag(o,LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_CLICKABLE); return o;
}
static lv_obj_t *label_at(lv_obj_t *parent,const char *text,int x,int y,int w,const lv_font_t *font,uint32_t color) {
    lv_obj_t *o=lv_label_create(parent); lv_obj_set_pos(o,x,y); lv_obj_set_width(o,w); lv_label_set_text(o,text);
    lv_obj_set_style_text_color(o,lv_color_hex(color),0); lv_obj_set_style_text_align(o,LV_TEXT_ALIGN_CENTER,0); lv_obj_set_style_text_font(o,font,0); return o;
}
static lv_obj_t *label(const char *text,int x,int y,int w) { return label_at(root,text,x,y,w,&lv_font_montserrat_20,0xf3f3ed); }
static lv_obj_t *small(const char *text,int x,int y,int w) { return label_at(root,text,x,y,w,&lv_font_montserrat_16,0xabb7aa); }
static void head(const char *name) { title=label(name,85,47,296); }
static void nav(lv_event_t *e) { requested_page=(int)(intptr_t)lv_event_get_user_data(e); }
static lv_obj_t *button(const char *text,int x,int y,int w,int h,lv_event_cb_t callback,int value) {
    lv_obj_t *o=lv_button_create(root); lv_obj_set_pos(o,x,y); lv_obj_set_size(o,w,h);
    lv_obj_set_style_bg_color(o,lv_color_hex(0x243029),0); lv_obj_set_style_bg_color(o,lv_color_hex(0x465744),LV_STATE_PRESSED);
    lv_obj_set_style_radius(o,17,0); lv_obj_set_style_shadow_width(o,0,0); lv_obj_set_style_border_width(o,0,0);
    lv_obj_t *l=lv_label_create(o); lv_label_set_text(l,text); lv_obj_set_style_text_font(l,&lv_font_montserrat_20,0); lv_obj_center(l);
    lv_obj_add_event_cb(o,callback,LV_EVENT_CLICKED,(void *)(intptr_t)value); return o;
}
static void primary(lv_obj_t *o) { lv_obj_set_style_bg_color(o,lv_color_hex(accent()),0); lv_obj_set_style_text_color(o,lv_color_hex(0x15221b),0); }
static void back(int target) { button("Back",173,385,120,44,nav,target); }
static void save_pet(void) { pb_services_save(&s,&preferences); }
static void care(lv_event_t *e) {
    int action=(int)(intptr_t)lv_event_get_user_data(e); pb_care(&s,(unsigned)action);
    mood=action==0?1:action==1?2:0; mood_until=lv_tick_get()+2400; last_refresh=0; save_pet();
}
static void timer_set(lv_event_t *e) { pb_timer_start(&s,(uint32_t)(uintptr_t)lv_event_get_user_data(e)); last_refresh=0; }
static void timer_toggle(lv_event_t *e) { (void)e; if(s.timer.remaining) s.timer.running=!s.timer.running; last_refresh=0; }
static void timer_cancel(lv_event_t *e) { (void)e; memset(&s.timer,0,sizeof(s.timer)); last_refresh=0; }
static void ttt_move(lv_event_t *e) {
    if(!pb_ttt_move(&s.ttt,(unsigned)(uintptr_t)lv_event_get_user_data(e))) return;
    if(!s.ttt.result) { int m=pb_ttt_best_move(&s.ttt); if(m>=0) pb_ttt_move(&s.ttt,(unsigned)m); }
    if(s.ttt.result) { s.pet.games++; pb_reward(&s,5); save_pet(); } last_refresh=0;
}
static void ttt_reset(lv_event_t *e) { (void)e; pb_ttt_reset(&s.ttt); last_refresh=0; }
static void memory_flip(lv_event_t *e) { pb_memory_flip(&s,(unsigned)(uintptr_t)lv_event_get_user_data(e)); last_refresh=0; }
static void memory_reset(lv_event_t *e) { (void)e; pb_memory_reset(&s); last_refresh=0; }
static void catch_hit(lv_event_t *e) { pb_catch_hit(&s,(unsigned)(uintptr_t)lv_event_get_user_data(e)); last_refresh=0; }
static void catch_start(lv_event_t *e) { (void)e; pb_catch_start(&s); last_refresh=0; }
static void sw_toggle(lv_event_t *e) { (void)e; stopwatch_running=!stopwatch_running; last_refresh=0; }
static void sw_reset(lv_event_t *e) { (void)e; stopwatch_running=false; stopwatch_ms=0; last_refresh=0; }
static void dice_roll(lv_event_t *e) { (void)e; char t[8]; snprintf(t,sizeof(t),"%u",(unsigned)pb_random(&s,6)+1); text_if_changed(live,t); }
static void theme(lv_event_t *e) { s.pet.theme=(uint8_t)(uintptr_t)lv_event_get_user_data(e); save_pet(); last_refresh=0; }
static void motion_toggle(lv_event_t *e) { (void)e; motion=!motion; last_refresh=0; }
static void save_click(lv_event_t *e) { (void)e; save_pet(); text_if_changed(status_label,"Save queued"); }
static void wifi_scan(lv_event_t *e) { (void)e; pb_services_scan(); }
static void wifi_off(lv_event_t *e) { (void)e; pb_services_wifi_off(); }
static void wifi_forget(lv_event_t *e) { (void)e; pb_services_forget_wifi(); }
static void choose_network(lv_event_t *e) { int i=(int)(intptr_t)lv_event_get_user_data(e); selected_ssid[0]=0; if(i>=0&&(unsigned)i<services.ap_count) snprintf(selected_ssid,sizeof(selected_ssid),"%s",services.aps[i].ssid); requested_page=WIFI_JOIN; }
static void input_focus(lv_event_t *e) { lv_keyboard_set_textarea(keyboard,lv_event_get_target(e)); }
static void connect_wifi(lv_event_t *e) {
    (void)e; bool remember=lv_obj_has_state(remember_box,LV_STATE_CHECKED);
    if(pb_services_connect(lv_textarea_get_text(ssid_input),lv_textarea_get_text(password_input),remember)) { lv_textarea_set_text(password_input,""); requested_page=WIFI; }
    else text_if_changed(title,"Check SSID / password");
}
static void ble_toggle(lv_event_t *e) { (void)e; pb_services_ble(!services.ble_enabled); }
static void clock_sync(lv_event_t *e) { (void)e; pb_services_sync_time(); }
static void timezone(lv_event_t *e) { int tz=preferences.timezone_minutes+(int)(intptr_t)lv_event_get_user_data(e); if(tz>=-720&&tz<=840) { preferences.timezone_minutes=(int16_t)tz; save_pet(); last_refresh=0; } }
static void clock_adjust(lv_event_t *e) { int n=(int)(intptr_t)lv_event_get_user_data(e); if(n==60||n==-60) manual_hour=(manual_hour+24+n/60)%24; else manual_minute=(manual_minute+60+n)%60; last_refresh=0; }
static void clock_set(lv_event_t *e) { (void)e; pb_services_set_time((uint64_t)(86400+manual_hour*3600+manual_minute*60-preferences.timezone_minutes*60),false); requested_page=CLOCK; }

static void make_pet(void) {
    pet_group=lv_obj_create(root); lv_obj_remove_style_all(pet_group); lv_obj_set_pos(pet_group,110,104); lv_obj_set_size(pet_group,246,182);
    lv_obj_remove_flag(pet_group,LV_OBJ_FLAG_SCROLLABLE); lv_obj_add_flag(pet_group,LV_OBJ_FLAG_CLICKABLE); lv_obj_add_event_cb(pet_group,care,LV_EVENT_CLICKED,(void *)1);
    uint32_t c=accent();
    shape(pet_group,39,4,43,72,22,c); shape(pet_group,164,4,43,72,22,c);
    shape(pet_group,49,16,23,43,12,0xf6d7cc); shape(pet_group,174,16,23,43,12,0xf6d7cc);
    shape(pet_group,51,146,47,24,14,c); shape(pet_group,148,146,47,24,14,c);
    shape(pet_group,8,82,40,51,22,c); shape(pet_group,198,82,40,51,22,c);
    pet_body=shape(pet_group,28,29,190,129,61,c);
    shape(pet_group,70,103,106,49,25,0xf6f1de);
    for(int i=0;i<2;i++) {
        pet_eyes[i]=shape(pet_group,76+i*70,65,24,33,12,0x23332b);
        pet_glints[i]=shape(pet_eyes[i],5,5,7,9,4,0xffffff);
        pet_blush[i]=shape(pet_group,53+i*116,99,25,13,7,0xf1a99f);
    }
    static const lv_point_precise_t smile[]={{0,0},{3,5},{9,7},{15,5},{18,0}};
    pet_mouth=lv_line_create(pet_group); lv_line_set_points(pet_mouth,smile,5); lv_obj_set_pos(pet_mouth,114,103); lv_obj_set_style_line_width(pet_mouth,3,0); lv_obj_set_style_line_color(pet_mouth,lv_color_hex(0x23332b),0); lv_obj_set_style_line_rounded(pet_mouth,true,0);
    pet_extra=label_at(pet_group,"",180,15,60,&lv_font_montserrat_20,0xf8d59a);
}
static void animate_pet(uint32_t now) {
    if(!pet_group) return;
    bool sleeping=s.pet.sleeping, reacting=(int32_t)(mood_until-now)>0;
    int phase=(int)(now%3200)/400, bob=motion?(phase<4?phase:7-phase):0;
    if(reacting&&!sleeping&&motion) bob=(int)(now%400)/100<2?-3:1;
    lv_obj_set_y(pet_group,104+bob);
    bool blink=motion&&(now%4600>4450);
    int height=sleeping||blink?5:preferences.eyes==1?17:33;
    int appearance=height*4+preferences.eyes, love=reacting&&mood==2;
    if(cached_eyes!=appearance) {
        for(int i=0;i<2;i++) {
            lv_obj_set_height(pet_eyes[i],height); lv_obj_set_y(pet_eyes[i],81-height/2);
            lv_obj_set_style_radius(pet_eyes[i],preferences.eyes==2?4:12,0);
            show(pet_glints[i],!sleeping&&!blink&&preferences.eyes!=1);
        }
        cached_eyes=appearance;
    }
    if(cached_love!=love) {
        for(int i=0;i<2;i++) lv_obj_set_style_bg_color(pet_blush[i],lv_color_hex(love?0xff8f9e:0xf1a99f),0);
        cached_love=love;
    }
    text_if_changed(pet_extra,sleeping?"z z":reacting?(mood==1?"*":"+"):"");
}
static void make_wifi_form(void) {
    head("Join Wi-Fi");
    ssid_input=lv_textarea_create(root); lv_obj_set_pos(ssid_input,86,87); lv_obj_set_size(ssid_input,294,45); lv_textarea_set_one_line(ssid_input,true); lv_textarea_set_max_length(ssid_input,32); lv_textarea_set_placeholder_text(ssid_input,"Network name"); lv_textarea_set_text(ssid_input,selected_ssid);
    password_input=lv_textarea_create(root); lv_obj_set_pos(password_input,86,139); lv_obj_set_size(password_input,294,45); lv_textarea_set_one_line(password_input,true); lv_textarea_set_max_length(password_input,64); lv_textarea_set_password_mode(password_input,true); lv_textarea_set_placeholder_text(password_input,"Password (blank if open)");
    remember_box=lv_checkbox_create(root); lv_obj_set_pos(remember_box,105,191); lv_checkbox_set_text(remember_box,"Remember on this device"); lv_obj_set_style_text_font(remember_box,&lv_font_montserrat_16,0);
    keyboard=lv_keyboard_create(root); lv_obj_set_pos(keyboard,63,218); lv_obj_set_size(keyboard,340,131); lv_obj_set_style_text_font(keyboard,&lv_font_montserrat_16,LV_PART_ITEMS); lv_keyboard_set_textarea(keyboard,selected_ssid[0]?password_input:ssid_input);
    lv_obj_add_event_cb(ssid_input,input_focus,LV_EVENT_FOCUSED,NULL); lv_obj_add_event_cb(password_input,input_focus,LV_EVENT_FOCUSED,NULL);
    primary(button("Connect",92,363,132,44,connect_wifi,0)); button("Cancel",242,363,132,44,nav,WIFI);
}
static void render(void) {
    live=sub=status_label=aux_label=title=pet_group=pet_body=pet_mouth=pet_extra=NULL;
    memset(cells,0,sizeof(cells)); memset(actions,0,sizeof(actions)); memset(bars,0,sizeof(bars));
    cached_eyes=cached_love=cached_theme=-1; cached_target=-2;
    lv_obj_clean(root); lv_obj_set_style_bg_color(root,lv_color_hex(0x0b1410),0); lv_obj_set_style_text_font(root,&lv_font_montserrat_20,0);
    char b[100];
    switch(page) {
    case HOME:
        status_label=small("",120,29,226); title=label(preferences.name,110,56,246); sub=small("",100,84,266); make_pet();
        live=small("",70,287,326);
        for(int i=0;i<3;i++) { bars[i]=lv_bar_create(root); lv_obj_set_pos(bars[i],90+i*100,314); lv_obj_set_size(bars[i],86,5); lv_obj_set_style_bg_color(bars[i],lv_color_hex(accent()),LV_PART_INDICATOR); }
        actions[0]=button("Feed",83,331,94,47,care,0); actions[1]=button("Pat",186,331,94,47,care,1); actions[2]=button("Nap",289,331,94,47,care,2);
        primary(button("Apps",166,391,134,40,nav,APPS)); break;
    case APPS: {
        head("A little of everything"); const char *names[]={"Focus","Games","Clock","Connect","Tools","Style"}; const int pages[]={FOCUS,GAMES,CLOCK,CONNECT,TOOLS,SETTINGS};
        for(int i=0;i<6;i++) { button(names[i],84+i%2*154,113+i/2*79,144,63,nav,pages[i]); }
        back(HOME); break;
    }
    case FOCUS:
        head("Focus together"); live=label_at(root,"",80,132,306,&lv_font_montserrat_40,accent()); sub=small("",70,192,326);
        actions[0]=button("Pause",103,243,124,54,timer_toggle,0); actions[1]=button("Cancel",239,243,124,54,timer_cancel,0);
        actions[2]=button("5 min",83,243,94,54,timer_set,300); snprintf(b,sizeof(b),"%u min",preferences.focus_minutes); actions[3]=button(b,186,243,94,54,timer_set,preferences.focus_minutes*60); actions[4]=button("50 min",289,243,94,54,timer_set,3000);
        button("1 min timer",143,312,180,45,timer_set,60); back(APPS); break;
    case GAMES:
        head("Little arcade"); button("Tic-tac-toe",108,117,250,59,nav,TTT); button("Memory garden",108,193,250,59,nav,MEMORY); button("Catch a spark",108,269,250,59,nav,CATCH); back(APPS); break;
    case TTT:
        head("Your turn"); for(int i=0;i<9;i++) cells[i]=button("",116+i%3*80,96+i/3*75,74,68,ttt_move,i); button("Again",173,327,120,42,ttt_reset,0); back(GAMES); break;
    case MEMORY:
        head("Memory garden"); for(int i=0;i<8;i++) cells[i]=button("?",85+i%4*76,128+i/4*83,68,74,memory_flip,i); button("New garden",143,314,180,45,memory_reset,0); back(GAMES); break;
    case CATCH:
        head("Catch a spark"); live=small("",90,84,286); for(int i=0;i<6;i++) cells[i]=button("",116+i%3*80,127+i/3*80,72,70,catch_hit,i); actions[0]=button("Start",153,307,160,47,catch_start,0); back(GAMES); break;
    case TOOLS:
        head("Everyday tools"); button("Stopwatch",85,125,144,65,nav,STOPWATCH); button("Breathe",239,125,144,65,nav,BREATHE); button("Dice",85,212,144,65,nav,DICE); button("Screen rest",239,212,144,65,nav,REST); back(APPS); break;
    case STOPWATCH:
        head("A moment, measured"); live=label_at(root,"",70,155,326,&lv_font_montserrat_40,accent()); actions[0]=button("Start",103,249,124,58,sw_toggle,0); button("Reset",239,249,124,58,sw_reset,0); back(TOOLS); break;
    case BREATHE:
        head("One quiet moment"); shape(root,153,121,160,160,80,0x243b2b); live=label("Breathe in",100,183,266); small("4 in / 4 hold / 6 out",90,303,286); back(TOOLS); break;
    case DICE:
        head("Let it roll"); live=label_at(root,"?",100,154,266,&lv_font_montserrat_40,accent()); button("Roll a die",133,258,200,60,dice_roll,0); back(TOOLS); break;
    case SETTINGS: {
        head("Make it yours"); const char *colors[]={"Moss","Sky","Peach","Lilac"}; for(int i=0;i<4;i++) cells[i]=button(colors[i],85+i%2*154,107+i/2*66,144,55,theme,i);
        actions[0]=button("Motion on",103,251,124,50,motion_toggle,0); button("Save pet",239,251,124,50,save_click,0); status_label=small("",67,321,332); back(APPS); break;
    }
    case CLOCK:
        head("Your little clock"); live=label_at(root,"--:--",70,130,326,&lv_font_montserrat_40,accent()); sub=label("",85,184,296); status_label=small("",70,220,326); aux_label=small("",90,254,286);
        button("Sync",103,300,124,48,clock_sync,0); button("Set time",239,300,124,48,nav,CLOCK_SET); back(APPS); break;
    case CLOCK_SET:
        head("Set clock"); live=label_at(root,"",95,102,276,&lv_font_montserrat_40,accent());
        button("Hour -",95,164,130,43,clock_adjust,-60); button("Hour +",241,164,130,43,clock_adjust,60); button("Min -",95,214,130,43,clock_adjust,-1); button("Min +",241,214,130,43,clock_adjust,1);
        sub=small("",135,272,196); button("-",83,270,44,36,timezone,-15); button("+",339,270,44,36,timezone,15); primary(button("Save time",143,322,180,47,clock_set,0)); back(CLOCK); break;
    case CONNECT:
        head("Stay connected"); button("Wi-Fi",113,127,240,61,nav,WIFI); button("Bluetooth",113,213,240,61,nav,BLUETOOTH); small("Clock + your pet, from Pet Studio",60,308,346); back(APPS); break;
    case WIFI: {
        head("Wi-Fi / 2.4 GHz"); status_label=small("",65,82,336);
        lv_obj_t *list=lv_obj_create(root); lv_obj_set_pos(list,91,122); lv_obj_set_size(list,284,188); lv_obj_set_style_pad_all(list,5,0); lv_obj_set_style_bg_color(list,lv_color_hex(0x142019),0); lv_obj_set_style_border_width(list,0,0);
        for(unsigned i=0;i<=services.ap_count;i++) { lv_obj_t *row=lv_button_create(list); lv_obj_set_pos(row,0,(int)i*47); lv_obj_set_size(row,272,42); lv_obj_set_style_bg_color(row,lv_color_hex(0x243029),0); lv_obj_set_style_shadow_width(row,0,0); lv_obj_t *l=lv_label_create(row); lv_obj_set_width(l,245); lv_label_set_long_mode(l,LV_LABEL_LONG_DOT); lv_label_set_text(l,i==services.ap_count?"Enter network manually":services.aps[i].ssid); lv_obj_set_style_text_font(l,&lv_font_montserrat_16,0); lv_obj_center(l); lv_obj_add_event_cb(row,choose_network,LV_EVENT_CLICKED,(void *)(intptr_t)(i==services.ap_count?-1:(int)i)); }
        button("Scan",78,326,96,44,wifi_scan,0); button("Off",185,326,96,44,wifi_off,0); button("Forget",292,326,96,44,wifi_forget,0); back(CONNECT); listed_count=services.ap_count; listed_scanning=services.scanning; break;
    }
    case WIFI_JOIN: make_wifi_form(); break;
    case BLUETOOTH:
        head("Companion link"); label("Pebble",100,113,266); status_label=small("",65,154,336); sub=small("",85,191,296);
        small("Open Pet Studio in Chrome or Edge.\nConnect, then send your design\nor sync your clock.",65,229,336);
        actions[0]=button("Enable 3 min",113,316,240,52,ble_toggle,0); back(CONNECT); break;
    case REST: lv_obj_set_style_bg_color(root,lv_color_black(),0); button("Wake",173,209,120,48,nav,HOME); break;
    }
    redraw=false; first_frame=true; last_refresh=0;
}
static void refresh(uint32_t now) {
    char b[100],time_text[9],date[24];
    switch(page) {
    case HOME:
        text_if_changed(title,preferences.name); snprintf(b,sizeof(b),"Level %u / %s",(unsigned)(1+s.pet.xp/100),s.pet.sleeping?"dreaming":"your little buddy"); text_if_changed(sub,b);
        if(services.clock_valid) { pb_clock_text(services.epoch,preferences.timezone_minutes,time_text,date); time_text[5]=0; snprintf(b,sizeof(b),"%s%s",time_text,services.wifi_connected?"   Wi-Fi":""); } else snprintf(b,sizeof(b),"A little life in your pocket"); text_if_changed(status_label,b);
        text_if_changed(live,s.pet.sleeping?"A little nap, a little dream.":(int32_t)(mood_until-now)>0?(mood==1?"Nom nom. Thank you!":"You make my day."):s.pet.food<25?"A snack would be lovely.":"Tap me. I like you already.");
        lv_bar_set_value(bars[0],s.pet.food,LV_ANIM_OFF); lv_bar_set_value(bars[1],s.pet.joy,LV_ANIM_OFF); lv_bar_set_value(bars[2],s.pet.energy,LV_ANIM_OFF); button_text(actions[2],s.pet.sleeping?"Wake":"Nap"); break;
    case FOCUS: {
        unsigned sec=s.timer.duration?(s.timer.remaining+999)/1000:preferences.focus_minutes*60;
        snprintf(b,sizeof(b),"%02u:%02u",sec/60,sec%60); text_if_changed(live,b); text_if_changed(sub,s.timer.complete?"A little win. Well done!":s.timer.running?"One thing at a time.":"Ready when you are.");
        show(actions[0],s.timer.remaining>0); show(actions[1],s.timer.remaining>0); for(int i=2;i<5;i++) show(actions[i],s.timer.remaining==0); button_text(actions[0],s.timer.running?"Pause":"Resume"); break;
    }
    case TTT:
        text_if_changed(title,s.ttt.result?(s.ttt.result==2?"A friendly draw":s.ttt.result==1?"You win!":"Pebble wins!"):"Your turn / X"); for(int i=0;i<9;i++) button_text(cells[i],s.ttt.cells[i]==1?"X":s.ttt.cells[i]==-1?"O":"."); break;
    case MEMORY:
        snprintf(b,sizeof(b),s.memory.won?"Your garden is blooming!":"Memory / %u of 4 pairs",s.memory.pairs); text_if_changed(title,b);
        for(int i=0;i<8;i++) { bool visible=(s.memory.matched&(1<<i))||s.memory.first==i||s.memory.second==i; char letter[2]={(char)('A'+s.memory.cards[i]),0}; button_text(cells[i],visible?letter:"?"); } break;
    case CATCH:
        snprintf(b,sizeof(b),"%u s / %u sparks",(unsigned)(s.catch_game.left_ms+999)/1000,s.catch_game.hits); text_if_changed(live,b); if(cached_target!=(s.catch_game.running?s.catch_game.target:-1)) { for(int i=0;i<6;i++) { bool active=s.catch_game.running&&s.catch_game.target==i; button_text(cells[i],active?"*":"."); lv_obj_set_style_bg_color(cells[i],lv_color_hex(active?accent():0x243029),0); lv_obj_set_style_text_color(cells[i],lv_color_hex(active?0x15221b:0xf3f3ed),0); } cached_target=s.catch_game.running?s.catch_game.target:-1; } show(actions[0],!s.catch_game.running); break;
    case STOPWATCH: { unsigned sec=stopwatch_ms/1000; snprintf(b,sizeof(b),"%02u:%02u.%u",sec/60,sec%60,(unsigned)(stopwatch_ms/100)%10); text_if_changed(live,b); button_text(actions[0],stopwatch_running?"Pause":"Start"); break; }
    case BREATHE: text_if_changed(live,breath_ms%14000<4000?"Breathe in":breath_ms%14000<8000?"Hold gently":"Breathe out"); break;
    case SETTINGS:
        if(cached_theme!=s.pet.theme) { for(int i=0;i<4;i++) { lv_obj_set_style_bg_color(cells[i],lv_color_hex(s.pet.theme==i?accent():0x243029),0); lv_obj_set_style_text_color(cells[i],lv_color_hex(s.pet.theme==i?0x15221b:0xf3f3ed),0); } cached_theme=s.pet.theme; } button_text(actions[0],motion?"Motion on":"Motion off"); text_if_changed(status_label,services.save_ok?"Pet saved locally":"Save unavailable / session only"); break;
    case CLOCK:
        if(services.clock_valid) { pb_clock_text(services.epoch,preferences.timezone_minutes,time_text,date); text_if_changed(live,time_text); text_if_changed(sub,services.clock_dated?date:"Manual time"); } else { text_if_changed(live,"--:--"); text_if_changed(sub,"Let\'s set the time"); }
        text_if_changed(status_label,services.time_status); snprintf(b,sizeof(b),"UTC%c%02d:%02d",preferences.timezone_minutes<0?'-':'+',abs(preferences.timezone_minutes)/60,abs(preferences.timezone_minutes)%60); text_if_changed(aux_label,b); break;
    case CLOCK_SET:
        snprintf(b,sizeof(b),"%02d:%02d",manual_hour,manual_minute); text_if_changed(live,b); snprintf(b,sizeof(b),"UTC%c%02d:%02d",preferences.timezone_minutes<0?'-':'+',abs(preferences.timezone_minutes)/60,abs(preferences.timezone_minutes)%60); text_if_changed(sub,b); break;
    case WIFI: text_if_changed(status_label,services.wifi_connected?services.ip:services.wifi_status); break;
    case BLUETOOTH:
        text_if_changed(status_label,services.ble_status); snprintf(b,sizeof(b),services.ble_enabled?"Link window: %lu seconds":"Turn on when you need it.",(unsigned long)services.ble_seconds); text_if_changed(sub,b); button_text(actions[0],services.ble_enabled?"Turn off":"Enable 3 min"); break;
    default: break;
    }
}
static void command_apply(const pb_command *command) {
    switch(command->kind) {
    case 'P': pb_care(&s,1); mood=2; mood_until=lv_tick_get()+2400; break;
    case 'N': snprintf(preferences.name,sizeof(preferences.name),"%s",command->name); break;
    case 'C': s.pet.theme=(uint8_t)command->value; if(page==HOME) redraw=true; break;
    case 'E': preferences.eyes=(uint8_t)command->value; break;
    case 'F': preferences.focus_minutes=(uint8_t)command->value; if(page==FOCUS) redraw=true; break;
    case 'Z': preferences.timezone_minutes=(int16_t)command->value; break;
    case 'T': pb_services_set_time((uint64_t)command->value,true); break;
    default: return;
    }
    last_refresh=0; save_pet(); pb_services_reply(command);
}
static void tick(lv_timer_t *timer) {
    (void)timer; uint32_t now=lv_tick_get(), dt=now-last_tick; last_tick=now;
    pb_tick(&s,dt); breath_ms+=dt; if(stopwatch_running) stopwatch_ms+=dt;
    pb_services_snapshot(&services); pb_command command; if(pb_services_next_command(&command)) command_apply(&command);
    if(requested_page!=page) { page=requested_page; redraw=true; if(page==BREATHE) breath_ms=0; if(page==CLOCK_SET&&services.clock_valid) { char t[9],d[24]; pb_clock_text(services.epoch,preferences.timezone_minutes,t,d); manual_hour=(t[0]-'0')*10+t[1]-'0'; manual_minute=(t[3]-'0')*10+t[4]-'0'; } }
    if(page==WIFI&&(listed_count!=services.ap_count||listed_scanning!=services.scanning)) redraw=true;
    if(redraw) render();
    if(first_frame || !last_refresh || (uint32_t)(now-last_refresh)>=100) { refresh(now); last_refresh=now; first_frame=false; }
    if(page==HOME&&(uint32_t)(now-last_face)>=80) { animate_pet(now); last_face=now; }
    if((uint32_t)(now-last_save)>=30000) { last_save=now; save_pet(); }
}
void pebble_ui_start(void) {
    pb_init(&s,lv_tick_get()+37); s.pet.theme=PB_DEFAULT_THEME; pb_services_init(&s,&preferences); pb_services_snapshot(&services);
    root=lv_screen_active(); lv_obj_remove_flag(root,LV_OBJ_FLAG_SCROLLABLE); lv_obj_set_style_pad_all(root,0,0);
    last_tick=last_save=lv_tick_get();
    for(lv_indev_t *input=lv_indev_get_next(NULL);input;input=lv_indev_get_next(input)) { lv_timer_t *read=lv_indev_get_read_timer(input); if(read) lv_timer_set_period(read,16); }
    lv_timer_t *refresh_timer=lv_display_get_refr_timer(lv_display_get_default()); if(refresh_timer) lv_timer_set_period(refresh_timer,33);
    render(); lv_timer_create(tick,20,NULL);
}
