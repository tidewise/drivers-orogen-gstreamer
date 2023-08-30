#ifndef gstreamer_TYPES_HPP
#define gstreamer_TYPES_HPP

#include <base/samples/Frame.hpp>
#include <string>

#define GST_USE_UNSTABLE_API

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
    };


    /** Data channel configuration
     *
     * The WebRTC components will dynamically create ports of type
     * /iodrivers_base/RawPacket to handle data channels. This configuration
     * is used to configure which channels to expect and what do to with them
     */
    struct DataChannelConfig {
        /** The label of the data channel */
        std::string label;
        /** The basename of the ports that will provide an interface to the channel
         *
         * The in port is suffixed with _in, the out port with _out.
         *
         * We recommend using the same value than `name`
         */
        std::string port_basename;
        /** Whether the local component creates the channel or not */
        bool create = false;
        /** Whether data received on the input port (that should be sent through)
         * should be sent as string or binary
         */
        bool binary_in = false;
    };
}

#endif
