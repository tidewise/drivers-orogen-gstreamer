cmake_minimum_required(VERSION 3.1)
project(gstreamer VERSION 0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/.orogen/config")
include(gstreamerBase)

# Do not forget to add syskit as a test dependency in manifest.xml
#  <test_depend name="tools/syskit" />
if (ROCK_TEST_ENABLED)
    enable_testing()
    find_package(Syskit REQUIRED)
    find_package(Rock REQUIRED)
    syskit_orogen_tests(test)
    add_subdirectory(test)

    set_tests_properties(test PROPERTIES
        ENVIRONMENT "ROBY_BASE_LOG_DIR=${ROCK_TEST_LOG_DIR};GST_DEBUG=3,webrtcbin:7;G_DEBUG=fatal_warnings"
    )
endif()

# FIND_PACKAGE(KDL)
# FIND_PACKAGE(OCL)

