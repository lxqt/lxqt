set(PROJECT_NAME lxqt-globalkeysd)
project(${PROJECT_NAME})

find_package(X11)
include_directories("${X11_INCLUDE_DIR}")

set(QT_DBUS_PREFIX "org.qtproject")

set(${PROJECT_NAME}_SOURCES
    main.cpp
    core.cpp
    daemon_adaptor.cpp
    native_adaptor.cpp
    client_proxy.cpp
    log_target.cpp
    pipe_utils.cpp
    string_utils.cpp
    base_action.cpp
    method_action.cpp
    client_action.cpp
    command_action.cpp
    meta_types.cpp
)

set(${PROJECT_NAME}_CPP_HEADERS
    log_target.h
    pipe_utils.h
    string_utils.h
    base_action.h
    method_action.h
    client_action.h
    command_action.h
    meta_types.h
)

set(${PROJECT_NAME}_FORMS
)

set(${PROJECT_NAME}_RESOURCES
)

set(${PROJECT_NAME}_TRANSLATIONS
)

set(${PROJECT_NAME}_DBUS_ADAPTORS
    org.lxqt.global_key_shortcuts.daemon.xml
    org.lxqt.global_key_shortcuts.native.xml
)

set_source_files_properties(org.lxqt.global_key_shortcuts.daemon.xml PROPERTIES
    INCLUDE daemon_adaptor.h
    PARENT_CLASSNAME DaemonAdaptor
    BASENAME org.lxqt.global_key_shortcuts.daemon
    CLASSNAME OrgLxqtGlobalActionDaemonAdaptor
)

set_source_files_properties(org.lxqt.global_key_shortcuts.native.xml PROPERTIES
    INCLUDE native_adaptor.h
    PARENT_CLASSNAME NativeAdaptor
    BASENAME org.lxqt.global_key_shortcuts.native
    CLASSNAME OrgLxqtGlobalActionNativeAdaptor
)

set(${PROJECT_NAME}_DBUS_INTERFACES
    org.lxqt.global_key_shortcuts.client.xml
)

set_source_files_properties(org.lxqt.global_key_shortcuts.client.xml PROPERTIES
    BASENAME org.lxqt.global_key_shortcuts.client
)

set(${PROJECT_NAME}_HEADERS
    ${${PROJECT_NAME}_CPP_HEADERS}
)

set(${PROJECT_NAME}_TRANSLATABLE
    ${${PROJECT_NAME}_SOURCES}
    ${${PROJECT_NAME}_HEADERS}
    ${${PROJECT_NAME}_FORMS}
)

foreach(DBUS_ADAPTOR ${${PROJECT_NAME}_DBUS_ADAPTORS})
    get_filename_component(DBUS_ADAPTOR_FILENAME ${DBUS_ADAPTOR} NAME)
    configure_file(
        ${DBUS_ADAPTOR}
        "${CMAKE_CURRENT_BINARY_DIR}/${DBUS_ADAPTOR_FILENAME}"
        @ONLY
    )
    get_source_file_property(DBUS_ADAPTOR_INCLUDE ${DBUS_ADAPTOR} INCLUDE)
    get_source_file_property(DBUS_ADAPTOR_PARENT_CLASSNAME ${DBUS_ADAPTOR} PARENT_CLASSNAME)
    get_source_file_property(DBUS_ADAPTOR_BASENAME ${DBUS_ADAPTOR} BASENAME)
    get_source_file_property(DBUS_ADAPTOR_CLASSNAME ${DBUS_ADAPTOR} CLASSNAME)
    if(DBUS_ADAPTOR_BASENAME)
        if(DBUS_ADAPTOR_CLASSNAME)
            qt5_add_dbus_adaptor(${PROJECT_NAME}_DBUS_ADAPTOR_FILES "${CMAKE_CURRENT_BINARY_DIR}/${DBUS_ADAPTOR_FILENAME}" ${DBUS_ADAPTOR_INCLUDE} ${DBUS_ADAPTOR_PARENT_CLASSNAME} ${DBUS_ADAPTOR_BASENAME} ${DBUS_ADAPTOR_CLASSNAME})
        else()
            qt5_add_dbus_adaptor(${PROJECT_NAME}_DBUS_ADAPTOR_FILES "${CMAKE_CURRENT_BINARY_DIR}/${DBUS_ADAPTOR_FILENAME}" ${DBUS_ADAPTOR_INCLUDE} ${DBUS_ADAPTOR_PARENT_CLASSNAME} ${DBUS_ADAPTOR_BASENAME})
        endif()
    else()
        qt5_add_dbus_adaptor(${PROJECT_NAME}_DBUS_ADAPTOR_FILES "${CMAKE_CURRENT_BINARY_DIR}/${DBUS_ADAPTOR_FILENAME}" ${DBUS_ADAPTOR_INCLUDE} ${DBUS_ADAPTOR_PARENT_CLASSNAME})
    endif()
