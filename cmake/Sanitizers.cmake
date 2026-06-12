option(CUTEMODEL_ENABLE_SANITIZERS
    "Build with AddressSanitizer and UndefinedBehaviorSanitizer" OFF)

add_library(cutemodel_sanitizers INTERFACE)
add_library(CuteModel::Sanitizers ALIAS cutemodel_sanitizers)

if(CUTEMODEL_ENABLE_SANITIZERS)
    if(MSVC)
        # MSVC only implements AddressSanitizer; it is enabled at compile
        # time and the linker picks up the runtime automatically.
        target_compile_options(cutemodel_sanitizers INTERFACE /fsanitize=address)
    else()
        target_compile_options(cutemodel_sanitizers INTERFACE
            -fsanitize=address,undefined
            -fno-omit-frame-pointer
            -fno-sanitize-recover=all)
        target_link_options(cutemodel_sanitizers INTERFACE
            -fsanitize=address,undefined)
    endif()
endif()
