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

    /** Response from each connected peer to the RTP pipeline */
    struct ReceivedResponse {
        /** The SSRC of this source */
        int rb_ssrc;
        /** The SSRC of the sender of this RR */
        int rb_sender_ssrc;
        /** Lost 8-bit fraction */
        int rb_fractionlost;
        /** Lost packets */
        int rb_packetslost;
        /** Highest received seqnum */
        int rb_exthighestseq;
        /** Reception jitter (in clock rate units) */
        int rb_jitter;
        /** Last SR time (seconds in NTP Short Format, 16.16 fixed point) */
        int rb_lsr;
        /** Delay since last SR (seconds in NTP Short Format, 16.16 fixed point) */
        int rb_dlsr;
        /** The round-trip time (seconds in NTP Short Format, 16.16 fixed point) */
        int rb_round_trip;
    };

    /** The RTP Stream Statistics
     *
     * This is the response when the stats is present on the pipeline
*/
    struct RTPStreamStatistics {
        /** The label of the stream */
        std::string label;

        // The following fields are always present.
        /** SSRC of this source */
        int ssrc;
        /** Confirmation if the source is a source of the session */
        bool internal;
        /** Confirmation if source is validated*/
        bool validated;
        /** Confirmation if a bye has been received */
        bool received_bye;
        /** Confirmation if the source was found as CSRC */
        bool is_csrc;
        /** Confirmation if the source is a sender */
        bool is_sender;
        /** First seqnum if known */
        int seqnum_base;
        /** Clock rate of the media */
        int clock_rate;

        // The following fields are only present when known.
        /** Origin of last received RTP Packet - Only present when known */
        std::string rtp_from;
        /** Origin of last received RTP Packet - Only present when known */
        std::string rtcp_from;

        // The following fields make sense for internal sources and will only increase
        // when "is-sender" is TRUE.
        /** Number of payload bytes we sent */
        int octets_sent;
        /** Number of packets we sent */
        int packets_sent;

        // The following fields make sense for non-internal sources and will only increase
        // when "is-sender" is TRUE.
        /** Total number of payload bytes received */
        int octets_received;
        /** Total number of packets received */
        int packets_received;
        /** Total number of bytes received including lower level headers overhead */
        int bytes_received;

        // Following fields are updated when "is-sender" is TRUE.
        /** Bitrate in bits/sec */
        int bitrate;
        /** Estimated amount of packets lost */
        int packets_lost;
        /** Estimated Jitter (in clock rate units) */
        int jitter;

        // Documentation not available
        int sent_pli_count;
        int recv_pli_count;
        int sent_fir_count;
        int recv_fir_count;
        int sent_nack_count;
        int recv_nack_count;
        int recv_packet_rate;

        // The last SR report this source sent. Only updates when "is-sender" is TRUE.
        /** The source has sent SR */
        bool have_sr;
        /** NTP Time of the SR (in NTP Timestamp Format, 32.32 fixed point) */
        int in_ntptime;
        /** RTP Time of SR (in clock rate units)*/
        int sr_rtptime;
        /** The number of bytes in the SR */
        int sr_octet_count;
        /** The number of packets in the SR */
        int sr_packet_count;

        // The following fields are only present for non-internal sources and represent
        // the content of the last RB packet that was sent to this source. These values
        // are only updated when the source is sending.
        /** Confirmation if a RB has been sent */
        bool sent_rb;
        /** calculated lost 8-bit fraction */
        int sent_rb_fractionlost;
        /** Lost Packets */
        int sent_rb_packetslost;
        /** Last seen seqnum */
        int sent_rb_exthighestseq;
        /** Jitter in clock rate units */
        int sent_rb_jitter;
        /** Last SR time (seconds in NTP Short Format, 16.16 fixed point) */
        int sent_rb_lsr;
        /** delay since last SR (seconds in NTP Short Format, 16.16 fixed point) */
        int sent_rb_dlsr;

        // The following field is present only for internal sources and contains an array
        // of the the most recent receiver reports from each peer. In unicast scenarios
        // this will be a single entry that is identical to the data provided by the
        // have-rb and rb-* fields, but in multicast there will be one entry in the array
        // for each peer that is sending receiver reports.
        std::vector<ReceivedResponse> received_krr;        
    };
}

#endif
