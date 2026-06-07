idf_component_register(
    SRCS
        "main.cpp"
        "display.cpp"
        "app_state.cpp"
        "nvs_store.cpp"
        "wifi_init.cpp"
        "wifi_provisioning.cpp"
        "mdns_init.cpp"
        "ma_api.cpp"
        "ui/ui.cpp"
        "ui/screen_now_playing.cpp"
        "ui/screens_aux.cpp"
    INCLUDE_DIRS "." "ui"
    REQUIRES
        nvs_flash esp_wifi esp_netif esp_event
        esp_driver_spi esp_lcd esp_timer
        espressif__mdns
        espressif__esp_websocket_client
        espressif__esp_lcd_sh8601
        sendspin__sendspin-cpp
        lvgl__lvgl
)
