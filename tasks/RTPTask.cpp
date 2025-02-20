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

std::vector<RTPSessionStatistics> RTPTask::RTPStats(std::vector<GstElement*> sessions)
{

    GstStructure stats;
    std::vector<RTPSessionStatistics> bin_statistics;
    for (size_t i = 0; i < sessions.size(); i++) {
        RTPSessionStatistics session_statistics;
        g_object_get(sessions[i], "stats", &stats, NULL);
        // Default statisticsd present in all sessions
        session_statistics.ssrc = fetchUnsignedInt(stats, "ssrc");
        session_statistics.internal = fetchBoolean(stats, "internal");
        session_statistics.validated = fetchBoolean(stats, "validated");
        session_statistics.received_bye = fetchBoolean(stats, "received_bye");
        session_statistics.is_csrc = fetchBoolean(stats, "is_csrc");
        session_statistics.is_sender = fetchBoolean(stats, "is_sender");
        session_statistics.seqnum_base = fetchUnsignedInt(stats, "seqnum_base");
        session_statistics.clock_rate = fetchUnsignedInt(stats, "clock_rate");

        // Receiver stats update
        session_statistics.receiver = RTPReceiverStats(stats);

        // Sender stats update
        if (session_statistics.is_sender) {
            session_statistics.sender = RTPSenderStats(stats);
        }

        session_statistics.timestamp = base::Time::now();
        bin_statistics.push_back(session_statistics);
    }

    return bin_statistics;
}

RTPReceiverStatistics RTPTask::RTPReceiverStats(GstStructure const stats)
{
    RTPReceiverStatistics receiver_statistics;

    receiver_statistics.rtp_from = fetchString(stats, "rtp_from");
    receiver_statistics.rtcp_from = fetchString(stats, "rtcp_from");
    receiver_statistics.received_nack_count = fetchUnsignedInt(stats, "recv_nack_count");
    receiver_statistics.received_packet_rate =
        fetchUnsignedInt(stats, "reck_packet_rate");
    receiver_statistics.received_picture_loss_count =
        fetchUnsignedInt(stats, "recv_pli_count");

    return receiver_statistics;
}

