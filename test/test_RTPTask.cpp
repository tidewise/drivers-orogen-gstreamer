#include <../tasks/RTPHelpers.hpp>
#include <gst/gststructure.h>
#include <gtest/gtest.h>

using namespace gstreamer;
using namespace base;
using namespace std;

struct RTPTaskTest : public ::testing::Test {
    RTPTaskTest()
    {
    }

    GstStructure* createSourceStatsStructure()
    {
        return gst_structure_new("application/x-rtp-source-stats",
            "ssrc",
            G_TYPE_UINT,
            77,
            "internal",
            G_TYPE_BOOLEAN,
            TRUE,
            "validated",
            G_TYPE_BOOLEAN,
            TRUE,
            "received-bye",
            G_TYPE_BOOLEAN,
            FALSE,
            "is-csrc",
            G_TYPE_BOOLEAN,
            FALSE,
            "is-sender",
            G_TYPE_BOOLEAN,
            TRUE,
            "seqnum-base",
            G_TYPE_INT,
            9258,
            "clock-rate",
            G_TYPE_INT,
            90000,
            "octets-sent",
            G_TYPE_UINT64,
            1,
            "packets-sent",
            G_TYPE_UINT64,
            2,
            "octets-received",
            G_TYPE_UINT64,
            3,
            "packets-received",
            G_TYPE_UINT64,
            4,
            "bytes-received",
            G_TYPE_UINT64,
            5,
            "bitrate",
            G_TYPE_UINT64,
            6,
            "packets-lost",
            G_TYPE_INT,
            7,
            "jitter",
            G_TYPE_UINT,
            8,
            "have-sr",
            G_TYPE_BOOLEAN,
            TRUE,
            "sr-ntptime",
            G_TYPE_UINT64,
            // NTP Timestamp of 2000-01-01 00:00:09 = Unix + Diff to 2000-01-01 00:00:09
            (uint64_t(2208988800 + 946684809) << 32),
            "sr-rtptime",
            G_TYPE_UINT,
            10,
            "sr-octet-count",
            G_TYPE_UINT,
            11,
            "sr-packet-count",
            G_TYPE_UINT,
            12,
            "sent-pli-count",
            G_TYPE_UINT,
            13,
            "sent-fir-count",
            G_TYPE_UINT,
            14,
            "sent-nack-count",
            G_TYPE_UINT,
            15,
            "recv-fir-count",
            G_TYPE_UINT,
            16,
            "sent-rb",
            G_TYPE_BOOLEAN,
            TRUE,
            "sent-rb-packetslost",
            G_TYPE_INT,
            17,
            "sent-rb-fractionlost",
            G_TYPE_UINT,
            18,
            "sent-rb-exthighestseq",
            G_TYPE_UINT,
            19,
            "sent-rb-jitter",
            G_TYPE_UINT,
            20,
            "sent-rb-lsr",
            G_TYPE_UINT,
            // NTP Timestamp 16.16:
            // 2000-01-01 00:00:21 = NTPToUnix + Unix to 2000-01-01 00:00:21
            // Shifted 16
            (uint32_t(2208988800 + 946684821) << 16),
            "sent-rb-dlsr",
            G_TYPE_UINT,
            // NTP Timestamp 16.16 of 22 seconds
            (uint32_t(22) << 16),
            "rtp-from",
            G_TYPE_STRING,
            "bla",
            "rtcp-from",
            G_TYPE_STRING,
            "ble",
            "recv-pli-count",
            G_TYPE_UINT,
            23,
            "recv-nack-count",
            G_TYPE_UINT,
            24,
            "recv-packet-rate",
            G_TYPE_UINT,
            25,
            NULL);
    }

    GstStructure* createSessionStatsStructure()
    {
        return gst_structure_new("application/x-rtp-session-stats",
            "recv-nack-count",
            G_TYPE_UINT,
            1,
            "rtx-drop-count",
            G_TYPE_UINT,
            2,
            "sent-nack-count",
            G_TYPE_UINT,
            3,
            "recv-rtx-req-count",
            G_TYPE_UINT,
            4,
            "sent-rtx-req-count",
            G_TYPE_UINT,
            5,
            NULL);
    }
};

TEST_F(RTPTaskTest, it_fills_RTPSourceStats)
{
    // Create structure (source-stats)
    GstStructure* source_stats = createSourceStatsStructure();

    // Get Source_Statistics
    RTPSourceStatistics source_statistics =
        extractRTPSourceStats(source_stats, std::string("bla"));

    ASSERT_EQ(source_statistics.stream_name, "bla");
    ASSERT_EQ(source_statistics.ssrc, 77);
    ASSERT_EQ(source_statistics.flags, 0x3);
    ASSERT_EQ(source_statistics.confirmations,
        "SOURCE_IS_FROM_SELF | SOURCE_IS_VALIDATED");
    ASSERT_EQ(source_statistics.seqnum_base, 9258);
    ASSERT_EQ(source_statistics.clock_rate, 90000);

    // Clean-up
    gst_structure_free(source_stats);
}

