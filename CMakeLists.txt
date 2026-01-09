# ESP32 Laser Obstacle Course - Main CMakeLists.txt
# This is a multi-module system that can be configured as Main Unit or Laser Unit

cmake_minimum_required(VERSION 3.16)

# Set ESP-ADF path (if ENABLE_SOUND_MANAGER is enabled)
if(CONFIG_ENABLE_SOUND_MANAGER)
    if(NOT DEFINED ENV{ADF_PATH})
        message(WARNING "ADF_PATH environment variable not set. Sound Manager will use stub implementation.")
    else()
        set(ADF_PATH $ENV{ADF_PATH})
        list(APPEND EXTRA_COMPONENT_DIRS "${ADF_PATH}/components")
        message(STATUS "ESP-ADF enabled: ${ADF_PATH}")
    endif()
endif()

# Include ESP-IDF CMake project setup
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# Project name
project(esp32_laser_parcour)
