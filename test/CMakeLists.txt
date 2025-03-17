rock_gtest(test_suite suite.cpp
                      test_RTPTask.cpp
    DEPS_PKGCONFIG gstreamer-1.0
                   orogen-orogen_default_gstreamer__RTPTask
                   orogen-project-gstreamer
) 

target_link_libraries(test_suite ${GSTREAMER_TASKLIB_DEPENDENT_LIBRARIES})