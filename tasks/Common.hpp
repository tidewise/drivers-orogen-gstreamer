/* Generated from orogen/lib/orogen/templates/tasks/Task.hpp */

#ifndef GSTREAMER_COMMON_TASK_HPP
#define GSTREAMER_COMMON_TASK_HPP

#include "gstreamer/CommonBase.hpp"
#include <base/samples/Frame.hpp>
#include <rtt/extras/ReadOnlyPointer.hpp>

#include <gst/app/gstappsink.h>
#include <gst/app/gstappsrc.h>
#include <gst/video/video-info.h>

#include "gstreamer/gstreamerTypes.hpp"

namespace gstreamer {
    /*! \class Common
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
         task('custom_task_name','gstreamer::Common')
     end
     \endverbatim
     *  It can be dynamically adapted when the deployment is called with a prefix
     argument.
     */
    class Common : public CommonBase {
        friend class CommonBase;

        RTT::os::Mutex m_sync;
        std::vector<std::string> m_error_queue;

    public:
        class DynamicPort {
            RTT::TaskContext* m_task = nullptr;
            RTT::base::PortInterface* m_port = nullptr;

        public:
            DynamicPort(RTT::TaskContext* task,
                RTT::base::InputPortInterface* port,
                bool event);
            DynamicPort(RTT::TaskContext* task, RTT::base::OutputPortInterface* port);
            DynamicPort(DynamicPort const&) = delete;
            DynamicPort(DynamicPort&&);
            ~DynamicPort();

            RTT::base::PortInterface* get() noexcept;
            RTT::base::PortInterface* release() noexcept;
        };

    protected:
        typedef base::samples::frame::Frame Frame;
        typedef RTT::extras::ReadOnlyPointer<Frame> ROPtrFrame;
        typedef RTT::InputPort<ROPtrFrame> FrameInputPort;
        typedef RTT::OutputPort<ROPtrFrame> FrameOutputPort;

        typedef base::samples::frame::frame_mode_t FrameMode;

        template <typename Port> struct ConfiguredPort {
            ROPtrFrame frame;
            Common* task;
            Port* port;
            FrameMode frame_mode;

            ConfiguredPort(Common&,
                Port&,
                FrameMode frame_mode = base::samples::frame::MODE_UNDEFINED);
        };

        struct ConfiguredInput : ConfiguredPort<FrameInputPort> {
            GstElement* appsrc = nullptr;
            GstVideoInfo info;
            uint32_t width = 0;
            uint32_t height = 0;

            ConfiguredInput(Common&, FrameInputPort&, GstElement*);
        };
        typedef ConfiguredPort<FrameOutputPort> ConfiguredOutput;

        std::list<ConfiguredInput> m_configured_inputs;
        std::list<ConfiguredOutput> m_configured_outputs;

        GstElement* m_pipeline = nullptr;

        void configureOutput(GstElement* pipeline,
            std::string const& appsink_name,
            base::samples::frame::frame_mode_t frame_mode,
            bool dynamic,
            FrameOutputPort& port);
        void configureInput(GstElement* pipeline,
            std::string const& name,
            bool dynamic,
            FrameInputPort& port);
        void waitFirstFrames(base::Time const& deadline);
        void queueError(std::string const& message);
        virtual void startPipeline();
        virtual void destroyPipeline();

        static GstFlowReturn sourcePushSample(GstElement* sink, ConfiguredOutput** data);
        static GstFlowReturn sinkNewSample(GstElement* sink, ConfiguredOutput* data);
        static bool sinkRawFrame(
            GstMapInfo& map_info,
            GstVideoInfo& video_info,
            Common::ConfiguredOutput* data,
            std::unique_ptr<base::samples::frame::Frame>& frame);
        static bool sinkCompressedFrame(
            GstMapInfo& map_info,
            GstVideoInfo& video_info,
            Common::ConfiguredOutput* data,
            std::unique_ptr<base::samples::frame::Frame>& frame);

        void processInputs();
        void pushRawFrame(GstElement* element, GstVideoInfo& info, Frame const& frame);
        void pushCompressedFrame(GstElement* element, Frame const& frame);
        std::vector<DynamicPort> m_dynamic_ports;

    public:
        /** TaskContext constructor for Common
         * \param name Name of the task. This name needs to be unique to make it
         * identifiable via nameservices. \param initial_state The initial TaskState of
         * the TaskContext. Default is Stopped state.
         */
        Common(std::string const& name = "gstreamer::Common");

        /** Default deconstructor of Common
         */
        ~Common();

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
