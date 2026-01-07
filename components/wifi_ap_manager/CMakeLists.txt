idf_component_register(
    SRCS "wifi_ap_manager.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_wifi esp_event esp_netif nvs_flash freertos
)
