/* Generated from orogen/lib/orogen/templates/tasks/Task.cpp */

#include "Common.hpp"
#include "Helpers.hpp"

using namespace gstreamer;
using namespace std;

Common::Common(std::string const& name)
    : CommonBase(name)
{
    _pipeline_initialization_timeout.set(base::Time::fromSeconds(5));
}

Common::~Common()
{
}

/// The following lines are template definitions for the various state machine
// hooks defined by Orocos::RTT. See Common.hpp for more detailed
// documentation about them.

bool Common::configureHook()
{
    if (!CommonBase::configureHook())
        return false;
    return true;
}
bool Common::startHook()
{
    if (!CommonBase::startHook())
        return false;

    mErrorQueue.clear();
    return true;
}
void Common::updateHook()
{
    CommonBase::updateHook();

    GstState state = GST_STATE_NULL;
    gst_element_get_state(GST_ELEMENT(mPipeline), &state, nullptr, 0);
    if (state != GST_STATE_PLAYING) {
        return;
    }

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
void Common::errorHook()
{
    CommonBase::errorHook();
}
void Common::stopHook()
{
    CommonBase::stopHook();
}
void Common::cleanupHook()
{
    CommonBase::cleanupHook();
}

void Common::destroyPipeline()
{
    gst_element_set_state(GST_ELEMENT(mPipeline), GST_STATE_NULL);
    gst_object_unref(mPipeline);
    mPipeline = nullptr;
    mConfiguredInputs.clear();
    mConfiguredOutputs.clear();
}

bool Common::startPipeline()
{
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
        ret = gst_element_get_state(GST_ELEMENT(mPipeline), NULL, NULL, timeout_ns);

        if (!processInputs()) {
            return false;
        }
    }

    if (ret == GST_STATE_CHANGE_FAILURE) {
        throw std::runtime_error("pipeline failed to start");
    }

    return true;
}

void Common::configureOutput(GstElement* pipeline,
    std::string const& appsink_name,
    base::samples::frame::frame_mode_t frame_mode,
    bool dynamic,
    FrameOutputPort& port)
{
    GstElement* appsink = gst_bin_get_by_name(GST_BIN(pipeline), appsink_name.c_str());
    if (!appsink) {
        throw std::runtime_error(
            "cannot find appsink element named " + appsink_name + "in pipeline");
    }
    GstUnrefGuard<GstElement> refguard(appsink);

    auto format = rawModeToGSTVideoFormat(frame_mode);
    GstCaps* caps = gst_caps_new_simple("video/x-raw",
        "format",
        G_TYPE_STRING,
        gst_video_format_to_string(format),
        NULL);
    if (!caps) {
        throw std::runtime_error("failed to generate caps");
    }

    GstUnrefGuard<GstCaps> caps_unref_guard(caps);
    g_object_set(appsink, "emit-signals", TRUE, "caps", caps, NULL);

    mConfiguredOutputs.emplace_back(
        std::move(ConfiguredOutput(dynamic, *this, &port, frame_mode)));

    port.setDataSample(mConfiguredOutputs.back().frame);
    g_signal_connect(appsink,
        "new-sample",
        G_CALLBACK(sinkNewSample),
        &mConfiguredOutputs.back());
}

void Common::configureInput(GstElement* pipeline,
    std::string const& name,
    bool dynamic,
    FrameInputPort& port)
{
    GstElement* appsrc = gst_bin_get_by_name(GST_BIN(pipeline), name.c_str());
    if (!appsrc) {
        throw std::runtime_error(
            "cannot find appsrc element named " + name + "in pipeline");
    }
    GstUnrefGuard<GstElement> refguard(appsrc);
    g_object_set(appsrc, "is-live", TRUE, NULL);

    mConfiguredInputs.emplace_back(
        std::move(ConfiguredInput(dynamic, appsrc, *this, &port)));
}

void Common::queueError(std::string const& message)
{
    RTT::os::MutexLock lock(mSync);
    mErrorQueue.push_back(message);
}

