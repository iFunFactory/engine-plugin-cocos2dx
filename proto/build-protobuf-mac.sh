#!/bin/bash -e
# Copyright (C) 2019 iFunFactory Inc. All Rights Reserved.
#
# This work is confidential and proprietary to iFunFactory Inc. and
# must not be used, disclosed, copied, or distributed without the prior
# consent of iFunFactory Inc.

set -e
echo Start build .proto files

# Setting proto library path
export DYLD_LIBRARY_PATH=$DYLD_LIBRARY_PATH:${PWD}/lib

# Setting user project name
PROTO_BUILD_PATH=${PWD}
PROJECT_SOURCE_DIR=../Classes

USER_PROTO_FILE_INPUT_PATH=${PROTO_BUILD_PATH}/${PROJECT_SOURCE_DIR}
USER_PROTO_FILE_OUT_PATH=${PROTO_BUILD_PATH}/${PROJECT_SOURCE_DIR}

if [ ${USER_PROTO_FILE_INPUT_PATH} == ${PROTO_BUILD_PATH} ]; then
    echo USER_PROTO_FILE_INPUT_PATH should not be the same PROTO_BUILD_PATH.
    echo This script automatically build .proto file on `funapi proto` and
    echo USER_PROTO_FILE_INPUT_PATH directory
    exit 1
fi

#############################################
# Build plugin's proto file. DO NOT EDIT!
#############################################

./protoc --cpp_out=${PROJECT_SOURCE_DIR} \
    funapi/management/maintenance_message.proto \
    funapi/network/fun_message.proto \
    funapi/network/ping_message.proto \
    funapi/service/multicast_message.proto \
    funapi/service/redirect_message.proto \
    funapi/distribution/fun_dedicated_server_rpc_message.proto

#############################################
# Build test and user proto files.
#############################################

# Build funapi test messages
./protoc --cpp_out=${PROJECT_SOURCE_DIR} \
    test_messages.proto \
    test_dedicated_server_rpc_messages.proto

# Copy user proto files to funapi plugin directory.
FILE_LIST=(`find ${USER_PROTO_FILE_INPUT_PATH} -maxdepth 1 -name "*.proto"`)
find ${USER_PROTO_FILE_INPUT_PATH} -maxdepth 1 -name "*.proto" -exec cp {} . \;
for file in "${FILE_LIST[@]}"; do
    cp ${file} .
done

# Build and remove copyed user message
for value in "${FILE_LIST[@]}"; do
    name=$(basename "$value" ".proto")
    ./protoc --cpp_out=${USER_PROTO_FILE_OUT_PATH} ${name}.proto
    rm -f ${name}.proto
done