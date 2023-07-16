include(FetchContent)

project(STMViewer_test)

set(EXECUTABLE ${CMAKE_PROJECT_NAME})

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
    ${CMAKE_SOURCE_DIR}/src/ElfReader
    ${CMAKE_SOURCE_DIR}/src/Variable
    ${CMAKE_SOURCE_DIR}/src/ScrollingBuffer)

target_link_libraries(GTest::GTest INTERFACE gtest_main gmock gmock_main)

add_executable(${EXECUTABLE} main.cpp
    ElfReaderTest.cpp
    VariableTest.cpp
    ScrollingBufferTest.cpp)

add_compile_options(-Wall -Wextra -Wpedantic -O3)

target_link_libraries(${EXECUTABLE}
    PRIVATE
    GTest::GTest)