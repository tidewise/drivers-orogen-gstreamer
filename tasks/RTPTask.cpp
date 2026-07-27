/* Generated from orogen/lib/orogen/templates/tasks/Task.cpp */

#include <base-logging/Logging.hpp>
#include <gst/gstcaps.h>
#include <set>

#include "Helpers.hpp"
#include "RTPHelpers.hpp"
#include "RTPTask.hpp"

using namespace std;
using namespace gstreamer;
using namespace gstreamer::memory;
using namespace gstreamer::rtpbin;
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
            extractRTPSourceStats(gst_stats, m_rtp_monitoring_config.rtpbin_name);
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

    m_rtp_monitoring_config = _rtp_monitoring_config.get();
    auto rtpbin = pipelineConfigure();

    std::vector<GstUnrefGuard<GstElement>> sessions;
    sessions.reserve(m_rtp_monitoring_config.sessions_id.size());
    for (uint32_t session_id : m_rtp_monitoring_config.sessions_id) {
        GstElement* session{nullptr};
        g_signal_emit_by_name(rtpbin.get(), "get-session", session_id, &session);
        if (!session) {
            throw std::runtime_error(
                "did not resolve provided session, wrong session ID " +
                to_string(session_id) + " ?");
        }
        sessions.emplace_back(session);
    }

    m_rtp_sessions = std::move(sessions);

    return true;
}

GstUnrefGuard<GstElement> RTPTask::pipelineConfigure()
{
    std::string& rtpbin_name{m_rtp_monitoring_config.rtpbin_name};
    GstUnrefGuard<GstElement> bin(
        gst_bin_get_by_name(m_pipeline.get(), rtpbin_name.c_str()));
    if (!bin.get()) {
        throw std::runtime_error(
            "cannot find element named " + rtpbin_name + " in pipeline");
    }

    auto receiver_mapping = _receiver_map.get();
    auto sender_mapping = _sender_map.get();

    uint8_t role{0};
    if (!receiver_mapping.undefined()) {
        role |= 0x1;
        LOG_DEBUG_S << "receiver mappings defined" << std::endl;
    }

    if (!sender_mapping.undefined()) {
        role |= 0x2;
        LOG_DEBUG_S << "sender mappings defined" << std::endl;
    }

    if (!role) {
        return bin;
    }

    switch (role) {
        case 0x01: {
            receiver::Context ctx = {m_pipeline, receiver_mapping};
            m_receiver_context = ctx;
        }
            receiver::setup(rtpbin_name, *m_receiver_context);
            break;
        case 0x02: {
            sender::Context ctx = {m_pipeline, sender_mapping};
            m_sender_context = ctx;
        }
            sender::setup(rtpbin_name, *m_sender_context);
            break;
        default:
            throw std::invalid_argument("The component can't be configured as sender "
                                        "and receiver simultaneously.");
    };

    return bin;
}

bool RTPTask::startHook()
{
    if (!RTPTaskBase::startHook())
        return false;

    return true;
}

void RTPTask::updateHook()
{
    RTPTaskBase::updateHook();

    RTPStatistics stats;
    stats.statistics.reserve(m_rtp_sessions.size());
    stats.time = base::Time::now();
    for (auto& element : m_rtp_sessions) {
        stats.statistics.push_back(updateRTPSessionStats(element.get()));
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
    m_receiver_context = std::nullopt;
    m_sender_context = std::nullopt;
    m_rtp_sessions.clear();
}