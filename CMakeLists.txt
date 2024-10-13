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

project(MCUViewer)

set(MCUVIEWER_VERSION 1.0.1)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

if(PRODUCTION)
    set(CMAKE_BUILD_TYPE Release)
    message("Building for production!")
else()
    set(CMAKE_BUILD_TYPE Debug)
endif()

if(WIN32 AND "${PLATFORM}" STREQUAL "native")
    set(LLVM_DIR "C:/msys64/mingw64/bin")
    set(CMAKE_PREFIX_PATH ${LLVM_DIR} ${CMAKE_PREFIX_PATH})
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fuse-ld=lld")
    set(CMAKE_AR "C:/msys64/mingw64/bin/llvm-ar.exe")
endif()

if(CMAKE_BUILD_TYPE STREQUAL "ReleaseWithDbg")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g1")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g1")
endif()

add_compile_options(-Wall
    -Wfatal-errors
    -Wextra
    -Wpedantic
    -Wno-unused-parameter
    -Wno-missing-field-initializers)

if(WIN32)
    add_compile_options(-Wa,-mbig-obj)
endif()

set(JLINK_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/jlink/inc/)

if(UNIX)
    include(FetchContent)
    FetchContent_Declare(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG v1.13.0)

    FetchContent_GetProperties(spdlog)

    if(NOT spdlog_POPULATED)
        FetchContent_Populate(spdlog)
        add_subdirectory(${spdlog_SOURCE_DIR} ${spdlog_BINARY_DIR})
    endif()

    # this is to avoid warnings from spdlog
    get_target_property(SPDLOG_IID spdlog INTERFACE_INCLUDE_DIRECTORIES)
    set_target_properties(spdlog PROPERTIES INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${SPDLOG_IID}")

    set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules)
    include(${CMAKE_MODULE_PATH}/Findlibusb.cmake)
    find_package(libusb REQUIRED)
    find_package(glfw3 REQUIRED)
    set(STLINK_LINUX ${CMAKE_CURRENT_SOURCE_DIR}/third_party/stlink/lib/linux/libstlink.a)
    set(JLINK_LINUX ${CMAKE_CURRENT_SOURCE_DIR}/third_party/jlink/lib/linux/libjlinkarm.so.7)
    set(INSTALL_PATH /usr/local/MCUViewer)
    set(LIB_INSTALL_PATH /usr/local/lib)
    set(DESKTOP_FILE_PATH /usr/share/applications)

    set(LIB_CONF_FILE ${CMAKE_CURRENT_SOURCE_DIR}/launch/install/Unix/MCUViewer.conf)
    set(LIB_CONF_FILE_PATH /etc/ld.so.conf.d/)

    if(APPLE)
        find_package(OpenGL REQUIRED)
        find_package(GLUT REQUIRED)

        # TODO distinguish between ARM and x86 */
        set(STLINK_MACOS ${CMAKE_CURRENT_SOURCE_DIR}/third_party/stlink/lib/macos/libstlink.a)
    else()
        set(STLINK_LINUX ${CMAKE_CURRENT_SOURCE_DIR}/third_party/stlink/lib/linux/libstlink.a)
    endif()
endif()

if(WIN32)
    enable_language("RC")
    set(ICON_RC "${CMAKE_CURRENT_SOURCE_DIR}/launch/install/Windows/icon.rc")
    set(GLFW3_WINDOWS ${CMAKE_CURRENT_SOURCE_DIR}/third_party/GLFW/lib/windows/glfw3.dll)
    set(STLINK_WINDOWS ${CMAKE_CURRENT_SOURCE_DIR}/third_party/stlink/lib/windows/libstlink.a)
    set(JLINK_WINDOWS ${CMAKE_CURRENT_SOURCE_DIR}/third_party/jlink/lib/windows/JLink_x64.dll)
    set(LIBUSB_WINDOWS ${CMAKE_CURRENT_SOURCE_DIR}/third_party/libusb/lib/windows/libusb-1.0.dll)
    set(LIBUSB_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/libusb/inc/libusb-1.0)
    set(SPDLOG_WINDOWS ${CMAKE_CURRENT_SOURCE_DIR}/third_party/spdlog/lib/windows/libspdlog.dll)
    set(GDB_WINDOWS ${CMAKE_CURRENT_SOURCE_DIR}/third_party/GDB/windows/gdb.exe)
    set(INSTALL_PATH bin)
