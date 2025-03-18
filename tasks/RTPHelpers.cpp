#include "RTPHelpers.hpp"

#include <base-logging/Logging.hpp>
#include "base/Time.hpp"

using namespace gstreamer;
using namespace std;

RTPSourceStatistics gstreamer::extractRTPSourceStats(const GstStructure* gst_stats,
    std::string rtpbin_name)
{
    RTPSourceStatistics stats;
    stats.stream_name = rtpbin_name;
    stats.ssrc = fetchUnsignedInt(gst_stats, "ssrc");
    stats.flags = fetchFlags(gst_stats);
    stats.confirmations = stats.flagsToString();
    stats.seqnum_base = fetchInt(gst_stats, "seqnum-base");
    stats.clock_rate = fetchInt(gst_stats, "clock-rate");
    return stats;
}

uint8_t gstreamer::fetchFlags(const GstStructure* gst_stats)
{
    uint8_t flags = 0;

    if (fetchBoolean(gst_stats, "internal"))
        flags |= 0x01;
    if (fetchBoolean(gst_stats, "validated"))
        flags |= 0x02;
    if (fetchBoolean(gst_stats, "received-bye"))
        flags |= 0x04;
    if (fetchBoolean(gst_stats, "is-csrc"))
        flags |= 0x08;

    return flags;
}

std::string RTPSourceStatistics::flagsToString()
{
    std::string result;

    if (flags & SOURCE_IS_FROM_SELF)
        result += "SOURCE_IS_FROM_SELF | ";
    if (flags & SOURCE_IS_VALIDATED)
        result += "SOURCE_IS_VALIDATED | ";
    if (flags & BYE_RECEIVED)
        result += "BYE_RECEIVED | ";
    if (flags & SOURCE_IS_CSRC)
        result += "SOURCE_IS_CSRC | ";

    // Remove " | " from last flag
    if (!result.empty()) {
        result.erase(result.length() - 3);
    }
    else {
        result = "NONE";
    }

    return result;
}

RTPReceiverStatistics gstreamer::extractRTPReceiverStats(const GstStructure* stats)
{
    RTPReceiverStatistics receiver_statistics;

    receiver_statistics.rtp_from = fetchString(stats, "rtp-from");
    receiver_statistics.rtcp_from = fetchString(stats, "rtcp-from");
    receiver_statistics.received_nack_count = fetchUnsignedInt(stats, "recv-nack-count");
    receiver_statistics.received_packet_rate =
        fetchUnsignedInt(stats, "recv-packet-rate");
    receiver_statistics.received_picture_loss_count =
        fetchUnsignedInt(stats, "recv-pli-count");

    return receiver_statistics;
}

RTPSenderStatistics gstreamer::extractRTPSenderStats(const GstStructure* stats,
    uint32_t clock_rate)
{
    RTPSenderStatistics sender_statistics;
    sender_statistics.payload_bytes_sent = fetch64UnsignedInt(stats, "octets-sent");
    sender_statistics.packets_sent = fetch64UnsignedInt(stats, "packets-sent");
    sender_statistics.payload_bytes_received =
        fetch64UnsignedInt(stats, "octets-received");
    sender_statistics.packets_received = fetch64UnsignedInt(stats, "packets-received");
    sender_statistics.bytes_received = fetch64UnsignedInt(stats, "bytes-received");
    sender_statistics.bitrate = fetch64UnsignedInt(stats, "bitrate");
    sender_statistics.packets_lost = fetchInt(stats, "packets-lost");
    // Only calculates jitter in ms if there's a valid clock rate
    if (clock_rate != 0) {
        sender_statistics.jitter =
            jitterToTime(fetchUnsignedInt(stats, "jitter"), clock_rate);
    }
    sender_statistics.sent_picture_loss_count = fetchUnsignedInt(stats, "sent-pli-count");
    sender_statistics.sent_full_image_request_count =
        fetchUnsignedInt(stats, "sent-fir-count");
    sender_statistics.received_full_image_request_count =
        fetchUnsignedInt(stats, "recv-fir-count");
    sender_statistics.sent_nack_count = fetchUnsignedInt(stats, "sent-nack-count");
    sender_statistics.have_sr = fetchBoolean(stats, "have-sr");
    uint64_t ntp_timestamp = fetch64UnsignedInt(stats, "sr-ntptime");
    sender_statistics.sr_ntptime = ntpToUnixEpoch(ntp_timestamp);
    sender_statistics.sr_rtptime_in_clock_rate_units =
        fetchUnsignedInt(stats, "sr-rtptime");
    sender_statistics.sr_octet_count = fetchUnsignedInt(stats, "sr-octet-count");
    sender_statistics.sr_packet_count = fetchUnsignedInt(stats, "sr-packet-count");
    sender_statistics.sent_receiver_block = fetchBoolean(stats, "sent-rb");
    sender_statistics.sent_receiver_block_fractionlost =
        fetchUnsignedInt(stats, "sent-rb-fractionlost");
    sender_statistics.sent_receiver_block_packetslost =
        fetchInt(stats, "sent-rb-packetslost");
    sender_statistics.sent_receiver_block_exthighestseq =
        fetchUnsignedInt(stats, "sent-rb-exthighestseq");
    // Only calculates jitter in ms if there's a valid clock rate
    if (clock_rate != 0) {
        sender_statistics.sent_receiver_block_jitter =
            jitterToTime(fetchUnsignedInt(stats, "sent-rb-jitter"), clock_rate);
    }
    sender_statistics.sent_receiver_block_lsr =
        lsrTimeToUnixEpoch(ntp_timestamp, fetchUnsignedInt(stats, "sent-rb-lsr"));
    sender_statistics.sent_receiver_block_dlsr =
        NTPShortToTime(fetchUnsignedInt(stats, "sent-rb-dlsr"));
    sender_statistics.peer_receiver_reports =
        extractPeerReceiverReports(stats, clock_rate, ntp_timestamp);

    return sender_statistics;
}

