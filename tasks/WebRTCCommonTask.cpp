/* Generated from orogen/lib/orogen/templates/tasks/Task.cpp */

#include "WebRTCCommonTask.hpp"
#include <base-logging/Logging.hpp>

using namespace gstreamer;
using namespace std;
using namespace webrtc_base;

WebRTCCommonTask::WebRTCCommonTask(std::string const& name)
    : WebRTCCommonTaskBase(name)
{
}

WebRTCCommonTask::~WebRTCCommonTask()
{
}

/// The following lines are template definitions for the various state machine
// hooks defined by Orocos::RTT. See WebRTCCommonTask.hpp for more detailed
// documentation about them.

bool WebRTCCommonTask::configureHook()
{
    if (!WebRTCCommonTaskBase::configureHook())
        return false;

    m_signalling_config = _signalling_config.get();
    return true;
}
bool WebRTCCommonTask::startHook()
{
    if (!WebRTCCommonTaskBase::startHook())
        return false;

    auto config = _signalling_config.get();

    SignallingMessage message;
    message.type = SIGNALLING_NEW_PEER;
    message.peer_id = config.self_peer_id;
    _signalling_out.write(message);

    if (!config.remote_peer_id.empty()) {
        if (config.polite) {
            SignallingMessage message;
            message.type = SIGNALLING_REQUEST_OFFER;
            message.peer_id = config.remote_peer_id;
            _signalling_out.write(message);
        }
    }
    return true;
}
void WebRTCCommonTask::updateHook()
{
    WebRTCCommonTaskBase::updateHook();
}
void WebRTCCommonTask::errorHook()
{
    WebRTCCommonTaskBase::errorHook();
}
void WebRTCCommonTask::stopHook()
{
    WebRTCCommonTaskBase::stopHook();
}
void WebRTCCommonTask::cleanupHook()
{
    WebRTCCommonTaskBase::cleanupHook();
}

void WebRTCCommonTask::callbackNegotiationNeeded(GstElement* webrtcbin, void* task)
{
    reinterpret_cast<WebRTCCommonTask*>(task)->onNegotiationNeeded(webrtcbin);
}
void WebRTCCommonTask::onNegotiationNeeded(GstElement* webrtcbin)
{
    if (!m_signalling_config.polite) {
        auto& data = m_peers[webrtcbin];
        GstPromise* promise =
            gst_promise_new_with_change_func(callbackOfferCreated, (gpointer)&data, NULL);
        g_signal_emit_by_name(G_OBJECT(webrtcbin), "create-offer", NULL, promise);
    }
}
void WebRTCCommonTask::callbackOfferCreated(GstPromise* promise, gpointer user_data)
{
    Peer const& peer = *reinterpret_cast<Peer*>(user_data);
    GstStructure const* reply = gst_promise_get_reply(promise);
    GstWebRTCSessionDescription* offer = NULL;
    gst_structure_get(reply, "offer", GST_TYPE_WEBRTC_SESSION_DESCRIPTION, &offer, NULL);
    gst_promise_unref(promise);

    GstPromise* local_description_promise = gst_promise_new();
    g_signal_emit_by_name(peer.webrtcbin,
        "set-local-description",
        offer,
        local_description_promise);
    gst_promise_interrupt(local_description_promise);
    gst_promise_unref(local_description_promise);

    peer.task->onOfferCreated(peer, *offer);
    gst_webrtc_session_description_free(offer);
}

void WebRTCCommonTask::onOfferCreated(Peer const& peer,
    GstWebRTCSessionDescription& offer)
{
    SignallingMessage msg;
    msg.type = SIGNALLING_OFFER;
    msg.peer_id = peer.peer_id;
    msg.message = gst_sdp_message_as_text(offer.sdp);
    _signalling_out.write(msg);
}
void WebRTCCommonTask::processSignallingMessage(GstElement* webrtcbin,
    SignallingMessage const& msg)
{
    if (msg.type == SIGNALLING_OFFER) {
        if (m_signalling_config.polite) {
            processRemoteDescription(webrtcbin, msg);
        }
    }
    else if (msg.type == SIGNALLING_ICE_CANDIDATE) {
        processICECandidate(webrtcbin, msg);
    }
    else if (msg.type == SIGNALLING_ANSWER) {
        if (!m_signalling_config.polite) {
            processRemoteDescription(webrtcbin, msg);
        }
    }
}

void WebRTCCommonTask::processICECandidate(GstElement* webrtcbin,
    webrtc_base::SignallingMessage const& msg)
{
    g_signal_emit_by_name(webrtcbin,
        "add-ice-candidate",
        msg.m_line,
        msg.message.c_str());
}

void WebRTCCommonTask::processRemoteDescription(GstElement* webrtcbin,
    SignallingMessage const& msg)
{
    GstSDPMessage* sdpMsg;
    int ret = gst_sdp_message_new(&sdpMsg);
    string sdp = msg.message;
    g_assert_cmphex(ret, ==, GST_SDP_OK);
    ret = gst_sdp_message_parse_buffer((guint8*)sdp.c_str(), sdp.size(), sdpMsg);
    if (ret != GST_SDP_OK) {
        LOG_ERROR_S << "could not parse SDP string: " << sdp;
        return;
    }

    GstWebRTCSessionDescription* answer =
        gst_webrtc_session_description_new(GST_WEBRTC_SDP_TYPE_ANSWER, sdpMsg);
    g_assert_nonnull(answer);

    GstPromise* promise = gst_promise_new();
    g_signal_emit_by_name(webrtcbin, "set-remote-description", answer, promise);
    gst_promise_interrupt(promise);
    gst_promise_unref(promise);
}

void WebRTCCommonTask::callbackICECandidate(GstElement* webrtcbin,
    guint m_line,
    gchar* candidate,
    gpointer user_data)
{
    Peer const& peer = *reinterpret_cast<Peer*>(user_data);
    peer.task->onICECandidate(peer, m_line, string(candidate));
}

void WebRTCCommonTask::onICECandidate(Peer const& peer,
    guint m_line,
    std::string candidate)
{
    SignallingMessage msg;
    msg.type = SIGNALLING_ICE_CANDIDATE;
    msg.peer_id = peer.peer_id;
    msg.message = candidate;
    msg.m_line = m_line;
    _signalling_out.write(msg);
}

void WebRTCCommonTask::configureWebRTCBin(string const& peer_id, GstElement* webrtcbin)
{
    auto& peer = m_peers[webrtcbin];
    peer.peer_id = peer_id;
    peer.task = this;
    peer.webrtcbin = webrtcbin;

    g_signal_connect(webrtcbin,
        "on-negotiation-needed",
        G_CALLBACK(callbackNegotiationNeeded),
        (gpointer)&peer);
    g_signal_connect(webrtcbin,
        "on-ice-candidate",
        G_CALLBACK(callbackICECandidate),
        (gpointer)&peer);
}

void WebRTCCommonTask::destroyPipeline()
{
    WebRTCCommonTaskBase::destroyPipeline();

    m_peers.clear();
}