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
    LOG_INFO_S << "stats: " << gst_structure_to_string(gst_stats);

    session_stats.recv_nack_count = fetchUnsignedInt(gst_stats, "recv-nack-count");
    session_stats.rtx_drop_count = fetchUnsignedInt(gst_stats, "rtx-drop-count");
    session_stats.sent_nack_count = fetchUnsignedInt(gst_stats, "sent-nack-count");

    GValueArray* gst_source_stats = nullptr;
    if (!gst_structure_get_array(gst_stats, "source-stats", &gst_source_stats)) {
        LOG_ERROR_S << "source-stats does not seem to be an array";
        return session_stats;
    }

    for (guint i = 0; i < gst_source_stats->n_values; i++) {
        GValue* value = g_value_array_get_nth(gst_source_stats, i);
        GstStructure* gst_stats = static_cast<GstStructure*>(g_value_get_boxed(value));

        bool sender = fetchBoolean(gst_stats, "is-sender");
        if (sender) {
            auto sender_stats = extractRTPSenderStats(gst_stats);
            sender_stats.source_stats = extractRTPSourceStats(gst_stats);
            session_stats.sender_stats.push_back(sender_stats);
        }
        else {
            auto receiver_stats = extractRTPReceiverStats(gst_stats);
            receiver_stats.source_stats = extractRTPSourceStats(gst_stats);
            session_stats.receiver_stats.push_back(receiver_stats);
        }
    }

    return session_stats;
}

RTPSourceStatistics RTPTask::extractRTPSourceStats(const GstStructure* gst_stats)
{
    RTPSourceStatistics stats;
    stats.ssrc = fetchUnsignedInt(gst_stats, "ssrc");
    stats.internal = fetchBoolean(gst_stats, "internal");
    stats.validated = fetchBoolean(gst_stats, "validated");
    stats.received_bye = fetchBoolean(gst_stats, "received-bye");
    stats.is_csrc = fetchBoolean(gst_stats, "is-csrc");
    stats.seqnum_base = fetchUnsignedInt(gst_stats, "seqnum-base");
    stats.clock_rate = fetchUnsignedInt(gst_stats, "clock-rate");
    return stats;
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
    sender_statistics.payload_bytes_sent = fetchUnsignedInt(stats, "octets-sent");
    sender_statistics.packets_sent = fetchUnsignedInt(stats, "packets-sent");
    sender_statistics.payload_bytes_received = fetchUnsignedInt(stats, "octets-received");
    sender_statistics.packets_received = fetchUnsignedInt(stats, "packets-received");
    sender_statistics.bytes_received = fetchUnsignedInt(stats, "bytes-received");
    sender_statistics.bitrate = fetchUnsignedInt(stats, "bitrate");
    sender_statistics.packets_lost = fetchUnsignedInt(stats, "packets-lost");
    sender_statistics.jitter = fetchUnsignedInt(stats, "jitter");
    sender_statistics.sent_picture_loss_count = fetchUnsignedInt(stats, "sent-pli-count");
    sender_statistics.sent_full_image_request_count =
        fetchUnsignedInt(stats, "sent-fir-count");
    sender_statistics.received_full_image_request_count =
        fetchUnsignedInt(stats, "recv-fir-count");
    sender_statistics.sent_nack_count = fetchUnsignedInt(stats, "sent-nack-count");
    sender_statistics.have_sr = fetchBoolean(stats, "have-sr");
    sender_statistics.in_ntptime = base::Time::fromSeconds(
        static_cast<double>(fetchUnsignedInt(stats, "in-ntptime")));
    sender_statistics.sr_rtptime = fetchUnsignedInt(stats, "sr-rtptime");
    sender_statistics.sr_octet_count = fetchUnsignedInt(stats, "sr-octet-count");
    sender_statistics.sr_packet_count = fetchUnsignedInt(stats, "sr-packet-count");
    sender_statistics.sent_receiver_block = fetchBoolean(stats, "sent-rb");
    sender_statistics.sent_receiver_block_fractionlost =
        fetchUnsignedInt(stats, "sent-rb-fractionlost");
    sender_statistics.sent_receiver_block_packetslost =
        fetchUnsignedInt(stats, "sent-rb-packets-lost");
    sender_statistics.sent_receiver_block_exthighestseq =
        fetchUnsignedInt(stats, "sent-rb-exthighestseq");
    sender_statistics.sent_receiver_block_jitter =
        fetchUnsignedInt(stats, "sent-rb-jitter");
    sender_statistics.sent_receiver_block_lsr = base::Time::fromSeconds(
        static_cast<double>(fetchUnsignedInt(stats, "sent-rb-lsr")));
    sender_statistics.sent_receiver_block_dlsr = base::Time::fromSeconds(
        static_cast<double>(fetchUnsignedInt(stats, "sent-rb-dlsr")));
    sender_statistics.payload_bytes_sent = fetchUnsignedInt(stats, "octets-sent");
    sender_statistics.packets_sent = fetchUnsignedInt(stats, "packets-sent");
    sender_statistics.payload_bytes_received = fetchUnsignedInt(stats, "octets-received");
    sender_statistics.packets_received = fetchUnsignedInt(stats, "packets-received");
    sender_statistics.bytes_received = fetchUnsignedInt(stats, "bytes-received");
    sender_statistics.bitrate = fetchUnsignedInt(stats, "bitrate");
    sender_statistics.packets_lost = fetchUnsignedInt(stats, "packets-lost");
    sender_statistics.jitter = fetchUnsignedInt(stats, "jitter");
    sender_statistics.sent_picture_loss_count = fetchUnsignedInt(stats, "sent-pli-count");
    sender_statistics.sent_full_image_request_count =
        fetchUnsignedInt(stats, "sent-fir-count");
    sender_statistics.received_full_image_request_count =
        fetchUnsignedInt(stats, "recv-fir-count");
    sender_statistics.sent_nack_count = fetchUnsignedInt(stats, "sent-nack-count");
    sender_statistics.have_sr = fetchBoolean(stats, "have-sr");
    sender_statistics.in_ntptime = base::Time::fromSeconds(
        static_cast<double>(fetchUnsignedInt(stats, "in-ntptime")));
    sender_statistics.sr_rtptime = fetchUnsignedInt(stats, "sr-rtptime");
    sender_statistics.sr_octet_count = fetchUnsignedInt(stats, "sr-octet-count");
    sender_statistics.sr_packet_count = fetchUnsignedInt(stats, "sr-packet-count");
    sender_statistics.sent_receiver_block = fetchBoolean(stats, "sent-rb");
    sender_statistics.sent_receiver_block_fractionlost =
        fetchUnsignedInt(stats, "sent-rb-fractionlost");
    sender_statistics.sent_receiver_block_packetslost =
        fetchUnsignedInt(stats, "sent-rb-packets-lost");
    sender_statistics.sent_receiver_block_exthighestseq =
        fetchUnsignedInt(stats, "sent-rb-exthighestseq");
    sender_statistics.sent_receiver_block_jitter =
        fetchUnsignedInt(stats, "sent-rb-jitter");
    sender_statistics.sent_receiver_block_lsr = base::Time::fromSeconds(
        static_cast<double>(fetchUnsignedInt(stats, "sent-rb-lsr")));
    sender_statistics.sent_receiver_block_dlsr = base::Time::fromSeconds(
        static_cast<double>(fetchUnsignedInt(stats, "sent-rb-dlsr")));
    sender_statistics.peer_receiver_reports =
        fetchPeerReceiverReports(stats, "receiver-rr");

    return sender_statistics;
}

