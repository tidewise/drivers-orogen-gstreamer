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

        auto format = frameModeToGSTFormat(outputConfig.frameMode);
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
            std::move(ConfiguredOutput(*this, port, outputConfig))
        );

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
    auto ret = gst_element_set_state(GST_ELEMENT(mPipeline), GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_ASYNC) {
        GstClockTime timeout_ns = 5000000000ULL;
        ret = gst_element_get_state(
            GST_ELEMENT(mPipeline), NULL, NULL, timeout_ns
        );
        if (ret == GST_STATE_CHANGE_ASYNC) {
            throw std::runtime_error(
                "pipeline blocked: failed to transition to PLAYING within 5s"
            );
        }
    }

    if (ret == GST_STATE_CHANGE_FAILURE) {
        throw std::runtime_error("pipeline failed to start");
    }
    return true;
}
void Task::updateHook()
{
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

GstFlowReturn Task::sinkNewSample(GstElement *sink, Task::ConfiguredOutput *data) {
    /* Retrieve the buffer */
    GstSample *sample = gst_app_sink_pull_sample(GST_APP_SINK(sink));
    GstUnrefGuard<GstSample> sample_unref_guard(sample);

    /* If we have a new sample we have to send it to our Rock frame */
    GstBuffer *buffer = gst_sample_get_buffer (sample);
    if (buffer == NULL){
        return GST_FLOW_OK;
    }

    GstVideoMeta* meta = gst_buffer_get_video_meta(buffer);
    if (!meta) {
        return GST_FLOW_OK;
    }

    int width = meta->width;
    int height = meta->height;

    GstMemory *memory = gst_buffer_get_memory(buffer, 0);
    GstUnrefGuard<GstMemory> memory_unref_guard(memory);

    GstMapInfo info;
    if (!gst_memory_map(memory, &info, GST_MAP_READ)) {
        return GST_FLOW_OK;
    }
    GstMemoryUnmapGuard memory_unmap_guard(memory, info);

    Frame* frame = data->frame.write_access();
    frame->init(width, height, 8, data->frameMode, -1, info.size);
    frame->time = base::Time::now();
    frame->frame_status = STATUS_VALID;

    uint8_t* pixels = &(frame->image[0]);
    if (frame->getNumberOfBytes() != info.size) {
        return GST_FLOW_OK;
    }

    std::memcpy(pixels, info.data, frame->getNumberOfBytes());
    data->frame.reset(frame);
    data->port->write(data->frame);
    return GST_FLOW_OK;
}


Task::ConfiguredOutput::ConfiguredOutput(
    Task& task, FrameOutputPort* port, OutputConfig const& config
)
    : task(task)
    , port(port)
    , frameMode(config.frameMode)
{
    Frame* f = new Frame();
    f->init(0, 0, 8, frameMode);
    frame.reset(f);
}

Task::ConfiguredOutput::ConfiguredOutput(ConfiguredOutput&& other)
    : task(other.task)
    , frame(other.frame.write_access())
    , port(other.port)
    , frameMode(other.frameMode) {
    other.port = nullptr;
}

Task::ConfiguredOutput::~ConfiguredOutput() {
    if (port) {
        task.ports()->removePort(port->getName());
        delete port;
    }
}
