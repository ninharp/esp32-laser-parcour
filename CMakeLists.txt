# ESP32 Laser Obstacle Course - Main CMakeLists.txt
# This is a multi-module system that can be configured as Main Unit, Laser Unit or Finish Unit

cmake_minimum_required(VERSION 3.16)

set (PROJECT esp32_laser_parcour)

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

# Include QConnex CMake Functions
include(${CMAKE_CURRENT_SOURCE_DIR}/tools/project.cmake)

# Include ESP-IDF CMake project setup
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

set (PROJECT_MAIN main)

# Project name
project(${PROJECT})

# Bump Version for project
add_project_version_info()

# Read configuration values from sdkconfig
read_sdkconfig_value("CONFIG_MODULE_ID" MODULE_ID)
read_sdkconfig_value("CONFIG_DEVICE_NAME" MODULE_DEVICE_NAME)
read_sdkconfig_value("CONFIG_WIFI_SSID" MODULE_WIFI_SSID)
read_sdkconfig_value("CONFIG_WIFI_PASSWORD" MODULE_WIFI_PASSWORD)

file(STRINGS "sdkconfig" CONFIG_LINES)
set(MODULE_ROLE "Unknown")
set(MODULE_ROLE_LOWER "unknown")
FOREACH(LINE ${CONFIG_LINES})
    IF(LINE MATCHES "^CONFIG_MODULE_ROLE_CONTROL=")
        set(MODULE_ROLE "Control")
        set(MODULE_ROLE_LOWER "control_module")
    ELSEIF(LINE MATCHES "^CONFIG_MODULE_ROLE_LASER=")
        set(MODULE_ROLE "Laser")
        set(MODULE_ROLE_LOWER "laser_module")
    ELSEIF(LINE MATCHES "^CONFIG_MODULE_ROLE_FINISH=")
        set(MODULE_ROLE "Finish")
        set(MODULE_ROLE_LOWER "finish_module")
    ENDIF()
ENDFOREACH()

# Firmware naming based on module role and ID
set(FIRMWARE_VERSION "v${VERSION_OUT}")
set(DIST_ROLE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/dist/${MODULE_ROLE_LOWER}/${FIRMWARE_VERSION}")
set(FIRMWARE_OTA_NAME "firmware_${MODULE_ROLE_LOWER}_module_id_${MODULE_ID}_${FIRMWARE_VERSION}.bin")
set(FIRMWARE_FULL_NAME "firmware_${MODULE_ROLE_LOWER}_module_id_${MODULE_ID}_${FIRMWARE_VERSION}_full.bin")

MESSAGE("Firmware will be saved to: ${DIST_ROLE_DIR}/")
MESSAGE("  - OTA:  ${FIRMWARE_OTA_NAME}")
MESSAGE("  - Full: ${FIRMWARE_FULL_NAME}")

add_custom_target(merge
                COMMENT "Merging firmware files and copying to dist/${MODULE_ROLE_LOWER}/${FIRMWARE_VERSION}/"   
                COMMAND ${CMAKE_COMMAND} -E make_directory ${DIST_ROLE_DIR}
                COMMAND ${ESPTOOLPY} merge_bin -o ${DIST_ROLE_DIR}/${FIRMWARE_FULL_NAME} @flash_project_args
                COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_NAME}.bin ${DIST_ROLE_DIR}/${FIRMWARE_OTA_NAME}
                WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                DEPENDS gen_project_binary bootloader
                USES_TERMINAL
                VERBATIM
                )

# Display Compile Time Information
MESSAGE(STATUS "${Green}")
MESSAGE(STATUS "--------------Module Info-------------")
MESSAGE(STATUS "Module ID: ${MODULE_ID}")
MESSAGE(STATUS "Module Role: ${MODULE_ROLE}")
MESSAGE(STATUS "Device Name: ${MODULE_DEVICE_NAME}")
MESSAGE(STATUS "WiFi SSID: ${MODULE_WIFI_SSID}")
MESSAGE(STATUS "WiFi Password: ${MODULE_WIFI_PASSWORD}")
MESSAGE(STATUS "--------------Compile Info------------")
MESSAGE(STATUS "IDF_PATH = ${IDF_PATH}")
MESSAGE(STATUS "ADF_PATH = ${ADF_PATH}")
MESSAGE(STATUS "IDF_TARGET = ${IDF_TARGET}")
MESSAGE(STATUS "PROJECT_NAME = ${PROJECT_NAME}")
MESSAGE(STATUS "PROJECT_VERSION = ${VERSION_OUT}")
MESSAGE(STATUS "PROJECT_MAIN = ${PROJECT_MAIN}")
MESSAGE(STATUS "PROJECT_DIR = ${PROJECT_DIR}")
MESSAGE(STATUS "BUILD_DIR = ${BUILD_DIR}")
MESSAGE(STATUS "SDKCONFIG = ${SDKCONFIG}")
MESSAGE(STATUS "SDKCONFIG_DEFAULTS = ${SDKCONFIG_DEFAULTS}")
# MESSAGE(STATUS "COMPILE_OPTIONS = ${COMPILE_OPTIONS}")
MESSAGE(STATUS "---------------------------------------")
MESSAGE(STATUS "CMAKE_SOURCE_DIR = ${CMAKE_SOURCE_DIR}")
MESSAGE(STATUS "CMAKE_BINARY_DIR = ${CMAKE_BINARY_DIR}")
MESSAGE(STATUS "---------------------------------------")
MESSAGE(STATUS "${ColorReset}")
