/* Generated from orogen/lib/orogen/templates/tasks/Task.cpp */

#include <base-logging/Logging.hpp>
#include <gst/gstcaps.h>
#include <set>

#include "Helpers.hpp"
#include "RTPTask.hpp"

using namespace std;
using namespace gstreamer;
using namespace base::samples::frame;

RTPTask::RTPTask(std::string const& name)
    : RTPTaskBase(name)
{
}

RTPTask::~RTPTask()
{
}

/// The following lines are template definitions for the various state machine
// hooks defined by Orocos::RTT. See Task.hpp for more detailed
// documentation about them.

RTPSessionStatistics RTPTask::extractRTPSessionStats(GstElement* session)
{
    RTPSessionStatistics session_stats;

    GstStructure* gst_stats = nullptr;
    g_object_get(session, "stats", &gst_stats, NULL);

    session_stats.recv_nack_count = fetchUnsignedInt(gst_stats, "recv-nack-count");
    session_stats.rtx_drop_count = fetchUnsignedInt(gst_stats, "rtx-drop-count");
    session_stats.sent_nack_count = fetchUnsignedInt(gst_stats, "sent-nack-count");
    session_stats.recv_rtx_req_count = fetchUnsignedInt(gst_stats, "recv-rtx-req-count");
    session_stats.sent_rtx_req_count = fetchUnsignedInt(gst_stats, "sent-rtx-req-count");

    const GValue* gst_source_stats_value =
        gst_structure_get_value(gst_stats, "source-stats");
    GValueArray* gst_source_stats =
        static_cast<GValueArray*>(g_value_get_boxed(gst_source_stats_value));

    if (!gst_stats) {
        throw std::runtime_error("gst-source-stats is not a boxed type");
    }

    for (guint i = 0; i < gst_source_stats->n_values; i++) {
        GValue* value = g_value_array_get_nth(gst_source_stats, i);
        GstStructure* gst_stats = static_cast<GstStructure*>(g_value_get_boxed(value));

        m_source_stats = extractRTPSourceStats(gst_stats);
        bool sender = fetchBoolean(gst_stats, "is-sender");
        if (sender) {
            auto sender_stats = extractRTPSenderStats(gst_stats);
            sender_stats.source_stats = m_source_stats;
            session_stats.sender_stats.push_back(sender_stats);
        }
        else {
            auto receiver_stats = extractRTPReceiverStats(gst_stats);
            receiver_stats.source_stats = m_source_stats;
            session_stats.receiver_stats.push_back(receiver_stats);
        }
    }

    gst_structure_free(gst_stats);
    return session_stats;
}

RTPSourceStatistics RTPTask::extractRTPSourceStats(const GstStructure* gst_stats)
{
    RTPSourceStatistics stats;
    stats.stream_name = m_rtp_monitored_sessions.rtpbin_name;
    stats.ssrc = fetchUnsignedInt(gst_stats, "ssrc");
    stats.flags = fetchFlags(gst_stats);
    stats.confirmations = stats.flagsToString();
    stats.seqnum_base = fetchInt(gst_stats, "seqnum-base");
    stats.clock_rate = fetchInt(gst_stats, "clock-rate");
    return stats;
}

uint8_t RTPTask::fetchFlags(const GstStructure* gst_stats)
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

RTPReceiverStatistics RTPTask::extractRTPReceiverStats(const GstStructure* stats)
{
    RTPReceiverStatistics receiver_statistics;

    receiver_statistics.rtp_from = fetchString(stats, "rtp-from");
    receiver_statistics.rtcp_from = fetchString(stats, "rtcp-from");
    receiver_statistics.received_nack_count = fetchUnsignedInt(stats, "recv-nack-count");
    receiver_statistics.received_packet_rate =
        fetchUnsignedInt(stats, "reck-packet_rate");
    receiver_statistics.received_picture_loss_count =
        fetchUnsignedInt(stats, "recv-pli-count");

    return receiver_statistics;
}

RTPSenderStatistics RTPTask::extractRTPSenderStats(const GstStructure* stats)
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
    if (m_source_stats.clock_rate != 0) {
        sender_statistics.jitter = jitterToTime(fetchUnsignedInt(stats, "jitter"));
    }
    sender_statistics.sent_picture_loss_count = fetchUnsignedInt(stats, "sent-pli-count");
    sender_statistics.sent_full_image_request_count =
        fetchUnsignedInt(stats, "sent-fir-count");
    sender_statistics.received_full_image_request_count =
        fetchUnsignedInt(stats, "recv-fir-count");
    sender_statistics.sent_nack_count = fetchUnsignedInt(stats, "sent-nack-count");
    sender_statistics.have_sr = fetchBoolean(stats, "have-sr");
    m_ntp_timestamp = fetch64UnsignedInt(stats, "sr-ntptime");
    sender_statistics.sr_ntptime = ntpToUnixEpoch(m_ntp_timestamp);
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
    if (m_source_stats.clock_rate != 0) {
        sender_statistics.sent_receiver_block_jitter =
            jitterToTime(fetchUnsignedInt(stats, "sent-rb-jitter"));
    }
    sender_statistics.sent_receiver_block_lsr =
        lsrTimeToUnixEpoch(m_ntp_timestamp, fetchUnsignedInt(stats, "sent-rb-lsr"));
    sender_statistics.sent_receiver_block_dlsr =
        NTPShortToTime(fetchUnsignedInt(stats, "sent-rb-dlsr"));
    sender_statistics.peer_receiver_reports = extractPeerReceiverReports(stats);

    return sender_statistics;
}

