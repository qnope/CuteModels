qt_add_executable(personqml main.cpp)

qt_add_qml_module(personqml
    URI CuteExamples
    VERSION 1.0
    QML_FILES Main.qml)

target_link_libraries(personqml PRIVATE
    cutemodel_example_common
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick)
