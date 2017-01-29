# Copyright 2017 Jan de Cuveland <cmail@cuveland.de>
cmake_minimum_required(VERSION 3.0.2 FATAL_ERROR)

# retrieve project version information
file(READ VERSION PDA_VERSION)
string(STRIP "${PDA_VERSION}" PDA_VERSION)
project(pda LANGUAGES C VERSION "${PDA_VERSION}")

# generate config.h
include(CheckIncludeFiles)
check_include_files(numa.h NUMA_AVAIL)
check_include_files(kmod.h KMOD_AVAIL)
set(MODPROBE_MODE TRUE CACHE BOOL "Enable modprobe mode")
file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/src)
configure_file(config.h.in ${PROJECT_BINARY_DIR}/src/config.h)

# generate symlinks to arch sources
foreach (source bar device_operator dma_buffer pci)
execute_process(
  COMMAND ${CMAKE_COMMAND} -E create_symlink
    ${PROJECT_SOURCE_DIR}/arch/linux_x86_uio/${source}_inc.c
    ${PROJECT_BINARY_DIR}/src/${source}.inc)
endforeach()

# build userspace library
file(GLOB LIB_SOURCES src/*.c)
add_library(pda-static STATIC ${LIB_SOURCES})
add_library(pda-shared SHARED ${LIB_SOURCES})
set_target_properties(pda-shared
  PROPERTIES POSITION_INDEPENDENT_CODE 1)
set_target_properties(pda-static pda-shared
  PROPERTIES OUTPUT_NAME pda CLEAN_DIRECT_OUTPUT 1)
foreach (lib pda-static pda-shared)
target_include_directories(${lib}
  PUBLIC include
  PRIVATE src
  PRIVATE patches/linux_uio
  PRIVATE ${PROJECT_BINARY_DIR}/src
)
endforeach()

# specify files to install
install(TARGETS pda-static ARCHIVE DESTINATION lib)
install(TARGETS pda-shared LIBRARY DESTINATION lib)
install(DIRECTORY include/ DESTINATION include)
