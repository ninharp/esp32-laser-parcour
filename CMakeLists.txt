# ESP32 Laser Obstacle Course - Main CMakeLists.txt
# This is a multi-module system that can be configured as Main Unit, Laser Unit or Finish Unit

cmake_minimum_required(VERSION 3.16)

# Set ESP-ADF path (load if available, will be used only if CONFIG_ENABLE_SOUND_MANAGER is enabled)
if(DEFINED ENV{ADF_PATH})
set(ADF_PATH $ENV{ADF_PATH})
include($ENV{ADF_PATH}/CMakeLists.txt)
list(APPEND EXTRA_COMPONENT_DIRS "${ADF_PATH}/components")
message(STATUS "ESP-ADF components loaded from: ${ADF_PATH}")
else()
message(STATUS "ADF_PATH not set - Sound Manager will use stub implementation if enabled")
endif()

# set(EXCLUDE_COMPONENTS 
#     "audio_board" "audio_hal" "audio_sal" 
#     "esp_peripherals" "display_service" "esp_dispatcher"
#     "tone_partition" "esp_actions" 
#     "bluetooth_service" "battery_service" "wifi_service"
#     "ota_service" "coredump_upload_service" "dueros_service"
#     "clouds" "playlist" "audio_recorder" "input_key_service"
#     "esp_event_cast" "esp_codec_dev"
# )

# Include ESP-IDF CMake project setup
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# Project name
project(esp32_laser_parcour)
