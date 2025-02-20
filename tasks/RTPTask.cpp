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
        g_object_get(sessions[i], "stats", &stats, NULL);
        m_rtp_statistics.timestamp = base::Time::now();
        fetchUnsignedInt(stats, "ssrc", &m_rtp_statistics.ssrc);
        fetchBoolean(stats, "internal", &m_rtp_statistics.internal);
        fetchBoolean(stats, "validated", &m_rtp_statistics.validated);
        fetchBoolean(stats, "received_bye", &m_rtp_statistics.received_bye);
        fetchBoolean(stats, "is_csrc", &m_rtp_statistics.is_csrc);
        fetchBoolean(stats, "is_sender", &m_rtp_statistics.is_sender);
        fetchUnsignedInt(stats, "seqnum_base", &m_rtp_statistics.seqnum_base);
        fetchUnsignedInt(stats, "clock_rate", &m_rtp_statistics.clock_rate);
        fetchString(stats, "rtp_from", &m_rtp_statistics.rtp_from);
        fetchString(stats, "rtcp_from", &m_rtp_statistics.rtcp_from);
        fetchUnsignedInt(stats, "octets_sent", &m_rtp_statistics.payload_bytes_sent);
        fetchUnsignedInt(stats, "packets_sent", &m_rtp_statistics.packets_sent);
        fetchUnsignedInt(stats,
            "octets_received",
            &m_rtp_statistics.payload_bytes_received);
        fetchUnsignedInt(stats, "packets_received", &m_rtp_statistics.packets_received);
        fetchUnsignedInt(stats, "bytes_received", &m_rtp_statistics.bytes_received);
        fetchUnsignedInt(stats, "bitrate", &m_rtp_statistics.bitrate);
        fetchUnsignedInt(stats, "packets_lost", &m_rtp_statistics.packets_lost);
        fetchUnsignedInt(stats, "jitter", &m_rtp_statistics.jitter);
        fetchUnsignedInt(stats,
            "sent_pli_count",
            &m_rtp_statistics.sent_picture_loss_count);
        fetchUnsignedInt(stats,
            "recv_pli_count",
            &m_rtp_statistics.received_picture_loss_count);
        fetchUnsignedInt(stats,
            "sent_fir_count",
            &m_rtp_statistics.sent_full_image_request_count);
        fetchUnsignedInt(stats,
            "recv_fir_count",
            &m_rtp_statistics.received_full_image_requestr_count);
        fetchUnsignedInt(stats, "sent_nack_count", &m_rtp_statistics.sent_nack_count);
        fetchUnsignedInt(stats, "recv_nack_count", &m_rtp_statistics.received_nack_count);
        fetchUnsignedInt(stats, "reck_packet_rate", &m_rtp_statistics.recv_packet_rate);
        fetchBoolean(stats, "have_sr", &m_rtp_statistics.have_sr);
        fetchUnsignedInt(stats, "in_ntptime", &m_rtp_statistics.in_ntptime);
        fetchUnsignedInt(stats, "sr_rtptime", &m_rtp_statistics.sr_rtptime);
        fetchUnsignedInt(stats, "sr_octet_count", &m_rtp_statistics.sr_octet_count);
        fetchUnsignedInt(stats, "sr_packet_count", &m_rtp_statistics.sr_packet_count);
        fetchBoolean(stats, "sent_rb", &m_rtp_statistics.sent_rb);
        fetchUnsignedInt(stats,
            "sent_rb_fractionlost",
            &m_rtp_statistics.sent_rb_fractionlost);
        fetchUnsignedInt(stats,
            "sent_rb_packets_lost",
            &m_rtp_statistics.sent_rb_packetslost);
        fetchUnsignedInt(stats,
            "sent_rb_exthighestseq",
            &m_rtp_statistics.sent_rb_exthighestseq);
        fetchUnsignedInt(stats, "sent_rb_jitter", &m_rtp_statistics.sent_rb_jitter);
        fetchUnsignedInt(stats, "sent_rb_lsr", &m_rtp_statistics.sent_rb_lsr);
        fetchUnsignedInt(stats, "sent_rb_dlsr", &m_rtp_statistics.sent_rb_dlsr);

        bin_statistics.push_back(m_rtp_statistics);
    }
    return bin_statistics;
}

void RTPTask::fetchBoolean(const GstStructure& structure,
    const char* fieldname,
    bool* boolean)
{
    gboolean gboolean_value;
    if (gst_structure_get_boolean(&structure, fieldname, &gboolean_value)) {
        *boolean = gboolean_value == TRUE;
    }
    LOG_INFO_S << "Field: \"" << fieldname << "\" not found or not a boolean";
}

template <typename T>
void RTPTask::fetchUnsignedInt(const GstStructure& structure,
    const char* fieldname,
    T* number)
{
    guint guint_value;
    if (gst_structure_get_uint(&structure, fieldname, &guint_value)) {
        if constexpr (std::is_same_v<T, base::Time>) {
            *number = base::Time::fromSeconds(static_cast<double>(guint_value));
        }
        else {
            *number = static_cast<T>(guint_value);
        }
    }
    LOG_INFO_S << "Field: \"" << fieldname << "\" not found or not a uint";
}

void RTPTask::fetchString(const GstStructure& structure,
    const char* fieldname,
    std::string* str_var)
{
    const gchar* gstr_value = gst_structure_get_string(&structure, fieldname);
    if (gstr_value != nullptr) {
        *str_var = std::string(gstr_value);
    }
    LOG_INFO_S << "Field: \"" << fieldname << "\" not found or not a string";
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