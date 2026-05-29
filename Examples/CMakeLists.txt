add_executable(refobject)

set_target_properties(refobject PROPERTIES AUTOMOC ON)

target_sources(refobject PRIVATE refobject/main.cpp)

target_link_libraries(refobject PRIVATE
    CuteModel::CuteModel
    Qt6::Core)
