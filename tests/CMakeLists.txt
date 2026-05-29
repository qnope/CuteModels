find_package(GTest CONFIG REQUIRED)

include(GoogleTest)

add_executable(cutemodel_tests
    test_variant_wrapper.cpp
    test_ref.cpp)

set_target_properties(cutemodel_tests PROPERTIES AUTOMOC ON)

target_link_libraries(cutemodel_tests PRIVATE
    CuteModel::CuteModel
    Qt6::Core
    GTest::gtest
    GTest::gtest_main)

gtest_discover_tests(cutemodel_tests)