bool gstreamer::fetchBoolean(const GstStructure* structure, const char* fieldname)
{
    gboolean gboolean_value = FALSE;
    if (!gst_structure_get_boolean(structure, fieldname, &gboolean_value)) {
        return false;
    }

    return gboolean_value;
}

uint64_t gstreamer::fetch64UnsignedInt(const GstStructure* structure, const char* fieldname)
{
    guint64 guint64_value = 0;
    if (!gst_structure_get_uint64(structure, fieldname, &guint64_value)) {
        return 0;
    }

    return guint64_value;
}

uint32_t gstreamer::fetchUnsignedInt(const GstStructure* structure, const char* fieldname)
{
    guint guint32_value = 0;
    if (!gst_structure_get_uint(structure, fieldname, &guint32_value)) {
        return 0;
    }

    return guint32_value;
}

int32_t gstreamer::fetchInt(const GstStructure* structure, const char* fieldname)
{
    gint gint_value = 0;
    if (!gst_structure_get_int(structure, fieldname, &gint_value)) {
        return 0;
    }

    return gint_value;
}

std::string gstreamer::fetchString(const GstStructure* structure, const char* fieldname)
{
    const gchar* gstr_value = gst_structure_get_string(structure, fieldname);
    std::string string("");
    if (gstr_value == nullptr) {
        return string;
    }

    // This should be freed according to documentation but
    // g_free was breaking tests with a invalid pointer error.
    // By assigning the string: assign + g_free = double free.
    // Therefore it's safe to assume this way does not leave any memory leaks.
    string.assign(gstr_value);
    return string;
}

base::Time gstreamer::ntpToUnixEpoch(uint64_t ntp_timestamp)
{
    // Extract seconds (upper 32 bits)
    uint32_t ntp_seconds = ntp_timestamp >> 32;

    // Extract fractional part (lower 32 bits)
    uint32_t ntp_fraction = ntp_timestamp & 0xFFFFFFFF;

    // Convert to UNIX time in seconds (1900 to 1970)
    uint32_t unix_seconds = ntp_seconds - 2208988800;

    // Convert fractional part to microseconds
    uint64_t microseconds = (static_cast<uint64_t>(ntp_fraction) * 1000000ULL) >> 32;

    // Combine seconds and microseconds
    return base::Time::fromMicroseconds(
        (static_cast<int64_t>(unix_seconds) * 1000000ULL) + microseconds);
}

// Only gets the seconds according to documentation
base::Time gstreamer::NTPShortToTime(uint32_t ntp_short)
{
    return base::Time::fromMicroseconds(
        (static_cast<uint64_t>(ntp_short) * 1000000ULL) >> 16);
}

