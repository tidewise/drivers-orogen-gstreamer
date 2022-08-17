/* Generated from orogen/lib/orogen/templates/tasks/Task.cpp */

#include <set>
#include <gst/gstcaps.h>

#include "Task.hpp"
#include "Helpers.hpp"
#include "aggregator/TimestampEstimator.hpp"

using namespace std;
using namespace gstreamer;
using namespace base::samples::frame;



// If equal to 1, the timestamp estimator will be used. Set to any other value to deactivate it.
const int Timestamp_Estimator_Activated = 1;

Task::Task(std::string const& name)
    : TaskBase(name)
{
    _pipeline_initialization_timeout.set(base::Time::fromSeconds(5));
}

Task::~Task()
{
}






/// The following lines are template definitions for the various state machine
// hooks defined by Orocos::RTT. See Task.hpp for more detailed
// documentation about them.

void Task::verifyNoNameCollision() {
    set<string> names;
    for (auto const& p : _outputs.get()) {
        if (!names.insert(p.name).second) {
            throw std::runtime_error("duplicate port name " + p.name);
        }
    }
}

bool Task::configureHook()
{
    if (! TaskBase::configureHook())
        return false;


    verifyNoNameCollision();
    auto* pipeline = constructPipeline();
    GstUnrefGuard<GstElement> unref_guard(pipeline);

    mConfiguredInputs.clear();
    configureInputs(pipeline);
    mConfiguredOutputs.clear();
    configureOutputs(pipeline);

    mPipeline = unref_guard.release();
    return true;
}

GstElement* Task::constructPipeline() {
    GError *error = NULL;
    gchar *descr = g_strdup(_pipeline.get().c_str());

    GstElement* element = gst_parse_launch(descr, &error);
    if (error != NULL) {
        string errorMessage = "could not construct pipeline: " + string(error->message);
        g_clear_error (&error);
        throw std::runtime_error(errorMessage);
    }

    return element;
}

void Task::configureInputs(GstElement* pipeline) {
    auto config = _inputs.get();
    for (auto const& inputConfig : config) {
        GstElement* appsrc =
            gst_bin_get_by_name(GST_BIN(pipeline), inputConfig.name.c_str());
        if (!appsrc) {
            throw std::runtime_error(
                "cannot find appsrc element named " + inputConfig.name +
                "in pipeline"
            );
        }
        GstUnrefGuard<GstElement> refguard(appsrc);
        g_object_set(
            appsrc,
            "is-live", TRUE,
            NULL
        );

        FrameInputPort* port = new FrameInputPort(inputConfig.name);
        ports()->addEventPort(inputConfig.name, *port);
        mConfiguredInputs.emplace_back(
            std::move(ConfiguredInput(appsrc, *this, port))
        );
    }
}

void Task::configureOutputs(GstElement* pipeline) {
    auto config = _outputs.get();
    for (auto const& outputConfig : config) {
        GstElement* appsink =
            gst_bin_get_by_name(GST_BIN(pipeline), outputConfig.name.c_str());
        if (!appsink) {
            throw std::runtime_error(
                "cannot find appsink element named " + outputConfig.name +
                "in pipeline"
            );
        }
        GstUnrefGuard<GstElement> refguard(appsink);

        auto format = frameModeToGSTFormat(outputConfig.frame_mode);
        GstCaps* caps = gst_caps_new_simple(
            "video/x-raw",
            "format", G_TYPE_STRING, gst_video_format_to_string(format),
            NULL
        );
        if (!caps) {
            throw std::runtime_error("failed to generate caps");
        }

        GstUnrefGuard<GstCaps> caps_unref_guard(caps);
        g_object_set(
            appsink,
            "emit-signals", TRUE,
            "caps", caps,
            NULL
        );

        FrameOutputPort* port = new FrameOutputPort(outputConfig.name);

        auto timestamperStatuPortName = outputConfig.name + "_timestamper_status";
        auto timestamperStatusPort = new TimestamperStatusPort(timestamperStatuPortName);

        ConfiguredOutput configuredOutput(*this, timestamperStatusPort, port, outputConfig.frame_mode);


        port->setDataSample(configuredOutput.frame);
        ports()->addPort(outputConfig.name, *port);
        ports()->addPort(timestamperStatuPortName, *timestamperStatusPort);
        mConfiguredOutputs.emplace_back(std::move(configuredOutput));

        mConfiguredOutputs.back().mTimestamper = aggregator::TimestampEstimator(
            outputConfig.window,
            outputConfig.period,
            outputConfig.sample_loss_threshold
        );

        g_signal_connect(
            appsink,
             // !!! HERE: last argument must have the lifetime of the task
            "new-sample", G_CALLBACK(sinkNewSample), &mConfiguredOutputs.back()
        );
        // mTimestamper = aggregator::TimestampEstimator(outputConfig.window,outputConfig.estimate,outputConfig.sample_loss_threshold);
    }

}

