idf_component_register(
    SRCS
        "main.cpp"
        "nvs_store.cpp"
        "wifi_init.cpp"
        "wifi_provisioning.cpp"
        "mdns_init.cpp"
        "ui/ui.cpp"
        "ui/screen_now_playing.cpp"
        "ui/screens_aux.cpp"
    INCLUDE_DIRS "." "ui"
    REQUIRES
        nvs_flash esp_wifi esp_netif esp_event
        espressif__mdns
        sendspin__sendspin-cpp
        lvgl__lvgl
)
