#ifndef RTP_HELPERS_HPP
#define RTP_HELPERS_HPP

#include "gst/rtp/rtp.h"
#include <base/Time.hpp>

namespace gstreamer {

    struct RTPMonitoringConfig {
        std::string rtpbin_name;
        std::vector<uint32_t> sessions_id;
    };

    struct RTPPeerReceiverReport {
        /** The SSRC of this source */
        uint16_t ssrc = 0;
        /** The SSRC of the sender of this Receiver Report */
        uint16_t sender_ssrc = 0;
        /** Lost 8-bit fraction */
        float fractionlost = 0;
        /** Lost packets */
        int32_t packetslost = 0;
        /** Highest received seqnum */
        uint32_t exthighestseq = 0;
        /** Reception jitter if clock rate is present */
        base::Time jitter;
        /** Last Sender Report time */
        base::Time lsr;
        /** Delay since last Sender Report */
        base::Time dlsr;
        /** The round-trip time */
        base::Time round_trip;
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
        int32_t seqnum_base = 0;
        /** Clock rate of the media */
        int32_t clock_rate = 0;
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
        uint64_t bitrate = 0;
        /** Estimated amount of packets lost */
        int32_t packets_lost = 0;
        /** Estimated Jitter if clock rate is available */
        base::Time jitter;

        // The last Sender Report report this source sent. Only updates when "is-sender"
        // is TRUE.
        /** The source has sent Sender Report */
        bool have_sr = false;
        /** NTP Time of the Sender Report */
        base::Time sr_ntptime;
        /** RTP Time of Sender Report in clock rate units*/
        uint32_t sr_rtptime_in_clock_rate_units;
        /** The number of bytes in the Sender Report */
        uint32_t sr_octet_count = 0;
        /** The number of packets in the Sender Report */
        uint32_t sr_packet_count = 0;

        // The following fields are only present for non-internal sources and represent
        // the content of the last Report Block Buffer packet that was sent to this
        // source. These values are only updated when the source is sending.
        /** Confirmation if a Report Block Buffer has been sent */
        bool sent_receiver_block = false;
        /** Lost Packets */
        uint32_t sent_receiver_block_packetslost = 0;
        /** calculated lost 8-bit fraction */
        float sent_receiver_block_fractionlost = 0;
        /** Last seen seqnum */
        uint32_t sent_receiver_block_exthighestseq = 0;
        /** Jitter if clock rate is available*/
        base::Time sent_receiver_block_jitter;
        /** Last Sender Report time */
        base::Time sent_receiver_block_lsr;
        /** delay since last Sender Report */
        base::Time sent_receiver_block_dlsr;

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
    /** RTP Helpers for statistics retrieval */
    /** Extract the Stats for the RTPSession
     *
     *  These are the stats from RTPSession as shown in the link below
     *  https://gstreamer.freedesktop.org/documentation/rtpmanager/RTPSession.html?gi-language=c#RTPSession:stats
     *
     * @param session [GstElement*] RTP Session BIN
     * @return [RTPSessionStatistics] Statistics from all RTP Sources in this RTP
     * Session
     */
    // RTPSessionStatistics extractRTPSessionStats(GstElement* session);

    /** Extract the stats which are always present on a RTP Source
     *
     * @param gst_stats [GstStructure*] stats from a given source
     * @param rtpbin_name [std::string] rtpbin_name
     * @return [RTPSourceStatistics] RTP Source obligatory stats
     */
    RTPSourceStatistics extractRTPSourceStats(GstStructure const* gst_stats,
        std::string rtpbin_name);

    /** Extract the Sender Stats from a RTP Source.
     *
     *  The sender stats are all the parameters that are updated when is-sender
     *  is TRUE and were defined by checking the documentation from
     *  https://gstreamer.freedesktop.org/documentation/rtpmanager/RTPSource.html?gi-language=c#RTPSource:stats
     *
     * @param stats [GstStructure*] stats from a given source
     * @param clock_rate [uint32_t] in clock rate units from RTPSource
     * @return [RTPSenderStatistics] RTP Sender Stats
     */
    RTPSenderStatistics extractRTPSenderStats(GstStructure const* stats,
        uint32_t clock_rate);

