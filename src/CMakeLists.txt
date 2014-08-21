# Copyright (C) 2013 iFunFactory Inc. All Rights Reserved.
#
# This work is confidential and proprietary to iFunFactory Inc. and
# must not be used, disclosed, copied, or distributed without the prior
# consent of iFunFactory Inc.


###############################################################################
# Extends the CPP inclusion path
###############################################################################

include_directories(${CMAKE_CURRENT_BINARY_DIR})

include_directories(${PROJECT_SOURCE_DIR}/rapidjson/include)



###############################################################################
# Targets to build
###############################################################################

set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -g -O0" )


# Main target to generate an application.
# Funapi game app is in the linux shared object form (i.e., .so)
# 'funapi_runner' which is bundled in the Funapi package will
# load the shared object to run the game app.
add_library(
    ${PROJECT_NAME} SHARED
    funapi_network.cc
    funapi_network.h)


add_executable(
    ${PROJECT_NAME}-tester
    funapi_network_tester.cc
)


add_dependencies(${PROJECT_NAME}-tester ${PROJECT_NAME})


target_link_libraries(${PROJECT_NAME}-tester ${PROJECT_NAME})
target_link_libraries(${PROJECT_NAME}-tester ${libcurl4-openssl-dev})

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  target_link_libraries(${PROJECT_NAME}-tester pthread rt)
endif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")



###############################################################################
# Generates intermediate output.
###############################################################################




###############################################################################
# Installation rules
###############################################################################

install(
    TARGETS ${PROJECT_NAME}
    LIBRARY DESTINATION lib)
