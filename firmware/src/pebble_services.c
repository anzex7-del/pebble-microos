/* Radio and flash work never runs in the LVGL input/render thread. */
#include "pebble_services.h"
#include "pebble_profile.h"
#include "tal_api.h"
#include "tal_wifi.h"
#include "tal_bluetooth.h"
#include "tkl_network.h"
#include <stdio.h>
#include <string.h>

static MUTEX_HANDLE lock;
static THREAD_HANDLE worker;
static pb_services_status status;
static bool storage_ok, scan_requested, join_requested, off_requested, forget_requested, sync_requested;
static bool ble_wanted, ble_initialized, ble_ready, ble_advertising, save_pending, remember_pending;
static uint32_t ble_until, clock_tick, poll_tick, sync_tick, connect_tick;
static uint64_t clock_ms;
static unsigned queue_head, queue_count;
static pb_command queue[8];
static TAL_BLE_PEER_INFO_T peer;
static char pending_reply[21];
static uint32_t save_tick;
static uint8_t saved_pet[PB_SAVE_SIZE], previous_pet[PB_SAVE_SIZE];
static pb_preferences saved_preferences, previous_preferences;
typedef struct { char ssid[33], password[65]; } wifi_credentials;
static wifi_credentials credentials, pending_credentials;

static uint32_t millis(void) { return (uint32_t)tal_system_get_millisecond(); }
static void take(void) { tal_mutex_lock(lock); }
static void give(void) { tal_mutex_unlock(lock); }
static void copy_text(char *dst, size_t size, const char *src) { snprintf(dst,size,"%s",src); }
static void wifi_message(const char *text) { take(); copy_text(status.wifi_status,sizeof(status.wifi_status),text); status.generation++; give(); }
static void ble_message(const char *text) { take(); copy_text(status.ble_status,sizeof(status.ble_status),text); status.generation++; give(); }

static void wifi_event(WF_EVENT_E event, void *arg) {
    (void)arg;
    take();
    if(event==WFE_CONNECT_FAILED) { copy_text(status.wifi_status,sizeof(status.wifi_status),"Connection failed. Check password."); status.wifi_connected=false; }
    if(event==WFE_DISCONNECTED) { status.wifi_connected=false; status.ip[0]=0; if(status.wifi_on) copy_text(status.wifi_status,sizeof(status.wifi_status),"Disconnected. Tap Connect to retry."); }
    status.generation++; give();
}
static void ble_event(TAL_BLE_EVT_PARAMS_T *event) {
    if(!lock || !event) return;
    take();
    switch(event->type) {
    case TAL_BLE_STACK_INIT:
        ble_ready=(event->ble_event.init==0);
        if(!ble_ready) { ble_wanted=false; status.ble_enabled=false; copy_text(status.ble_status,sizeof(status.ble_status),"Bluetooth initialization failed"); }
        break;
    case TAL_BLE_EVT_PERIPHERAL_CONNECT:
        if(event->ble_event.connect.result==0) { peer=event->ble_event.connect.peer; status.ble_connected=true; ble_advertising=false; copy_text(pending_reply,sizeof(pending_reply),"OK:V"); copy_text(status.ble_status,sizeof(status.ble_status),"Connected to companion"); }
        break;
    case TAL_BLE_EVT_DISCONNECT:
        status.ble_connected=false; ble_advertising=false; queue_count=0; pending_reply[0]=0;
        copy_text(status.ble_status,sizeof(status.ble_status),ble_wanted?"Ready to reconnect":"Bluetooth off");
        break;
    case TAL_BLE_EVT_WRITE_REQ: {
        pb_command command;
        if(ble_wanted && status.ble_connected && (int32_t)(ble_until-millis())>0 &&
           pb_command_parse(event->ble_event.write_report.report.p_data,event->ble_event.write_report.report.len,&command)) {
            if(queue_count<8) { queue[(queue_head+queue_count)%8]=command; queue_count++; }
            else copy_text(pending_reply,sizeof(pending_reply),"ERR!");
        } else copy_text(pending_reply,sizeof(pending_reply),"ERR!");
        break;
    }
    default: break;
    }
    status.generation++; give();
}

