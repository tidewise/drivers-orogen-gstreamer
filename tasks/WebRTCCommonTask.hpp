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
        struct Peer {
            std::string peer_id;
            GstElement* webrtcbin = nullptr;
            WebRTCCommonTask* task = nullptr;

            GstWebRTCSignalingState signaling_state = GST_WEBRTC_SIGNALING_STATE_STABLE;
            GstWebRTCPeerConnectionState peer_connection_state =
                GST_WEBRTC_PEER_CONNECTION_STATE_NEW;
            GstWebRTCICEConnectionState ice_connection_state =
                GST_WEBRTC_ICE_CONNECTION_STATE_NEW;
            GstWebRTCICEGatheringState ice_gathering_state =
                GST_WEBRTC_ICE_GATHERING_STATE_NEW;
        };

        typedef std::map<GstElement*, Peer> PeerMap;
        PeerMap m_peers;
        PeerMap::iterator findPeerByID(std::string const& peer_id);

        SignallingConfig m_signalling_config;

        void destroyPipeline() override;
        void configureWebRTCBin(std::string const& peer_id, GstElement* webrtcbin);

        void onNegotiationNeeded(Peer& peer);
        static void callbackNegotiationNeeded(GstElement* promise, void* user_data);

        void onOfferCreated(Peer const& peer, GstWebRTCSessionDescription& offer);
        static void callbackOfferCreated(GstPromise* promise, void* user_data);

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
        void processRemoteDescription(GstElement* webrtcbin,
            webrtc_base::SignallingMessage const& msg);

        void processSignallingMessage(GstElement* webrtcbin,
            webrtc_base::SignallingMessage const& msg);

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
