# Copyright 2017 Jan de Cuveland <cmail@cuveland.de>
cmake_minimum_required(VERSION 3.0.2 FATAL_ERROR)

# retrieve project version information
file(READ VERSION PDA_VERSION)
string(STRIP "${PDA_VERSION}" PDA_VERSION)
project(pda LANGUAGES C VERSION "${PDA_VERSION}")
math(EXPR SO_VERSION "${pda_VERSION_MAJOR} - ${pda_VERSION_PATCH}")

# generate config.h
include(CheckIncludeFiles)
check_include_files(numa.h NUMA_AVAIL)
check_include_files(libkmod.h KMOD_AVAIL)
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
add_compile_options(-std=gnu99 -Wall -Wunused-result -fno-tree-vectorize)
file(GLOB LIB_SOURCES src/*.c)
add_library(pda-static STATIC ${LIB_SOURCES})
add_library(pda-shared SHARED ${LIB_SOURCES})
set_target_properties(pda-shared
  PROPERTIES POSITION_INDEPENDENT_CODE 1
  SOVERSION "${SO_VERSION}"
  VERSION "${SO_VERSION}.${pda_VERSION_PATCH}.${pda_VERSION_MINOR}")
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
target_link_libraries(pda-shared PUBLIC pci PUBLIC pthread)
if (${NUMA_AVAIL})
  target_link_libraries(pda-shared PUBLIC numa)
endif()
if (${KMOD_AVAIL})
  target_link_libraries(pda-shared PUBLIC kmod)
endif()

# specify files to install
install(TARGETS pda-static ARCHIVE DESTINATION lib)
install(TARGETS pda-shared LIBRARY DESTINATION lib)
install(DIRECTORY include/ DESTINATION include)

# build debian package - experimental
set(CPACK_GENERATOR DEB)
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY
  "CBM FLES FLIB low-level user space access library")
set(CPACK_PACKAGE_NAME libpda)
set(CPACK_PACKAGE_CONTACT
  "Jan de Cuveland <cuveland@compeng.uni-frankfurt.de>")
set(CPACK_PACKAGE_VERSION_MAJOR ${pda_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${pda_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${pda_VERSION_PATCH})
set(CPACK_STRIP_FILES TRUE)
set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE amd64)
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6 (>= 2.15)")
set(CPACK_DEBIAN_PACKAGE_PRIORITY optional)
set(CPACK_DEBIAN_PACKAGE_SECTION libs)
include(CPack)