RTPSenderStatistics RTPTask::RTPSenderStats(GstStructure const stats)
{
    RTPSenderStatistics sender_statistics;
    sender_statistics.payload_bytes_sent = fetchUnsignedInt(stats, "octets_sent");
    sender_statistics.packets_sent = fetchUnsignedInt(stats, "packets_sent");
    sender_statistics.payload_bytes_received = fetchUnsignedInt(stats, "octets_received");
    sender_statistics.packets_received = fetchUnsignedInt(stats, "packets_received");
    sender_statistics.bytes_received = fetchUnsignedInt(stats, "bytes_received");
    sender_statistics.bitrate = fetchUnsignedInt(stats, "bitrate");
    sender_statistics.packets_lost = fetchUnsignedInt(stats, "packets_lost");
    sender_statistics.jitter = fetchUnsignedInt(stats, "jitter");
    sender_statistics.sent_picture_loss_count = fetchUnsignedInt(stats, "sent_pli_count");
    sender_statistics.sent_full_image_request_count =
        fetchUnsignedInt(stats, "sent_fir_count");
    sender_statistics.received_full_image_request_count =
        fetchUnsignedInt(stats, "recv_fir_count");
    sender_statistics.sent_nack_count = fetchUnsignedInt(stats, "sent_nack_count");
    sender_statistics.have_sr = fetchBoolean(stats, "have_sr");
    sender_statistics.in_ntptime = base::Time::fromSeconds(
        static_cast<double>(fetchUnsignedInt(stats, "in_ntptime")));
    sender_statistics.sr_rtptime = fetchUnsignedInt(stats, "sr_rtptime");
    sender_statistics.sr_octet_count = fetchUnsignedInt(stats, "sr_octet_count");
    sender_statistics.sr_packet_count = fetchUnsignedInt(stats, "sr_packet_count");
    sender_statistics.sent_receiver_block = fetchBoolean(stats, "sent_rb");
    sender_statistics.sent_receiver_block_fractionlost =
        fetchUnsignedInt(stats, "sent_rb_fractionlost");
    sender_statistics.sent_receiver_block_packetslost =
        fetchUnsignedInt(stats, "sent_rb_packets_lost");
    sender_statistics.sent_receiver_block_exthighestseq =
        fetchUnsignedInt(stats, "sent_rb_exthighestseq");
    sender_statistics.sent_receiver_block_jitter =
        fetchUnsignedInt(stats, "sent_rb_jitter");
    sender_statistics.sent_receiver_block_lsr = base::Time::fromSeconds(
        static_cast<double>(fetchUnsignedInt(stats, "sent_rb_lsr")));
    sender_statistics.sent_receiver_block_dlsr = base::Time::fromSeconds(
        static_cast<double>(fetchUnsignedInt(stats, "sent_rb_dlsr")));
    sender_statistics.payload_bytes_sent = fetchUnsignedInt(stats, "octets_sent");
    sender_statistics.packets_sent = fetchUnsignedInt(stats, "packets_sent");
    sender_statistics.payload_bytes_received = fetchUnsignedInt(stats, "octets_received");
    sender_statistics.packets_received = fetchUnsignedInt(stats, "packets_received");
    sender_statistics.bytes_received = fetchUnsignedInt(stats, "bytes_received");
    sender_statistics.bitrate = fetchUnsignedInt(stats, "bitrate");
    sender_statistics.packets_lost = fetchUnsignedInt(stats, "packets_lost");
    sender_statistics.jitter = fetchUnsignedInt(stats, "jitter");
    sender_statistics.sent_picture_loss_count = fetchUnsignedInt(stats, "sent_pli_count");
    sender_statistics.sent_full_image_request_count =
        fetchUnsignedInt(stats, "sent_fir_count");
    sender_statistics.received_full_image_request_count =
        fetchUnsignedInt(stats, "recv_fir_count");
    sender_statistics.sent_nack_count = fetchUnsignedInt(stats, "sent_nack_count");
    sender_statistics.have_sr = fetchBoolean(stats, "have_sr");
    sender_statistics.in_ntptime = base::Time::fromSeconds(
        static_cast<double>(fetchUnsignedInt(stats, "in_ntptime")));
    sender_statistics.sr_rtptime = fetchUnsignedInt(stats, "sr_rtptime");
    sender_statistics.sr_octet_count = fetchUnsignedInt(stats, "sr_octet_count");
    sender_statistics.sr_packet_count = fetchUnsignedInt(stats, "sr_packet_count");
    sender_statistics.sent_receiver_block = fetchBoolean(stats, "sent_rb");
    sender_statistics.sent_receiver_block_fractionlost =
        fetchUnsignedInt(stats, "sent_rb_fractionlost");
    sender_statistics.sent_receiver_block_packetslost =
        fetchUnsignedInt(stats, "sent_rb_packets_lost");
    sender_statistics.sent_receiver_block_exthighestseq =
        fetchUnsignedInt(stats, "sent_rb_exthighestseq");
    sender_statistics.sent_receiver_block_jitter =
        fetchUnsignedInt(stats, "sent_rb_jitter");
    sender_statistics.sent_receiver_block_lsr = base::Time::fromSeconds(
        static_cast<double>(fetchUnsignedInt(stats, "sent_rb_lsr")));
    sender_statistics.sent_receiver_block_dlsr = base::Time::fromSeconds(
        static_cast<double>(fetchUnsignedInt(stats, "sent_rb_dlsr")));
    sender_statistics.peer_receiver_reports = fetchPeerReceiverReports(stats, "receiver-rr");

    return sender_statistics;
}

