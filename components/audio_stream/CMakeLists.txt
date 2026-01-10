# Local override of ESP-ADF audio_stream component
# Removes dependencies on audio_board, esp_peripherals, etc. that we don't use
# Uses source files from ESP-ADF but with minimal dependencies

set(ADF_STREAM_PATH "$ENV{ADF_PATH}/components/audio_stream")

# Source files we actually use
set(COMPONENT_SRCS
    "${ADF_STREAM_PATH}/i2s_stream_idf5.c"  # Use IDF 5.x version with new I2S API
    "${ADF_STREAM_PATH}/fatfs_stream.c"
)

# Only include what we actually need - remove audio_board, esp_peripherals, etc
set(COMPONENT_REQUIRES 
    audio_pipeline 
    driver 
    fatfs
    spiffs
    esp-adf-libs
    mbedtls
)

set(COMPONENT_ADD_INCLUDEDIRS 
    "${ADF_STREAM_PATH}/include"
    "${ADF_STREAM_PATH}"
    "." # Add local component directory for board_pins_config.h
)

register_component()
