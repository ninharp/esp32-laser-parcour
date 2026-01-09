# Determine which module source to compile based on configuration
set(MODULE_SRCS "main.c" "logging_config.c")

if(CONFIG_MODULE_ROLE_CONTROL)
    list(APPEND MODULE_SRCS "module_control.c")
endif()

if(CONFIG_MODULE_ROLE_LASER)
    list(APPEND MODULE_SRCS "module_laser.c")
endif()

if(CONFIG_MODULE_ROLE_FINISH)
    list(APPEND MODULE_SRCS "module_finish.c")
endif()

idf_component_register(
    SRCS ${MODULE_SRCS}
    INCLUDE_DIRS "."
    REQUIRES nvs_flash esp_wifi esp_netif esp_event esp_http_server driver
             display_manager game_logic espnow_manager laser_control sensor_manager
             wifi_ap_manager button_handler buzzer web_server sd_card_manager sound_manager
)
