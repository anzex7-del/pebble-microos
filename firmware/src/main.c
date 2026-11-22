#include "tuya_cloud_types.h"
#include "tal_api.h"
#include "tkl_output.h"
#include "lv_vendor.h"
#include "board_com_api.h"
#include "pebble_profile.h"
void pebble_ui_start(void);
static THREAD_HANDLE app_thread;
static void run(void *arg) {
    (void)arg;
    tal_log_init(TAL_LOG_LEVEL_DEBUG,4096,(TAL_LOG_OUTPUT_CB)tkl_log_output);
    PR_NOTICE("Pebble microOS %s / T5AI preview",PB_FIRMWARE_VERSION);
    if(board_register_hardware()!=OPRT_OK) { PR_ERR("Board initialization failed"); return; }
    lv_vendor_init(DISPLAY_NAME);
    lv_vendor_disp_lock(); pebble_ui_start(); lv_vendor_disp_unlock();
    lv_vendor_start(5,1024*12);
    tal_thread_delete(app_thread); app_thread=NULL;
}
void tuya_app_main(void) {
    THREAD_CFG_T cfg={0}; cfg.stackDepth=1024*6; cfg.priority=THREAD_PRIO_1; cfg.thrdname="pebble";
    if(tal_thread_create_and_start(&app_thread,NULL,NULL,run,NULL,&cfg)!=OPRT_OK) PR_ERR("Pebble: task creation failed");
}
