/* Generated from orogen/lib/orogen/templates/tasks/Task.cpp */

#include "WebRTCSendTask.hpp"
#include "Helpers.hpp"
#include <base-logging/Logging.hpp>

using namespace gstreamer;
using namespace std;
using namespace webrtc_base;

WebRTCSendTask::WebRTCSendTask(std::string const& name)
    : WebRTCSendTaskBase(name)
{
}

WebRTCSendTask::~WebRTCSendTask()
{
}

/// The following lines are template definitions for the various state machine
// hooks defined by Orocos::RTT. See WebRTCSendTask.hpp for more detailed
// documentation about them.

bool WebRTCSendTask::configureHook()
{
    if (!WebRTCSendTaskBase::configureHook())
        return false;

    mPipeline = createPipeline();
    return true;
}
bool WebRTCSendTask::startHook()
{
    if (!WebRTCSendTaskBase::startHook())
        return false;

    startPipeline();
    return true;
}
void WebRTCSendTask::updateHook()
{
    WebRTCSendTaskBase::updateHook();
}

void WebRTCSendTask::processSignallingMessage(SignallingMessage const& message)
{
    LOG_INFO_S << "Signalling: " << message.type << " from " << message.from << ": "
               << message.message;
    auto sig_config = _signalling_config.get();
    if (!sig_config.remote_peer_id.empty() && sig_config.remote_peer_id != message.from) {
        LOG_ERROR_S << "Received signalling message from " + message.from +
                           " but this component is configured to establish "
                           "connection with " +
                           sig_config.remote_peer_id + " only";
        return;
    }

    if (message.type == SIGNALLING_REQUEST_OFFER) {
        if (sig_config.polite) {
            LOG_ERROR_S << "Polite peer trying to request offer, but polite is true";
            return;
        }

        handlePeerDisconnection(message.from);
        configurePeer(message.from);
        return;
    }
    else if (message.type == SIGNALLING_OFFER) {
        auto sig_config = _signalling_config.get();
        if (!sig_config.polite) {
            LOG_ERROR_S << "Received offer, but polite is false";
            return;
        }

        handlePeerDisconnection(message.from);
        configurePeer(message.from);
    }

    auto peer_it = findPeerByID(message.from);
    if (peer_it == m_peers.end()) {
        LOG_ERROR_S << "Receiving signalling from " << message.from
                    << " but never received a start-of-negotiation message";
        return;
    }
    WebRTCCommonTask::processSignallingMessage(peer_it->second.webrtcbin, message);
}
void WebRTCSendTask::errorHook()
{
    WebRTCSendTaskBase::errorHook();
}
void WebRTCSendTask::stopHook()
{
    WebRTCSendTaskBase::stopHook();
}
void WebRTCSendTask::cleanupHook()
{
    WebRTCSendTaskBase::cleanupHook();
}

GstElement* WebRTCSendTask::createPipeline()
{
    string pipelineDefinition = "appsrc format=3 do-timestamp=TRUE "
                                "is-live=true name=src !" +
                                _encoding_pipeline.get() + " ! tee name=splitter";

    GError* error = nullptr;
    GstElement* pipeline = gst_parse_launch(pipelineDefinition.c_str(), &error);
    if (error) {
        string message(error->message);
        g_error_free(error);
        throw std::runtime_error(
            "could not create encoding pipeline " + pipelineDefinition + ": " + message);
    }

    configureInput(pipeline, "src", false, _video_in);
    return pipeline;
}

void WebRTCSendTask::configurePeer(string const& peer_id)
{
    GstElement* bin = gst_bin_new((peer_id + "_bin").c_str());
    GstElement* queue = gst_element_factory_make("queue", ("q_" + peer_id).c_str());
    GstElement* webrtc = gst_element_factory_make("webrtcbin", peer_id.c_str());
    gst_bin_add_many(GST_BIN(bin), queue, webrtc, nullptr);
    gst_element_link_many(queue, webrtc, nullptr);
    GstUnrefGuard<GstPad> queue_sink(gst_element_get_static_pad(queue, "sink"));
    auto pad = gst_ghost_pad_new("sink", queue_sink.get());
    gst_element_add_pad(bin, pad);
    gst_bin_add(GST_BIN(mPipeline), bin);

    GstUnrefGuard<GstElement> splitter(
        gst_bin_get_by_name(GST_BIN(mPipeline), "splitter"));
    GstUnrefGuard<GstPad> srcpad(gst_element_get_request_pad(splitter.get(), "src_%u"));
    gst_pad_link(srcpad.get(), pad);

    configureWebRTCBin(peer_id, webrtc);
    auto& elements = m_dynamic_elements[webrtc];
    elements.tee_pad = srcpad.get();
    elements.bin = bin;

    bool ret = gst_element_sync_state_with_parent(bin);
    if (!ret) {
        throw std::runtime_error(
            "failed to create streaming elements for peer " + peer_id);
    }

    LOG_INFO_S << "Configured pipeline for peer " << peer_id;
    GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(mPipeline),
        GST_DEBUG_GRAPH_SHOW_ALL,
        "sender-configured");
}

void WebRTCSendTask::handlePeerDisconnection(std::string const& peer_id)
{
    auto peer_it = findPeerByID(peer_id);
    if (peer_it != m_peers.end()) {
        disconnectPeer(peer_it);
    }
}

void WebRTCSendTask::disconnectPeer(PeerMap::iterator peer_it)
{
    Peer peer = peer_it->second;
    m_peers.erase(peer_it);

    auto it = m_dynamic_elements.find(peer.webrtcbin);
    auto elements = it->second;
    m_dynamic_elements.erase(it);

    gst_element_set_state(elements.bin, GST_STATE_NULL);
    GstUnrefGuard<GstElement> splitter(
        gst_bin_get_by_name(GST_BIN(mPipeline), "splitter"));
    gst_element_unlink_many(splitter.get(), elements.bin, nullptr);
    gst_bin_remove_many(GST_BIN(mPipeline), elements.bin, nullptr);

    gst_element_release_request_pad(splitter.get(), elements.tee_pad);
}

void WebRTCSendTask::destroyPipeline()
{
    m_dynamic_elements.clear();

    WebRTCSendTaskBase::destroyPipeline();
}
