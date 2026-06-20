add_executable(refobject)

set_target_properties(refobject PROPERTIES AUTOMOC ON)

target_sources(refobject PRIVATE refobject/main.cpp)

target_link_libraries(refobject PRIVATE
    CuteModel::CuteModel
    Qt6::Core)

find_package(Qt6 REQUIRED COMPONENTS Gui Widgets Qml Quick)

qt_standard_project_setup(REQUIRES 6.5)

add_subdirectory(common)
add_subdirectory(personwidgets)
add_subdirectory(personqml)
