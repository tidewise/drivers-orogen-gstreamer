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

void RTPTask::extractPipelineStatus()
{
    GstStructure* stats = NULL;
    g_object_get(m_rtpbin, "stats", &stats, NULL);
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
    fetchUnsignedInt(stats, "octets_sent", &m_rtp_statistics.octets_sent);
    fetchUnsignedInt(stats, "packets_sent", &m_rtp_statistics.packets_sent);
    fetchUnsignedInt(stats, "octets_received", &m_rtp_statistics.octets_received);
    fetchUnsignedInt(stats, "packets_received", &m_rtp_statistics.packets_received);
    fetchUnsignedInt(stats, "bytes_received", &m_rtp_statistics.bytes_received);
    fetchUnsignedInt(stats, "bitrate", &m_rtp_statistics.bitrate);
    fetchUnsignedInt(stats, "packets_lost", &m_rtp_statistics.packets_lost);
    fetchUnsignedInt(stats, "jitter", &m_rtp_statistics.jitter);
    fetchUnsignedInt(stats, "sent_pli_count", &m_rtp_statistics.sent_pli_count);
    fetchUnsignedInt(stats, "recv_pli_count", &m_rtp_statistics.recv_pli_count);
    fetchUnsignedInt(stats, "sent_fir_count", &m_rtp_statistics.sent_fir_count);
    fetchUnsignedInt(stats, "recv_fir_count", &m_rtp_statistics.recv_fir_count);
    fetchUnsignedInt(stats, "sent_nack_count", &m_rtp_statistics.sent_nack_count);
    fetchUnsignedInt(stats, "recv_nack_count", &m_rtp_statistics.recv_nack_count);
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
}

void RTPTask::fetchBoolean(const GstStructure* structure,
    const char* fieldname,
    bool *boolean)
{
    gboolean gboolean_value;
    if (gst_structure_get_boolean(structure, fieldname, &gboolean_value)) {
        *boolean = gboolean_value == TRUE;
    }
    LOG_INFO_S << "Field: \"" << fieldname << "\" not found or not a boolean";
}

void RTPTask::fetchUnsignedInt(const GstStructure* structure,
    const char* fieldname,
    int *number)
{
    guint guint_value;
    if (gst_structure_get_uint(structure, fieldname, &guint_value)) {
        *number = static_cast<int>(guint_value);
    }
    LOG_INFO_S << "Field: \"" << fieldname << "\" not found or not a uint";
}

void RTPTask::fetchString(const GstStructure* structure,
    const char* fieldname,
    std::string *str_var)
{
    const gchar* gstr_value = gst_structure_get_string(structure, fieldname);
    if (gstr_value != nullptr) {
        *str_var = std::string(gstr_value);
    }
    LOG_INFO_S << "Field: \"" << fieldname << "\" not found or not a string";
}

bool RTPTask::configureHook()
{
    m_rtpbin = gst_bin_get_by_name(GST_BIN(m_pipeline), "rtpbin");
    if (!m_rtpbin) {
        throw std::runtime_error("cannot find element named rtpbin in pipeline");
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
    extractPipelineStatus();
    _rtp_statistics.write(m_rtp_statistics);
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