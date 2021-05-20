#ifndef gstreamer_TYPES_HPP
#define gstreamer_TYPES_HPP

#include <string>
#include <base/samples/Frame.hpp>

namespace gstreamer {
    /** Description of an injection of images from an image input port to
     * the gstreamer pipeline
     */
    struct InputConfig {
        /** Input port name */
        std::string name;
        /** Expected pixel format */
        base::samples::frame::frame_mode_t frameMode = base::samples::frame::MODE_RGB;
        /** Expected width */
        size_t width;
        /** Expected height */
        size_t height;
    };

    /** Description of an export from the gstreamer pipeline to an image port */
    struct OutputConfig {
        /** Output name
         *
         * This is the name of both the output port and of the appsink element
         * in the pipeline it will be connected to
         */
        std::string name;
        /** Target pixel format */
        base::samples::frame::frame_mode_t frameMode = base::samples::frame::MODE_RGB;
    };
}

#endif

