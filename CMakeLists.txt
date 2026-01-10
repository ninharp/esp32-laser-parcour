# ESP32 Laser Obstacle Course - Main CMakeLists.txt
# This is a multi-module system that can be configured as Main Unit, Laser Unit or Finish Unit

cmake_minimum_required(VERSION 3.16)

# Set ESP-ADF path (load if available, will be used only if CONFIG_ENABLE_SOUND_MANAGER is enabled)
if(DEFINED ENV{ADF_PATH})
    set(ADF_PATH $ENV{ADF_PATH})
    list(APPEND EXTRA_COMPONENT_DIRS "${ADF_PATH}/components")
    message(STATUS "ESP-ADF components loaded from: ${ADF_PATH}")
else()
    message(STATUS "ADF_PATH not set - Sound Manager will use stub implementation if enabled")
endif()

# Include ESP-IDF CMake project setup
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# Project name
project(esp32_laser_parcour)
