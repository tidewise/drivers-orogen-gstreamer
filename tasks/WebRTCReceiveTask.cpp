/* Generated from orogen/lib/orogen/templates/tasks/Task.cpp */

#include "WebRTCReceiveTask.hpp"
#include "Helpers.hpp"
#include <base-logging/Logging.hpp>

using namespace gstreamer;
using namespace std;
using namespace webrtc_base;

/*
 * Relevant GStreamer example:
 * https://gitlab.freedesktop.org/gstreamer/gst-examples/-/blob/master/webrtc/sendrecv/gst/webrtc-sendrecv.c
 */

WebRTCReceiveTask::WebRTCReceiveTask(std::string const& name)
    : WebRTCReceiveTaskBase(name)
{
}

WebRTCReceiveTask::~WebRTCReceiveTask()
{
}

/// The following lines are template definitions for the various state machine
// hooks defined by Orocos::RTT. See WebRTCReceiveTask.hpp for more detailed
// documentation about them.

bool WebRTCReceiveTask::configureHook()
{
    if (!WebRTCReceiveTaskBase::configureHook())
        return false;

    auto config = _signalling.get();
    if (!config.polite && !config.remote_peer_id.empty()) {
        m_pipeline = createPipeline(config.remote_peer_id);
    }
    m_decode_queue_max_size_time = _decode_queue_max_size_time.get();

    return true;
}
bool WebRTCReceiveTask::startHook()
{
    if (!WebRTCReceiveTaskBase::startHook())
        return false;

    if (m_pipeline) {
        startPipeline();
    }

    return true;
}
void WebRTCReceiveTask::updateHook()
{
    WebRTCReceiveTaskBase::updateHook();
}
void WebRTCReceiveTask::processSignallingMessage(SignallingMessage const& message)
{
    LOG_INFO_S << "Signalling: " << message.type << " from " << message.from << ": "
               << message.message;
    auto sig_config = _signalling.get();
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

        if (m_pipeline) {
            destroyPipeline();
        }
        m_pipeline = createPipeline(message.from);
        startPipeline();
        return;
    }
    else if (message.type == SIGNALLING_OFFER) {
        auto sig_config = _signalling.get();
        if (!sig_config.polite) {
            LOG_ERROR_S << "Received offer, but polite is false";
            return;
        }

        if (m_pipeline) {
            destroyPipeline();
        }
        m_pipeline = createPipeline(message.from);
        startPipeline();
    }

    auto webrtcbin = m_peers.begin()->first;
    WebRTCCommonTask::processSignallingMessage(webrtcbin, message);
}

void WebRTCReceiveTask::updatePeersStats()
{
    WebRTCPeerStats stats;
    if (!m_peers.empty()) {
        auto& peer = *m_peers.begin();
        _stats.write(peer.second);
    }
    else {
        _stats.write(WebRTCPeerStats());
    }
}

void WebRTCReceiveTask::handlePeerDisconnection(std::string const& peer_id)
{
    if (peer_id != getCurrentPeer()) {
        return;
    }

    destroyPipeline();
}

void WebRTCReceiveTask::errorHook()
{
    WebRTCReceiveTaskBase::errorHook();
}
void WebRTCReceiveTask::stopHook()
{
    if (m_pipeline) {
        destroyPipeline();
    }

    WebRTCReceiveTaskBase::stopHook();
}
void WebRTCReceiveTask::cleanupHook()
{
    WebRTCReceiveTaskBase::cleanupHook();
}

