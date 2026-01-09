idf_component_register(
    SRCS 
        "sound_manager.c"
        "audio_output.c"
    INCLUDE_DIRS "include"
    REQUIRES 
        esp_adc
        nvs_flash
        fatfs
        sd_card_manager
        buzzer
        audio_pipeline
        audio_stream
        esp-adf-libs
    PRIV_REQUIRES
        esp_http_server
)