endif()

set(PROJECT_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/commons.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Gui/Gui.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Gui/GuiPlots.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Gui/GuiSwoPlots.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Gui/GuiSwoControl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Gui/GuiAbout.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Gui/GuiImportVariables.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Gui/GuiAcqusition.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Gui/GuiHelper.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/MemoryReader/MemoryReader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/MemoryReader/StlinkDebugProbe.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/MemoryReader/JlinkDebugProbe.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Plot/Plot.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Variable/Variable.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/MovingAverage/MovingAverage.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ConfigHandler/ConfigHandler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/FileHandler/NFDFileHandler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ImguiPlugins/ImguiPlugins.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/stlink/inc/spdlogWrapper.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/PlotHandler/PlotHandlerBase.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/PlotHandler/PlotHandler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/PlotHandler/TracePlotHandler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/TraceReader/TraceReader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/TraceReader/StlinkTraceProbe.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/TraceReader/JlinkTraceProbe.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/GdbParser/GdbParser.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/CSVStreamer/CSVStreamer.cpp)

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

set_source_files_properties(
    ${IMPLOT_SOURCES}
    PROPERTIES
    COMPILE_FLAGS "-w"
)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/third_party/nfd/)

set(EXECUTABLE ${CMAKE_PROJECT_NAME})

add_executable(${EXECUTABLE} ${PROJECT_SOURCES} ${IMGUI_SOURCES} ${IMPLOT_SOURCES} ${ICON_RC})

set(GIT_VERSION_HPP ${CMAKE_CURRENT_SOURCE_DIR}/src/gitversion.hpp)

# Add a custom command to generate gitversion.hpp
add_custom_command(
    OUTPUT ${GIT_VERSION_HPP}
    COMMAND ${CMAKE_COMMAND} -E env PYTHONPATH=${CMAKE_CURRENT_SOURCE_DIR} python3 ${CMAKE_CURRENT_SOURCE_DIR}/launch/addGitVersion.py
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/.git/HEAD
    COMMENT "Generating ${GIT_VERSION_HPP}..."
    VERBATIM
)

add_custom_target(addGitVersion ALL DEPENDS ${GIT_VERSION_HPP})
add_dependencies(${EXECUTABLE} addGitVersion)

string(REGEX MATCH "([0-9]+)\\.([0-9]+)\\.([0-9]+)" _ "${MCUVIEWER_VERSION}")
target_compile_definitions(${EXECUTABLE}
    PRIVATE MCUVIEWER_VERSION_MAJOR=${CMAKE_MATCH_1}
    PRIVATE MCUVIEWER_VERSION_MINOR=${CMAKE_MATCH_2}
    PRIVATE MCUVIEWER_VERSION_REVISION=${CMAKE_MATCH_3}
)

target_include_directories(${EXECUTABLE} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Gui
    ${CMAKE_CURRENT_SOURCE_DIR}/src/MemoryReader
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Plot
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ScrollingBuffer
    ${CMAKE_CURRENT_SOURCE_DIR}/src/MovingAverage
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Variable
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ConfigHandler
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ImguiPlugins
    ${CMAKE_CURRENT_SOURCE_DIR}/src/FileHandler
    ${CMAKE_CURRENT_SOURCE_DIR}/src/PlotHandler
    ${CMAKE_CURRENT_SOURCE_DIR}/src/TraceReader
    ${CMAKE_CURRENT_SOURCE_DIR}/src/RingBuffer
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Statistics
    ${CMAKE_CURRENT_SOURCE_DIR}/src/GdbParser
    ${CMAKE_CURRENT_SOURCE_DIR}/src/CSVStreamer)

