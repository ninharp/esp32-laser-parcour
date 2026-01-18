# Sound Manager Component
# Provides audio playback via I2S (MAX98357A amplifier) using ESP-ADF pipeline
# Falls back to stub implementation if ESP-ADF not available

set(COMPONENT_SRCS "sound_manager.c")
set(COMPONENT_ADD_INCLUDEDIRS "include")

# Base requirements (always needed)
set(COMPONENT_REQUIRES
    esp_adc
    nvs_flash
    fatfs
    sd_card_manager
    buzzer
)

# Add ESP-ADF components only if ADF_PATH is set
if(DEFINED ENV{ADF_PATH})
    list(APPEND COMPONENT_REQUIRES
        audio_pipeline
        audio_stream
        esp-adf-libs
        audio_hal
        esp_peripherals
    )
    message(STATUS "Sound Manager: ESP-ADF components included")
else()
    message(STATUS "Sound Manager: Building with stub implementation (ADF_PATH not set)")
endif()

idf_component_register(
    SRCS ${COMPONENT_SRCS}
    INCLUDE_DIRS ${COMPONENT_ADD_INCLUDEDIRS}
    REQUIRES ${COMPONENT_REQUIRES}
    PRIV_REQUIRES esp_http_server
)