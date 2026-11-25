#include "pebble_usb.h"
#include "tal_api.h"
#include "tkl_uart.h"
#include <stdio.h>
#include <string.h>

/* CH342F channel A shares the bootloader UART after boot. Leave the log UART
 * and the bridge's DTR/RTS reset signals alone. */
#define USB_UART TUYA_UART_NUM_0
static THREAD_HANDLE worker;
static MUTEX_HANDLE lock;
static enum { SLOT_EMPTY, SLOT_QUEUED, SLOT_APPLYING, SLOT_APPLIED } slot;
static pb_command pending;
static uint32_t queued_at;


static void write_line(const char *line) {
    tkl_uart_write(USB_UART, (void *)line, (uint16_t)strlen(line));
}

static void handle_line(const char *line, bool *identified, uint32_t *last_hello, uint32_t now) {
    if (!strcmp(line, "PEBBLE HELLO 1")) {
        *identified = true;
        *last_hello = now;
        write_line("PEBBLE READY 1\n");
        return;
    }
    static const char prefix[] = "PEBBLE SET ";
    if (strncmp(line, prefix, sizeof(prefix) - 1)) return;
    const char *text = line + sizeof(prefix) - 1;
    pb_command cmd;
    if (!*identified || now - *last_hello >= 30000 ||
        !pb_command_parse((const uint8_t *)text, strlen(text), &cmd) ||
        !strchr("NCEF", cmd.kind)) {
        write_line("PEBBLE ERR\n");
        return;
    }
    tal_mutex_lock(lock);
    bool accepted = slot == SLOT_EMPTY;
    if (accepted) {
        pending = cmd;
        queued_at = now;
        slot = SLOT_QUEUED;
    }
    tal_mutex_unlock(lock);
    if (!accepted) {
        write_line("PEBBLE ERR\n");
        return;
    }
    /* Complete this exchange before blocking on the next incoming byte. */
    for (;;) {
        tal_system_sleep(10);
        tal_mutex_lock(lock);
        bool applied = slot == SLOT_APPLIED;
        bool expired = slot == SLOT_QUEUED &&
                       (uint32_t)tal_system_get_millisecond() - queued_at >= 2500;
        if (applied || expired) slot = SLOT_EMPTY;
        tal_mutex_unlock(lock);
        if (applied) {
            char reply[PB_LINK_MAX + 16];
            snprintf(reply, sizeof(reply), "PEBBLE OK %s\n", text);
            write_line(reply);
            break;
        }
        if (expired) { write_line("PEBBLE ERR\n"); break; }
    }
}

static void run_usb(void *arg) {
    (void)arg;
    pb_usb_line line = {0};
    bool identified = false;
    uint32_t last_hello = 0;
    for (;;) {
        uint8_t byte;
        if (tkl_uart_read(USB_UART, &byte, 1) == 1) {
            if (pb_usb_line_feed(&line, byte))
                handle_line(line.text, &identified, &last_hello, (uint32_t)tal_system_get_millisecond());
        } else tal_system_sleep(20);
    }
}

void pb_usb_start(void) {
    TUYA_UART_BASE_CFG_T cfg = {0};
    cfg.baudrate = 115200;
    cfg.databits = TUYA_UART_DATA_LEN_8BIT;
    cfg.stopbits = TUYA_UART_STOP_LEN_1BIT;
    cfg.parity = TUYA_UART_PARITY_TYPE_NONE;
    if (tkl_uart_init(USB_UART, &cfg) != OPRT_OK) { PR_ERR("Pebble USB port unavailable"); return; }
    if (tal_mutex_create_init(&lock) != OPRT_OK) {
        tkl_uart_deinit(USB_UART); lock = NULL; return;
    }
    THREAD_CFG_T task = {.stackDepth = 3072, .priority = THREAD_PRIO_1, .thrdname = "pebble_usb"};
    if (tal_thread_create_and_start(&worker, NULL, NULL, run_usb, NULL, &task) != OPRT_OK) {
        tal_mutex_release(lock); lock = NULL; tkl_uart_deinit(USB_UART);
        PR_ERR("Pebble USB task unavailable");
    }
}

void pb_usb_apply_pending(void (*apply)(const pb_command *)) {
    if (!lock) return;
    pb_command cmd;
    tal_mutex_lock(lock);
    bool ready = slot == SLOT_QUEUED && (uint32_t)tal_system_get_millisecond() - queued_at < 2500;
    if (ready) { cmd = pending; slot = SLOT_APPLYING; }
    tal_mutex_unlock(lock);
    if (ready) {
        apply(&cmd);
        tal_mutex_lock(lock);
        slot = SLOT_APPLIED;
        tal_mutex_unlock(lock);
    }
}
