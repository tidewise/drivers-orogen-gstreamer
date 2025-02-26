#ifndef RTPTASK_HPP
#define RTPTASK_HPP

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

        /** Extract the GstStructure for the RTP Statistics*/
        RTPSessionStatistics extractRTPSessionStats(GstElement* session);

        RTPSourceStatistics extractRTPSourceStats(GstStructure const* stats);

        /** Extract the GstStructure for the RTP Sender Statistics */
        RTPSenderStatistics extractRTPSenderStats(GstStructure const* stats);

        /** Extract the GstStructure for the RTP Receiver Statistics */
        RTPReceiverStatistics extractRTPReceiverStats(GstStructure const* stats);

        /** Fetch the gbooleans and converts it to bool */
        bool fetchBoolean(const GstStructure* structure, const char* fieldname);

        /** Fetchs a 64 unsigned int and converts it to int */
        uint64_t fetch64UnsignedInt(const GstStructure* structure, const char* fieldname);

        /** Fetchs a 32 unsigned int and converts it to int */
        uint32_t fetchUnsignedInt(const GstStructure* structure, const char* fieldname);

        uint32_t fetchInt(const GstStructure* structure, const char* fieldname);

        /** Fetchs a gstring and converts it to std::string */
        std::string fetchString(const GstStructure* structure, const char* fieldname);

        /** Extracts a array of receiver reports contained on a GST_TYPE_LIST of
         * GstStructures on the "received-rr" field of a RTPSource*/
        std::vector<RTPPeerReceiverReport> extractPeerReceiverReports(
            const GstStructure* structure);

        /** Converts a GValueArray to a GstStructure */
        GstStructure* createSourceStatsStructure(GArray* source_stats);

        /** RTP Monitored Settings */
        RTPMonitoredSessions m_rtp_monitored_sessions;
        /** RTP bin element */
        GstElement* m_bin;
        /** RTP sessions element */
        std::vector<GstElement*> m_rtp_internal_sessions;

        /**
         * RTPReceiverReport:
         *
         * A receiver report structure.
         */
        struct RTPReceiverReport {
            gboolean is_valid;
            guint32 ssrc; /* which source is the report about */
            guint8 fractionlost;
            guint32 packetslost;
            guint32 exthighestseq;
            guint32 jitter;
            guint32 lsr;
            guint32 dlsr;
            guint32 round_trip;
        };

        /**
         * RTPSenderReport:
         *
         * A sender report structure.
         */
        struct RTPSenderReport {
            gboolean is_valid;
            guint64 ntptime;
            guint32 rtptime;
            guint32 packet_count;
            guint32 octet_count;
            GstClockTime time;
        };

        /**
         * RTPSourceStats:
         * @packets_received: number of received packets in total
         * @prev_received: number of packets received in previous reporting
         *                       interval
         * @octets_received: number of payload bytes received
         * @bytes_received: number of total bytes received including headers and lower
         *                 protocol level overhead
         * @max_seqnr: highest sequence number received
         * @transit: previous transit time used for calculating @jitter
         * @jitter: current jitter (in clock rate units scaled by 16 for precision)
         * @prev_rtptime: previous time when an RTP packet was received
         * @prev_rtcptime: previous time when an RTCP packet was received
         * @last_rtptime: time when last RTP packet received
         * @last_rtcptime: time when last RTCP packet received
         * @curr_rr: index of current @rr block
         * @rr: previous and current receiver report block
         * @curr_sr: index of current @sr block
         * @sr: previous and current sender report block
         *
         * Stats about a source.
         */
        struct RTPSourcesStats {
            GstStructure* s;
            gboolean is_sender;
            gboolean internal;
            gchar* address_str;
            gboolean have_rb;
            // Test value, change to 0 afterwards
            guint32 ssrc = 89437;
            guint8 fractionlost = 0;
            gint32 packetslost = 0;
            guint32 exthighestseq = 0;
            guint32 jitter = 0;
            guint32 lsr = 0;
            guint32 dlsr = 0;
            guint32 round_trip = 0;
            gboolean have_sr;
            GstClockTime time = 0;
            guint64 ntptime = 0;
            guint32 rtptime = 0;
            guint32 packet_count = 0;
            guint32 octet_count = 0;
            guint64 packets_received;
            guint64 octets_received;
            guint64 bytes_received;
            guint32 prev_expected;
            guint32 prev_received;
            guint16 max_seq;
            guint64 cycles;
            guint32 base_seq;
            guint32 bad_seq;
            guint32 transit;
            guint64 packets_sent;
            guint64 octets_sent;
            guint sent_pli_count;
            guint recv_pli_count;
            guint sent_fir_count;
            guint recv_fir_count;
            guint sent_nack_count;
            guint recv_nack_count;

            /* when we received stuff */
            GstClockTime prev_rtptime;
            GstClockTime prev_rtcptime;
            GstClockTime last_rtptime;
            GstClockTime last_rtcptime;

            /* sender and receiver reports */
            gint curr_rr;
            RTPReceiverReport rr[2];
            gint curr_sr;
            RTPSenderReport sr[2];
        };

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