bool RTPTask::fetchBoolean(const GstStructure& structure, const char* fieldname)
{
    gboolean gboolean_value;
    if (gst_structure_get_boolean(&structure, fieldname, &gboolean_value)) {
        return gboolean_value == TRUE;
    }
    LOG_INFO_S << "Field: \"" << fieldname << "\" not found or not a boolean";
    return false;
}

uint64_t RTPTask::fetchUnsignedInt(const GstStructure& structure, const char* fieldname)
{
    guint guint_value;
    if (gst_structure_get_uint(&structure, fieldname, &guint_value)) {
        return static_cast<uint64_t>(guint_value);
    }
    LOG_INFO_S << "Field: \"" << fieldname << "\" not found or not a uint";
    return 0;
}

std::string RTPTask::fetchString(const GstStructure& structure, const char* fieldname)
{
    const gchar* gstr_value = gst_structure_get_string(&structure, fieldname);
    if (gstr_value != nullptr) {
        return std::string(gstr_value);
    }
    LOG_INFO_S << "Field: \"" << fieldname << "\" not found or not a string";
    return std::string("");
}

std::vector<RTPPeerReceiverReport> RTPTask::fetchPeerReceiverReports(
    const GstStructure& structure,
    const char* fieldname)
{
    std::vector<RTPPeerReceiverReport> all_reports;
    const GValue* g_array_value;
    g_array_value = gst_structure_get_value(&structure, fieldname);

    if (!g_array_value) {
        LOG_INFO_S << "peer-receiver_reports are empty";
        return all_reports;
    }

    size_t array_size = gst_value_array_get_size(g_array_value);
    for (size_t i = 0; i < array_size; i++) {
        const GValue* report_value = gst_value_array_get_value(g_array_value, i);
        if (report_value) {
            const GstStructure* report_structure = gst_value_get_structure(report_value);
            RTPPeerReceiverReport report;
            report.ssrc = fetchUnsignedInt(*report_structure, "rb-ssrc");
            report.sender_ssrc = fetchUnsignedInt(*report_structure, "rb-sender-ssrc");
            report.fractionlost = fetchUnsignedInt(*report_structure, "rb-fractionlost");
            report.packetslost = fetchUnsignedInt(*report_structure, "rb-packetslost");
            report.exthighestseq =
                fetchUnsignedInt(*report_structure, "rb-exthighestseq");
            report.jitter = fetchUnsignedInt(*report_structure, "rb-jitter");
            report.lsr = base::Time::fromSeconds(
                static_cast<double>(fetchUnsignedInt(*report_structure, "rb-lsr")));
            report.dlsr = base::Time::fromSeconds(
                static_cast<double>(fetchUnsignedInt(*report_structure, "rb-dlsr")));
            report.round_trip = base::Time::fromSeconds(static_cast<double>(
                fetchUnsignedInt(*report_structure, "rb-round-trip")));
            all_reports.push_back(report);
        }
    }

    return all_reports;
}

bool RTPTask::configureHook()
{
    auto rtp_sessions = _rtp_monitored_sessions.get();

    m_bin = gst_bin_get_by_name(GST_BIN(m_pipeline), (rtp_sessions.rtpbin_name.c_str()));
    if (!m_bin) {
        throw std::runtime_error(
            "cannot find element named " + rtp_sessions.rtpbin_name + " in pipeline");
    }

    for (size_t i = 0; i < rtp_sessions.sessions.size(); i++) {
        GstElement* session = nullptr;
        g_signal_emit_by_name(m_bin,
            "get-internal-session",
            rtp_sessions.sessions[i].session_id,
            &session);
        m_rtp_internal_sessions.push_back(session);
    }

    if (!RTPTaskBase::configureHook())
        return false;
    return true;
}

bool RTPTask::startHook()
{
    if (!RTPTaskBase::startHook())
        return false;
    return true;
}

void RTPTask::updateHook()
{
    std::vector<RTPSessionStatistics> stats = RTPStats(m_rtp_internal_sessions);
    _rtp_statistics.write(stats);

    RTPTaskBase::updateHook();
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