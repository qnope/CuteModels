cmake_minimum_required(VERSION 3.23)

project(CuteModel
    VERSION 0.1.0
    DESCRIPTION "Templated Qt model types"
    LANGUAGES CXX)

option(CUTEMODEL_BUILD_TESTS "Build CuteModel tests" ON)
option(CUTEMODEL_BUILD_EXAMPLES "Build CuteModel examples" ON)

include(cmake/Sanitizers.cmake)

foreach(_qt_hint QT_ROOT_DIR Qt6_DIR QTDIR CMAKE_PREFIX_PATH)
    if(DEFINED ENV{${_qt_hint}})
        list(PREPEND CMAKE_PREFIX_PATH "$ENV{${_qt_hint}}")
    endif()
endforeach()

find_package(Qt6 REQUIRED COMPONENTS Core)

add_subdirectory(CuteModel)

if(CUTEMODEL_BUILD_EXAMPLES)
    add_subdirectory(Examples)
endif()

if(CUTEMODEL_BUILD_TESTS)
    enable_testing()
    add_subdirectory(Tests)
endif()
