
:: Copyright (C) 2019 iFunFactory Inc. All Rights Reserved.
::
:: This work is confidential and proprietary to iFunFactory Inc. and
:: must not be used, disclosed, copied, or distributed without the prior
:: consent of iFunFactory Inc.

@ECHO OFF
ECHO Start build .proto files

setLocal enableDelayedExpansion

SET PROTO_BUILD_PATH=%cd%
SET PROJECT_SOURCE_DIR=..\Classes

SET USER_PROTO_FILE_INPUT_PATH=%PROTO_BUILD_PATH%\%PROJECT_SOURCE_DIR%
SET USER_PROTO_FILE_OUT_PATH=%PROTO_BUILD_PATH%\%PROJECT_SOURCE_DIR%

if %USER_PROTO_FILE_INPUT_PATH% EQU %PROTO_BUILD_PATH% (
    ECHO USER_PROTO_FILE_INPUT_PATH should not be the same PROTO_BUILD_PATH.
    ECHO This script automatically build .proto file on `funapi proto` and
    ECHO USER_PROTO_FILE_INPUT_PATH directory
    GOTO:EOF
)

:::::::::::::::::::::::::::::::::::::::::::::
:: Build plugin's proto file. DO NOT EDIT!
:::::::::::::::::::::::::::::::::::::::::::::

.\protoc --cpp_out=%PROJECT_SOURCE_DIR% ^
    funapi\management\maintenance_message.proto ^
    funapi\network\fun_message.proto ^
    funapi\network\ping_message.proto ^
    funapi\service\multicast_message.proto ^
    funapi\service\redirect_message.proto ^
    funapi\distribution\fun_dedicated_server_rpc_message.proto

if %errorlevel% NEQ 0 (
    GOTO:EOF
)

::::::::::::::::::::::::::::::::::::::::::::::::::
:: Build test and user proto files.
::::::::::::::::::::::::::::::::::::::::::::::::::

:: Build funapi test messages
.\protoc --cpp_out=%PROJECT_SOURCE_DIR% ^
    test_dedicated_server_rpc_messages.proto ^
    test_messages.proto

if %errorlevel% NEQ 0 (
    GOTO:EOF
)

:: Copy user proto files to funapi plugin directory.
SET num=0
FOR %%I IN (%USER_PROTO_FILE_INPUT_PATH%\*.proto) DO (
    :: save copyed files name
    SET /a num+=1
    SET FILE_LIST!num!=%%I
    copy %%I .

    if %errorlevel% NEQ 0 (
        GOTO:EOF
    )
)

:: Build and remove copyed user message
FOR /L %%i IN (1,1,!num!) Do (
    FOR /F %%a in ("!FILE_LIST%%i!") DO (
        .\protoc --cpp_out=%USER_PROTO_FILE_OUT_PATH% %%~na%%~xa

        if %errorlevel% NEQ 0 (
            GOTO:EOF
        )

        DEL /F %%~na%%~xa

        if %errorlevel% NEQ 0 (
            GOTO:EOF
        )
    )
)

:: Build Finish
GOTO:EOF