bool RTPTask::fetchBoolean(const GstStructure* structure, const char* fieldname)
{
    if (gst_structure_has_field(structure, fieldname) == FALSE) {
        LOG_INFO_S << "Field: \"" << fieldname << "\" not found";
        return false;
    }

    gboolean gboolean_value;
    gst_structure_get_boolean(structure, fieldname, &gboolean_value);
    if (gboolean_value == TRUE) {
        return true;
    }

    return false;
}

uint64_t RTPTask::fetchUnsignedInt(const GstStructure* structure, const char* fieldname)
{
    if (gst_structure_has_field(structure, fieldname) == FALSE) {
        LOG_INFO_S << "Field: \"" << fieldname << "\" not found";
        return 0;
    }

    guint guint_value;
    if (gst_structure_get_uint(structure, fieldname, &guint_value) != FALSE) {
        return static_cast<uint64_t>(guint_value);
    }

    LOG_INFO_S << "Invalid guint value for" << fieldname;
    return 0;
}

std::string RTPTask::fetchString(const GstStructure* structure, const char* fieldname)
{
    if (gst_structure_has_field(structure, fieldname) == FALSE) {
        LOG_INFO_S << "Field: \"" << fieldname << "\" not found";
        return std::string("");
    }

    const gchar* gstr_value = gst_structure_get_string(structure, fieldname);
    if (gstr_value != nullptr) {
        return std::string(gstr_value);
    }
    
    LOG_INFO_S << "Invalid value for " << fieldname;
    return std::string("");
}

std::vector<RTPPeerReceiverReport> RTPTask::fetchPeerReceiverReports(
    const GstStructure* structure,
    const char* fieldname)
{
    const GValue* g_array_value = gst_structure_get_value(structure, fieldname);

    if (!g_array_value) {
        LOG_INFO_S << "peer-receiver_reports are empty";
        return {};
    }

    std::vector<RTPPeerReceiverReport> all_reports;
    size_t array_size = gst_value_array_get_size(g_array_value);
    for (size_t i = 0; i < array_size; i++) {
        const GValue* report_value = gst_value_array_get_value(g_array_value, i);
        if (!report_value) {
            continue;
        }

        const GstStructure* report_structure = gst_value_get_structure(report_value);
        RTPPeerReceiverReport report;
        report.ssrc = fetchUnsignedInt(report_structure, "rb-ssrc");
        report.sender_ssrc = fetchUnsignedInt(report_structure, "rb-sender-ssrc");
        report.fractionlost = fetchUnsignedInt(report_structure, "rb-fractionlost");
        report.packetslost = fetchUnsignedInt(report_structure, "rb-packetslost");
        report.exthighestseq = fetchUnsignedInt(report_structure, "rb-exthighestseq");
        report.jitter = fetchUnsignedInt(report_structure, "rb-jitter");
        report.lsr = base::Time::fromSeconds(
            static_cast<double>(fetchUnsignedInt(report_structure, "rb-lsr")));
        report.dlsr = base::Time::fromSeconds(
            static_cast<double>(fetchUnsignedInt(report_structure, "rb-dlsr")));
        report.round_trip = base::Time::fromSeconds(
            static_cast<double>(fetchUnsignedInt(report_structure, "rb-round-trip")));
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

    // this should be inside the configure hook, but i don't know if it will work..
    // m_pipeline is not a bin there. But i'm not sure it is here either, after all
    // the problem was a timeout the task was also stopped on wetpaint.

    for (auto monitored_session : m_rtp_monitored_sessions.sessions) {
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