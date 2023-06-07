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

    m_signalling_config = _signalling.get();
    configureDataChannels();
    return true;
}

bool WebRTCCommonTask::startHook()
{
    if (!WebRTCCommonTaskBase::startHook())
        return false;

    m_last_offer_request = base::Time();

    auto config = m_signalling_config;
    if (config.polite && !config.remote_peer_id.empty()) {
        handleOfferRequestTimeout();
    }
    return true;
}
void WebRTCCommonTask::updateHook()
{
    WebRTCCommonTaskBase::updateHook();

    readSignallingIn();
    handleNegotiationTimeouts();
    sendDataChannelData();

    auto config = m_signalling_config;
    if (config.polite && !config.remote_peer_id.empty() &&
        !hasPeerID(config.remote_peer_id)) {
        LOG_INFO_S << "HAS PEER " << hasPeerID(config.remote_peer_id);
        handleOfferRequestTimeout();
    }
}

void WebRTCCommonTask::readSignallingIn()
{
    auto config = m_signalling_config;

    webrtc_base::SignallingMessage message;
    while (_signalling_in.read(message) == RTT::NewData) {
        if (!message.to.empty() && message.to != config.self_peer_id) {
            LOG_INFO_S << "Received signalling message for " << message.to << " on peer "
                       << config.self_peer_id;
            continue;
        }
        else if (!config.remote_peer_id.empty()) {
            if (message.from != config.remote_peer_id) {
                LOG_INFO_S << "Received signalling message from " << message.from
                           << " but remote_peer_id is set to " << config.remote_peer_id;
                continue;
            }
        }

        if (message.type == SIGNALLING_PEER_DISCONNECT) {
            SignallingMessage ack;
            ack.from = config.self_peer_id;
            ack.to = message.from;
            ack.type = SIGNALLING_PEER_DISCONNECTED;
            _signalling_out.write(ack);

            handlePeerDisconnection(message.from);
            updatePeersStats();
            return;
        }
        else if (message.type == SIGNALLING_PEER_DISCONNECTED) {
            handlePeerDisconnection(message.from);
            updatePeersStats();
            return;
        }

        auto peer_it = findPeerByID(message.from);
        if (peer_it != m_peers.end()) {
            peer_it->second.last_signalling_message = base::Time::now();
        }

        processSignallingMessage(message);
        updatePeersStats();
    }
}

void WebRTCCommonTask::handleOfferRequestTimeout()
{
    auto config = m_signalling_config;
    if (!config.polite || config.remote_peer_id.empty()) {
        return;
    }

    if (base::Time::now() - m_last_offer_request < config.offer_request_timeout) {
        return;
    }

    LOG_INFO_S << "Sending offer request to peer '" << config.remote_peer_id << "'";

    SignallingMessage request_msg;
    request_msg.type = SIGNALLING_REQUEST_OFFER;
    request_msg.from = config.self_peer_id;
    request_msg.to = config.remote_peer_id;
    _signalling_out.write(request_msg);
    m_last_offer_request = base::Time::now();
}

void WebRTCCommonTask::sendDataChannelData()
{
    iodrivers_base::RawPacket packet;
    for (auto& channel : m_data_channels) {
        if (!channel.channel) {
            continue;
        }

        while (channel.read(packet) == RTT::NewData) {
            if (channel.config.binary_in) {
                LOG_INFO_S << "Data channel " << channel.config.label
                           << " sending binary data";
                GBytes* gbytes = g_bytes_new(packet.data.data(), packet.data.size());
                g_signal_emit_by_name(channel.channel, "send-data", gbytes, nullptr);
            }
            else {
                LOG_INFO_S << "Data channel " << channel.config.label
                           << " sending string data";
                char const* bytes_as_char = reinterpret_cast<char*>(packet.data.data());
                string str(bytes_as_char, bytes_as_char + packet.data.size());
                g_signal_emit_by_name(channel.channel,
                    "send-string",
                    str.c_str(),
                    nullptr);
            }
        }
    }
}

