cmake_minimum_required(VERSION 3.20)
project(RKengServer VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(LIB_DIR  ${CMAKE_CURRENT_SOURCE_DIR}/../lib)
set(SRC_DIR  ${CMAKE_CURRENT_SOURCE_DIR}/src)

file(GLOB_RECURSE SERVER_SOURCES
    ${SRC_DIR}/core/*.cpp
    ${SRC_DIR}/network/*.cpp
    ${SRC_DIR}/world/*.cpp
)

add_executable(RKengServer ${SRC_DIR}/main_server.cpp ${SERVER_SOURCES})

target_include_directories(RKengServer PRIVATE ${SRC_DIR} ${LIB_DIR})

if(WIN32)
    target_link_libraries(RKengServer PRIVATE ws2_32)
endif()

if(MSVC)
    target_compile_options(RKengServer PRIVATE
        $<$<CONFIG:Debug>:   /Od /Zi /W4>
        $<$<CONFIG:Release>: /O2>
    )
else()
    target_compile_options(RKengServer PRIVATE
        $<$<CONFIG:Debug>:   -g -O0 -Wall>
        $<$<CONFIG:Release>: -O3>
    )
endif()

target_compile_definitions(RKengServer PRIVATE
    $<$<CONFIG:Debug>:   RK_DEBUG>
    $<$<CONFIG:Release>: RK_RELEASE>
)

message(STATUS "[RKengServer] Configured.")
