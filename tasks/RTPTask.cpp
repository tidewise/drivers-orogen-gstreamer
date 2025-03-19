/* Generated from orogen/lib/orogen/templates/tasks/Task.cpp */

#include <base-logging/Logging.hpp>
#include <gst/gstcaps.h>
#include <set>

#include "Helpers.hpp"
#include "RTPHelpers.hpp"
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

RTPSessionStatistics RTPTask::updateRTPSessionStats(GstElement* session)
{
    RTPSessionStatistics session_stats;

    GstStructure* gst_stats = nullptr;
    g_object_get(session, "stats", &gst_stats, NULL);

    if (!gst_stats) {
        throw std::runtime_error("gst-stats is not a boxed type");
    }

    session_stats = extractRTPSessionStats(gst_stats);

    const GValue* gst_source_stats_value =
        gst_structure_get_value(gst_stats, "source-stats");
    GValueArray* gst_source_stats =
        static_cast<GValueArray*>(g_value_get_boxed(gst_source_stats_value));

    for (guint i = 0; i < gst_source_stats->n_values; i++) {
        GValue* value = g_value_array_get_nth(gst_source_stats, i);
        GstStructure* gst_stats = static_cast<GstStructure*>(g_value_get_boxed(value));

        auto source_stats =
            extractRTPSourceStats(gst_stats, m_rtp_monitored_sessions.rtpbin_name);
        bool sender = fetchBoolean(gst_stats, "is-sender");
        if (sender) {
            auto sender_stats = extractRTPSenderStats(gst_stats, source_stats.clock_rate);
            sender_stats.source_stats = source_stats;
            session_stats.sender_stats.push_back(sender_stats);
        }
        else {
            auto receiver_stats = extractRTPReceiverStats(gst_stats);
            receiver_stats.source_stats = source_stats;
            session_stats.receiver_stats.push_back(receiver_stats);
        }
    }

    gst_structure_free(gst_stats);
    return session_stats;
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

    // Free previous session variables.
    // This cannot be done on stop hook since it
    // will make tasks die unexpectedly on process server
    for (auto const& internal_session : m_rtp_sessions) {
        g_free(internal_session);
    }
    // "Clear" m_rtp_sessions
    m_rtp_sessions.clear();

    return true;
}

bool RTPTask::startHook()
{
    if (!RTPTaskBase::startHook())
        return false;

    std::vector<GstElement*> sessions;
    for (auto const& monitored_session : m_rtp_monitored_sessions.sessions) {
        GstElement* session = nullptr;
        g_signal_emit_by_name(m_bin,
            "get-session",
            monitored_session.session_id,
            &session);
        if (!session) {
            throw std::runtime_error(
                "did not resolve provided session, wrong session ID " +
                to_string(monitored_session.session_id) + " ?");
        }
        sessions.push_back(session);
    }

    m_rtp_sessions = sessions;
    return true;
}

void RTPTask::updateHook()
{
    RTPTaskBase::updateHook();

    RTPStatistics stats;
    stats.time = base::Time::now();
    for (auto const& element : m_rtp_sessions) {
        stats.statistics.push_back(updateRTPSessionStats(element));
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