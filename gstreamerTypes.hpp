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

        /** How long until we abort an unsuccessful negotiation and try to connect again
         */
        base::Time negotiation_timeout = base::Time::fromSeconds(10);

        /** How long we wait for a signalling message until we abort and try again
         *
         * This is different from negotiation_timeout. messaging_timeout will
         * react quicker, but won't detect that the negotiation goes nowhere.
         */
        base::Time messaging_timeout = base::Time::fromMilliseconds(200);

        /** How long we wait for an offer to arrive before sending an offer request again
         */
        base::Time offer_request_timeout = base::Time::fromMilliseconds(200);

        /** Bundle policy
         *
         * Testing shows that as of GStreamer 1.20, data channels require MAX_BUNDLE
         * when trying to talk between a SendTask and a ReceiveTask. Not sure whether
         * it could work in different settings, but let's stick to max bundle by default
         * for now
         */
        GstWebRTCBundlePolicy bundle_policy = GST_WEBRTC_BUNDLE_POLICY_MAX_BUNDLE;
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