void pb_services_snapshot(pb_services_status *out) {
    if(!lock) { memset(out,0,sizeof(*out)); copy_text(out->wifi_status,sizeof(out->wifi_status),"Services unavailable"); return; }
    take(); *out=status;
    if(status.clock_valid) out->epoch=(clock_ms+(uint32_t)(millis()-clock_tick))/1000;
    int32_t left=(int32_t)(ble_until-millis()); out->ble_seconds=ble_wanted&&left>0?(uint32_t)(left+999)/1000:0;
    give();
}
void pb_services_save(const pb_state *state, const pb_preferences *preferences) {
    if(!lock||!storage_ok) return;
    uint8_t data[PB_SAVE_SIZE]; pb_save(state,data);
    take(); memcpy(saved_pet,data,sizeof(data)); saved_preferences=*preferences; save_pending=true; save_tick=millis(); give();
}
void pb_services_scan(void) { if(lock) { take(); scan_requested=true; status.wifi_on=true; status.scanning=true; give(); } }
bool pb_services_connect(const char *ssid, const char *password, bool remember) {
    if(!lock||!pb_wifi_credentials_valid(ssid,password)) return false;
    take(); copy_text(pending_credentials.ssid,sizeof(pending_credentials.ssid),ssid); copy_text(pending_credentials.password,sizeof(pending_credentials.password),password);
    remember_pending=remember; join_requested=true; status.wifi_on=true; copy_text(status.wifi_status,sizeof(status.wifi_status),"Connecting..."); status.generation++; give(); return true;
}
void pb_services_wifi_off(void) { if(lock) { take(); off_requested=true; join_requested=scan_requested=false; status.wifi_on=false; give(); } }
void pb_services_forget_wifi(void) { if(lock) { take(); forget_requested=true; off_requested=true; join_requested=scan_requested=false; status.wifi_on=false; give(); } }
void pb_services_ble(bool enabled) { if(lock) { take(); ble_wanted=enabled; status.ble_enabled=enabled; ble_until=millis()+180000; if(!enabled) queue_count=0; status.generation++; give(); } }
bool pb_services_next_command(pb_command *out) {
    if(!lock) return false;
    take();
    if(!queue_count || !ble_wanted || (int32_t)(ble_until-millis())<=0) { queue_count=0; give(); return false; }
    *out=queue[queue_head]; queue_head=(queue_head+1)%8; queue_count--; give(); return true;
}
void pb_services_reply(const pb_command *command) {
    if(!lock||!command) return;
    take();
    if(command->kind=='N') snprintf(pending_reply,sizeof(pending_reply),"OK:N%s",command->name);
    else if(command->kind=='P') copy_text(pending_reply,sizeof(pending_reply),"OK:P");
    else snprintf(pending_reply,sizeof(pending_reply),"OK:%c%lld",command->kind,(long long)command->value);
    give();
}
void pb_services_sync_time(void) { if(lock) { take(); sync_requested=true; give(); } }
void pb_services_set_time(uint64_t epoch, bool dated) {
    if(!lock) return;
    take(); clock_ms=epoch*1000; clock_tick=millis(); status.clock_valid=true; status.clock_dated=dated;
    copy_text(status.time_status,sizeof(status.time_status),dated?"Time synchronized":"Time set manually"); give();
}

