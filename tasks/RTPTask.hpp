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

        /** Extract the Stats for the RTPSession
         *
         *  These are the stats from RTPSession as shown in the link below
         *  https://gstreamer.freedesktop.org/documentation/rtpmanager/RTPSession.html?gi-language=c#RTPSession:stats
         *
         * @param session [GstElement*] RTP Session BIN
         * @return [RTPSessionStatistics] Statistics from all RTP Sources in this RTP
         * Session
         */
        RTPSessionStatistics extractRTPSessionStats(GstElement* session);

        /** Extract the stats which are always present on a RTP Source
         *
         * @param stat [GstStructure*] stats from a given source
         * @return [RTPSourceStatistics] RTP Source obligatory stats
         */
        RTPSourceStatistics extractRTPSourceStats(GstStructure const* stat);

        /** Extract the Sender Stats from a RTP Source.
         *
         *  The sender stats are all the parameters that are updated when is-sender
         *  is TRUE and were defined by checking the documentation from
         *  https://gstreamer.freedesktop.org/documentation/rtpmanager/RTPSource.html?gi-language=c#RTPSource:stats
         *
         * @param stats [GstStructure*] stats from a given source
         * @return [RTPSenderStatistics] RTP Sender Stats
         */
        RTPSenderStatistics extractRTPSenderStats(GstStructure const* stats);

        /** Extract the Stats from a RTP Receiver
         *
         *  The receiver stats are all the parameters that are updated when is-sender
         *  is FALSE and were defined by checking the documentation from
         *  https://gstreamer.freedesktop.org/documentation/rtpmanager/RTPSource.html?gi-language=c#RTPSource:stats
         *
         * @param stats [GstStructure*] stats from a given source
         * @return [RTPSenderStatistics] RTP Receiver Stats
         */
        RTPReceiverStatistics extractRTPReceiverStats(GstStructure const* stats);

        /** Fetch the gbooleans and converts it to bool
         *
         *  @param structure [GstStructure*]
         *  @param fieldname [char*]
         *  @return [bool]
         */
        static bool fetchBoolean(const GstStructure* structure, const char* fieldname);

        /** Fetchs a 64 unsigned int
         *
         *  @param structure[GstStructure*]
         *  @param fieldname [char*] The name of the field containing the variable
         *  @return [uint64_t]
         */
        static uint64_t fetch64UnsignedInt(const GstStructure* structure, const char* fieldname);

        /** Fetchs a 32 unsigned int
         *
         *  @param structure[GstStructure*]
         *  @param fieldname [char*] The name of the field containing the variable
         *  @return [uint32_t]
         */
        static uint32_t fetchUnsignedInt(const GstStructure* structure, const char* fieldname);

        /** Fetchs a int
         *
         *  @param structure[GstStructure*]
         *  @param fieldname [char*] The name of the field containing the variable
         *  @return [int32_t]
         */
        static int32_t fetchInt(const GstStructure* structure, const char* fieldname);

        /** Fetchs a gstring and converts it to std::string
         *
         *  @param structure [GstStructure*]
         *  @param fieldname [char*] The name of the field containing the variable
         *  @return [std::string]
         */
        static std::string fetchString(const GstStructure* structure, const char* fieldname);

        /** Updates flags of a RTPSourceStatistics struct
         *
         *  @param gst_stats [GstStructure]
         *  @return [uint8_t]
         */
        static uint8_t fetchFlags(const GstStructure* gst_stats);

        /** NTP Timestamp format to base::Time from Unix Epoch
         *
         * @param ntp_timestamp [uint64_t] NTP Timestamp 32.32
         * @return [base::Time] Time from Unix epoch
         */
        base::Time ntpToUnixEpoch(uint64_t ntp_timestamp);

        /**
         * Transforms a Last Sender Report Time (NTP Format 16.16 Fixed Point) to
         * microseconds from UnixEpoch using the NTP Timestamp from the report to
         * define the fixed point (16 most significative bits).
         *
         * This works because the fixed time is the middle 32 bits out of 64
         * in the NTP timestamp.
         *
         * For further explanations on how this calculation works refer to
         * https://www.ietf.org/rfc/rfc1889.txt
         * pages 23-24(Diagram and NTP Timestamp) and 27 (Last SR).
         *
         * @param ntp_timestamp NTP Timestamp from the RTCP Packet
         * @param ntp_short Last SR Time (NTP Format 16.16 Fixed Point)
         * @return [base::Time] Time from Unix epoch
         */
        base::Time lsrTimeToUnixEpoch(uint64_t ntp_timestamp, uint32_t ntp_short);

        /** NTP Short Format to base::Time
         *
         * @param ntp_short NTP Short Format 16.16 Fixed Point
         * @return [base::Time] Time from Unix epoch
         */
        base::Time NTPShortToTime(uint32_t ntp_short);

        /** Converts Jitter from Clock Rate Units to base::Time
         *
         * @param jitter Jitter in Clock Rate Units
         * @return [base::Time] Time from Unix epoch
         */
        base::Time jitterToTime(uint32_t jitter);

        /** Extracts a array of receiver reports contained on a GST_TYPE_LIST of
         * GstStructures on the "received-rr" field of a RTPSource
         * 
         * @param structure [GstStructure*]
         * @return [std::vector<RTPPeerReceiverReport>] Received peer reports
         * */
        std::vector<RTPPeerReceiverReport> extractPeerReceiverReports(
            const GstStructure* structure);

        /** RTP Monitored Settings */
        RTPMonitoredSessions m_rtp_monitored_sessions;
        /** RTP bin element */
        GstElement* m_bin;
        /** RTP sessions element */
        std::vector<GstElement*> m_rtp_sessions;
        /** Source stats which are always present */
        RTPSourceStatistics m_source_stats;
        /** NTP Timestamp */
        uint64_t m_ntp_timestamp;

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