add_executable(refobject)

set_target_properties(refobject PROPERTIES AUTOMOC ON)

target_sources(refobject PRIVATE refobject/main.cpp)

target_link_libraries(refobject PRIVATE
    CuteModel::CuteModel
    Qt6::Core)

find_package(Qt6 REQUIRED COMPONENTS Gui Widgets Qml Quick)

# Qt's macOS link interface still pulls in the legacy AGL framework, which the
# macOS 15 SDK removed. AGL is unreferenced, so strip it from the imported Qt
# targets to keep the link valid regardless of the SDK or linker in use.
if(APPLE)
    foreach(_qt_target Qt6::Gui Qt6::GuiPrivate Qt6::Widgets Qt6::Qml Qt6::Quick)
        if(TARGET ${_qt_target})
            get_target_property(_qt_libs ${_qt_target} INTERFACE_LINK_LIBRARIES)
            if(_qt_libs)
                list(FILTER _qt_libs EXCLUDE REGEX "AGL")
                set_target_properties(${_qt_target} PROPERTIES
                    INTERFACE_LINK_LIBRARIES "${_qt_libs}")
            endif()
        endif()
    endforeach()
endif()

qt_standard_project_setup(REQUIRES 6.5)

add_subdirectory(common)
add_subdirectory(personwidgets)
add_subdirectory(personqml)
