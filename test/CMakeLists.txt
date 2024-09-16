include(FetchContent)

project(MCUViewer_test)

set(EXECUTABLE ${CMAKE_PROJECT_NAME})

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_STANDARD 20)

enable_testing()

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG release-1.11.0
)
FetchContent_MakeAvailable(googletest)

add_library(GTest::GTest INTERFACE IMPORTED)

include_directories(${EXECUTABLE}
    ${CMAKE_SOURCE_DIR}/src/
    ${CMAKE_SOURCE_DIR}/src/Plot
    ${CMAKE_SOURCE_DIR}/src/ElfReader
    ${CMAKE_SOURCE_DIR}/src/Variable
    ${CMAKE_SOURCE_DIR}/src/ScrollingBuffer
    ${CMAKE_SOURCE_DIR}/src/RingBuffer
    ${CMAKE_SOURCE_DIR}/src/TraceReader
    ${CMAKE_SOURCE_DIR}/src/Statistics
    ${CMAKE_SOURCE_DIR}/src/GdbParser)

include_directories(${EXECUTABLE} SYSTEM PRIVATE
    ${CMAKE_SOURCE_DIR}/third_party/spdlog/inc)

set(SOURCES
    ${CMAKE_SOURCE_DIR}/src/TraceReader/TraceReader.cpp)

target_link_libraries(GTest::GTest INTERFACE gtest_main gmock gmock_main)

add_executable(${EXECUTABLE} main.cpp
    ScrollingBufferTest.cpp
    RingBufferTest.cpp
    TraceReaderTest.cpp
    StatisticsTest.cpp
    GdbParserTest.cpp
    ${SOURCES})

add_compile_options(-Wall -Wextra -Wpedantic)

target_link_libraries(${EXECUTABLE}
    PRIVATE
    GTest::GTest)

# Copy test elf file to binary directory
set(SOURCE_FILE "${CMAKE_CURRENT_SOURCE_DIR}/testFiles/MCUViewer_test.elf")
set(DESTINATION_DIR "${CMAKE_CURRENT_BINARY_DIR}")

add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${SOURCE_FILE}"
        "${DESTINATION_DIR}"
    COMMENT "Copying file to executable directory"
)

add_custom_target(copy_file ALL
    DEPENDS ${SOURCE_FILE}
)

add_dependencies(${EXECUTABLE} copy_file)