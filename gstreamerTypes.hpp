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
    };

    /** Description of an export from the gstreamer pipeline to an image port */
    struct OutputConfig {
        /** Output name
         *
         * This is the name of both the output port and of the appsink element
         * in the pipeline it will be connected to
         */
        // Building the Timestamper Estimator with window of at least 100 periods (5s at 30fps) and a period estimation (33ms for 30Hz)
        std::string name;
        /** Target pixel format */
        base::samples::frame::frame_mode_t frame_mode = base::samples::frame::MODE_RGB;

        base::Time window = base::Time::fromSeconds(5);
        base::Time period = base::Time::fromMilliseconds(0);
        int sample_loss_threshold = 2;
    };
}

#endif

