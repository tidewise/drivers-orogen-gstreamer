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

    return true;
}
void WebRTCSendTask::updateHook()
{
    WebRTCSendTaskBase::updateHook();

    webrtc_base::SignallingMessage message;
    while (_signalling_in.read(message) == RTT::NewData) {
        auto sig_config = _signalling_config.get();
        if (!sig_config.remote_peer_id.empty() &&
            sig_config.remote_peer_id != message.peer_id) {
            LOG_ERROR_S << "Received signalling message from " + message.peer_id +
                               " but this component is configured to establish "
                               "connection with " +
                               sig_config.remote_peer_id + " only";
            continue;
        }

        if (message.type == SIGNALLING_REQUEST_OFFER) {
            if (sig_config.polite) {
                LOG_ERROR_S << "Polite peer trying to request offer, but polite is true";
                continue;
            }

            auto peer_it = findPeerByID(message.peer_id);
            if (peer_it != m_peers.end()) {
                disconnectPeer(peer_it);
            }
            configurePeer(message.peer_id);
            continue;
        }
        else if (message.type == SIGNALLING_OFFER) {
            auto sig_config = _signalling_config.get();
            if (!sig_config.polite) {
                LOG_ERROR_S << "Received offer, but polite is false";
                continue;
            }

            auto peer_it = findPeerByID(message.peer_id);
            if (peer_it != m_peers.end()) {
                disconnectPeer(peer_it);
            }
            configurePeer(message.peer_id);
        }

        auto peer_it = findPeerByID(message.peer_id);
        if (peer_it != m_peers.end()) {
            LOG_ERROR_S << "Receiving signalling from " << message.peer_id
                        << " but never received a start-of-negotiation message";
            continue;
        }
        processSignallingMessage(peer_it->second.webrtcbin, message);
    }
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
    string pipelineDefinition = "appsrc do-timestamp=TRUE is-live=true name=src !" +
                                _encoding_pipeline.get() + " ! tee name=splitter";

    GError* error = nullptr;
    GstElement* pipeline = gst_parse_launch(pipelineDefinition.c_str(), &error);
    if (error) {
        string message(error->message);
        g_error_free(error);
        throw std::runtime_error(
            "could not create encoding pipeline " + pipelineDefinition + ": " + message);
    }

    return pipeline;
}

void WebRTCSendTask::configurePeer(string const& peer_id)
{
    GstUnrefGuard<GstElement> queue(gst_element_factory_make("queue", peer_id.c_str()));
    GstUnrefGuard<GstElement> bin(gst_element_factory_make("webrtcbin", peer_id.c_str()));
    gst_bin_add_many(GST_BIN(mPipeline), queue.get(), bin.get(), NULL);
    gst_element_link_many(queue.get(), bin.get());

    GstUnrefGuard<GstElement> splitter(
        gst_bin_get_by_name(GST_BIN(mPipeline), "splitter"));

    auto srcpad = gst_element_get_request_pad(splitter.get(), "src%d");
    auto sinkpad = gst_element_get_static_pad(bin.get(), "sink");
    gst_pad_link(srcpad, sinkpad);
}

void WebRTCSendTask::disconnectPeer(PeerMap::iterator peer_it)
{
    Peer peer = peer_it->second;
    m_peers.erase(peer_it);

    if (m_peers.size() == 1) {
        gst_element_set_state(mPipeline, GST_STATE_PAUSED);
    }

    gst_element_set_state(peer.webrtcbin, GST_STATE_NULL);
    GstUnrefGuard<GstElement> splitter(
        gst_bin_get_by_name(GST_BIN(mPipeline), "splitter"));
    gst_element_unlink(splitter.get(), peer.webrtcbin);
    gst_bin_remove(GST_BIN(mPipeline), peer.webrtcbin);
}