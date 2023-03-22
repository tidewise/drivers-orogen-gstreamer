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
        if (message.type == SIGNALLING_NEW_PEER) {
            continue;
        }

        LOG_INFO_S << "Signalling: " << message.type << " from " << message.from << ": "
                   << message.message;
        auto sig_config = _signalling_config.get();
        if (!sig_config.remote_peer_id.empty() &&
            sig_config.remote_peer_id != message.from) {
            LOG_ERROR_S << "Received signalling message from " + message.from +
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

            auto peer_it = findPeerByID(message.from);
            if (peer_it != m_peers.end()) {
                disconnectPeer(peer_it);
            }
            configurePeer(message.from);
            continue;
        }
        else if (message.type == SIGNALLING_OFFER) {
            auto sig_config = _signalling_config.get();
            if (!sig_config.polite) {
                LOG_ERROR_S << "Received offer, but polite is false";
                continue;
            }

            auto peer_it = findPeerByID(message.from);
            if (peer_it != m_peers.end()) {
                disconnectPeer(peer_it);
            }
            configurePeer(message.from);
        }

        auto peer_it = findPeerByID(message.from);
        if (peer_it == m_peers.end()) {
            LOG_ERROR_S << "Receiving signalling from " << message.from
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
    GstElement* queue = gst_element_factory_make("queue", ("q_" + peer_id).c_str());
    GstElement* bin = gst_element_factory_make("webrtcbin", peer_id.c_str());
    gst_bin_add_many(GST_BIN(mPipeline), queue, bin, NULL);
    gst_element_link_many(queue, bin, NULL);

    GstUnrefGuard<GstElement> splitter(
        gst_bin_get_by_name(GST_BIN(mPipeline), "splitter"));

    GstUnrefGuard<GstPad> srcpad(gst_element_get_request_pad(splitter.get(), "src_%u"));
    GstUnrefGuard<GstPad> sinkpad(gst_element_get_static_pad(queue, "sink"));
    gst_pad_link(srcpad.get(), sinkpad.get());
    m_tee_pads[bin] = srcpad.get();

    auto& peer = m_peers[bin];
    peer.peer_id = peer_id;
    peer.task = this;
    peer.webrtcbin = bin;
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

    auto it = m_tee_pads.find(peer.webrtcbin);
    gst_element_release_request_pad(splitter.get(), it->second);
    m_tee_pads.erase(it);
}