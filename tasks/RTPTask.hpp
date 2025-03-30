#ifndef GSTREAMER_RTPTASK_TASK_HPP
#define GSTREAMER_RTPTASK_TASK_HPP

#include "gst/rtp/rtp.h"
#include "gstreamer/RTPTaskBase.hpp"
#include <base/Float.hpp>

namespace gstreamer {
    /*! \class RTPTask
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
         task('custom_task_name','gstreamer::RTPTask')
     end
     \endverbatim
     *  It can be dynamically adapted when the deployment is called with a prefix
     argument.
     */
    class RTPTask : public RTPTaskBase {
        friend class RTPTaskBase;

    public:
        /** TaskContext constructor for Task
         * \param name Name of the task. This name needs to be unique to make
         *             it identifiable via nameservices.
         * \param initial_state The initial TaskState of the TaskContext.
         *                      This is deprecated. It should always be the
         *                      configure state.
         */
        RTPTask(std::string const& name = "gstreamer::RTPTask");

        ~RTPTask();

        /** Update the Stats for the RTPSession
         *
         *  These are the stats from RTPSession as shown in the link below
         *  https://gstreamer.freedesktop.org/documentation/rtpmanager/RTPSession.html?gi-language=c#RTPSession:stats
         *
         * @param session [GstElement*] RTP Session BIN
         * @return [RTPSessionStatistics] Statistics from all RTP Sources in this RTP
         * Session
         */
        RTPSessionStatistics updateRTPSessionStats(GstElement* session);

        /** RTP Monitored Settings */
        RTPMonitoringConfig m_rtp_monitoring_config;
        /** RTP bin element */
        GstElement* m_bin;
        /** RTP sessions element */
        std::vector<GstElement*> m_rtp_sessions;

        /**
         * Hook called when the state machine transitions from PreOperational to
         * Stopped.
         *
         * If the code throws an exception, the transition will be aborted
         * and the component will end in the EXCEPTION state instead
         *
         * @return true if the transition can continue, false otherwise
         */
        bool configureHook();

        /**
         * Hook called when the state machine transition from Stopped to Running
         *
         * If the code throws an exception, the transition will be aborted
         * and the component will end in the EXCEPTION state instead
         *
         * @return true if the transition is successful, false otherwise
         */
        bool startHook();

        /**
         * Hook called on trigger in the Running state
         *
         * When this hook is exactly called depends on the chosen task's activity.
         * For instance, if the task context is declared as periodic in the orogen
         * specification, the task will be called at a fixed period.
         *
         * See Rock's documentation for a list of available triggering mechanisms
         *
         * The error(), exception() and fatal() calls, when called in this hook,
         * allow to get into the associated RunTimeError, Exception and
         * FatalError states.
         */
        void updateHook();

        /**
         * Hook called in the RuntimeError state, under the same conditions than
         * the updateHook
         *
         * Call recover() to go back in the Runtime state.
         */
        void errorHook();

        /**
         * Hook called when the component transitions out of the Running state
         *
         * This is called for any transition out of running, that is the
         * transitions to Stopped, Exception and Fatal
         */
        void stopHook();

        /**
         * Hook called on all transitions to PreOperational
         *
         * This is called for all transitions into PreOperational, that is
         * either from Stopped or Exception
         */
        void cleanupHook();
    };
}

#endif