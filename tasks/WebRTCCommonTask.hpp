/* Generated from orogen/lib/orogen/templates/tasks/Task.hpp */

#ifndef GSTREAMER_WEBRTCCOMMONTASK_TASK_HPP
#define GSTREAMER_WEBRTCCOMMONTASK_TASK_HPP

#include "gstreamer/WebRTCCommonTaskBase.hpp"

#include <gst/gst.h>

#define GST_USE_UNSTABLE_API
#include <gst/webrtc/webrtc.h>

namespace gstreamer {
    /*! \class WebRTCCommonTask
     * \brief The task context provides and requires services. It uses an ExecutionEngine
     to perform its functions.
     * Essential interfaces are operations, data flow ports and properties. These
     interfaces have been defined using the oroGen specification.
     * In order to modify the interfaces you should (re)use oroGen and rely on the
     associated workflow.
     *
     * \details
     * The name of a TaskContext is primarily defined via:
     \verbatim
     deployment 'deployment_name'
         task('custom_task_name','gstreamer::WebRTCCommonTask')
     end
     \endverbatim
     *  It can be dynamically adapted when the deployment is called with a prefix
     argument.
     */
    class WebRTCCommonTask : public WebRTCCommonTaskBase {
        friend class WebRTCCommonTaskBase;

    protected:
        struct Peer : WebRTCPeerStats {
            GstElement* webrtcbin = nullptr;
            WebRTCCommonTask* task = nullptr;

            base::Time negotiation_start;
            base::Time last_signalling_message;
            base::Time connection_start;
        };

        base::Time m_last_offer_request;
        base::Time m_latency;
        bool m_drop_on_latency;

        typedef std::map<GstElement*, Peer> PeerMap;
        PeerMap m_peers;
        PeerMap::const_iterator findPeerByID(std::string const& peer_id) const;
        PeerMap::iterator findPeerByID(std::string const& peer_id);
        bool hasPeerID(std::string const& peer_id) const;
        bool isNegotiating(std::string const& peer_id) const;

        void webrtcStateChange(Peer& peer);
        bool isPeerDisconnected(Peer const& peer) const;

        SignallingConfig m_signalling_config;

        typedef RTT::OutputPort<iodrivers_base::RawPacket> DataChannelOutPort;
        typedef RTT::InputPort<iodrivers_base::RawPacket> DataChannelInPort;
        struct DataChannel {
            DataChannelConfig config;
            WebRTCCommonTask* task = nullptr;
            DynamicPort in_port;
            DynamicPort out_port;
            GstWebRTCDataChannel* channel = nullptr;

            DataChannel(DataChannelConfig const& config,
                WebRTCCommonTask* task,
                DataChannelInPort* in_port,
                DataChannelOutPort* out_port);
            DataChannel(DataChannel const&) = delete;
            DataChannel(DataChannel&&);
            ~DataChannel();

            DataChannelInPort* releaseIn();
            DataChannelOutPort* releaseOut();
            DataChannelInPort& getIn();
            DataChannelOutPort& getOut();

            void write(iodrivers_base::RawPacket const& packet);
            RTT::FlowStatus read(iodrivers_base::RawPacket& packet);
        };

        typedef std::list<DataChannel> DataChannels;
        DataChannels m_data_channels;

        DataChannels::const_iterator findDataChannelByLabel(
            std::string const& label) const;
        DataChannels::iterator findDataChannelByLabel(std::string const& label);

        static void callbackDataChannelNew(GstElement* webrtcbin,
            GstWebRTCDataChannel* channel,
            gpointer user_data);
        static void callbackDataChannelOpen(GstWebRTCDataChannel* gstchannel,
            gpointer user_data);
        static void callbackDataChannelBinary(GstWebRTCDataChannel* channel,
            GBytes* data,
            gpointer user_data);
        static void callbackDataChannelString(GstWebRTCDataChannel* channel,
            gchar* data,
            gpointer user_data);
        static void callbackDataChannelClosed(GstWebRTCDataChannel* channel,
            gpointer user_data);
        void onDataChannelNew(Peer const& peer, GstWebRTCDataChannel* channel);
        void onDataChannelOpen(DataChannel& channel, GstWebRTCDataChannel* gst);
        void onDataChannelIn(DataChannel& channel, uint8_t const* data, size_t length);
        void onDataChannelClosed(DataChannel& channel);
        void sendDataChannelData();

        void destroyPipeline() override;
        Peer& configureWebRTCBin(std::string const& peer_id, GstElement* webrtcbin);
        void configureDataChannels();
        void createDataChannels(Peer& peer);

        void onNegotiationNeeded(Peer& peer);
        static void callbackNegotiationNeeded(GstElement* promise, void* user_data);

