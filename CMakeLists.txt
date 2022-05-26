# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(blinky)

target_sources(app PRIVATE src/main.c)

# Custom files and folders

target_sources(app PRIVATE
    src/remote_service/remote.c
)

# target_sources(app PRIVATE
#     src/adxl345/adxl345.c
# )

zephyr_library_include_directories(src/remote_service)
# zephyr_library_include_directories(src/adxl345)