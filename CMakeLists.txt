cmake_minimum_required(VERSION 3.14)
project(LavaFrame)

# Deps
add_subdirectory(${CMAKE_SOURCE_DIR}/LavaFrame/thirdparty/SDL)
add_subdirectory(${CMAKE_SOURCE_DIR}/LavaFrame/thirdparty/oidn_git)

file(GLOB LAVAFRAME_SOURCES
    # Deps
    ${CMAKE_SOURCE_DIR}/LavaFrame/thirdparty/imgui/*.cpp
    ${CMAKE_SOURCE_DIR}/LavaFrame/thirdparty/RadeonRays/*.cpp
    ${CMAKE_SOURCE_DIR}/LavaFrame/thirdparty/imguizmo/*.cpp    
    ${CMAKE_SOURCE_DIR}/LavaFrame/thirdparty/gl3w/*.c

    ${CMAKE_SOURCE_DIR}/LavaFrame/*.cpp
)

include_directories(
    ${CMAKE_SOURCE_DIR}/LavaFrame
    # Deps 
    ${CMAKE_SOURCE_DIR}/LavaFrame/thirdparty/gl3w
    ${CMAKE_SOURCE_DIR}/LavaFrame/thirdparty/imgui
    ${CMAKE_SOURCE_DIR}/LavaFrame/thirdparty/tinydir
    ${CMAKE_SOURCE_DIR}/LavaFrame/thirdparty/stb
    ${CMAKE_SOURCE_DIR}/LavaFrame/thirdparty/RadeonRays
    ${CMAKE_SOURCE_DIR}/LavaFrame/thirdparty/imguizmo
    
)

add_executable(Main ${LAVAFRAME_SOURCES})
target_link_libraries(Main OpenImageDenoise GL SDL2::SDL2 ${CMAKE_DL_LIBS})