        void onSessionDescriptionCreated(Peer const& peer,
            GstWebRTCSessionDescription& description,
            bool is_offer);
        static void callbackAnswerCreated(GstPromise* promise, gpointer user_data);
        static void callbackOfferCreated(GstPromise* promise, gpointer user_data);
        static void callbackSessionDescriptionCreated(GstPromise* promise,
            void* user_data,
            bool is_offer);

        static void callbackSignalingStateChange(GstElement* webrtcbin,
            GParamSpec* pspec,
            gpointer user_data);
        static void callbackConnectionStateChange(GstElement* webrtcbin,
            GParamSpec* pspec,
            gpointer user_data);
        static void callbackICEConnectionStateChange(GstElement* webrtcbin,
            GParamSpec* pspec,
            gpointer user_data);
        static void callbackICEGatheringStateChange(GstElement* webrtcbin,
            GParamSpec* pspec,
            gpointer user_data);

        void onICECandidate(Peer const& peer, guint mline_index, std::string candidate);
        static void callbackICECandidate(GstElement* element,
            unsigned int mline_index,
            char* candidate,
            void* user_data);

        void processICECandidate(GstElement* webrtcbin,
            webrtc_base::SignallingMessage const& msg);
        void processOffer(GstElement* webrtcbin,
            webrtc_base::SignallingMessage const& msg);
        void requestAnswer(Peer const& peer);
        void processAnswer(GstElement* webrtcbin,
            webrtc_base::SignallingMessage const& msg);
        void processRemoteDescription(GstElement* webrtcbin,
            webrtc_base::SignallingMessage const& msg,
            GstWebRTCSDPType sdp_type);

        /** Read and handle messages from signalling_in */
        void readSignallingIn();

        /** Handler called by the task updateHook to process incoming signalling messages
         */
        virtual void processSignallingMessage(
            webrtc_base::SignallingMessage const& msg) = 0;

        /** Handler called by the task updateHook to handle peer disconnection
         */
        virtual void handlePeerDisconnection(std::string const& peer_id) = 0;

        void handleNegotiationTimeouts();
        void handleOfferRequestTimeout();

        /** Common handling of signalling messages for the webrtcbin */
        void processSignallingMessage(GstElement* webrtcbin,
            webrtc_base::SignallingMessage const& msg);
        void waitForMessage(std::function<bool(webrtc_base::SignallingMessage const&)> f,
            base::Time const& timeout = base::Time::fromSeconds(2),
            base::Time const& poll_period = base::Time::fromMilliseconds(10));

        virtual void updatePeersStats() = 0;

    public:
        /** TaskContext constructor for WebRTCCommonTask
         * \param name Name of the task. This name needs to be unique to make it
         * identifiable via nameservices. \param initial_state The initial TaskState of
         * the TaskContext. Default is Stopped state.
         */
        WebRTCCommonTask(std::string const& name = "gstreamer::WebRTCCommonTask");

        /** Default deconstructor of WebRTCCommonTask
         */
        ~WebRTCCommonTask();

        /** This hook is called by Orocos when the state machine transitions
         * from PreOperational to Stopped. If it returns false, then the
         * component will stay in PreOperational. Otherwise, it goes into
         * Stopped.
         *
         * It is meaningful only if the #needs_configuration has been specified
         * in the task context definition with (for example):
         \verbatim
         task_context "TaskName" do
           needs_configuration
           ...
         end
         \endverbatim
         */
        bool configureHook();

        /** This hook is called by Orocos when the state machine transitions
         * from Stopped to Running. If it returns false, then the component will
         * stay in Stopped. Otherwise, it goes into Running and updateHook()
         * will be called.
         */
        bool startHook();

        /** This hook is called by Orocos when the component is in the Running
         * state, at each activity step. Here, the activity gives the "ticks"
         * when the hook should be called.
         *
         * The error(), exception() and fatal() calls, when called in this hook,
         * allow to get into the associated RunTimeError, Exception and
         * FatalError states.
         *
         * In the first case, updateHook() is still called, and recover() allows
         * you to go back into the Running state.  In the second case, the
         * errorHook() will be called instead of updateHook(). In Exception, the
         * component is stopped and recover() needs to be called before starting
         * it again. Finally, FatalError cannot be recovered.
         */
        void updateHook();

        /** This hook is called by Orocos when the component is in the
         * RunTimeError state, at each activity step. See the discussion in
         * updateHook() about triggering options.
         *
         * Call recover() to go back in the Runtime state.
         */
        void errorHook();

        /** This hook is called by Orocos when the state machine transitions
         * from Running to Stopped after stop() has been called.
         */
        void stopHook();

        /** This hook is called by Orocos when the state machine transitions
         * from Stopped to PreOperational, requiring the call to configureHook()
         * before calling start() again.
         */
        void cleanupHook();
    };
}

#endif
