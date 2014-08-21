# Copyright (C) 2013 iFunFactory Inc. All Rights Reserved.
#
# This work is confidential and proprietary to iFunFactory Inc. and
# must not be used, disclosed, copied, or distributed without the prior
# consent of iFunFactory Inc.


###############################################################################
# Environment setting
###############################################################################

# Enforces a minimum version.
cmake_minimum_required(VERSION 2.8)


# Sets cmake PROJECT_NAME var.
# Enables only C++.
project(funapi_plugin_cocos2dx CXX)



###############################################################################
# Utility functions.
###############################################################################

function (CheckRequirements)
endfunction ()



###############################################################################
# Requirements
###############################################################################

# Checks if everything is ready.
CheckRequirements()

find_program(PROTOBUF_PROTOC_EXECUTABLE NAMES protoc)
if (NOT PROTOBUF_PROTOC_EXECUTABLE)
  message(FATAL_ERROR "Protoc is required.")
endif ()

find_path(PROTOBUF_INCLUDE_DIR google/protobuf/message.h)
include_directories(${PROTOBUF_INCLUDE_DIR})

find_library(libcurl4-openssl-dev libcurl.so)
find_library(libprotobuf libprotobuf.so)



###############################################################################
# Targets to build
###############################################################################

# Work-around to re-trigger cmake after VERSION changes.
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/VERSION
               ${CMAKE_CURRENT_BINARY_DIR}/VERSION.ignoreme)



###############################################################################
# Installation rules
###############################################################################

# Ubuntu uses /usr instead of /usr/local.
set(CMAKE_INSTALL_PREFIX "/usr")


# Also bundles auxiliary files.
install(
    FILES VERSION LICENSE README
    DESTINATION share/${PROJECT_NAME})



###############################################################################
# Sub-modules
###############################################################################

# Keeps adding targets/commands.
add_subdirectory(src)



###############################################################################
# Packaging rules
###############################################################################

# Makes a tgz package.
# Funapi expects a package name in the form of $name_$version_install.tar.gz.
string(REPLACE "_" "-" NORMALIZED_NAME ${PROJECT_NAME})
file(READ ${CMAKE_CURRENT_SOURCE_DIR}/VERSION VERSION)
string(STRIP ${VERSION} VERSION)

set(CPACK_GENERATOR "TGZ")
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY 0)
set(CPACK_PACKAGING_INSTALL_PREFIX "/usr")
set(CPACK_PACKAGE_NAME ${NORMALIZED_NAME})
set(CPACK_PACKAGE_VERSION ${VERSION})
set(CPACK_PACKAGE_FILE_NAME ${NORMALIZED_NAME}_${VERSION}_install)
set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "_CPack_Packages")

include(CPack)
