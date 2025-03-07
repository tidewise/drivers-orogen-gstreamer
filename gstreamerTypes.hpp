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

    struct RTPMonitoredSession {
        std::string name;
        int session_id;
    };

    struct RTPMonitoredSessions {
        std::string rtpbin_name;
        std::vector<RTPMonitoredSession> sessions;
    };

    struct RTPPeerReceiverReport {
        /** The SSRC of this source */
        uint16_t ssrc = 0;
        /** The SSRC of the sender of this Receiver Report */
        uint16_t sender_ssrc = 0;
        /** Lost 8-bit fraction */
        float fractionlost = 0;
        /** Lost packets */
        uint32_t packetslost = 0;
        /** Highest received seqnum */
        uint32_t exthighestseq = 0;
        /** Reception jitter (in clock rate units) */
        uint32_t jitter = 0;
        /** Last Sender Report time */
        double lsr;
        /** Delay since last Sender Report */
        double dlsr;
        /** The round-trip time */
        double round_trip;
    };

    /** The RTP Session Statistics
     *
     * This is the response when the stats is present in the session
     */
    struct RTPSourceStatistics {
        /** The label of the stream */
        std::string stream_name;

        // The following fields are always present.
        /** SSRC of this source */
        uint16_t ssrc = 0;

        enum Flags {
            SOURCE_IS_FROM_SELF = 0x1,
            SOURCE_IS_VALIDATED = 0x2,
            BYE_RECEIVED = 0x4,
            SOURCE_IS_CSRC = 0x8
        };

        /** @meta bitfield /gstreamer/RTPSourceStatistics/Flags */
        uint8_t flags = 0;

        /** Returns a string corresponding to the \see flags.
         * The method is located inside RTPTask.cpp */
        std::string flagsToString();

        /** RTP Bitfield from the received GstStructure confirmations as a string */
        std::string confirmations = "";

        /** First seqnum if known */
        uint32_t seqnum_base = 0;
        /** Clock rate of the media */
        uint32_t clock_rate = 0;
    };

    struct RTPSenderStatistics {
        RTPSourceStatistics source_stats;

        // The following fields make sense for internal sources
        /** Number of payload bytes we sent */
        uint64_t payload_bytes_sent = 0;
        /** Number of packets we sent */
        uint64_t packets_sent = 0;

        // The following fields make sense for non-internal sources and will only increase
        // when "is-sender" is TRUE.
        /** Total number of payload bytes received */
        uint64_t payload_bytes_received = 0;
        /** Total number of packets received */
        uint64_t packets_received = 0;
        /** Total number of bytes received including lower level headers overhead */
        uint64_t bytes_received = 0;

        // Following fields are updated when "is-sender" is TRUE.
        /** Bitrate in bits/sec */
        uint32_t bitrate = 0;
        /** Estimated amount of packets lost */
        uint32_t packets_lost = 0;
        /** Estimated Jitter (in clock rate units) */
        uint32_t jitter = 0;

        // The last Sender Report report this source sent. Only updates when "is-sender"
        // is TRUE.
        /** The source has sent Sender Report */
        bool have_sr = false;
        /** NTP Time of the Sender Report */
        base::Time sr_ntptime;
        /** RTP Time of Sender Report (in clock rate units)*/
        uint32_t sr_rtptime = 0;
        /** The number of bytes in the Sender Report */
        uint32_t sr_octet_count = 0;
        /** The number of packets in the Sender Report */
        uint32_t sr_packet_count = 0;

        // The following fields are only present for non-internal sources and represent
        // the content of the last RTB Buffer packet that was sent to this source. These
        // values are only updated when the source is sending.
        /** Confirmation if a RTB Buffer has been sent */
        bool sent_receiver_block = false;
        /** Lost Packets */
        uint16_t sent_receiver_block_packetslost = 0;
        /** calculated lost 8-bit fraction */
        float sent_receiver_block_fractionlost = 0;
        /** Last seen seqnum */
        uint32_t sent_receiver_block_exthighestseq = 0;
        /** Jitter in clock rate units */
        uint32_t sent_receiver_block_jitter = 0;
        /** Last Sender Report time in seconds */
        double sent_receiver_block_lsr;
        /** delay since last Sender Report in seconds */
        double sent_receiver_block_dlsr;

        // Documentation not available
        uint32_t sent_picture_loss_count = 0;
        uint32_t sent_full_image_request_count = 0;
        uint32_t sent_nack_count = 0;
        uint32_t received_full_image_request_count = 0;

        // The following field is present only for internal sources and contains an array
        // of the the most recent receiver reports from each peer. In unicast scenarios
        // this will be a single entry that is identical to the data provided by the
        // have-rb and rb-* fields, but in multicast there will be one entry in the array
        // for each peer that is sending receiver reports.
        std::vector<RTPPeerReceiverReport> peer_receiver_reports;
    };

    struct RTPReceiverStatistics {
        RTPSourceStatistics source_stats;

        // The following fields are only present when known.
        /** Origin of last received RTP Packet - Only present when known */
        std::string rtp_from = std::string("");
        /** Origin of last received RTP Packet - Only present when known */
        std::string rtcp_from = std::string("");

        // Documentation not available
        uint32_t received_picture_loss_count = 0;
        uint32_t received_nack_count = 0;
        uint64_t received_packet_rate = 0;
    };

    struct RTPSessionStatistics {
        uint64_t rtx_drop_count = 0;
        uint64_t sent_nack_count = 0;
        uint64_t recv_nack_count = 0;
        uint64_t sent_rtx_req_count = 0;
        uint64_t recv_rtx_req_count = 0;

        std::vector<RTPSenderStatistics> sender_stats;
        std::vector<RTPReceiverStatistics> receiver_stats;
    };

    struct RTPStatistics {
        base::Time time;
        std::vector<RTPSessionStatistics> statistics;
    };
}

#endif
