# Wi-Fi, Bluetooth and clock — preview 0.3

For **Waveshare T5-E1-Touch-AMOLED-1.75**. These are implemented preview features; their radio and display behavior still needs a physical-board acceptance pass. The owner confirmed that the 0.2 baseline boots and accepts touch, with noticeable lag.

## Set the clock

The home screen shows hours/minutes; **Apps → Clock** shows seconds and date. The initial timezone is **UTC+04:00 (Tbilisi)**. Time is set in one of three ways:

- Connect Wi-Fi to fetch internet time automatically. **Clock → Sync** retries it.
- In Pet Studio, connect Bluetooth and choose **Sync clock**. This sends your computer's time and timezone.
- **Clock → Set time** changes hours/minutes and UTC offset in 15-minute steps, then **Save time**. Manual setting shows “Manual time” instead of inventing a calendar date.

The clock runs while the board is powered. The external PCF85063 RTC is not integrated yet, so a cold boot requires Wi-Fi, a companion sync or manual time. Timezone is saved. Daylight-saving changes are not automatic; sync from your computer or adjust the offset. Focus and stopwatch timers use uptime and are unaffected by clock correction.

## Join Wi-Fi

1. Open **Apps → Connect → Wi-Fi → Scan**.
2. Tap a network, or choose **Enter network manually** for a hidden SSID.
3. Enter its password on the device. Leave it blank for an open network.
4. Optionally tick **Remember on this device**, then **Connect**. Remembering is off by default.
5. A successful connection shows its IP address. Internet time then syncs automatically.

Uses the board's 2.4 GHz station interface. Captive-portal login and enterprise/802.1X setup are not included. **Off** disconnects; a remembered network remains for the next boot. **Forget** disconnects and removes the saved network. An unchecked Remember box also removes any previously saved network when connecting.

A remembered credential is stored in device KV using the application key. This is convenience storage, not protection against an attacker with firmware/physical access. Passwords are not sent to Pet Studio, a repository, BLE or a cloud account. Internet time sends a standard unauthenticated NTP request to `time.cloudflare.com` on UDP port 123. Network/DNS/NTP failures leave manual and companion time available.

## Send a design over USB

Plug a data cable into a running Pebble, close the flashing tool and serial monitor, and open Pet Studio in desktop Chrome or Edge. Choose **Connect via USB**, select the WCH/CH342 download channel (port A), then **Send this design**. The Studio confirms each design setting after the firmware applies it. If the board restarts when the port opens, wait for Home and reconnect. The second serial channel carries diagnostic logs. Bluetooth remains available for designs, pats and clock sync.

## Send a pet from Studio

1. Install a successful **0.3** firmware build; earlier firmware does not support this protocol.
2. On Pebble, open **Apps → Connect → Bluetooth → Enable 3 min**.
3. Open [Pet Studio](https://anzex7-del.github.io/pebble-microos/) in Chrome or Edge on a Bluetooth-equipped computer or supported Android device.
4. Choose **Connect to Pebble**, then select **Pebble** in the browser dialog.
5. Use **Send this design**, **Sync clock**, or **Send a pat**. Wait for confirmation; wait another two seconds before powering off so the queued settings save completes.
6. Choose **Disconnect**, and turn Bluetooth off on the device or let its three-minute window expire.

This is a BLE GATT companion link. It does not provide Bluetooth audio, Classic pairing, file transfer, notifications, Wi-Fi provisioning or firmware updates. Safari/iPhone browsers and embedded previews without Web Bluetooth cannot use this bridge. The website disables connection controls when the API is unavailable. No application account, pairing PIN or authenticated ownership is implemented: a nearby compatible client can change pet settings while the window is open. Wi-Fi passwords are never part of this protocol.

If a transfer fails, the page reports how many settings the device confirmed. Reconnect and send again. Finish Wi-Fi scans before transferring; radio operations share the background worker. The device echoes the full applied command, so a successful GATT write alone is never shown as an applied setting.

## Developer protocol

The pinned TAL BLE v2 service is `0xFD50`. Device name: `Pebble`.

| Characteristic | UUID | Use |
|---|---|---|
| Command | `00000001-0000-1001-8001-00805f9b07d0` | Write |
| Acknowledgment | `00000003-0000-1001-8001-00805f9b07d0` | Read |

Connection handshake reads `OK:V`. Commands are ASCII and at most 20 bytes. A successful command returns `OK:` plus that command; invalid commands return `ERR!`. Names use 1–16 ASCII letters/numbers with internal spaces, hyphens or underscores. Color/eye indices match `web/profile.js`.

| Command | Example | Meaning |
|---|---|---|
| `N<name>` | `NMilo` | Pet name |
| `C<0..3>` | `C2` | Peach color |
| `E<0..2>` | `E1` | Dreamy eyes |
| `F<5..60>` | `F25` | Focus minutes |
| `Z<minutes>` | `Z240` | UTC offset, −720 to +840 in steps of 15 |
| `T<Unix seconds>` | `T1795392000` | UTC time, 2024–2099 |
| `P` | `P` | Pat |

Commands are validated in the radio callback, applied on the LVGL thread, and acknowledged by the background worker. The command queue holds eight entries. Saves coalesce after one second without another change; periodic pet-state saves run every 30 seconds. Invalid payloads cannot invoke file or shell operations.
