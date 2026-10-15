cmake_minimum_required(VERSION 3.25)
project(P900Fixer VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
if(MSVC)
    add_compile_options(/utf-8 /W4 /permissive-)
endif()

find_package(OpenEXR CONFIG REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Widgets Concurrent)

# Headless core: destripe and EXR IO, no Qt.
add_library(p900_core STATIC src/core/Destripe.cpp src/core/ExrFix.cpp)
target_include_directories(p900_core PUBLIC src)
target_link_libraries(p900_core PUBLIC OpenEXR::OpenEXR)

qt_standard_project_setup()
qt_add_executable(P900Fixer WIN32 src/app/main.cpp src/app/MainWindow.cpp)
target_link_libraries(P900Fixer PRIVATE p900_core Qt6::Widgets Qt6::Concurrent)

add_executable(p900fix src/app/cli.cpp)
target_link_libraries(p900fix PRIVATE p900_core)

# Copy Qt's DLLs and plugins beside the .exe so the bin folder runs as is
# (vcpkg's applocal step copies OpenEXR's).
if(WIN32)
    find_program(WINDEPLOYQT windeployqt HINTS ${Qt6_DIR}/../../../bin)
    if(WINDEPLOYQT)
        add_custom_command(TARGET P900Fixer POST_BUILD
            COMMAND ${WINDEPLOYQT} --no-translations --no-system-d3d-compiler --no-opengl-sw
                    --no-compiler-runtime $<TARGET_FILE:P900Fixer>
            COMMENT "windeployqt")
    endif()
endif()

enable_testing()
add_executable(test_destripe tests/test_destripe.cpp)
target_link_libraries(test_destripe PRIVATE p900_core)
add_test(NAME destripe COMMAND test_destripe)