base::Time gstreamer::lsrTimeToUnixEpoch(uint64_t ntp_timestamp, uint32_t ntp_short)
{
    // Extract ntp_timestamp most significative bits
    uint16_t ntp_fixed_seconds = ntp_timestamp >> 48;

    // Extract seconds from ntp_short
    uint16_t ntp_short_seconds = ntp_short >> 16;

    // Create complete seconds from values above
    uint32_t ntp_seconds =
        (static_cast<uint32_t>(ntp_fixed_seconds) << 16) + ntp_short_seconds;

    // We use the ntp timestamp to calculate the fixed point for the
    // last Sender Report timestamp but this can be problem if
    // if the value of the seconds for the timestamps 17-bit
    // doesn't change at the same moment, possibly causing one lsr sample to
    // "go back in time" every 65536 seconds.
    uint32_t ntp_timestamp_seconds = ntp_timestamp >> 32;
    uint32_t seconds_offset = 0; // has to have a starting value or it breaks.
    if ((ntp_timestamp_seconds & 0x8000) && !(ntp_short_seconds & 0x8000)) {
        // ntp_timestamp is 17-bit seconds before ntp_short_seconds
        seconds_offset = 0x10000;
    }
    else if (!(ntp_timestamp_seconds & 0x8000) && (ntp_short_seconds & 0x8000)) {
        // ntp_timestamp is 17-bit seconds after ntp_short_seconds
        seconds_offset = -0x10000;
    }

    // fix time representation
    ntp_seconds =
        (ntp_timestamp_seconds & 0xFFFF0000) + seconds_offset + ntp_short_seconds;

    // Convert ntp seconds to unix seconds
    uint64_t unix_seconds = ntp_seconds - 2208988800;

    // Get fractional part of ntp_short
    uint64_t fractional = ntp_short & 0xFFFF;

    // Create complete ntp timestamp using extracted values
    uint64_t microseconds = (fractional * 1000000ULL) >> 32;

    return base::Time::fromMicroseconds((unix_seconds * 1000000ULL) + microseconds);
}

base::Time gstreamer::jitterToTime(uint32_t jitter, uint32_t clock_rate)
{
    if (jitter != 0) {
        return base::Time::fromMicroseconds(
            (static_cast<uint64_t>(jitter) * 1000000ULL) / clock_rate);
    }

    return base::Time::fromSeconds(0);
}

std::vector<RTPPeerReceiverReport> gstreamer::extractPeerReceiverReports(
    const GstStructure* structure,
    uint32_t clock_rate,
    uint64_t ntp_timestamp)
{
    const GValue* g_array_value = gst_structure_get_value(structure, "received-rr");

    if (!g_array_value) {
        LOG_INFO_S << "peer-receiver_reports are empty";
        return {};
    }

    std::vector<RTPPeerReceiverReport> all_reports;
    size_t array_size = gst_value_array_get_size(g_array_value);
    for (size_t i = 0; i < array_size; i++) {
        const GValue* report_value = gst_value_array_get_value(g_array_value, i);
        if (!report_value) {
            return all_reports;
        }

        const GstStructure* report_structure = gst_value_get_structure(report_value);
        if (!report_structure) {
            return all_reports;
        }

        RTPPeerReceiverReport report;
        report.ssrc = fetchUnsignedInt(report_structure, "rb-ssrc");
        report.sender_ssrc = fetchUnsignedInt(report_structure, "rb-sender-ssrc");
        report.fractionlost = fetchUnsignedInt(report_structure, "rb-fractionlost");
        report.packetslost = fetchInt(report_structure, "rb-packetslost");
        report.exthighestseq = fetchUnsignedInt(report_structure, "rb-exthighestseq");
        // Only calculates jitter in ms if there's a valid clock rate
        if (clock_rate != 0) {
            report.jitter =
                jitterToTime(fetchUnsignedInt(report_structure, "rb-jitter"), clock_rate);
        }
        report.lsr = lsrTimeToUnixEpoch(ntp_timestamp,
            fetchUnsignedInt(report_structure, "rb-lsr"));
        report.dlsr = NTPShortToTime(fetchUnsignedInt(report_structure, "rb-dlsr"));
        report.round_trip =
            NTPShortToTime(fetchUnsignedInt(report_structure, "rb-round-trip"));
        all_reports.push_back(report);
    }

    return all_reports;
}