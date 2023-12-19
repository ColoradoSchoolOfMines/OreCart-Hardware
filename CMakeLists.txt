#
# Copyright (c) 2020 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(https_client)

set(gen_dir ${ZEPHYR_BINARY_DIR}/include/generated/)

set(SRC_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.c

    ## Modules
    # Server Module
    src/modules/server_module/nrf9160_setup.c
    src/modules/server_module/socket.c
    src/modules/server_module/server_module.c
    src/modules/server_module/net_tools.c
)

generate_inc_file_for_target(
    app
    res/cert/server_dos.der
    ${gen_dir}/server_dos.der.inc
)

# NORDIC SDK APP START
target_sources(app PRIVATE ${SRC_FILES})
# NORDIC SDK APP END
