// mdns_init.h — advertise the Sendspin player service. Ported from sendspin-xiao.
#pragma once
#include "esp_err.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
esp_err_t mdns_advertise_sendspin(const char* instance_name, uint16_t port, const char* path);
#ifdef __cplusplus
}
#endif
