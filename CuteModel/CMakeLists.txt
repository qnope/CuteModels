add_library(CuteModel STATIC)
add_library(CuteModel::CuteModel ALIAS CuteModel)

set_target_properties(CuteModel PROPERTIES AUTOMOC ON)

target_sources(CuteModel
    PRIVATE
        src/RefBase.cpp
    PUBLIC
        FILE_SET cutemodel_headers TYPE HEADERS
        BASE_DIRS include
        FILES
            include/CuteModel/ValueRole.h
            include/CuteModel/RefBase.h
            include/CuteModel/VariantWrapper.h
            include/CuteModel/ItemProxy.h
            include/CuteModel/BaseModel.h
            include/CuteModel/BasicListModel.h)

target_compile_features(CuteModel PUBLIC cxx_std_17)

target_link_libraries(CuteModel PUBLIC Qt6::Core)

# Install / export so the library can be consumed (e.g. via vcpkg).
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(TARGETS CuteModel
    EXPORT CuteModelTargets
    FILE_SET cutemodel_headers DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

install(EXPORT CuteModelTargets
    NAMESPACE CuteModel::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/CuteModel)

configure_package_config_file(
    ${PROJECT_SOURCE_DIR}/cmake/CuteModelConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/CuteModelConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/CuteModel)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/CuteModelConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/CuteModelConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/CuteModelConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/CuteModel)
