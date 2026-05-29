cmake_minimum_required(VERSION 3.21)

project(CuteModel
    VERSION 0.1.0
    DESCRIPTION "Templated Qt model types"
    LANGUAGES CXX)

option(CUTEMODEL_BUILD_TESTS "Build CuteModel tests" ON)
option(CUTEMODEL_BUILD_EXAMPLES "Build CuteModel examples" ON)

# Locate Qt automatically. If it is not on the default search paths, honor a set
# of environment hints so no absolute path needs to live in the build files.
# CMake already reads CMAKE_PREFIX_PATH from the environment; the loop below adds
# the common Qt-specific variables on top of it.
foreach(_qt_hint QT_ROOT_DIR Qt6_DIR QTDIR CMAKE_PREFIX_PATH)
    if(DEFINED ENV{${_qt_hint}})
        list(PREPEND CMAKE_PREFIX_PATH "$ENV{${_qt_hint}}")
    endif()
endforeach()

find_package(Qt6 REQUIRED COMPONENTS Core)

add_subdirectory(src)

if(CUTEMODEL_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if(CUTEMODEL_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
