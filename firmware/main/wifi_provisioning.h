// wifi_provisioning.h — WiFi connect + SoftAP captive-portal provisioning.
//
// Boot logic: try NVS-saved creds -> on fail/none, start SoftAP setup mode
// ("MA-ESPortable-XXXX" + http://192.168.4.1). Ongoing config is via the
// on-LAN web page once connected. (BLE provisioning deferred — see README.)
#pragma once
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    WIFI_STATE_BOOT,
    WIFI_STATE_CONNECTING,
    WIFI_STATE_CONNECTED,
    WIFI_STATE_PROVISIONING,   // SoftAP captive portal active
    WIFI_STATE_FAILED,
} wifi_state_t;

// Init NVS-backed WiFi. Tries saved creds; falls back to SoftAP provisioning.
void wifi_prov_init(void);

// Current connection state (for the UI / status).
wifi_state_t wifi_prov_state(void);

// Force re-provisioning: clear saved creds and reboot into SoftAP setup mode.
void wifi_prov_forget_and_reboot(void);

#ifdef __cplusplus
}
#endif
