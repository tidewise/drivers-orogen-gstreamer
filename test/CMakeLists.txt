rock_gtest(test_suite suite.cpp
            test_RTPTask.cpp
    SOURCES ../tasks/RTPHelpers.cpp
    HEADERS ../tasks/RTPHelpers.hpp
    DEPS_PKGCONFIG gstreamer-1.0
                   base-types
) 

target_link_libraries(test_suite ${GSTREAMER_TASKLIB_DEPENDENT_LIBRARIES})