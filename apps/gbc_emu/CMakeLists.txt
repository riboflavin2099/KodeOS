cmake_minimum_required(VERSION 3.22)

# GBC emulator scaffold. An app is its own firmware, so this directory is a
# project of its own and is not added to the kodeOS build.
#
# The chip has to be known before kbsp's sdkconfig.board is read. On a clean
# tree ESP-IDF would otherwise guess esp32 and cache it.
set(IDF_TARGET esp32p4)

# This copy lives inside the KodeOS tree and builds against that tree's sdk/
# submodule (the commit FEASIBILITY.md was written from). A catalogue app
# must not do this: kode-apps rule B3 rejects EXTRA_COMPONENT_DIRS. Moving
# the app out of the tree means deleting the block below, renaming
# main/idf_component.catalog.yml to main/idf_component.yml, then pointing
# CONFIG_PARTITION_TABLE_CUSTOM_FILENAME at
# managed_components/kodediy__kbsp/partitions.csv.
set(KODE_SDK_DIR "${CMAKE_CURRENT_LIST_DIR}/../../sdk")
set(KBSP_BOARD "${KODE_SDK_DIR}/components/kbsp/sdkconfig.board")
if(NOT EXISTS "${KBSP_BOARD}")
    message(FATAL_ERROR
        "The SDK submodule is not checked out (${KBSP_BOARD} is missing). "
        "From the KodeOS root run: git submodule update --init sdk")
endif()
# The component manager stays on: SDK components still fetch LVGL and the
# panel driver. main/ has no idf_component.yml, so it does not also fetch a
# second kode. The catalogue pin is main/idf_component.catalog.yml and is
# renamed to idf_component.yml when this app leaves the tree.
set(EXTRA_COMPONENT_DIRS "${KODE_SDK_DIR}/components")
set(SDKCONFIG_DEFAULTS
    "${KBSP_BOARD};${CMAKE_CURRENT_LIST_DIR}/sdkconfig.defaults")

if(NOT DEFINED PROJECT_VER)
    set(PROJECT_VER "00.00.01")
endif()

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# The project name is the app's identity on the Dot. The image is build/gbc_emu.bin.
project(gbc_emu)
