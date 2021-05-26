/* Generated from orogen/lib/orogen/templates/tasks/Task.cpp */

#include <set>
#include <gst/gstcaps.h>

#include "Task.hpp"
#include "Helpers.hpp"

using namespace std;
using namespace gstreamer;
using namespace base::samples::frame;

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
        ports()->addPort(outputConfig.name, *port);
        mConfiguredOutputs.emplace_back(
            std::move(ConfiguredOutput(*this, port, outputConfig.frame_mode))
        );

        port->setDataSample(mConfiguredOutputs.back().frame);
        g_signal_connect(
            appsink,
            "new-sample", G_CALLBACK(sinkNewSample), &mConfiguredOutputs.back()
        );
    }
}

bool Task::startHook()
{
    if (! TaskBase::startHook())
        return false;

    base::Time deadline = base::Time::now() + _pipeline_initialization_timeout.get();

    gst_element_set_state(GST_ELEMENT(mPipeline), GST_STATE_PAUSED);
    waitFirstFrames(deadline);

    auto ret = gst_element_set_state(GST_ELEMENT(mPipeline), GST_STATE_PLAYING);
    while (ret == GST_STATE_CHANGE_ASYNC) {
        if (base::Time::now() > deadline) {
            throw std::runtime_error("GStreamer pipeline failed to initialize within 5s");
        }

        GstClockTime timeout_ns = 10000000ULL;
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
    return true;
}
void Task::updateHook()
{
    if (!processInputs()) {
        return;
    }
    TaskBase::updateHook();
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

        pushFrame(configuredInput.appsrc, *configuredInput.frame);
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
            if (!pushFrame(configuredInput.appsrc, *configuredInput.frame)) {
                exception(GSTREAMER_ERROR);
                return false;
            }
        }
    }
    return true;
}

bool Task::pushFrame(GstElement* element, Frame const& frame) {
    /* Create a buffer to wrap the last received image */
    GstBuffer *buffer = gst_buffer_new_and_alloc(frame.image.size());
    GstUnrefGuard<GstBuffer> unref_guard(buffer);

    gst_buffer_fill(buffer, 0, frame.image.data(), frame.image.size());
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

    Frame* frame = data->frame.write_access();
    frame->init(width, height, 8, data->frameMode, -1, mapInfo.size);
    frame->time = base::Time::now();
    frame->frame_status = STATUS_VALID;

    uint8_t* pixels = &(frame->image[0]);
    if (frame->getNumberOfBytes() != mapInfo.size) {
        return GST_FLOW_OK;
    }

    std::memcpy(pixels, mapInfo.data, frame->getNumberOfBytes());
    data->frame.reset(frame);
    data->port->write(data->frame);
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