target_include_directories(${EXECUTABLE} SYSTEM PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/stlink/inc/
    ${JLINK_INCLUDE_DIR}
    ${LIBUSB_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui/backends/
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui/
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/GLFW/inc/
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/GLFW/inc/GLFW/
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/implot/
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/mINI/src/mini/
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/nfd/src/include/
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/spdlog/inc/
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/CLI11/inc/)

if(APPLE)
    include_directories(${OPENGL_INCLUDE_DIRS} ${GLUT_INCLUDE_DIRS} ${STLINK_INCLUDE_DIRS})
    target_link_libraries(${EXECUTABLE} ${STLINK_MACOS} ${LIBUSB_LIBRARY} ${OPENGL_LIBRARIES} ${GLUT_LIBRARY} spdlog::spdlog pthread dl glfw nfd)
elseif(UNIX)
    target_link_libraries(${EXECUTABLE} ${STLINK_LINUX} ${LIBUSB_LIBRARY} ${JLINK_LINUX} spdlog::spdlog pthread dl GL glfw nfd)
elseif(WIN32)
    target_link_libraries(${EXECUTABLE} ${GLFW3_WINDOWS} ${STLINK_WINDOWS} ${LIBUSB_WINDOWS} ${SPDLOG_WINDOWS} ${JLINK_WINDOWS} -static ssp opengl32 nfd -static-libstdc++ -static-libgcc)
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

    add_custom_command(TARGET ${EXECUTABLE} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${GDB_WINDOWS}
        ${CMAKE_BINARY_DIR}/gdb.exe)

    add_custom_command(TARGET ${EXECUTABLE} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${JLINK_WINDOWS}
        ${CMAKE_BINARY_DIR})
endif()

add_custom_command(TARGET ${EXECUTABLE} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui.ini
    ${CMAKE_BINARY_DIR}/imgui.ini)

install(TARGETS ${EXECUTABLE} RUNTIME DESTINATION ${INSTALL_PATH} COMPONENT applications)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/imgui.ini DESTINATION ${INSTALL_PATH})
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/third_party/stlink/chips DESTINATION ${INSTALL_PATH})

if(WIN32)
    if(PRODUCTION)
        set_target_properties(${EXECUTABLE} PROPERTIES WIN32_EXECUTABLE TRUE)
    endif()

    install(FILES ${LIBUSB_WINDOWS} ${GLFW3_WINDOWS} ${GDB_WINDOWS} ${JLINK_WINDOWS} DESTINATION ${INSTALL_PATH})
    set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "
    SetOutPath \\\"$INSTDIR\\\\bin\\\"
    CreateShortCut \\\"$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\${EXECUTABLE}.lnk\\\" \\\"$INSTDIR\\\\bin\\\\${EXECUTABLE}.exe\\\"
    CreateShortCut \\\"$DESKTOP\\\\${EXECUTABLE}.lnk\\\" \\\"$INSTDIR\\\\bin\\\\${EXECUTABLE}.exe\\\"")
    set(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS "
    Delete \\\"$SMPROGRAMS\\\\$START_MENU\\\\${EXECUTABLE}.lnk\\\"
    Delete \\\"$DESKTOP\\\\${EXECUTABLE}.lnk\\\"")
endif()

if(UNIX)
    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/launch/install/Unix/icon.png DESTINATION ${INSTALL_PATH})
    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/launch/install/Unix/MCUViewer.desktop
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
        DESTINATION ${DESKTOP_FILE_PATH})
    install(FILES ${JLINK_LINUX}
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
        DESTINATION ${LIB_INSTALL_PATH})
    install(FILES ${LIB_CONF_FILE}
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
        DESTINATION ${LIB_CONF_FILE_PATH})
    set(CPACK_GENERATOR "DEB;RPM")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libglfw3 | libglfw3-wayland, libgl1, libglib2.0-0, libgtk-3-0, libstdc++6, libusb-1.0-0, gdb")
endif()

set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${MCUVIEWER_VERSION})
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
set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${PROJECT_SOURCE_DIR}/launch/install/Unix/postinst")
include(CPack)