TEST_F(RTPTaskTest, it_fills_RTPSenderStatistics)
{
    // Create structure (source-stats)
    GstStructure* source_stats = createSourceStatsStructure();

    // Get Source statistics
    RTPSenderStatistics sender_statistics = extractRTPSenderStats(source_stats, 2);

    ASSERT_EQ(sender_statistics.payload_bytes_sent, 1);
    ASSERT_EQ(sender_statistics.packets_sent, 2);
    ASSERT_EQ(sender_statistics.payload_bytes_received, 3);
    ASSERT_EQ(sender_statistics.packets_received, 4);
    ASSERT_EQ(sender_statistics.bytes_received, 5);
    ASSERT_EQ(sender_statistics.bitrate, 6);
    ASSERT_EQ(sender_statistics.packets_lost, 7);
    ASSERT_EQ(sender_statistics.jitter, base::Time::fromSeconds(4.0));
    ASSERT_EQ(sender_statistics.have_sr, true);
    // Year 2000 00:00:00 2000-01-01 in Unix = 946684800
    ASSERT_EQ(sender_statistics.sr_ntptime, base::Time::fromSeconds(9.0 + 946684800));
    ASSERT_EQ(sender_statistics.sr_rtptime_in_clock_rate_units, 10);
    ASSERT_EQ(sender_statistics.sr_octet_count, 11);
    ASSERT_EQ(sender_statistics.sr_packet_count, 12);
    ASSERT_EQ(sender_statistics.sent_picture_loss_count, 13);
    ASSERT_EQ(sender_statistics.sent_full_image_request_count, 14);
    ASSERT_EQ(sender_statistics.sent_nack_count, 15);
    ASSERT_EQ(sender_statistics.received_full_image_request_count, 16);
    ASSERT_EQ(sender_statistics.sent_receiver_block, true);
    ASSERT_EQ(sender_statistics.sent_receiver_block_packetslost, 17);
    ASSERT_EQ(sender_statistics.sent_receiver_block_fractionlost, 18);
    ASSERT_EQ(sender_statistics.sent_receiver_block_exthighestseq, 19);
    ASSERT_EQ(sender_statistics.sent_receiver_block_jitter,
        base::Time::fromSeconds(10.0));
    ASSERT_EQ(sender_statistics.sent_receiver_block_lsr,
        base::Time::fromSeconds(21.0 + 946684800));
    ASSERT_EQ(sender_statistics.sent_receiver_block_dlsr, base::Time::fromSeconds(22.0));

    // Clean-up
    gst_structure_free(source_stats);
}

TEST_F(RTPTaskTest, it_fills_RTPReceiverStatistics)
{
    // Create structure (source-stats)
    GstStructure* source_stats = createSourceStatsStructure();

    // Get Sender Statistics
    RTPReceiverStatistics receiver_statistics = extractRTPReceiverStats(source_stats);

    ASSERT_EQ(receiver_statistics.rtp_from, "bla");
    ASSERT_EQ(receiver_statistics.rtcp_from, "ble");
    ASSERT_EQ(receiver_statistics.received_picture_loss_count, 23);
    ASSERT_EQ(receiver_statistics.received_nack_count, 24);
    ASSERT_EQ(receiver_statistics.received_packet_rate, 25);

    // Clean-up
    gst_structure_free(source_stats);
}

TEST_F(RTPTaskTest, it_fills_RTPSessionStatistics)
{
    // Create structure (source-stats)
    GstStructure* session_stats = createSessionStatsStructure();

    // Get Sender Statistics
    RTPSessionStatistics session_statistics = extractRTPSessionStats(session_stats);

    ASSERT_EQ(session_statistics.recv_nack_count, 1);
    ASSERT_EQ(session_statistics.rtx_drop_count, 2);
    ASSERT_EQ(session_statistics.sent_nack_count, 3);
    ASSERT_EQ(session_statistics.recv_rtx_req_count, 4);
    ASSERT_EQ(session_statistics.sent_rtx_req_count, 5);

    // Clean-up
    gst_structure_free(session_stats);
}

TEST_F(RTPTaskTest, it_properly_apply_time_offset)
{
    // The numbers will be expressed as HEX, since it's easier to understand
    // First testing if the ntp_timestamp is older than the NTP_Short
    uint64_t ntp_timestamp = uint64_t(0xBF00FFFF) << 32; // 3204513791
    uint32_t ntp_short = uint32_t(1) << 16;
    base::Time adjusted_time = lsrTimeToUnixEpoch(ntp_timestamp, ntp_short);

    // NTP Timestamp + 2
    base::Time expected_time = base::Time::fromSeconds(995524993);

    ASSERT_EQ(adjusted_time, expected_time);

    // Now if the ntp_timestamp is newer than the NTP_Short
    ntp_timestamp = uint64_t(0xBF010000) << 32; // 3204513792
    ntp_short = uint32_t(0xFFFF) << 16;

    adjusted_time = lsrTimeToUnixEpoch(ntp_timestamp, ntp_short);

    // NTP Timestamp - 1
    expected_time = base::Time::fromSeconds(995524991);

    ASSERT_EQ(adjusted_time, expected_time);
}