void WebRTCCommonTask::handleNegotiationTimeouts()
{
    vector<string> timed_out_peers;
    for (auto& peer_map : m_peers) {
        auto& p = peer_map.second;
        if (!p.connection_start.isNull()) {
            continue;
        }

        if ((p.last_signalling_message - base::Time::now()) >
                m_signalling_config.messaging_timeout ||
            (p.negotiation_start - base::Time::now()) >
                m_signalling_config.negotiation_timeout) {
            LOG_INFO_S << "Peer " << p.peer_id << " negotiation timeout";
            timed_out_peers.push_back(p.peer_id);
        }
    }

    for (auto peer_id : timed_out_peers) {
        handlePeerDisconnection(peer_id);
    }
}

void WebRTCCommonTask::errorHook()
{
    WebRTCCommonTaskBase::errorHook();
}

class TimeoutError : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

void WebRTCCommonTask::stopHook()
{
    string peer_id = m_signalling_config.self_peer_id;

    SignallingMessage msg;
    msg.type = SIGNALLING_PEER_DISCONNECT;
    msg.from = peer_id;
    _signalling_out.write(msg);

    // Signalling component should be mirroring this to us to allow for proper
    // synchronization
    try {
        waitForMessage([peer_id](SignallingMessage const& msg_in) {
            return msg_in.type == SIGNALLING_PEER_DISCONNECTED &&
                   msg_in.message == peer_id;
        });
    }
    catch (TimeoutError&) {
        LOG_INFO_S
            << "Timed out waiting for disconnection ack from the signalling server";
        // ignore, it is not critical for the well-functioning of the webrtc components
    }

    WebRTCCommonTaskBase::stopHook();
}
void WebRTCCommonTask::cleanupHook()
{
    m_data_channels.clear();
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
        LOG_INFO_S << "Expecting offer from " << peer.peer_id << " signaling state is "
                   << peer.signaling_state;
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
    requestAnswer(peer);
}

