set(CUTEMODEL_PUBLIC_HEADERS
    ${CMAKE_SOURCE_DIR}/include/CuteModel/ValueRole.h
    ${CMAKE_SOURCE_DIR}/include/CuteModel/RefBase.h
    ${CMAKE_SOURCE_DIR}/include/CuteModel/Ref.h
    ${CMAKE_SOURCE_DIR}/include/CuteModel/VariantWrapper.h
    ${CMAKE_SOURCE_DIR}/include/CuteModel/GetAsRefObject.h)

add_library(CuteModel STATIC
    RefBase.cpp
    ${CUTEMODEL_PUBLIC_HEADERS})

add_library(CuteModel::CuteModel ALIAS CuteModel)

set_target_properties(CuteModel PROPERTIES AUTOMOC ON)

target_compile_features(CuteModel PUBLIC cxx_std_17)

target_include_directories(CuteModel PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)

target_link_libraries(CuteModel PUBLIC Qt6::Core)

# Install / export so the library can be consumed (e.g. via vcpkg).
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(TARGETS CuteModel
    EXPORT CuteModelTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/CuteModel
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(EXPORT CuteModelTargets
    NAMESPACE CuteModel::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/CuteModel)

configure_package_config_file(
    ${CMAKE_SOURCE_DIR}/cmake/CuteModelConfig.cmake.in
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
