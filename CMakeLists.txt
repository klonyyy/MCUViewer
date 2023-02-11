cmake_minimum_required(VERSION 3.5)

if(MAKE_TESTS)
    add_subdirectory(test)
    return()
endif()

project(STMViewer)

set(MDTOOL_VERSION 1.0.0)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules)
include(${CMAKE_MODULE_PATH}/Findlibusb.cmake)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_STANDARD 17)

add_compile_options(-Wall
    -Wextra
    -Wno-unused-parameter
    -Wno-missing-field-initializers)

find_package(libusb REQUIRED)
find_package(SDL2 REQUIRED)

set(PROJECT_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ElfReader/ElfReader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Gui/Gui.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/VarReader/VarReader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Plot/Plot.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Variable/Variable.cpp)

set(IMGUI_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui/imgui.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui/imgui_demo.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui/imgui_draw.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui/imgui_tables.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui/imgui_widgets.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui/backends/imgui_impl_sdl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui/backends/imgui_impl_opengl3.cpp)

set(IMPLOT_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/implot/implot.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/implot/implot_demo.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/implot/implot_items.cpp)

set(EXECUTABLE ${CMAKE_PROJECT_NAME})

add_executable(${EXECUTABLE} ${PROJECT_SOURCES} ${IMGUI_SOURCES} ${IMPLOT_SOURCES})

target_include_directories(${EXECUTABLE} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ElfReader
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Gui
    ${CMAKE_CURRENT_SOURCE_DIR}/src/VarReader
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Plot
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ScrollingBuffer
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Variable)

target_include_directories(${EXECUTABLE} SYSTEM PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/stlink/inc
    ${LIBUSB_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui/backends/
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui/
    ${SDL2_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/implot/)

target_link_libraries(${EXECUTABLE} pthread GL dl SDL2 ${CMAKE_CURRENT_SOURCE_DIR}/third_party/stlink/lib/libstlink.a ${LIBUSB_LIBRARY})