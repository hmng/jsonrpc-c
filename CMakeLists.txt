cmake_minimum_required(VERSION 3.15)

project(jsonrcp-c
    LANGUAGES C
)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

set(CMAKE_C_STANDARD 11)
set(SRCS src/jsonrpc-c.c src/cJSON.c)
set(HDRS include/jsonrpc-c.h include/cJSON.h)

find_package(Libev REQUIRED)

add_library(
        jsonrpc-c
        STATIC ${HDRS} ${SRCS}
)
target_include_directories(
        jsonrpc-c
        PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        ${LIBEV_INCLUDE_DIRS}
)
target_link_libraries(
        jsonrpc-c
        PUBLIC
        ${LIBEV_LIBRARIES}
)

add_library(
        jsonrpc-c-shared
        SHARED ${HDRS} ${SRCS}
)
target_include_directories(
        jsonrpc-c-shared
        PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        ${LIBEV_INCLUDE_DIRS}
)
target_link_libraries(
        jsonrpc-c-shared
        PUBLIC
        ${LIBEV_LIBRARIES}
)