bool Task::startHook()
{
    if (! TaskBase::startHook())
        return false;

    mErrorQueue.clear();
    base::Time deadline = base::Time::now() + _pipeline_initialization_timeout.get();

    gst_element_set_state(GST_ELEMENT(mPipeline), GST_STATE_PAUSED);
    waitFirstFrames(deadline);

    auto ret = gst_element_set_state(GST_ELEMENT(mPipeline), GST_STATE_PLAYING);
    while (ret == GST_STATE_CHANGE_ASYNC) {
        if (base::Time::now() > deadline) {
            throw std::runtime_error("GStreamer pipeline failed to initialize within "
                                     "the configured time");
        }

        GstClockTime timeout_ns = 50000000ULL;
        ret = gst_element_get_state(
            GST_ELEMENT(mPipeline), NULL, NULL, timeout_ns
        );

        if (!processInputs()) {
            return false;
        }
    }

    if (ret == GST_STATE_CHANGE_FAILURE) {
        throw std::runtime_error("pipeline failed to start");
    }
    // Resetting the timestamper estimator.
    for (auto& configuredOutput : mConfiguredOutputs){
        configuredOutput.mTimestamper.reset();
    }

    return true;
}

void Task::queueError(std::string const& message) {
    RTT::os::MutexLock lock(mSync);
    mErrorQueue.push_back(message);
}
void Task::updateHook()
{
    TaskBase::updateHook();

    if (!processInputs()) {
        return;
    }

    {
        RTT::os::MutexLock lock(mSync);
        if (!mErrorQueue.empty()) {
            throw std::runtime_error(mErrorQueue.front());
        }
    }
}

void Task::errorHook()
{
    TaskBase::errorHook();
}
void Task::stopHook()
{
    gst_element_set_state(GST_ELEMENT(mPipeline), GST_STATE_PAUSED);
    TaskBase::stopHook();
}
void Task::cleanupHook()
{
    gst_element_set_state(GST_ELEMENT(mPipeline), GST_STATE_NULL);
    gst_object_unref(mPipeline);
    mConfiguredOutputs.clear();
    TaskBase::cleanupHook();
}

void Task::waitFirstFrames(base::Time const& deadline) {
    bool all = false;
    while(!all) {
        all = true;
        for (auto& configuredInput : mConfiguredInputs) {
            if (configuredInput.port->read(configuredInput.frame, false) == RTT::NoData) {
                all = false;
                continue;
            }
        }

        if (base::Time::now() > deadline) {
            throw std::runtime_error("timed out while waiting for the first frames");
        }
    }

    for (auto& configuredInput : mConfiguredInputs) {
        configuredInput.frameMode = configuredInput.frame->frame_mode;
        configuredInput.width = configuredInput.frame->size.width;
        configuredInput.height = configuredInput.frame->size.height;

        auto format = frameModeToGSTFormat(configuredInput.frameMode);
        GstCaps* caps = gst_caps_new_simple(
            "video/x-raw",
            "format", G_TYPE_STRING, gst_video_format_to_string(format),
            "width", G_TYPE_INT, configuredInput.width,
            "height", G_TYPE_INT, configuredInput.height,
            NULL
        );
        if (!caps) {
            throw std::runtime_error("failed to generate caps");
        }
        GstUnrefGuard<GstCaps> caps_unref_guard(caps);
        g_object_set(
            configuredInput.appsrc,
            "caps", caps,
            NULL
        );

        gst_video_info_from_caps(&configuredInput.info, caps);
        pushFrame(configuredInput.appsrc, configuredInput.info, *configuredInput.frame);
    }
}

bool Task::processInputs() {
    for (auto& configuredInput : mConfiguredInputs) {
        while (configuredInput.port->read(configuredInput.frame, false) == RTT::NewData) {
            Frame const& frame = *configuredInput.frame;
            if (frame.frame_mode != configuredInput.frameMode ||
                frame.size.width != configuredInput.width ||
                frame.size.height != configuredInput.height) {
                exception(INPUT_FRAME_CHANGED_PARAMETERS);
                return false;
            }

            if (!pushFrame(configuredInput.appsrc, configuredInput.info, *configuredInput.frame)) {
                exception(GSTREAMER_ERROR);
                return false;
            }
        }
    }
    return true;
}

bool Task::pushFrame(GstElement* element, GstVideoInfo& info, Frame const& frame) {
    /* Create a buffer to wrap the last received image */
    GstBuffer *buffer = gst_buffer_new_and_alloc(info.size);
    GstUnrefGuard<GstBuffer> unref_guard(buffer);

    int sourceStride = frame.getRowSize();
    int targetStride = GST_VIDEO_INFO_PLANE_STRIDE(&info, 0);
    if (targetStride != sourceStride) {
        GstVideoFrame vframe;
        gst_video_frame_map(&vframe, &info, buffer, GST_MAP_WRITE);
        GstUnrefGuard<GstVideoFrame> memory_unmap_guard(&vframe);
        guint8* pixels = static_cast<uint8_t*>(GST_VIDEO_FRAME_PLANE_DATA(&vframe, 0));
        int targetStride = GST_VIDEO_FRAME_PLANE_STRIDE(&vframe, 0);
        int rowSize = frame.getRowSize();
        for (int i = 0; i < info.height; ++i) {
            memcpy(pixels + i * targetStride,
                   frame.image.data() + i * sourceStride,
                   rowSize);
        }
    }
    else {
        gst_buffer_fill(buffer, 0, frame.image.data(), frame.image.size());
    }

    GstFlowReturn ret;
    g_signal_emit_by_name(element, "push-buffer", buffer, &ret);

    return ret == GST_FLOW_OK;
}

