find_package(Qt6 REQUIRED COMPONENTS Test)
find_package(GTest CONFIG REQUIRED)

include(GoogleTest)

add_executable(cutemodel_tests)

set_target_properties(cutemodel_tests PROPERTIES AUTOMOC ON)

target_sources(cutemodel_tests PRIVATE
    test_value_role.cpp
    test_ref.cpp
    test_base_model.cpp
    test_basic_list_model.cpp
    test_basic_list_model_columns.cpp
    test_basic_table_model.cpp
    test_range_list.cpp
    test_item_proxy.cpp)

target_include_directories(cutemodel_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(cutemodel_tests PRIVATE
    CuteModel::CuteModel
    Qt6::Core
    Qt6::Test
    GTest::gtest
    GTest::gtest_main)

gtest_discover_tests(cutemodel_tests)
