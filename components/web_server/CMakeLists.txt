idf_component_register(
    SRCS "web_server.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_http_server json nvs_flash game_logic
    PRIV_REQUIRES wifi_ap_manager
)
