#ifndef gstreamer_TYPES_HPP
#define gstreamer_TYPES_HPP

#include <base/samples/Frame.hpp>
#include <string>

#define GST_USE_UNSTABLE_API
#include <gst/webrtc/webrtc.h>

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

    struct SignallingConfig {
        /** This component's role in the signalling process
         *
         * Cf.
         * https://developer.mozilla.org/en-US/docs/Web/API/WebRTC_API/Perfect_negotiation
         * Note that in the 'polite' role the component will never send an offer
         */
        bool polite = true;
        /** This component's peer ID */
        std::string self_peer_id;
        /** The remote's end peer ID, if known
         *
         * Leave empty if it will be known dynamically (if the component supports it)
         */
        std::string remote_peer_id;
    };

    struct WebRTCPeerStats {
        std::string peer_id;
        GstWebRTCSignalingState signaling_state = GST_WEBRTC_SIGNALING_STATE_STABLE;
        GstWebRTCPeerConnectionState peer_connection_state =
            GST_WEBRTC_PEER_CONNECTION_STATE_NEW;
        GstWebRTCICEConnectionState ice_connection_state =
            GST_WEBRTC_ICE_CONNECTION_STATE_NEW;
        GstWebRTCICEGatheringState ice_gathering_state =
            GST_WEBRTC_ICE_GATHERING_STATE_NEW;
    };

    struct WebRTCSendStats {
        std::vector<WebRTCPeerStats> peers;
    };
}

#endif
