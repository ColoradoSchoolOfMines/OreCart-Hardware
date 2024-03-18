#
# Copyright (c) 2020 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(orecart)

set(gen_dir ${ZEPHYR_BINARY_DIR}/include/generated/)

set(SRC_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/src/common/logging.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/common/tools.cpp

    ## Modules
    # Net Module
    ${CMAKE_CURRENT_SOURCE_DIR}/src/modules/net_module/socket.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/modules/net_module/nrf9160_setup.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/modules/net_module/net_module.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/modules/net_module/net_scheduler.cpp

    # Tracking Module
    ${CMAKE_CURRENT_SOURCE_DIR}/src/modules/tracking_module/tracking_module.cpp
)

# target_include_directories(app PRIVATE
#     ${CMAKE_CURRENT_SOURCE_DIR}/src/modules/net_module
# )

# generate_inc_file_for_target(
#     app
#     res/cert/server_dos.der
#     ${gen_dir}/server_dos.der.inc
# )

# NORDIC SDK APP START
target_sources(app PRIVATE ${SRC_FILES})
# NORDIC SDK APP END
