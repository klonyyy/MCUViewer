cmake_minimum_required(VERSION 3.8)

if(MAKE_TESTS)
    add_subdirectory(test)
    return()
endif()

if(NOT DEFINED PLATFORM)
    set(PLATFORM "native")
    message("Compiling for native system")
elseif("${PLATFORM}" STREQUAL "WIN")
    message("Cross - compiling for Windows")
    set(CMAKE_CXX_COMPILER /usr/bin/x86_64-w64-mingw32-g++-posix)
    set(CMAKE_C_COMPILER /usr/bin/x86_64-w64-mingw32-gcc-posix)
    set(CMAKE_RC_COMPILER /usr/bin/x86_64-w64-mingw32-windres)
    set(CMAKE_SYSTEM_NAME Windows)
endif()

project(STMViewer)

set(STMVIEWER_VERSION 0.1.2)

set(CMAKE_BUILD_TYPE Release)
set(CMAKE_CXX_STANDARD 17)

add_compile_options(-Wall
    -Wextra
    -Wno-unused-parameter
    -Wno-missing-field-initializers)

if(UNIX)
    set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules)
    include(${CMAKE_MODULE_PATH}/Findlibusb.cmake)
    find_package(libusb REQUIRED)
    find_package(glfw3 REQUIRED)
    set(STLINK_LINUX ${CMAKE_CURRENT_SOURCE_DIR}/third_party/stlink/lib/linux/libstlink.a)
    set(SPDLOG_LINUX ${CMAKE_CURRENT_SOURCE_DIR}/third_party/spdlog/lib/linux/libspdlog.so.1.11.0)
    set(INSTALL_PATH /usr/local/STMViewer)
    set(DESKTOP_FILE_PATH /usr/share/applications)
endif()

if(WIN32)
    enable_language("RC")
    set(ICON_RC "${CMAKE_CURRENT_SOURCE_DIR}/launch/icon.rc")
    set(GLFW3_WINDOWS ${CMAKE_CURRENT_SOURCE_DIR}/third_party/GLFW/lib/windows/glfw3.dll)
    set(STLINK_WINDOWS ${CMAKE_CURRENT_SOURCE_DIR}/third_party/stlink/lib/windows/libstlink.a)
    set(LIBUSB_WINDOWS ${CMAKE_CURRENT_SOURCE_DIR}/third_party/libusb/lib/windows/libusb-1.0.dll)
    set(LIBUSB_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/libusb/inc/libusb-1.0)
    set(SPDLOG_WINDOWS ${CMAKE_CURRENT_SOURCE_DIR}/third_party/spdlog/lib/windows/libspdlog.dll)
    set(INSTALL_PATH bin)
endif()



set(PROJECT_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ElfReader/ElfReader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Gui/Gui.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Gui/GuiPlots.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/TargetMemoryHandler/TargetMemoryHandler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/TargetMemoryHandler/StlinkHandler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Plot/Plot.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Variable/Variable.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ConfigHandler/ConfigHandler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Plot/PlotHandler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/FileHandler/NFDFileHandler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ImguiPlugins/ImguiPlugins.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/stlink/inc/spdlogWrapper.cpp)

set(IMGUI_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui/imgui.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui/imgui_demo.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui/imgui_draw.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui/imgui_tables.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui/imgui_widgets.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui/backends/imgui_impl_glfw.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui/backends/imgui_impl_opengl3.cpp)

set(IMPLOT_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/implot/implot.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/implot/implot_demo.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/implot/implot_items.cpp)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/third_party/nfd/)

set(EXECUTABLE ${CMAKE_PROJECT_NAME})

add_executable(${EXECUTABLE} ${PROJECT_SOURCES} ${IMGUI_SOURCES} ${IMPLOT_SOURCES} ${ICON_RC})

string(REGEX MATCH "([0-9]+)\\.([0-9]+)\\.([0-9]+)" _ "${STMVIEWER_VERSION}")
target_compile_definitions(${EXECUTABLE}
    PRIVATE STMVIEWER_VERSION_MAJOR=${CMAKE_MATCH_1}
    PRIVATE STMVIEWER_VERSION_MINOR=${CMAKE_MATCH_2}
    PRIVATE STMVIEWER_VERSION_REVISION=${CMAKE_MATCH_3}
)

