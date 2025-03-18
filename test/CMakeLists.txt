rock_gtest(test_suite suite.cpp
                      test_RTPTask.cpp
    DEPS_PKGCONFIG gstreamer-1.0
                   gstreamer-tasks-gnulinux
                   base-types
) 

target_link_libraries(test_suite ${GSTREAMER_TASKLIB_DEPENDENT_LIBRARIES})