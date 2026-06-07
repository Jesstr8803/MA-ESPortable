// sendspin_client.h — Music Assistant connection via the Sendspin SDK.
#pragma once
#ifdef __cplusplus
extern "C" {
#endif
// Start the Sendspin client (own task, core 1). Advertises via mDNS; pushes
// now-playing metadata to the UI. Call after WiFi is connected.
void sendspin_start(const char *device_name);
#ifdef __cplusplus
}
#endif