void WebRTCCommonTask::requestAnswer(Peer const& peer)
{
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
    GstSDPMessage* sdp_msg;
    int ret = gst_sdp_message_new(&sdp_msg);
    string sdp = msg.message;
    g_assert_cmphex(ret, ==, GST_SDP_OK);
    ret = gst_sdp_message_parse_buffer((guint8*)sdp.c_str(), sdp.size(), sdp_msg);
    if (ret != GST_SDP_OK) {
        LOG_ERROR_S << "could not parse SDP string: " << sdp;
        return;
    }

    GstWebRTCSessionDescription* description =
        gst_webrtc_session_description_new(sdp_type, sdp_msg);
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

void WebRTCCommonTask::webrtcStateChange(Peer& peer)
{
    if (peer.connection_start.isNull() &&
        peer.ice_connection_state == GST_WEBRTC_ICE_CONNECTION_STATE_CONNECTED) {
        peer.connection_start = base::Time::now();
    }

    if (isPeerDisconnected(peer)) {
        handlePeerDisconnection(peer.peer_id);
    }

    updatePeersStats();
}
void WebRTCCommonTask::callbackSignalingStateChange(GstElement* webrtcbin,
    GParamSpec* pspec,
    gpointer user_data)
{
    auto& peer(*reinterpret_cast<WebRTCCommonTask::Peer*>(user_data));
    g_object_get(webrtcbin, "signaling-state", &peer.signaling_state, NULL);
    LOG_INFO_S << "Signaling state change: " << peer.signaling_state;
    peer.task->updatePeersStats();
}

void WebRTCCommonTask::callbackConnectionStateChange(GstElement* webrtcbin,
    GParamSpec* pspec,
    gpointer user_data)
{
    auto& peer(*reinterpret_cast<WebRTCCommonTask::Peer*>(user_data));
    g_object_get(webrtcbin, "connection-state", &peer.peer_connection_state, NULL);
    LOG_INFO_S << "Peer connection state change: " << peer.peer_connection_state;
    peer.task->updatePeersStats();
}

void WebRTCCommonTask::callbackICEConnectionStateChange(GstElement* webrtcbin,
    GParamSpec* pspec,
    gpointer user_data)
{
    auto& peer(*reinterpret_cast<WebRTCCommonTask::Peer*>(user_data));
    g_object_get(webrtcbin, "ice-connection-state", &peer.ice_connection_state, NULL);
    peer.task->webrtcStateChange(peer);
}

void WebRTCCommonTask::callbackICEGatheringStateChange(GstElement* webrtcbin,
    GParamSpec* pspec,
    gpointer user_data)
{
    auto& peer(*reinterpret_cast<WebRTCCommonTask::Peer*>(user_data));
    g_object_get(webrtcbin, "ice-gathering-state", &peer.ice_gathering_state, NULL);
    LOG_INFO_S << "ICE Gathering state" << peer.ice_gathering_state;
    peer.task->updatePeersStats();
}

void WebRTCCommonTask::callbackDataChannelNew(GstElement* webrtcbin,
    GstWebRTCDataChannel* channel,
    gpointer user_data)
{
    auto& peer(*reinterpret_cast<WebRTCCommonTask::Peer*>(user_data));
    peer.task->onDataChannelNew(peer, channel);
}

void WebRTCCommonTask::onDataChannelNew(Peer const& peer,
    GstWebRTCDataChannel* gstchannel)
{
    char* label_c = nullptr;
    g_object_get(gstchannel, "label", &label_c, nullptr);
    string label(label_c);
    g_free(label_c);

    auto channel_it = findDataChannelByLabel(label);
    if (channel_it == m_data_channels.end()) {
        LOG_INFO_S << "Ignoring channel with label '" << label << "'";
        return;
    }
    auto& channel = *channel_it;

    LOG_INFO_S << "Data channel: new " << label;

    g_signal_connect(gstchannel,
        "on-open",
        G_CALLBACK(callbackDataChannelOpen),
        (gpointer)&channel);
    g_signal_connect(gstchannel,
        "on-message-data",
        G_CALLBACK(callbackDataChannelBinary),
        (gpointer)&channel);
    g_signal_connect(gstchannel,
        "on-message-string",
        G_CALLBACK(callbackDataChannelString),
        (gpointer)&channel);
    g_signal_connect(gstchannel,
        "on-close",
        G_CALLBACK(callbackDataChannelClosed),
        (gpointer)&channel);
}

void WebRTCCommonTask::callbackDataChannelOpen(GstWebRTCDataChannel* gstchannel,
    gpointer user_data)
{
    DataChannel& channel = *reinterpret_cast<DataChannel*>(user_data);
    channel.task->onDataChannelOpen(channel, gstchannel);
}

void WebRTCCommonTask::onDataChannelOpen(DataChannel& channel, GstWebRTCDataChannel* gst)
{
    LOG_INFO_S << "Data channel " << channel.config.label << " opened";
    channel.channel = gst;
}

void WebRTCCommonTask::callbackDataChannelBinary(GstWebRTCDataChannel* gstchannel,
    GBytes* data,
    gpointer user_data)
{
    gsize size = 0;
    uint8_t const* bytes = static_cast<uint8_t const*>(g_bytes_get_data(data, &size));
    if (!data) {
        return;
    }

    DataChannel& channel = *reinterpret_cast<DataChannel*>(user_data);
    channel.task->onDataChannelIn(channel, bytes, size);
}

void WebRTCCommonTask::callbackDataChannelString(GstWebRTCDataChannel* gstchannel,
    gchar* data,
    gpointer user_data)
{
    DataChannel& channel = *reinterpret_cast<DataChannel*>(user_data);
    channel.task->onDataChannelIn(channel,
        reinterpret_cast<uint8_t const*>(data),
        strlen(data));
}
void WebRTCCommonTask::onDataChannelIn(DataChannel& channel,
    uint8_t const* data,
    size_t size)
{
    LOG_INFO_S << "Data channel " << channel.config.label << " received data";
    iodrivers_base::RawPacket packet;
    packet.time = base::Time::now();
    packet.data.insert(packet.data.end(), data, data + size);
    channel.write(packet);
}

void WebRTCCommonTask::callbackDataChannelClosed(GstWebRTCDataChannel* gstchannel,
    gpointer user_data)
{
    DataChannel& channel = *reinterpret_cast<DataChannel*>(user_data);
    channel.task->onDataChannelClosed(channel);
}
void WebRTCCommonTask::onDataChannelClosed(DataChannel& channel)
{
    LOG_INFO_S << "Data channel " << channel.config.label << " closed";
    channel.channel = nullptr;
}

WebRTCCommonTask::Peer& WebRTCCommonTask::configureWebRTCBin(string const& peer_id,
    GstElement* webrtcbin)
{
    auto& peer = m_peers[webrtcbin];
    peer.peer_id = peer_id;
    peer.task = this;
    peer.webrtcbin = webrtcbin;
    peer.negotiation_start = base::Time::now();
    peer.last_signalling_message = base::Time::now();

    g_object_set(webrtcbin, "bundle-policy", m_signalling_config.bundle_policy, nullptr);
    g_object_set(webrtcbin, "latency", 50, NULL);

    GstElement* rtpbin = gst_bin_get_by_name(GST_BIN(webrtcbin), "rtpbin");
    g_object_set(rtpbin, "drop-on-latency", true, NULL);
    gst_object_unref(rtpbin);

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
    g_signal_connect(webrtcbin,
        "on-data-channel",
        G_CALLBACK(callbackDataChannelNew),
        (gpointer)&peer);

    return peer;
}

void WebRTCCommonTask::createDataChannels(Peer& peer)
{
    LOG_INFO_S << "Creating data channels";
    for (auto const& c : m_data_channels) {
        if (!c.config.create) {
            continue;
        }

        GstWebRTCDataChannel* channel = nullptr;
        g_signal_emit_by_name(peer.webrtcbin,
            "create-data-channel",
            c.config.label.c_str(),
            nullptr,
            &channel);
        onDataChannelNew(peer, channel);
    }
}

void WebRTCCommonTask::destroyPipeline()
{
    WebRTCCommonTaskBase::destroyPipeline();

    m_peers.clear();
}

WebRTCCommonTask::PeerMap::const_iterator WebRTCCommonTask::findPeerByID(
    std::string const& peer_id) const
{
    return const_cast<WebRTCCommonTask*>(this)->findPeerByID(peer_id);
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

bool WebRTCCommonTask::hasPeerID(std::string const& peer_id) const
{
    return findPeerByID(peer_id) != m_peers.end();
}

void WebRTCCommonTask::waitForMessage(
    std::function<bool(webrtc_base::SignallingMessage const&)> f,
    base::Time const& timeout,
    base::Time const& poll_period)
{
    base::Time deadline = base::Time::now() + timeout;

    do {
        SignallingMessage msg;
        while (_signalling_in.read(msg) == RTT::NewData) {
            if (f(msg)) {
                return;
            }
        }

        usleep(poll_period.toMicroseconds());
    } while (base::Time::now() <= deadline);

    throw TimeoutError("did not receive message in time");
}

bool WebRTCCommonTask::isNegotiating(string const& peer_id) const
{
    auto peer_it = findPeerByID(peer_id);
    if (peer_it == m_peers.end()) {
        return false;
    }

    auto const& p = peer_it->second;
    return p.peer_connection_state != GST_WEBRTC_PEER_CONNECTION_STATE_NEW ||
           p.ice_connection_state != GST_WEBRTC_ICE_CONNECTION_STATE_CONNECTED;
}

bool WebRTCCommonTask::isPeerDisconnected(Peer const& peer) const
{
    constexpr std::array<GstWebRTCICEConnectionState, 3> ice_states = {
        GST_WEBRTC_ICE_CONNECTION_STATE_DISCONNECTED,
        GST_WEBRTC_ICE_CONNECTION_STATE_FAILED,
        GST_WEBRTC_ICE_CONNECTION_STATE_CLOSED};
    constexpr std::array<GstWebRTCPeerConnectionState, 3> peer_states = {
        GST_WEBRTC_PEER_CONNECTION_STATE_DISCONNECTED,
        GST_WEBRTC_PEER_CONNECTION_STATE_FAILED,
        GST_WEBRTC_PEER_CONNECTION_STATE_CLOSED};

    if (find(begin(ice_states), end(ice_states), peer.ice_connection_state) !=
        ice_states.end()) {
        return true;
    }

    if (find(begin(peer_states), end(peer_states), peer.peer_connection_state) !=
        peer_states.end()) {
        return true;
    }

    return false;
}

void WebRTCCommonTask::configureDataChannels()
{
    std::list<DataChannel> channels;
    for (auto const& c : _data_channels.get()) {
        string out_port_name = c.port_basename + "_out";
        string in_port_name = c.port_basename + "_in";
        if (provides()->hasService(out_port_name) ||
            provides()->hasService(in_port_name)) {
            throw std::runtime_error("setting up ports for channel " + c.label +
                                     " would collide with existing ports");
        }

        DataChannelInPort* in_p = new DataChannelInPort(in_port_name);
        DataChannelOutPort* out_p = new DataChannelOutPort(out_port_name);

        DataChannel channel(c, this, in_p, out_p);
        channels.emplace_back(move(channel));
    }

    m_data_channels = move(channels);
}

WebRTCCommonTask::DataChannels::const_iterator WebRTCCommonTask::findDataChannelByLabel(
    std::string const& label) const
{
    return const_cast<WebRTCCommonTask*>(this)->findDataChannelByLabel(label);
}
WebRTCCommonTask::DataChannels::iterator WebRTCCommonTask::findDataChannelByLabel(
    std::string const& label)
{
    return find_if(begin(m_data_channels),
        end(m_data_channels),
        [label](DataChannel const& c) { return c.config.label == label; });
}

WebRTCCommonTask::DataChannel::DataChannel(DataChannelConfig const& config,
    WebRTCCommonTask* task,
    WebRTCCommonTask::DataChannelInPort* in_port,
    WebRTCCommonTask::DataChannelOutPort* out_port)
    : config(config)
    , task(task)
    , in_port(task, in_port, true)
    , out_port(task, out_port)
{
}

WebRTCCommonTask::DataChannel::~DataChannel()
{
}

WebRTCCommonTask::DataChannel::DataChannel(DataChannel&& src)
    : config(src.config)
    , task(src.task)
    , in_port(move(src.in_port))
    , out_port(move(src.out_port))
{
}

WebRTCCommonTask::DataChannelInPort* WebRTCCommonTask::DataChannel::releaseIn()
{
    return static_cast<DataChannelInPort*>(in_port.release());
}

WebRTCCommonTask::DataChannelInPort& WebRTCCommonTask::DataChannel::getIn()
{
    return *static_cast<DataChannelInPort*>(in_port.get());
}

WebRTCCommonTask::DataChannelOutPort& WebRTCCommonTask::DataChannel::getOut()
{
    return *static_cast<DataChannelOutPort*>(out_port.get());
}

WebRTCCommonTask::DataChannelOutPort* WebRTCCommonTask::DataChannel::releaseOut()
{
    return static_cast<DataChannelOutPort*>(out_port.release());
}

void WebRTCCommonTask::DataChannel::write(iodrivers_base::RawPacket const& packet)
{
    return getOut().write(packet);
}
RTT::FlowStatus WebRTCCommonTask::DataChannel::read(iodrivers_base::RawPacket& packet)
{
    return getIn().read(packet);
}