void Common::waitFirstFrames(base::Time const& deadline)
{
    bool all = false;
    while (!all) {
        all = true;
        for (auto& configuredInput : mConfiguredInputs) {
            if (configuredInput.port->read(configuredInput.frame, false) == RTT::NoData) {
                all = false;
                continue;
            }
        }

        if (!all && base::Time::now() > deadline) {
            throw std::runtime_error("timed out while waiting for the first frames");
        }
    }

    for (auto& configuredInput : mConfiguredInputs) {
        configuredInput.frameMode = configuredInput.frame->frame_mode;
        configuredInput.width = configuredInput.frame->size.width;
        configuredInput.height = configuredInput.frame->size.height;

        GstCaps* caps = frameModeToGSTCaps(configuredInput.frameMode);
        GstStructure* str = gst_caps_get_structure(caps, 0);
        gst_structure_set(str,
            "width",
            G_TYPE_INT,
            configuredInput.width,
            "height",
            G_TYPE_INT,
            configuredInput.height,
            NULL);
        GstUnrefGuard<GstCaps> caps_unref_guard(caps);
        g_object_set(configuredInput.appsrc, "caps", caps, NULL);

        gst_video_info_from_caps(&configuredInput.info, caps);
        pushFrame(configuredInput.appsrc, configuredInput.info, *configuredInput.frame);
    }
}

bool Common::processInputs()
{
    for (auto& configuredInput : mConfiguredInputs) {
        while (configuredInput.port->read(configuredInput.frame, false) == RTT::NewData) {
            Frame const& frame = *configuredInput.frame;
            if (frame.frame_mode != configuredInput.frameMode ||
                frame.size.width != configuredInput.width ||
                frame.size.height != configuredInput.height) {
                exception(INPUT_FRAME_CHANGED_PARAMETERS);
                return false;
            }

            if (!pushFrame(configuredInput.appsrc,
                    configuredInput.info,
                    *configuredInput.frame)) {
                exception(GSTREAMER_ERROR);
                return false;
            }
        }
    }
    return true;
}

bool Common::pushFrame(GstElement* element, GstVideoInfo& info, Frame const& frame)
{
    /* Create a buffer to wrap the last received image */
    GstBuffer* buffer = gst_buffer_new_and_alloc(info.size);
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

GstFlowReturn Common::sinkNewSample(GstElement* sink, Common::ConfiguredOutput* data)
{
    /* Retrieve the buffer */
    GstSample* sample = gst_app_sink_pull_sample(GST_APP_SINK(sink));
    GstUnrefGuard<GstSample> sample_unref_guard(sample);

    /* If we have a new sample we have to send it to our Rock frame */
    GstBuffer* buffer = gst_sample_get_buffer(sample);
    if (buffer == NULL) {
        return GST_FLOW_OK;
    }

    GstVideoInfo videoInfo;
    {
        GstCaps* caps = gst_sample_get_caps(sample);
        gst_video_info_from_caps(&videoInfo, caps);
    }

    int width = videoInfo.width;
    int height = videoInfo.height;

    GstMemory* memory = gst_buffer_get_memory(buffer, 0);
    GstUnrefGuard<GstMemory> memory_unref_guard(memory);

    GstMapInfo mapInfo;
    if (!gst_memory_map(memory, &mapInfo, GST_MAP_READ)) {
        return GST_FLOW_OK;
    }
    GstMemoryUnmapGuard memory_unmap_guard(memory, mapInfo);

    std::unique_ptr<Frame> frame(data->frame.write_access());
    frame->init(width, height, 8, data->frameMode);

    frame->time = base::Time::now();
    frame->frame_status = base::samples::frame::STATUS_VALID;

    uint8_t* pixels = &(frame->image[0]);
    if (frame->getNumberOfBytes() > mapInfo.size) {
        data->task.queueError(
            "Inconsistent number of bytes. Rock's image type calculated " +
            to_string(frame->getNumberOfBytes()) + " while GStreamer only has " +
            to_string(mapInfo.size));
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
    return GST_FLOW_OK;
}

template <typename Port>
Common::ConfiguredPort<Port>::ConfiguredPort(bool dynamic,
    Common& task,
    Port* port,
    FrameMode frameMode)
    : dynamic(dynamic)
    , task(task)
    , port(port)
    , frameMode(frameMode)
{
    Frame* f = new Frame();
    f->init(0, 0, 8, frameMode);
    frame.reset(f);
}

template <typename Port>
Common::ConfiguredPort<Port>::ConfiguredPort(ConfiguredPort&& other)
    : dynamic(other.dynamic)
    , task(other.task)
    , frame(other.frame.write_access())
    , port(other.port)
    , frameMode(other.frameMode)
{
    other.port = nullptr;
}

template <typename Port> Common::ConfiguredPort<Port>::~ConfiguredPort()
{
    if (port && dynamic) {
        task.ports()->removePort(port->getName());
        delete port;
    }
}

Common::ConfiguredInput::ConfiguredInput(bool dynamic,
    GstElement* appsrc,
    Common& task,
    Common::FrameInputPort* port)
    : ConfiguredPort<Common::FrameInputPort>(dynamic, task, port)
    , appsrc(appsrc)
{
}
