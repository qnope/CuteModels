option(CUTEMODEL_ENABLE_SANITIZERS
    "Build with AddressSanitizer and UndefinedBehaviorSanitizer" OFF)

add_library(cutemodel_sanitizers INTERFACE)
add_library(CuteModel::Sanitizers ALIAS cutemodel_sanitizers)

if(CUTEMODEL_ENABLE_SANITIZERS)
    if(MSVC)
        # MSVC only implements AddressSanitizer. CMake's default Debug flags
        # include /RTC1, which is incompatible with /fsanitize=address, so it
        # has to be stripped here (this file is included from the top-level
        # scope, before any target is created).
        string(REPLACE "/RTC1" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
        target_compile_options(cutemodel_sanitizers INTERFACE /fsanitize=address)
        # ASan does not support incremental linking (CMake's Debug default).
        target_link_options(cutemodel_sanitizers INTERFACE /INCREMENTAL:NO)
    else()
        target_compile_options(cutemodel_sanitizers INTERFACE
            -fsanitize=address,undefined
            -fno-omit-frame-pointer
            -fno-sanitize-recover=all)
        target_link_options(cutemodel_sanitizers INTERFACE
            -fsanitize=address,undefined)
    endif()
endif()