static void scan_networks(void) {
    wifi_message("Looking for 2.4 GHz networks...");
    if(tal_wifi_set_work_mode(WWM_STATION)!=OPRT_OK) { take(); status.scanning=false; give(); wifi_message("Wi-Fi could not start"); return; }
    AP_IF_S *found=NULL; uint32_t count=0;
    OPERATE_RET result=tal_wifi_all_ap_scan(&found,&count);
    pb_ap list[PB_AP_MAX]={0}; unsigned used=0;
    if(result==OPRT_OK&&found) for(uint32_t i=0;i<count;i++) {
        unsigned n=found[i].s_len; if(n==0||n>32) continue;
        char name[33]={0}; memcpy(name,found[i].ssid,n);
        bool duplicate=false; for(unsigned j=0;j<used;j++) if(!strcmp(list[j].ssid,name)) { duplicate=true; if(found[i].rssi>list[j].rssi) list[j].rssi=found[i].rssi; break; }
        if(duplicate) continue;
        unsigned pos=used; if(used==PB_AP_MAX) { pos=0; for(unsigned j=1;j<used;j++) if(list[j].rssi<list[pos].rssi) pos=j; if(found[i].rssi<=list[pos].rssi) continue; } else used++;
        copy_text(list[pos].ssid,sizeof(list[pos].ssid),name); list[pos].rssi=found[i].rssi;
    }
    if(found) tal_wifi_release_ap(found);
    for(unsigned i=0;i<used;i++) for(unsigned j=i+1;j<used;j++) if(list[j].rssi>list[i].rssi) { pb_ap ap=list[i]; list[i]=list[j]; list[j]=ap; }
    take(); status.ap_count=used; memcpy(status.aps,list,sizeof(list)); status.scanning=false; status.generation++; give();
    wifi_message(result==OPRT_OK?(used?"Choose your network":"No networks found. Try again."):"Scan failed. Try again.");
}
static void sync_network_time(void) {
    take(); bool connected=status.wifi_connected; copy_text(status.time_status,sizeof(status.time_status),connected?"Syncing internet time...":"Connect Wi-Fi to sync"); give();
    if(!connected) return;
    /* Standard unauthenticated SNTP for display time only, never security decisions. */
    TUYA_IP_ADDR_T address=0, source=0; uint16_t port=0; uint8_t request[48]={0}, reply[96]={0};
    request[0]=0x23; for(unsigned i=40;i<48;i++) request[i]=(uint8_t)tal_system_get_random(256);
    bool ok=false; uint64_t epoch=0;
    if(tkl_net_gethostbyname("time.cloudflare.com",&address)==OPRT_OK) {
        int fd=tkl_net_socket_create(PROTOCOL_UDP);
        if(fd>=0) {
            bool bounded=tkl_net_set_timeout(fd,3000,TRANS_RECV)==OPRT_OK && tkl_net_set_timeout(fd,3000,TRANS_SEND)==OPRT_OK;
            if(bounded && tkl_net_send_to(fd,request,sizeof(request),address,123)==(int)sizeof(request)) {
                int n=tkl_net_recvfrom(fd,reply,sizeof(reply),&source,&port);
                if(n>=48&&source==address&&port==123) ok=pb_ntp_parse(reply,(size_t)n,request+40,&epoch);
            }
            tkl_net_close(fd);
        }
    }
    if(ok) pb_services_set_time(epoch,true);
    take(); copy_text(status.time_status,sizeof(status.time_status),ok?"Internet time synchronized":"Time sync failed. Retry or use phone."); give();
    sync_tick=millis();
}
static void radios_tick(void) {
    bool desired, connected, ready, advertising; char ack[21]; TAL_BLE_PEER_INFO_T remote;
    take();
    if(ble_wanted && (int32_t)(ble_until-millis())<=0) { ble_wanted=false; status.ble_enabled=false; queue_count=0; }
    desired=ble_wanted; connected=status.ble_connected; ready=ble_ready; advertising=ble_advertising; remote=peer; memcpy(ack,pending_reply,sizeof(ack)); pending_reply[0]=0;
    give();
    if(desired&&!ble_initialized) {
        ble_initialized=true; ble_message("Starting Bluetooth...");
        if(tal_ble_bt_init(TAL_BLE_ROLE_PERIPERAL,ble_event)!=OPRT_OK) { take(); ble_wanted=status.ble_enabled=false; give(); ble_initialized=false; ble_message("Bluetooth could not start"); }
        return;
    }
    if(!desired) {
        if(advertising) tal_ble_advertising_stop();
        if(connected) tal_ble_disconnect(remote);
        take(); ble_advertising=false; copy_text(status.ble_status,sizeof(status.ble_status),"Bluetooth off"); give();
        return;
    }
    if(ready&&!connected&&!advertising) {
        static uint8_t adv[]={2,1,6,3,3,0x50,0xFD};
        static uint8_t rsp[]={7,9,'P','e','b','b','l','e'};
        TAL_BLE_DATA_T a={sizeof(adv),adv}, b={sizeof(rsp),rsp};
        OPERATE_RET result=tal_ble_advertising_data_set(&a,&b);
        if(result==OPRT_OK) result=tal_ble_advertising_start(TUYAOS_BLE_DEFAULT_ADV_PARAM);
        take(); ble_advertising=(result==OPRT_OK); give();
        ble_message(result==OPRT_OK?"Discoverable as Pebble":"Bluetooth advertising failed");
    }
    if(connected&&ack[0]) {
        TAL_BLE_DATA_T data={(uint16_t)strlen(ack),(uint8_t *)ack}; tal_ble_server_common_read_update(&data);
    }
}
static void run_services(void *arg) {
    (void)arg;
    bool wifi_ready=tal_wifi_init(wifi_event)==OPRT_OK;
    if(!wifi_ready) wifi_message("Wi-Fi initialization failed");
    for(;;) {
        uint32_t now;
        bool scan,join,off,forget,sync,save,remember; wifi_credentials next; pb_preferences preferences; uint8_t pet[PB_SAVE_SIZE];
        take();
        now=millis(); clock_ms+=(uint32_t)(now-clock_tick); clock_tick=now;
        scan=scan_requested; scan_requested=false; join=join_requested; join_requested=false;
        off=off_requested; off_requested=false; forget=forget_requested; forget_requested=false;
        sync=sync_requested; sync_requested=false; save=save_pending && (uint32_t)(now-save_tick)>=1000; if(save) save_pending=false;
        next=pending_credentials; if(join) memset(pending_credentials.password,0,sizeof(pending_credentials.password)); remember=remember_pending; preferences=saved_preferences; memcpy(pet,saved_pet,sizeof(pet));
        give();
        if(save&&storage_ok) {
            bool ok=true;
            if(memcmp(pet,previous_pet,sizeof(pet))) { ok=(tal_kv_set("pebble.v1",pet,sizeof(pet))==OPRT_OK); if(ok) memcpy(previous_pet,pet,sizeof(pet)); }
            if(memcmp(&preferences,&previous_preferences,sizeof(preferences))) { bool pref_ok=(tal_kv_set("pebble.prefs.v1",(uint8_t *)&preferences,sizeof(preferences))==OPRT_OK); if(pref_ok) previous_preferences=preferences; ok=ok&&pref_ok; }
            take(); status.save_ok=ok; give();
        }
        if(off&&wifi_ready) { tal_wifi_station_disconnect(); tal_wifi_set_work_mode(WWM_POWERDOWN); memset(&credentials,0,sizeof(credentials)); take(); status.wifi_connected=false; status.scanning=false; status.ip[0]=0; give(); wifi_message("Wi-Fi off"); }
        if(forget&&storage_ok) { tal_kv_del("pebble.wifi.v1"); memset(&next,0,sizeof(next)); take(); memset(&pending_credentials,0,sizeof(pending_credentials)); give(); wifi_message("Saved network removed"); }
        if(!wifi_ready&&(scan||join)) { take(); status.scanning=false; give(); wifi_message("Wi-Fi unavailable. Restart to retry."); }
        if(scan&&wifi_ready&&!off) scan_networks();
        if(join&&wifi_ready&&!off) {
            credentials=next; tal_wifi_station_disconnect();
            OPERATE_RET result=tal_wifi_set_work_mode(WWM_STATION);
            if(result==OPRT_OK) result=tal_wifi_station_connect((int8_t *)credentials.ssid,(int8_t *)credentials.password);
            connect_tick=millis(); wifi_message(result==OPRT_OK?"Connecting...":"Connection request failed");
            if(storage_ok) { if(remember) tal_kv_set("pebble.wifi.v1",(uint8_t *)&credentials,sizeof(credentials)); else tal_kv_del("pebble.wifi.v1"); }
            memset(next.password,0,sizeof(next.password));
        }
        if(wifi_ready && (uint32_t)(now-poll_tick)>=1000) {
            poll_tick=now; WF_STATION_STAT_E station=WSS_IDLE; tal_wifi_station_get_status(&station);
            take(); bool was=status.wifi_connected, enabled=status.wifi_on; give();
            bool online=enabled&&station==WSS_GOT_IP;
            if(online) {
                NW_IP_S ip={0}; tal_wifi_get_ip(WF_STATION,&ip);
                take(); status.wifi_connected=true; copy_text(status.ip,sizeof(status.ip),ip.ip); copy_text(status.wifi_status,sizeof(status.wifi_status),"Connected"); give();
                if(!was || (uint32_t)(now-sync_tick)>=21600000) sync=true;
            } else {
                take(); status.wifi_connected=false; give();
                if(enabled&&station==WSS_PASSWD_WRONG) wifi_message("Incorrect password. Try again.");
                else if(enabled&&station==WSS_NO_AP_FOUND) wifi_message("Network not found");
                else if(enabled&&station==WSS_CONNECTING&&(uint32_t)(now-connect_tick)>30000) { tal_wifi_station_disconnect(); wifi_message("Connection timed out. Try again."); }
            }
        }
        radios_tick();
        if(sync) sync_network_time();
        tal_system_sleep(50);
    }
}
void pb_services_init(pb_state *state, pb_preferences *preferences) {
    memset(&status,0,sizeof(status)); memset(preferences,0,sizeof(*preferences));
    preferences->version=1; copy_text(preferences->name,sizeof(preferences->name),PB_PET_NAME);
    preferences->eyes=PB_EYE_STYLE; preferences->focus_minutes=PB_FOCUS_MINUTES; preferences->timezone_minutes=240;
    if(tal_mutex_create_init(&lock)!=OPRT_OK) { lock=NULL; return; }
    tal_kv_cfg_t cfg={.seed="pebblelocal2026",.key="pebblekvlocalv1"};
    storage_ok=(tal_kv_init(&cfg)==OPRT_OK);
    if(storage_ok) {
        uint8_t *data=NULL; size_t size=0;
        if(tal_kv_get("pebble.v1",&data,&size)==OPRT_OK) { pb_load(state,data,size); tal_kv_free(data); }
        if(tal_kv_get("pebble.prefs.v1",&data,&size)==OPRT_OK) {
            if(size==sizeof(*preferences)) {
                pb_preferences p; memcpy(&p,data,size); p.name[16]=0; uint8_t command[18]={'N'}; memcpy(command+1,p.name,strlen(p.name)); pb_command parsed;
                if(p.version==1&&p.eyes<=2&&p.focus_minutes>=5&&p.focus_minutes<=60&&p.timezone_minutes>=-720&&p.timezone_minutes<=840&&p.timezone_minutes%15==0&&pb_command_parse(command,strlen(p.name)+1,&parsed)) *preferences=p;
            }
            tal_kv_free(data);
        }
        if(tal_kv_get("pebble.wifi.v1",&data,&size)==OPRT_OK) {
            if(size==sizeof(credentials)) { memcpy(&credentials,data,size); credentials.ssid[32]=0; credentials.password[64]=0;
                if(pb_wifi_credentials_valid(credentials.ssid,credentials.password)) { pending_credentials=credentials; join_requested=true; remember_pending=true; status.wifi_on=true; }
            }
            tal_kv_free(data);
        }
    }
    pb_save(state,previous_pet); previous_preferences=*preferences; saved_preferences=*preferences; memcpy(saved_pet,previous_pet,sizeof(saved_pet));
    status.save_ok=storage_ok; copy_text(status.wifi_status,sizeof(status.wifi_status),"Wi-Fi off"); copy_text(status.ble_status,sizeof(status.ble_status),"Bluetooth off"); copy_text(status.time_status,sizeof(status.time_status),"Set time or connect Wi-Fi");
    clock_tick=millis(); tal_sw_timer_init(); tal_workq_init();
    THREAD_CFG_T config={0}; config.stackDepth=8192; config.priority=THREAD_PRIO_1; config.thrdname="pebble_io";
    status.ready=(tal_thread_create_and_start(&worker,NULL,NULL,run_services,NULL,&config)==OPRT_OK);
    if(!status.ready) wifi_message("Background services failed");
}
