cmake_minimum_required(VERSION 3.14)
project(LavaFrame)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set( SDL_STATIC ON CACHE BOOL "" FORCE )
set( SDL_SHARED OFF CACHE BOOL "" FORCE )
set( OIDN_STATIC_LIB ON )
set( BUILD_SHARED_LIBS OFF )

set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

file(GLOB LAVAFRAME_SOURCES
    ${CMAKE_SOURCE_DIR}/LavaFrame/Sources/*.cpp
    ${CMAKE_SOURCE_DIR}/LavaFrame/Sources/core/*.cpp
    ${CMAKE_SOURCE_DIR}/LavaFrame/Sources/loaders/*.cpp
)

file(GLOB imgui

    ${CMAKE_SOURCE_DIR}/LavaFrame/Headers/*.h
    ${CMAKE_SOURCE_DIR}/LavaFrame/Headers/math/*.h
)

file(GLOB THIRDPARTY_SOURCES
    
    ${CMAKE_SOURCE_DIR}/LavaFrame/thirdparty/RadeonRays/*.h
    ${CMAKE_SOURCE_DIR}/LavaFrame/thirdparty/gl3w/GL/*.h
    ${CMAKE_SOURCE_DIR}/LavaFrame/thirdparty/imgui/*.h
    ${CMAKE_SOURCE_DIR}/LavaFrame/thirdparty/tinydir/*.h
    ${CMAKE_SOURCE_DIR}/LavaFrame/thirdparty/stb/*.h
    ${CMAKE_SOURCE_DIR}/LavaFrame/thirdparty/imguizmo/*.h

    ${CMAKE_SOURCE_DIR}/LavaFrame/thirdparty/imgui/*.cpp
    ${CMAKE_SOURCE_DIR}/LavaFrame/thirdparty/RadeonRays/*.cpp
    ${CMAKE_SOURCE_DIR}/LavaFrame/thirdparty/imguizmo/*.cpp    
    ${CMAKE_SOURCE_DIR}/LavaFrame/thirdparty/gl3w/*.c
)

add_executable(Main ${LAVAFRAME_SOURCES} ${LAVAFRAME_HEADERS} ${THIRDPARTY_SOURCES})

add_subdirectory(${CMAKE_SOURCE_DIR}/LavaFrame/thirdparty/SDL)
add_subdirectory(${CMAKE_SOURCE_DIR}/LavaFrame/thirdparty/oidn_git)

add_library(SDL2::SDL2 ALIAS SDL2-static)

set_target_properties(
    OpenImageDenoise oidnBenchmark
    oidnDenoise oidnTest uninstall utils
    common SDL2-static SDL2main dnnl 
    PROPERTIES FOLDER Deps
)

target_include_directories(Main PUBLIC    
    ${CMAKE_SOURCE_DIR}/LavaFrame/Headers
    ${CMAKE_SOURCE_DIR}/LavaFrame/Headers/math
    ${CMAKE_SOURCE_DIR}/LavaFrame/thirdparty/RadeonRays
    ${CMAKE_SOURCE_DIR}/LavaFrame/thirdparty/gl3w
    ${CMAKE_SOURCE_DIR}/LavaFrame/thirdparty/imgui
    ${CMAKE_SOURCE_DIR}/LavaFrame/thirdparty/tinydir
    ${CMAKE_SOURCE_DIR}/LavaFrame/thirdparty/stb
    ${CMAKE_SOURCE_DIR}/LavaFrame/thirdparty/imguizmo
)
target_link_libraries(Main SDL2main SDL2-static OpenImageDenoise opengl32 ${CMAKE_DL_LIBS})

source_group(
    TREE ${CMAKE_SOURCE_DIR}
    FILES ${LAVAFRAME_SOURCES}
)

source_group(
    TREE ${CMAKE_SOURCE_DIR}
    FILES ${LAVAFRAME_HEADERS}
)

source_group(
    TREE ${CMAKE_SOURCE_DIR}
    FILES ${THIRDPARTY_SOURCES}
)
