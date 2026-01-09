idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES nvs_flash esp_wifi esp_netif esp_event esp_http_server driver
             display_manager game_logic espnow_manager laser_control sensor_manager
             wifi_ap_manager button_handler buzzer web_server sd_card_manager
)
