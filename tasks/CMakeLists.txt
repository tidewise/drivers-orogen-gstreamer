# Generated from orogen/lib/orogen/templates/tasks/CMakeLists.txt

include(gstreamerTaskLib)
ADD_LIBRARY(${GSTREAMER_TASKLIB_NAME} SHARED
    ${GSTREAMER_TASKLIB_SOURCES})
add_dependencies(${GSTREAMER_TASKLIB_NAME}
    regen-typekit)


TARGET_LINK_LIBRARIES(${GSTREAMER_TASKLIB_NAME}
    ${OrocosRTT_LIBRARIES}
    ${QT_LIBRARIES}
    ${GSTREAMER_TASKLIB_DEPENDENT_LIBRARIES})
SET_TARGET_PROPERTIES(${GSTREAMER_TASKLIB_NAME}
    PROPERTIES LINK_INTERFACE_LIBRARIES "${GSTREAMER_TASKLIB_INTERFACE_LIBRARIES}")
SET_TARGET_PROPERTIES(${GSTREAMER_TASKLIB_NAME}
    PROPERTIES INTERFACE_LINK_LIBRARIES "${GSTREAMER_TASKLIB_INTERFACE_LIBRARIES}")

INSTALL(TARGETS ${GSTREAMER_TASKLIB_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib/orocos)

INSTALL(FILES ${GSTREAMER_TASKLIB_HEADERS}
    DESTINATION include/orocos/gstreamer)