string WebRTCReceiveTask::getCurrentPeer() const
{
    if (m_peers.empty()) {
        return string();
    }

    return m_peers.begin()->second.peer_id;
}
GstElement* WebRTCReceiveTask::createPipeline(string const& peer_id)
{
    GstUnrefGuard<GstElement> pipe(gst_pipeline_new("receivepipe"));
    GstElement* webrtc = gst_element_factory_make("webrtcbin", "webrtc");
    if (!webrtc) {
        throw std::runtime_error("could not resolve webrtc element in created pipeline");
    }

    gst_bin_add(GST_BIN(pipe.get()), webrtc);
    configureWebRTCBin(peer_id, webrtc);
    g_signal_connect(webrtc, "pad-added", G_CALLBACK(callbackIncomingStream), this);

    LOG_INFO_S << "Created pipeline " << hasPeer(peer_id);

    return pipe.release();

    // TODO: look into TWCC
}

void WebRTCReceiveTask::handleVideoStream(GstElement* bin, GstPad* pad)
{
    GstElement* conv = gst_element_factory_make("videoconvert", NULL);
    GstElement* sink = gst_element_factory_make("appsink", "video_out");
    // The max buffers is set to 1, so it gets the first value and send
    // it directly, so it disables the buffer behavior
    g_object_set(sink, "max-buffers", 1, NULL);
    g_object_set(sink, "drop", true, NULL);

    gst_bin_add_many(GST_BIN(bin), conv, sink, NULL);
    gst_element_sync_state_with_parent(conv);
    gst_element_sync_state_with_parent(sink);
    gst_element_link_many(conv, sink, NULL);
    configureOutput(bin, "video_out", _frame_mode.get(), false, _video_out);

    GstPad* qpad = gst_element_get_static_pad(conv, "sink");

    GstPadLinkReturn ret = gst_pad_link(pad, qpad);
    g_assert_cmphex(ret, ==, GST_PAD_LINK_OK);
}

void WebRTCReceiveTask::callbackIncomingDecodebinStream(GstElement* decodebin,
    GstPad* pad,
    WebRTCReceiveTask* task)
{
    LOG_INFO_S << "New decodebin stream";

    if (!gst_pad_has_current_caps(pad)) {
        g_printerr("Pad '%s' has no caps, can't do anything, ignoring\n",
            GST_PAD_NAME(pad));
        return;
    }

    GstElement* bin = GST_ELEMENT(gst_element_get_parent(decodebin));
    task->handleVideoStream(bin, pad);
}

void WebRTCReceiveTask::callbackIncomingStream(GstElement* webrtcbin,
    GstPad* pad,
    WebRTCReceiveTask* task)
{
    task->onIncomingStream(webrtcbin, pad);
}

void WebRTCReceiveTask::onIncomingStream(GstElement* webrtcbin, GstPad* pad)
{
    LOG_INFO_S << "New stream received";

    if (GST_PAD_DIRECTION(pad) != GST_PAD_SRC) {
        LOG_INFO_S << "New stream pad is sink, not source. Ignoring";
        return;
    }

    auto const& peer = m_peers[webrtcbin];
    GstElement* bin = gst_bin_new((peer.peer_id + "_receivebin").c_str());
    gst_bin_add(GST_BIN(m_pipeline), bin);

    GstElement* q = gst_element_factory_make("queue", NULL);
    GstElement* decodebin = gst_element_factory_make("decodebin", NULL);
    gst_bin_add_many(GST_BIN(bin), decodebin, q, NULL);
    gst_element_link_many(q, decodebin, NULL);

    g_object_set(q, "leaky", 1, NULL); // downstream - drop old buffers
    g_object_set(q,
        "max-size-time",
        m_decode_queue_max_size_time.toMicroseconds() * 1000,
        NULL);

    g_signal_connect(decodebin,
        "pad-added",
        G_CALLBACK(callbackIncomingDecodebinStream),
        this);

    GstUnrefGuard<GstPad> q_sink(gst_element_get_static_pad(q, "sink"));
    auto bin_pad = gst_ghost_pad_new("sink", q_sink.get());
    gst_element_add_pad(bin, bin_pad);
    gst_pad_link(pad, bin_pad);

    gst_element_sync_state_with_parent(bin);
    gst_element_sync_state_with_parent(decodebin);
}
