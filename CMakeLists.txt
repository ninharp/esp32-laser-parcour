# ESP32 Laser Obstacle Course - Main CMakeLists.txt
# This is a multi-module system that can be configured as Main Unit or Laser Unit

cmake_minimum_required(VERSION 3.16)

# Include ESP-IDF CMake project setup
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# Project name
project(esp32_laser_parcour)
