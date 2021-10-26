cmake_minimum_required(VERSION 3.14)
project(LavaFrame)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

add_subdirectory(${CMAKE_SOURCE_DIR}/LavaFrame/thirdparty/SDL)
add_subdirectory(${CMAKE_SOURCE_DIR}/LavaFrame/thirdparty/oidn_git)

set_target_properties(
    SDL2 OpenImageDenoise oidnBenchmark
    oidnDenoise oidnTest uninstall utils
    common SDL2-static SDL2main dnnl 
    PROPERTIES FOLDER Deps
)

file(GLOB LAVAFRAME_SOURCES
    ${CMAKE_SOURCE_DIR}/LavaFrame/Sources/*.cpp
    ${CMAKE_SOURCE_DIR}/LavaFrame/Sources/core/*.cpp
    ${CMAKE_SOURCE_DIR}/LavaFrame/Sources/loaders/*.cpp
)

file(GLOB LAVAFRAME_HEADERS
    ${CMAKE_SOURCE_DIR}/LavaFrame/Headers/*.h
    ${CMAKE_SOURCE_DIR}/LavaFrame/Headers/math/*.h
)

file(GLOB THIRDPARTY_SOURCES
    ${CMAKE_SOURCE_DIR}/LavaFrame/thirdparty/imgui/*.cpp
    ${CMAKE_SOURCE_DIR}/LavaFrame/thirdparty/RadeonRays/*.cpp
    ${CMAKE_SOURCE_DIR}/LavaFrame/thirdparty/imguizmo/*.cpp    
    ${CMAKE_SOURCE_DIR}/LavaFrame/thirdparty/gl3w/*.c

    ${CMAKE_SOURCE_DIR}/LavaFrame/thirdparty/gl3w/*.h
    ${CMAKE_SOURCE_DIR}/LavaFrame/thirdparty/imgui/*.h
    ${CMAKE_SOURCE_DIR}/LavaFrame/thirdparty/tinydir/*.h
    ${CMAKE_SOURCE_DIR}/LavaFrame/thirdparty/stb/*.h
    ${CMAKE_SOURCE_DIR}/LavaFrame/thirdparty/RadeonRays/*.h
    ${CMAKE_SOURCE_DIR}/LavaFrame/thirdparty/imguizmo/*.h
)


add_executable(Main ${LAVAFRAME_SOURCES} ${THIRDPARTY_SOURCES})
target_include_directories(Main PUBLIC    
    ${CMAKE_SOURCE_DIR}/LavaFrame/Headers
    ${CMAKE_SOURCE_DIR}/LavaFrame/Headers/math
)
target_link_libraries(Main OpenImageDenoise GL SDL2::SDL2 ${CMAKE_DL_LIBS})

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