bool RTPTask::fetchBoolean(const GstStructure* structure, const char* fieldname)
{
    gboolean gboolean_value;
    if (!gst_structure_get_boolean(structure, fieldname, &gboolean_value)) {
        return false;
    }

    return gboolean_value;
}

uint64_t RTPTask::fetch64UnsignedInt(const GstStructure* structure, const char* fieldname)
{
    guint64 guint64_value;
    if (!gst_structure_get_uint64(structure, fieldname, &guint64_value)) {
        return 0;
    }

    return guint64_value;
}

uint32_t RTPTask::fetchUnsignedInt(const GstStructure* structure, const char* fieldname)
{
    guint guint32_value;
    if (!gst_structure_get_uint(structure, fieldname, &guint32_value)) {
        return 0;
    }

    return guint32_value;
}

int32_t RTPTask::fetchInt(const GstStructure* structure, const char* fieldname)
{
    gint gint_value;
    if (!gst_structure_get_int(structure, fieldname, &gint_value)) {
        return 0;
    }

    return gint_value;
}

std::string RTPTask::fetchString(const GstStructure* structure, const char* fieldname)
{
    const gchar* gstr_value = gst_structure_get_string(structure, fieldname);
    if (gstr_value == nullptr) {
        return std::string("");
    }

    return std::string(gstr_value);
}

base::Time RTPTask::ntpToUnixEpoch(uint64_t ntp_timestamp)
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
base::Time RTPTask::NTPShortToTime(uint32_t ntp_short)
{
    return base::Time::fromMicroseconds(
        (static_cast<uint64_t>(ntp_short) * 1000000ULL) >> 16);
}

base::Time RTPTask::lsrTimeToUnixEpoch(uint64_t ntp_timestamp, uint32_t ntp_short)
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
    uint32_t seconds_offset;
    if ((ntp_timestamp_seconds & 0x8000) && !(ntp_short_seconds & 0x8000)) {
        // ntp_timestamp is 17-bit seconds before ntp_short_seconds
        seconds_offset = 0x10000;
    }
    else if (!(ntp_timestamp_seconds & 0x8000) && (ntp_short_seconds & 0x8000)) {
        // ntp_timestamp is 17-bit seconds after ntp_short_seconds
        seconds_offset = -0x10000;
    }

    // fix time representation
    ntp_seconds = (ntp_timestamp_seconds & 0xFFFF0000) + seconds_offset + ntp_short_seconds;

    // Convert ntp seconds to unix seconds
    uint64_t unix_seconds = ntp_seconds - 2208988800;

    // Get fractional part of ntp_short
    uint64_t fractional = ntp_short & 0xFFFF;

    // Create complete ntp timestamp using extracted values
    uint64_t microseconds = (fractional * 1000000ULL) >> 32;

    return base::Time::fromMicroseconds((unix_seconds * 1000000ULL) + microseconds);
}

base::Time RTPTask::jitterToTime(uint32_t jitter)
{
    return base::Time::fromMicroseconds(
        (static_cast<uint64_t>(jitter) * 1000000ULL) / m_source_stats.clock_rate);
}

std::vector<RTPPeerReceiverReport> RTPTask::extractPeerReceiverReports(
    const GstStructure* structure)
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
        if (m_source_stats.clock_rate != 0) {
            report.jitter = jitterToTime(fetchUnsignedInt(report_structure, "rb-jitter"));
        }
        report.lsr = lsrTimeToUnixEpoch(m_ntp_timestamp,
            fetchUnsignedInt(report_structure, "rb-lsr"));
        report.dlsr = NTPShortToTime(fetchUnsignedInt(report_structure, "rb-dlsr"));
        report.round_trip =
            NTPShortToTime(fetchUnsignedInt(report_structure, "rb-round-trip"));
        all_reports.push_back(report);
    }

    return all_reports;
}

bool RTPTask::configureHook()
{
    if (!RTPTaskBase::configureHook())
        return false;

    m_rtp_monitored_sessions = _rtp_monitored_sessions.get();
    m_bin = gst_bin_get_by_name(GST_BIN(m_pipeline),
        m_rtp_monitored_sessions.rtpbin_name.c_str());
    if (!m_bin) {
        throw std::runtime_error("cannot find element named " +
                                 m_rtp_monitored_sessions.rtpbin_name + " in pipeline");
    }

    return true;
}

bool RTPTask::startHook()
{
    if (!RTPTaskBase::startHook())
        return false;

    for (auto const& monitored_session : m_rtp_monitored_sessions.sessions) {
        GstElement* session = nullptr;
        g_signal_emit_by_name(m_bin,
            "get-internal-session",
            monitored_session.session_id,
            &session);
        if (!session) {
            throw std::runtime_error(
                "did not resolve provided session, wrong session ID " +
                to_string(monitored_session.session_id) + " ?");
        }
        m_rtp_internal_sessions.push_back(session);
    }

    return true;
}

void RTPTask::updateHook()
{
    RTPTaskBase::updateHook();

    RTPStatistics stats;
    stats.time = base::Time::now();
    for (auto const& element : m_rtp_internal_sessions) {
        stats.statistics.push_back(extractRTPSessionStats(element));
    }
    _rtp_statistics.write(stats);
}

void RTPTask::errorHook()
{
    RTPTaskBase::errorHook();
}
void RTPTask::stopHook()
{
    RTPTaskBase::stopHook();
}
void RTPTask::cleanupHook()
{
    RTPTaskBase::cleanupHook();
}