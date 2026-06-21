find_package(Qt6 REQUIRED COMPONENTS Qml)

add_library(CuteModelQml INTERFACE)
add_library(CuteModel::Qml ALIAS CuteModelQml)

target_sources(CuteModelQml
    INTERFACE
        FILE_SET cutemodelqml_headers TYPE HEADERS
        BASE_DIRS ${CMAKE_SOURCE_DIR}/CuteModel/include
        FILES ${CMAKE_SOURCE_DIR}/CuteModel/include/CuteModel/RefMap.h)

target_compile_features(CuteModelQml INTERFACE cxx_std_17)

target_link_libraries(CuteModelQml INTERFACE
    CuteModel::CuteModel
    Qt6::Qml)

include(GNUInstallDirs)

install(TARGETS CuteModelQml
    EXPORT CuteModelTargets
    FILE_SET cutemodelqml_headers DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
