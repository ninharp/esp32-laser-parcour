# idf_component_register(
#     SRCS 
#         "sound_manager.c"
#     INCLUDE_DIRS "include"
#     REQUIRES 
#         esp_adc
#         nvs_flash
#         fatfs
#         sd_card_manager
#         buzzer
#         audio_pipeline
#         audio_stream
#         esp-adf-libs
#     PRIV_REQUIRES
#         esp_http_server
# )

set(COMPONENT_SRCS "sound_manager.c")
set(COMPONENT_ADD_INCLUDEDIRS "." "include")
set(COMPONENT_REQUIRES
    esp_adc
    nvs_flash
    fatfs
    sd_card_manager
    buzzer
    audio_pipeline
    audio_stream
    esp-adf-libs
    audio_hal
    esp_peripherals
    )

register_component()