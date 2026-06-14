add_executable(refobject)

set_target_properties(refobject PROPERTIES AUTOMOC ON)

target_sources(refobject PRIVATE refobject/main.cpp)

target_link_libraries(refobject PRIVATE
    CuteModel::CuteModel
    Qt6::Core)

find_package(Qt6 REQUIRED COMPONENTS Widgets Quick Qml QuickControls2)

qt_standard_project_setup()

add_library(cutemodel_examples_common STATIC
    common/Person.h
    common/PersonModels.h
    common/PersonModels.cpp
    common/PersonViewController.h
    common/PersonViewController.cpp)

set_target_properties(cutemodel_examples_common PROPERTIES AUTOMOC ON)

target_include_directories(cutemodel_examples_common PUBLIC common)

target_link_libraries(cutemodel_examples_common PUBLIC
    CuteModel::CuteModel
    Qt6::Core)

add_executable(widgets_list)
set_target_properties(widgets_list PROPERTIES AUTOMOC ON)
target_sources(widgets_list PRIVATE widgets_list/main.cpp)
target_link_libraries(widgets_list PRIVATE cutemodel_examples_common Qt6::Widgets)

add_executable(widgets_table)
set_target_properties(widgets_table PROPERTIES AUTOMOC ON)
target_sources(widgets_table PRIVATE widgets_table/main.cpp)
target_link_libraries(widgets_table PRIVATE cutemodel_examples_common Qt6::Widgets)

add_executable(widgets_tree)
set_target_properties(widgets_tree PROPERTIES AUTOMOC ON)
target_sources(widgets_tree PRIVATE widgets_tree/main.cpp)
target_link_libraries(widgets_tree PRIVATE cutemodel_examples_common Qt6::Widgets)

qt_add_executable(qml_list qml_list/main.cpp)
qt_add_qml_module(qml_list
    URI CuteListExample
    VERSION 1.0
    RESOURCE_PREFIX "/"
    QML_FILES qml_list/Main.qml)
target_link_libraries(qml_list PRIVATE cutemodel_examples_common Qt6::Quick Qt6::Qml)

qt_add_executable(qml_table qml_table/main.cpp)
qt_add_qml_module(qml_table
    URI CuteTableExample
    VERSION 1.0
    RESOURCE_PREFIX "/"
    QML_FILES qml_table/Main.qml)
target_link_libraries(qml_table PRIVATE cutemodel_examples_common Qt6::Quick Qt6::Qml)

qt_add_executable(qml_tree qml_tree/main.cpp)
qt_add_qml_module(qml_tree
    URI CuteTreeExample
    VERSION 1.0
    RESOURCE_PREFIX "/"
    QML_FILES qml_tree/Main.qml)
target_link_libraries(qml_tree PRIVATE cutemodel_examples_common Qt6::Quick Qt6::Qml)