    /** Extract the Stats from a RTP Receiver
     *
     *  The receiver stats are all the parameters that are updated when is-sender
     *  is FALSE and were defined by checking the documentation from
     *  https://gstreamer.freedesktop.org/documentation/rtpmanager/RTPSource.html?gi-language=c#RTPSource:stats
     *
     * @param stats [GstStructure*] stats from a given source
     * @return [RTPSenderStatistics] RTP Receiver Stats
     */
    RTPReceiverStatistics extractRTPReceiverStats(GstStructure const* stats);

    /** Extract the Stats from a RTP Session
     * 
     * @param stats [GstStructure*] stats from a given session
     * @return [RTPSessionStats] RTP Session Stats
     */
    RTPSessionStatistics extractRTPSessionStats(GstStructure const* gst_stats);

    /** Fetch the gbooleans and converts it to bool
     *
     *  @param structure [GstStructure*]
     *  @param fieldname [char*]
     *  @return [bool]
     */
    bool fetchBoolean(const GstStructure* structure, const char* fieldname);

    /** Fetchs a 64 unsigned int
     *
     *  @param structure[GstStructure*]
     *  @param fieldname [char*] The name of the field containing the variable
     *  @return [uint64_t]
     */
    uint64_t fetch64UnsignedInt(const GstStructure* structure,
        const char* fieldname);

    /** Fetchs a 32 unsigned int
     *
     *  @param structure[GstStructure*]
     *  @param fieldname [char*] The name of the field containing the variable
     *  @return [uint32_t]
     */
    uint32_t fetchUnsignedInt(const GstStructure* structure,
        const char* fieldname);

    /** Fetchs a int
     *
     *  @param structure[GstStructure*]
     *  @param fieldname [char*] The name of the field containing the variable
     *  @return [int32_t]
     */
    int32_t fetchInt(const GstStructure* structure, const char* fieldname);

    /** Fetchs a gstring and converts it to std::string
     *
     *  @param structure [GstStructure*]
     *  @param fieldname [char*] The name of the field containing the variable
     *  @return [std::string]
     */
    std::string fetchString(const GstStructure* structure, const char* fieldname);

    /** Updates flags of a RTPSourceStatistics struct
     *
     *  @param gst_stats [GstStructure]
     *  @return [uint8_t]
     */
    uint8_t fetchFlags(const GstStructure* gst_stats);

    /** NTP Timestamp format to base::Time from Unix Epoch
     *
     * @param ntp_timestamp [uint64_t] NTP Timestamp 32.32
     * @return [base::Time] Time from Unix epoch
     */
    base::Time ntpToUnixEpoch(uint64_t ntp_timestamp);

    /**
     * Transforms a Last Sender Report Time (NTP Format 16.16 Fixed Point) to
     * microseconds from UnixEpoch using the NTP Timestamp from the report to
     * define the fixed point (16 most significative bits).
     *
     * This works because the fixed time is the middle 32 bits out of 64
     * in the NTP timestamp.
     *
     * For further explanations on how this calculation works refer to
     * https://www.ietf.org/rfc/rfc1889.txt
     * pages 23-24(Diagram and NTP Timestamp) and 27 (Last SR).
     *
     * @param ntp_timestamp NTP Timestamp from the RTCP Packet
     * @param ntp_short Last SR Time (NTP Format 16.16 Fixed Point)
     * @return [base::Time] Time from Unix epoch
     */
    base::Time lsrTimeToUnixEpoch(uint64_t ntp_timestamp, uint32_t ntp_short);

    /** NTP Short Format to base::Time
     *
     * @param ntp_short NTP Short Format 16.16 Fixed Point
     * @return [base::Time] Time from Unix epoch
     */
    base::Time NTPShortToTime(uint32_t ntp_short);

    /** Converts Jitter from Clock Rate Units to base::Time
     *
     * @param jitter Jitter in Clock Rate Units
     * @param clock_rate Units from RTP Source
     * @return [base::Time] Time from Unix epoch
     */
    base::Time jitterToTime(uint32_t jitter, uint32_t clock_rate);

    /** Extracts a array of receiver reports contained on a GST_TYPE_LIST of
     * GstStructures on the "received-rr" field of a RTPSource
     *
     * @param structure [GstStructure*]
     * @param clock_rate [uint32_t] Units
     * @param ntp_timestamp NTP Timestamp of the RTP Source 32.32 format
     * @return [std::vector<RTPPeerReceiverReport>] Received peer reports
     * */
    std::vector<RTPPeerReceiverReport> extractPeerReceiverReports(
        const GstStructure* structure,
        uint32_t clock_rate,
        uint64_t ntp_timestamp);
}

#endif