endforeach()

foreach(DBUS_INTERFACE ${${PROJECT_NAME}_DBUS_INTERFACES})
    get_filename_component(DBUS_INTERFACE_FILENAME ${DBUS_INTERFACE} NAME)
    configure_file(
        ${DBUS_INTERFACE}
        "${CMAKE_CURRENT_BINARY_DIR}/${DBUS_INTERFACE_FILENAME}"
        @ONLY
    )
    get_source_file_property(DBUS_INTERFACE_BASENAME ${DBUS_INTERFACE} BASENAME)
    get_source_file_property(DBUS_INTERFACE_INCLUDE ${DBUS_INTERFACE} INCLUDE)
    get_source_file_property(DBUS_INTERFACE_CLASSNAME ${DBUS_INTERFACE} CLASSNAME)
    get_source_file_property(DBUS_INTERFACE_NO_NAMESPACE ${DBUS_INTERFACE} NO_NAMESPACE)
    set_source_files_properties("${CMAKE_CURRENT_BINARY_DIR}/${DBUS_INTERFACE_FILENAME}" PROPERTIES
        INCLUDE ${DBUS_INTERFACE_INCLUDE}
        CLASSNAME ${DBUS_INTERFACE_CLASSNAME}
        NO_NAMESPACE ${DBUS_INTERFACE_NO_NAMESPACE}
    )
    qt5_add_dbus_interface(${PROJECT_NAME}_DBUS_INTERFACE_FILES "${CMAKE_CURRENT_BINARY_DIR}/${DBUS_INTERFACE_FILENAME}" ${DBUS_INTERFACE_BASENAME})
endforeach()

qt5_add_resources(${PROJECT_NAME}_RESOURCE_FILES ${${PROJECT_NAME}_RESOURCES})

set(${PROJECT_NAME}_GENERATED_FILES
    ${${PROJECT_NAME}_FORM_FILES}
    ${${PROJECT_NAME}_RESOURCE_FILES}
    ${${PROJECT_NAME}_QM_FILES}
    ${${PROJECT_NAME}_DBUS_INTERFACE_FILES}
    ${${PROJECT_NAME}_DBUS_ADAPTOR_FILES}
)

set(${PROJECT_NAME}_ALL_FILES
    ${${PROJECT_NAME}_SOURCES}
    ${${PROJECT_NAME}_HEADERS}
    ${${PROJECT_NAME}_GENERATED_FILES}
)

set_property(SOURCE
        ${${PROJECT_NAME}_DBUS_INTERFACE_FILES}
        ${${PROJECT_NAME}_DBUS_ADAPTOR_FILES}
    PROPERTY SKIP_AUTOGEN ON
)

add_executable(${PROJECT_NAME} ${${PROJECT_NAME}_ALL_FILES})
target_link_libraries(${PROJECT_NAME} ${X11_LIBRARIES} lxqt Qt5::Widgets Qt5::DBus)
lxqt_enable_target_exceptions(${PROJECT_NAME} PRIVATE)

install(TARGETS
    ${PROJECT_NAME}
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    COMPONENT Runtime
)
