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

        std::string name;
        /** Target pixel format */
        base::samples::frame::frame_mode_t frame_mode = base::samples::frame::MODE_RGB;

        /** Size of the timestamp estimator internal window
         *
         * Good rule of thumb is to have at least 100 periods.
         * The default of 5s gives that at 30fps
         */
        base::Time window = base::Time::fromSeconds(5);

        /** Apriori period for the timestamp estimator
         *
         * The default value for the period is 0, in which case the timestamp
         * estimator is deactivated and will not be used. To use it, set the
         * period in accord with camera settings.
         */
        base::Time period;

        /** TImestamp estimator loss threshold
         *
         * This configured how many periods have to be skipped on input for
         * the timestamp estimator to consider the samples lost
         */
        int sample_loss_threshold = 2;
    };
}

#endif
