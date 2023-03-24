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
    message.from = config.self_peer_id;
    _signalling_out.write(message);

    if (!config.remote_peer_id.empty()) {
        if (config.polite) {
            SignallingMessage message;
            message.type = SIGNALLING_REQUEST_OFFER;
            message.from = config.self_peer_id;
            message.to = config.remote_peer_id;
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

void WebRTCCommonTask::callbackNegotiationNeeded(GstElement* webrtcbin, void* user_data)
{
    auto& peer(*reinterpret_cast<Peer*>(user_data));
    peer.task->onNegotiationNeeded(peer);
}
void WebRTCCommonTask::onNegotiationNeeded(Peer& peer)
{
    LOG_INFO_S << "Negotiation needed with peer " << peer.peer_id;
    if (!m_signalling_config.polite) {
        LOG_INFO_S << "Creating offer for " << peer.peer_id;
        GstPromise* promise =
            gst_promise_new_with_change_func(callbackOfferCreated, (gpointer)&peer, NULL);
        g_signal_emit_by_name(G_OBJECT(peer.webrtcbin), "create-offer", NULL, promise);
    }
    else {
        LOG_INFO_S << "Expecting offer from " << peer.peer_id;
    }
}
void WebRTCCommonTask::callbackAnswerCreated(GstPromise* promise, gpointer user_data)
{
    callbackSessionDescriptionCreated(promise, user_data, false);
}
void WebRTCCommonTask::callbackOfferCreated(GstPromise* promise, gpointer user_data)
{
    callbackSessionDescriptionCreated(promise, user_data, true);
}
void WebRTCCommonTask::callbackSessionDescriptionCreated(GstPromise* promise,
    gpointer user_data,
    bool is_offer)
{
    Peer const& peer = *reinterpret_cast<Peer*>(user_data);
    GstStructure const* reply = gst_promise_get_reply(promise);
    GstWebRTCSessionDescription* description = NULL;
    char const* field = is_offer ? "offer" : "answer";
    gst_structure_get(reply,
        field,
        GST_TYPE_WEBRTC_SESSION_DESCRIPTION,
        &description,
        NULL);
    gst_promise_unref(promise);

    GstPromise* local_description_promise = gst_promise_new();
    g_signal_emit_by_name(peer.webrtcbin,
        "set-local-description",
        description,
        local_description_promise);
    gst_promise_interrupt(local_description_promise);
    gst_promise_unref(local_description_promise);

    LOG_INFO_S << "Received offer/answer from webrtcbin";
    peer.task->onSessionDescriptionCreated(peer, *description, is_offer);
    gst_webrtc_session_description_free(description);
}

void WebRTCCommonTask::onSessionDescriptionCreated(Peer const& peer,
    GstWebRTCSessionDescription& description,
    bool is_offer)
{
    SignallingMessage msg;
    msg.type = is_offer ? SIGNALLING_OFFER : SIGNALLING_ANSWER;
    msg.from = m_signalling_config.self_peer_id;
    msg.to = peer.peer_id;
    msg.message = gst_sdp_message_as_text(description.sdp);

    LOG_INFO_S << "Sending session description (offer=" << is_offer << ") to " << msg.to
               << " : " << msg.message;
    _signalling_out.write(msg);
}
void WebRTCCommonTask::processSignallingMessage(GstElement* webrtcbin,
    SignallingMessage const& msg)
{
    if (msg.type == SIGNALLING_OFFER) {
        if (m_signalling_config.polite) {
            processOffer(webrtcbin, msg);
        }
    }
    else if (msg.type == SIGNALLING_ICE_CANDIDATE) {
        processICECandidate(webrtcbin, msg);
    }
    else if (msg.type == SIGNALLING_ANSWER) {
        if (!m_signalling_config.polite) {
            processAnswer(webrtcbin, msg);
        }
    }
}

void WebRTCCommonTask::processICECandidate(GstElement* webrtcbin,
    webrtc_base::SignallingMessage const& msg)
{
    LOG_INFO_S << "got ICE candidate from " << msg.from << ": " << msg.message;
    g_signal_emit_by_name(webrtcbin,
        "add-ice-candidate",
        msg.m_line,
        msg.message.c_str());
}

void WebRTCCommonTask::processOffer(GstElement* webrtcbin, SignallingMessage const& msg)
{
    LOG_INFO_S << "Setting remote offer from " << msg.from << ": " << msg.message;
    processRemoteDescription(webrtcbin, msg, GST_WEBRTC_SDP_TYPE_OFFER);

    auto& peer = m_peers.at(webrtcbin);
    GstPromise* promise =
        gst_promise_new_with_change_func(callbackAnswerCreated, (gpointer)&peer, NULL);
    LOG_INFO_S << "Requesting answer from webrtcbin";
    g_signal_emit_by_name(G_OBJECT(peer.webrtcbin), "create-answer", NULL, promise);
}

void WebRTCCommonTask::processAnswer(GstElement* webrtcbin, SignallingMessage const& msg)
{
    LOG_INFO_S << "Setting remote answer from " << msg.from << ": " << msg.message;
    processRemoteDescription(webrtcbin, msg, GST_WEBRTC_SDP_TYPE_ANSWER);
}

void WebRTCCommonTask::processRemoteDescription(GstElement* webrtcbin,
    SignallingMessage const& msg,
    GstWebRTCSDPType sdp_type)
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

    GstWebRTCSessionDescription* description =
        gst_webrtc_session_description_new(sdp_type, sdpMsg);
    g_assert_nonnull(description);

    GstPromise* promise = gst_promise_new();
    g_signal_emit_by_name(webrtcbin, "set-remote-description", description, promise);
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
    msg.from = m_signalling_config.self_peer_id;
    msg.to = peer.peer_id;
    msg.message = candidate;
    msg.m_line = m_line;
    _signalling_out.write(msg);
}

void WebRTCCommonTask::callbackSignalingStateChange(GstElement* webrtcbin,
    GParamSpec* pspec,
    gpointer user_data)
{
    auto& peer(*reinterpret_cast<WebRTCCommonTask::Peer*>(user_data));
    g_object_get(webrtcbin, "signaling-state", &peer.signaling_state, NULL);
    LOG_INFO_S << "Signaling state change: " << peer.signaling_state;
}

void WebRTCCommonTask::callbackConnectionStateChange(GstElement* webrtcbin,
    GParamSpec* pspec,
    gpointer user_data)
{
    auto& peer(*reinterpret_cast<WebRTCCommonTask::Peer*>(user_data));
    g_object_get(webrtcbin, "connection-state", &peer.peer_connection_state, NULL);
    LOG_INFO_S << "Peer connection state change: " << peer.peer_connection_state;
}

void WebRTCCommonTask::callbackICEConnectionStateChange(GstElement* webrtcbin,
    GParamSpec* pspec,
    gpointer user_data)
{
    auto& peer(*reinterpret_cast<WebRTCCommonTask::Peer*>(user_data));
    g_object_get(webrtcbin, "ice-connection-state", &peer.ice_connection_state, NULL);
    LOG_INFO_S << "ICE Connection state" << peer.ice_connection_state;
}

void WebRTCCommonTask::callbackICEGatheringStateChange(GstElement* webrtcbin,
    GParamSpec* pspec,
    gpointer user_data)
{
    auto& peer(*reinterpret_cast<WebRTCCommonTask::Peer*>(user_data));
    g_object_get(webrtcbin, "ice-gathering-state", &peer.ice_gathering_state, NULL);
    LOG_INFO_S << "ICE Gathering state" << peer.ice_gathering_state;
}

void WebRTCCommonTask::configureWebRTCBin(string const& peer_id, GstElement* webrtcbin)
{
    auto& peer = m_peers[webrtcbin];
    peer.peer_id = peer_id;
    peer.task = this;
    peer.webrtcbin = webrtcbin;

    g_signal_connect(webrtcbin,
        "notify::signaling-state",
        G_CALLBACK(callbackSignalingStateChange),
        (gpointer)&peer);
    g_signal_connect(webrtcbin,
        "notify::ice-gathering-state",
        G_CALLBACK(callbackICEGatheringStateChange),
        (gpointer)&peer);
    g_signal_connect(webrtcbin,
        "notify::ice-connection-state",
        G_CALLBACK(callbackICEConnectionStateChange),
        (gpointer)&peer);
    g_signal_connect(webrtcbin,
        "notify::connection-state",
        G_CALLBACK(callbackConnectionStateChange),
        (gpointer)&peer);
    g_signal_connect(webrtcbin,
        "on-ice-candidate",
        G_CALLBACK(callbackICECandidate),
        (gpointer)&peer);
    g_signal_connect(webrtcbin,
        "on-negotiation-needed",
        G_CALLBACK(callbackNegotiationNeeded),
        (gpointer)&peer);
}

void WebRTCCommonTask::destroyPipeline()
{
    WebRTCCommonTaskBase::destroyPipeline();

    m_peers.clear();
}

WebRTCCommonTask::PeerMap::iterator WebRTCCommonTask::findPeerByID(
    std::string const& peer_id)
{
    return find_if(m_peers.begin(),
        m_peers.end(),
        [peer_id](std::pair<GstElement*, Peer> const& v) {
            return v.second.peer_id == peer_id;
        });
}