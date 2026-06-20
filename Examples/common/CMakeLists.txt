add_library(cutemodel_example_common STATIC
    include/example/Person.h
    include/example/PersonListModel.h
    include/example/PersonRef.h
    include/example/PersonController.h
    src/Person.cpp
    src/PersonListModel.cpp
    src/PersonRef.cpp
    src/PersonController.cpp)

set_target_properties(cutemodel_example_common PROPERTIES AUTOMOC ON)

target_include_directories(cutemodel_example_common PUBLIC include)

target_link_libraries(cutemodel_example_common PUBLIC
    CuteModel::CuteModel
    Qt6::Core)