target_include_directories(${EXECUTABLE} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ElfReader
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Gui
    ${CMAKE_CURRENT_SOURCE_DIR}/src/TargetMemoryHandler
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Plot
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ScrollingBuffer
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Variable
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ConfigHandler
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ImguiPlugins
    ${CMAKE_CURRENT_SOURCE_DIR}/src/FileHandler)

target_include_directories(${EXECUTABLE} SYSTEM PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/stlink/inc/
    ${LIBUSB_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui/backends/
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui/
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/GLFW/inc/
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/GLFW/inc/GLFW/
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/implot/
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/mINI/src/mini/
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/nfd/src/include/
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/spdlog/inc/)

if(UNIX)
    target_link_libraries(${EXECUTABLE} ${STLINK_LINUX} ${LIBUSB_LIBRARY} ${SPDLOG_LINUX} pthread dl GL glfw nfd)
elseif(WIN32)
    target_link_libraries(${EXECUTABLE} ${GLFW3_WINDOWS} ${STLINK_WINDOWS} ${LIBUSB_WINDOWS} ${SPDLOG_WINDOWS} -static opengl32 nfd -static-libstdc++ -static-libgcc)
endif()

if(WIN32)
    add_custom_command(TARGET ${EXECUTABLE} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${GLFW3_WINDOWS}
        ${CMAKE_BINARY_DIR}/glfw3.dll)

    add_custom_command(TARGET ${EXECUTABLE} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${LIBUSB_WINDOWS}
        ${CMAKE_BINARY_DIR}/libusb-1.0.dll)
endif()

add_custom_command(TARGET ${EXECUTABLE} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui.ini
    ${CMAKE_BINARY_DIR}/imgui.ini)

install(TARGETS ${EXECUTABLE} RUNTIME DESTINATION ${INSTALL_PATH} COMPONENT applications)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/imgui.ini DESTINATION ${INSTALL_PATH})

if(WIN32)
    if(PRODUCTION)
        set_target_properties(${EXECUTABLE} PROPERTIES WIN32_EXECUTABLE TRUE)
    endif()
    install(FILES ${LIBUSB_WINDOWS} ${STLINK_WINDOWS} ${GLFW3_WINDOWS} DESTINATION ${INSTALL_PATH})
    set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "
    SetOutPath \\\"$INSTDIR\\\\bin\\\"
    CreateShortCut \\\"$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\${EXECUTABLE}.lnk\\\" \\\"$INSTDIR\\\\bin\\\\${EXECUTABLE}.exe\\\"
    CreateShortCut \\\"$DESKTOP\\\\${EXECUTABLE}.lnk\\\" \\\"$INSTDIR\\\\bin\\\\${EXECUTABLE}.exe\\\"")
    set(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS "
    Delete \\\"$SMPROGRAMS\\\\$START_MENU\\\\${EXECUTABLE}.lnk\\\"
    Delete \\\"$DESKTOP\\\\${EXECUTABLE}.lnk\\\"")
endif()

if(UNIX)
    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/third_party/stlink/chips DESTINATION ${INSTALL_PATH})
    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/launch/icon.png DESTINATION ${INSTALL_PATH})
    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/launch/STMViewer.desktop
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
        DESTINATION ${DESKTOP_FILE_PATH})
    set(CPACK_GENERATOR "DEB")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libglfw3 | libglfw3-wayland, libgl1, libglib2.0-0, libgtk-3-0, libstdc++6, libusb-1.0-0, gdb")
endif()

set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${STMVIEWER_VERSION})
set(CPACK_PACKAGE_VERSION ${CPACK_PROJECT_VERSION})
set(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE)
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Piotr Wasilewski <piwasilewski@interia.pl>")
set(CPACK_PACKAGE_VENDOR "klonyyy")
set(CPACK_PACKAGE_DESCRIPTION "STM32 no-overhead realtime variable viewer.")
set(CPACK_NSIS_CREATE_UNINSTALLER ON)
set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
set(CPACK_NSIS_MODIFY_PATH ON)
set(CPACK_ALL_INSTALL_TYPES Full Developer)
set(CPACK_COMPONENT_APPLICATIONS_INSTALL_TYPES Full)
include(CPack)