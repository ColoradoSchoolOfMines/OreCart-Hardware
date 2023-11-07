#
# Copyright (c) 2020 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(https_client)

set(SRC_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.c

    ## Modules
    # Server Module
    src/modules/server_module/nrf9160_setup.c
    src/modules/server_module/socket.c
    src/modules/server_module/server_module.c
)

# NORDIC SDK APP START
target_sources(app PRIVATE ${SRC_FILES})
# NORDIC SDK APP END
