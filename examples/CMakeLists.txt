add_executable(refobject refobject/main.cpp)

set_target_properties(refobject PROPERTIES AUTOMOC ON)

target_link_libraries(refobject PRIVATE
    CuteModel::CuteModel
    Qt6::Core)
