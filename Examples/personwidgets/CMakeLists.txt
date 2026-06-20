add_executable(personwidgets main.cpp)

set_target_properties(personwidgets PROPERTIES AUTOMOC ON)

target_link_libraries(personwidgets PRIVATE
    cutemodel_example_common
    Qt6::Widgets)
