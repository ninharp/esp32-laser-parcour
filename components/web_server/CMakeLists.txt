idf_component_register(
    SRCS 
        "web_server.c"
        "sound_api.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_http_server json nvs_flash game_logic sd_card_manager sound_manager
    PRIV_REQUIRES wifi_ap_manager
    EMBED_TXTFILES 
        "index.html"
        "sounds.html"
)
