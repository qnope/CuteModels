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

# Qt < 6.6 headers (e.g. qvarlengtharray.h) use stdext::checked_array_iterator,
# which current MSVC STL disables by default. Restore the escape hatch so those
# LTS versions still compile with an up-to-date MSVC.
if(MSVC AND Qt6_VERSION VERSION_LESS 6.6)
    add_compile_definitions(
        _HAS_DEPRECATED_STDEXT_ARR_ITERS=1
        _SILENCE_STDEXT_ARR_ITERS_DEPRECATION_WARNING)
endif()

add_subdirectory(CuteModel)
add_subdirectory(CuteModelQml)

if(CUTEMODEL_BUILD_EXAMPLES)
    add_subdirectory(Examples)
endif()

if(CUTEMODEL_BUILD_TESTS)
    enable_testing()
    add_subdirectory(Tests)
endif()