GstFlowReturn Task::sinkNewSample(GstElement *sink, Task::ConfiguredOutput *data) {
    /* Retrieve the buffer */
    GstSample *sample = gst_app_sink_pull_sample(GST_APP_SINK(sink));
    GstUnrefGuard<GstSample> sample_unref_guard(sample);

    /* If we have a new sample we have to send it to our Rock frame */
    GstBuffer *buffer = gst_sample_get_buffer (sample);
    if (buffer == NULL){
        return GST_FLOW_OK;
    }

    GstVideoInfo videoInfo;
    {
        GstCaps* caps = gst_sample_get_caps(sample);
        gst_video_info_from_caps(&videoInfo, caps);
    }

    int width = videoInfo.width;
    int height = videoInfo.height;

    GstMemory *memory = gst_buffer_get_memory(buffer, 0);
    GstUnrefGuard<GstMemory> memory_unref_guard(memory);

    GstMapInfo mapInfo;
    if (!gst_memory_map(memory, &mapInfo, GST_MAP_READ)) {
        return GST_FLOW_OK;
    }
    GstMemoryUnmapGuard memory_unmap_guard(memory, mapInfo);

    std::unique_ptr<Frame> frame(data->frame.write_access());
    frame->init(width, height, 8, data->frameMode);

    auto now = base::Time::now();
    frame->received_time = now;
    if (Timestamp_Estimator_Activated == 1){
        frame->time = data->mTimestamper.update(now);
    }else{
        frame->time = now;

    }
    frame->frame_status = STATUS_VALID;



    uint8_t* pixels = &(frame->image[0]);
    if (frame->getNumberOfBytes() > mapInfo.size) {
        data->task.queueError(
            "Inconsistent number of bytes. Rock's image type calculated " +
            to_string(frame->getNumberOfBytes()) + " while GStreamer only has " +
            to_string(mapInfo.size)
        );
        return GST_FLOW_OK;
    }

    int sourceStride = videoInfo.stride[0];
    int targetStride = frame->getRowSize();
    if (sourceStride != targetStride) {
        for (int i = 0; i < height; ++i) {
            std::memcpy(pixels + targetStride * i,
                        mapInfo.data + sourceStride * i,
                        frame->getRowSize());
        }
    }
    else {
        std::memcpy(pixels, mapInfo.data, frame->getNumberOfBytes());
    }
    data->frame.reset(frame.release());
    data->port->write(data->frame);
    data->mTimestamperStatusPort->write(
        data->mTimestamper.getStatus()
    );
    return GST_FLOW_OK;
}


template<typename Port>
Task::ConfiguredPort<Port>::ConfiguredPort(
    Task& task, Port* port, FrameMode frameMode
)
    : task(task)
    , port(port)
    , frameMode(frameMode)
{
    Frame* f = new Frame();
    f->init(0, 0, 8, frameMode);
    frame.reset(f);
}

template<typename Port>
Task::ConfiguredPort<Port>::ConfiguredPort(ConfiguredPort&& other)
    : task(other.task)
    , frame(other.frame.write_access())
    , port(other.port)
    , frameMode(other.frameMode) {
    other.port = nullptr;
}

template<typename Port>
Task::ConfiguredPort<Port>::~ConfiguredPort() {
    if (port) {
        task.ports()->removePort(port->getName());
        delete port;
    }
}

Task::ConfiguredInput::ConfiguredInput(
    GstElement* appsrc, Task& task, Task::FrameInputPort* port
)
    : ConfiguredPort<Task::FrameInputPort>(task, port)
    , appsrc(appsrc) {
}


Task::ConfiguredOutput::ConfiguredOutput(
    Task& task, Task::TimestamperStatusPort* statusPort, Task::FrameOutputPort* port, FrameMode frameMode
)
    : ConfiguredPort<Task::FrameOutputPort>(task, port, frameMode)
    , mTimestamperStatusPort(statusPort) {
}
Task::ConfiguredOutput::ConfiguredOutput(ConfiguredOutput&& src)
    : ConfiguredPort<Task::FrameOutputPort>(std::move(src))
    , mTimestamper(src.mTimestamper)
    , mTimestamperStatusPort(src.mTimestamperStatusPort) {
    src.mTimestamperStatusPort = nullptr;
}
Task::ConfiguredOutput::~ConfiguredOutput() {
    if (mTimestamperStatusPort) {
        task.ports()->removePort(mTimestamperStatusPort->getName());
        delete mTimestamperStatusPort;
    }
}