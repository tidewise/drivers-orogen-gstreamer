/* Generated from orogen/lib/orogen/templates/tasks/Task.cpp */

#include "WebRTCReceiveTask.hpp"
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

    return true;
}
bool WebRTCReceiveTask::startHook()
{
    if (!WebRTCReceiveTaskBase::startHook())
        return false;

    auto config = _signalling_config.get();
    if (!config.polite && !config.remote_peer_id.empty()) {
        mPipeline = createPipeline(config.remote_peer_id);
        startPipeline();
    }
    else if (config.polite) {
        mPipeline = createPipeline();
        startPipeline();
    }

    return true;
}
void WebRTCReceiveTask::updateHook()
{
    WebRTCReceiveTaskBase::updateHook();

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

            if (mPipeline) {
                destroyPipeline();
            }
            mPipeline = createPipeline(message.peer_id);
            continue;
        }
        else if (message.type == SIGNALLING_OFFER) {
            auto sig_config = _signalling_config.get();
            if (!sig_config.polite) {
                LOG_ERROR_S << "Received offer, but polite is false";
                continue;
            }

            m_peers.begin()->second.peer_id = message.peer_id;
        }

        auto webrtcbin = m_peers.begin()->first;
        processSignallingMessage(webrtcbin, message);
    }
}
void WebRTCReceiveTask::errorHook()
{
    WebRTCReceiveTaskBase::errorHook();
}
void WebRTCReceiveTask::stopHook()
{
    WebRTCReceiveTaskBase::stopHook();
}
void WebRTCReceiveTask::cleanupHook()
{
    WebRTCReceiveTaskBase::cleanupHook();
}

GstElement* WebRTCReceiveTask::createPipeline(string const& peer_id)
{
    GError* error;
    GstElement* pipe = gst_parse_launch("webrtcbin name=webrtc", &error);

    if (error) {
        std::string msg(error->message);
        g_error_free(error);
        throw std::runtime_error("Failed to create pipeline: " + msg);
    }

    GstElement* webrtc = gst_bin_get_by_name(GST_BIN(pipe), "webrtc");
    configureWebRTCBin(peer_id, webrtc);
    g_signal_connect(webrtc, "pad-added", G_CALLBACK(callbackIncomingStream), this);
    gst_object_unref(webrtc);

    return pipe;

    // TODO: look into TWCC
}

void WebRTCReceiveTask::handleVideoStream(GstElement* bin, GstPad* pad)
{
    GstElement* q = gst_element_factory_make("queue", NULL);
    g_assert_nonnull(q);
    GstElement* conv = gst_element_factory_make("videoconvert", NULL);
    g_assert_nonnull(conv);
    GstElement* sink = gst_element_factory_make("appsink", NULL);
    gst_element_set_name(sink, "video_out");
    g_assert_nonnull(sink);
    configureOutput(bin, "video_out", _frame_mode.get(), false, _video_out);

    gst_bin_add_many(GST_BIN(bin), q, conv, sink, NULL);
    gst_element_sync_state_with_parent(q);
    gst_element_sync_state_with_parent(conv);
    gst_element_sync_state_with_parent(sink);
    gst_element_link_many(q, conv, sink, NULL);

    GstPad* qpad = gst_element_get_static_pad(q, "sink");

    GstPadLinkReturn ret = gst_pad_link(pad, qpad);
    g_assert_cmphex(ret, ==, GST_PAD_LINK_OK);
}

void WebRTCReceiveTask::callbackIncomingDecodebinStream(GstElement* decodebin,
    GstPad* pad,
    WebRTCReceiveTask* task)
{
    if (!gst_pad_has_current_caps(pad)) {
        g_printerr("Pad '%s' has no caps, can't do anything, ignoring\n",
            GST_PAD_NAME(pad));
        return;
    }

    GstCaps* caps = gst_pad_get_current_caps(pad);
    const gchar* name = gst_structure_get_name(gst_caps_get_structure(caps, 0));
    if (g_str_has_prefix(name, "video")) {
        GstElement* bin = GST_ELEMENT(gst_element_get_parent(decodebin));
        task->handleVideoStream(bin, pad);
    }
    else {
        gst_printerr("Unknown pad %s, ignoring", GST_PAD_NAME(pad));
    }
}

void WebRTCReceiveTask::callbackIncomingStream(GstElement* webrtcbin,
    GstPad* pad,
    WebRTCReceiveTask* task)
{
    task->onIncomingStream(webrtcbin, pad);
}

void WebRTCReceiveTask::onIncomingStream(GstElement* webrtcbin, GstPad* pad)
{
    if (GST_PAD_DIRECTION(pad) != GST_PAD_SRC) {
        return;
    }

    GstElement* decodebin = gst_element_factory_make("decodebin", NULL);
    g_signal_connect(decodebin,
        "pad-added",
        G_CALLBACK(callbackIncomingDecodebinStream),
        this);
    gst_bin_add(GST_BIN(webrtcbin), decodebin);
    gst_element_sync_state_with_parent(decodebin);

    GstPad* decode_sink = gst_element_get_static_pad(decodebin, "sink");
    gst_pad_link(pad, decode_sink);
    gst_object_unref(decode_sink);
}