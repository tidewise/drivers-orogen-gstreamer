/* Generated from orogen/lib/orogen/templates/tasks/Task.cpp */

#include "Common.hpp"
#include "Helpers.hpp"

#include <chrono>
#include <thread>

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

    m_dynamic_ports.clear();
    return true;
}
bool Common::startHook()
{
    if (!CommonBase::startHook())
        return false;

    m_error_queue.clear();
    return true;
}
void Common::updateHook()
{
    CommonBase::updateHook();

    if (!m_pipeline) {
        return;
    }

    GstState state = GST_STATE_NULL;
    gst_element_get_state(GST_ELEMENT(m_pipeline), &state, nullptr, 0);
    if (state != GST_STATE_PLAYING) {
        return;
    }

    processInputs();

    {
        RTT::os::MutexLock lock(m_sync);
        if (!m_error_queue.empty()) {
            throw std::runtime_error(m_error_queue.front());
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
    m_dynamic_ports.clear();

    CommonBase::cleanupHook();
}

void Common::destroyPipeline()
{
    gst_element_set_state(GST_ELEMENT(m_pipeline), GST_STATE_NULL);
    gst_object_unref(m_pipeline);
    m_pipeline = nullptr;
    m_configured_inputs.clear();
    m_configured_outputs.clear();
}

void Common::startPipeline()
{
    base::Time deadline = base::Time::now() + _pipeline_initialization_timeout.get();

    gst_element_set_state(GST_ELEMENT(m_pipeline), GST_STATE_PAUSED);
    waitForInitialData(deadline);
    auto ret = gst_element_set_state(GST_ELEMENT(m_pipeline), GST_STATE_PLAYING);
    while (ret == GST_STATE_CHANGE_ASYNC) {
        if (base::Time::now() > deadline) {
            throw std::runtime_error("GStreamer pipeline failed to initialize within "
                                     "the configured time");
        }

        GstClockTime timeout_ns = 50000000ULL;
        ret = gst_element_get_state(GST_ELEMENT(m_pipeline), NULL, NULL, timeout_ns);

        processInputs();
    }

    if (ret == GST_STATE_CHANGE_FAILURE) {
        throw std::runtime_error("pipeline failed to start");
    }
}

void Common::waitForInitialData(base::Time const& deadline)
{
    waitFirstFrames(deadline);
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

    GstCaps* caps;
    if (frame_mode == base::samples::frame::MODE_JPEG) {
        caps = gst_caps_new_empty_simple("image/jpeg");
    }
    else {
        auto format = rawModeToGSTVideoFormat(frame_mode);
        caps = gst_caps_new_simple("video/x-raw",
            "format",
            G_TYPE_STRING,
            gst_video_format_to_string(format),
            NULL);
    }
    if (!caps) {
        throw std::runtime_error("failed to generate caps");
    }

    GstUnrefGuard<GstCaps> caps_unref_guard(caps);
    g_object_set(appsink, "emit-signals", TRUE, "caps", caps, NULL);

    m_configured_outputs.push_back(ConfiguredOutput(*this, port, frame_mode));

    port.setDataSample(m_configured_outputs.back().frame);
    g_signal_connect(appsink,
        "new-sample",
        G_CALLBACK(sinkNewSample),
        &m_configured_outputs.back());
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

    m_configured_inputs.push_back(ConfiguredInput(*this, port, appsrc));
}

void Common::queueError(std::string const& message)
{
    RTT::os::MutexLock lock(m_sync);
    m_error_queue.push_back(message);
}

void Common::waitFirstFrames(base::Time const& deadline)
{
    bool all = false;
    while (!all) {
        all = true;
        for (auto& configured_input : m_configured_inputs) {
            if (configured_input.port->read(configured_input.frame, false) == RTT::NoData) {
                all = false;
                continue;
            }
        }

        if (!all && base::Time::now() > deadline) {
            throw std::runtime_error("timed out while waiting for the first frames");
        }
        this_thread::sleep_for(10ms);
    }

    for (auto& configured_input : m_configured_inputs) {
        configured_input.frame_mode = configured_input.frame->frame_mode;
        configured_input.width = configured_input.frame->size.width;
        configured_input.height = configured_input.frame->size.height;
        GstCaps* caps = frameModeToGSTCaps(configured_input.frame_mode);
        GstStructure* str = gst_caps_get_structure(caps, 0);
        gst_structure_set(str,
            "width",
            G_TYPE_INT,
            configured_input.width,
            "height",
            G_TYPE_INT,
            configured_input.height,
            NULL);
        GstUnrefGuard<GstCaps> caps_unref_guard(caps);
        g_object_set(configured_input.appsrc, "caps", caps, NULL);

        gst_video_info_from_caps(&configured_input.info, caps);
        configured_input.frame->frame_mode < base::samples::frame::COMPRESSED_MODES
            ? pushRawFrame(configured_input.appsrc,
                  configured_input.info,
                  *configured_input.frame)
            : pushCompressedFrame(configured_input.appsrc, *configured_input.frame);
    }
}

void Common::processInputs()
{
    for (auto& configured_input : m_configured_inputs) {
        while (configured_input.port->read(configured_input.frame, false) == RTT::NewData) {
            Frame const& frame = *configured_input.frame;
            if (frame.frame_mode != configured_input.frame_mode ||
                frame.size.width != configured_input.width ||
                frame.size.height != configured_input.height) {
                exception(INPUT_FRAME_CHANGED_PARAMETERS);
                throw std::runtime_error("input frame changed parameters");
            }

            configured_input.frame->frame_mode < base::samples::frame::COMPRESSED_MODES
                ? pushRawFrame(configured_input.appsrc,
                      configured_input.info,
                      *configured_input.frame)
                : pushCompressedFrame(configured_input.appsrc, *configured_input.frame);
        }
    }
}
void Common::pushCompressedFrame(GstElement* element, Frame const& frame)
{
    /* Create a buffer to wrap the last received image */
    GstBuffer* buffer = gst_buffer_new_and_alloc(frame.image.size());
    GstUnrefGuard<GstBuffer> unref_guard(buffer);
    gst_buffer_fill(buffer, 0, frame.image.data(), frame.image.size());

    GstFlowReturn ret;
    g_signal_emit_by_name(element, "push-buffer", buffer, &ret);

    if (ret != GST_FLOW_OK) {
        throw std::runtime_error("failed to push buffer");
    }
}

void Common::pushRawFrame(GstElement* element, GstVideoInfo& info, Frame const& frame)
{
    /* Create a buffer to wrap the last received image */
    GstBuffer* buffer = gst_buffer_new_and_alloc(info.size);
    GstUnrefGuard<GstBuffer> unref_guard(buffer);
    int source_stride = frame.getRowSize();
    int target_stride = GST_VIDEO_INFO_PLANE_STRIDE(&info, 0);
    if (target_stride != source_stride) {
        GstVideoFrame v_frame;
        gst_video_frame_map(&v_frame, &info, buffer, GST_MAP_WRITE);
        GstUnrefGuard<GstVideoFrame> memory_unmap_guard(&v_frame);
        guint8* pixels = static_cast<uint8_t*>(GST_VIDEO_FRAME_PLANE_DATA(&v_frame, 0));
        int target_stride = GST_VIDEO_FRAME_PLANE_STRIDE(&v_frame, 0);
        int row_size = frame.getRowSize();
        for (int i = 0; i < info.height; ++i) {
            memcpy(pixels + i * target_stride,
                frame.image.data() + i * source_stride,
                row_size);
        }
    }
    else {
        gst_buffer_fill(buffer, 0, frame.image.data(), frame.image.size());
    }

    GstFlowReturn ret;
    g_signal_emit_by_name(element, "push-buffer", buffer, &ret);

    if (ret != GST_FLOW_OK) {
        throw std::runtime_error("failed to push buffer");
    }
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

    GstVideoInfo video_info;
    {
        GstCaps* caps = gst_sample_get_caps(sample);
        gst_video_info_from_caps(&video_info, caps);
    }

    GstMemory* memory = gst_buffer_get_memory(buffer, 0);
    GstUnrefGuard<GstMemory> memory_unref_guard(memory);

    GstMapInfo map_info;
    if (!gst_memory_map(memory, &map_info, GST_MAP_READ)) {
        return GST_FLOW_OK;
    }
    GstMemoryUnmapGuard memory_unmap_guard(memory, map_info);

    std::unique_ptr<Frame> frame(data->frame.try_write_access());
    if (!frame) {
        frame.reset(new base::samples::frame::Frame());
    }

    bool status = data->frame_mode < base::samples::frame::COMPRESSED_MODES
                      ? sinkRawFrame(map_info, video_info, data, frame)
                      : sinkCompressedFrame(map_info, video_info, data, frame);

    if (status == false) {
        return GST_FLOW_OK;
    }

    frame->time = base::Time::now();
    frame->frame_status = base::samples::frame::STATUS_VALID;

    data->frame.reset(frame.release());
    data->port->write(data->frame);
    return GST_FLOW_OK;
}

bool Common::sinkRawFrame(GstMapInfo& map_info,
    GstVideoInfo& video_info,
    Common::ConfiguredOutput* data,
    std::unique_ptr<Frame>& frame)
{
    int width = video_info.width;
    int height = video_info.height;

    frame->init(width, height, 8, data->frame_mode, -1);

    uint8_t* pixels = &(frame->image[0]);

    if (frame->getNumberOfBytes() > map_info.size) {
        data->task->queueError(
            "Inconsistent number of bytes. Rock's image type calculated " +
            to_string(frame->getNumberOfBytes()) + " while GStreamer only has " +
            to_string(map_info.size));
        return false;
    }

    int source_stride = video_info.stride[0];
    int target_stride = frame->getRowSize();
    if (source_stride != target_stride) {
        for (int i = 0; i < height; ++i) {
            std::memcpy(pixels + target_stride * i,
                map_info.data + source_stride * i,
                frame->getRowSize());
        }
    }
    else {
        std::memcpy(pixels, map_info.data, frame->getNumberOfBytes());
    }
    return true;
}

bool Common::sinkCompressedFrame(GstMapInfo& map_info,
    GstVideoInfo& video_info,
    Common::ConfiguredOutput* data,
    std::unique_ptr<Frame>& frame)
{
    int width = video_info.width;
    int height = video_info.height;

    frame->init(width, height, 8, data->frame_mode, -1, map_info.size);

    uint8_t* pixels = &(frame->image[0]);
    std::memcpy(pixels, map_info.data, frame->getNumberOfBytes());

    return true;
}

template <typename Port>
Common::ConfiguredPort<Port>::ConfiguredPort(Common& task,
    Port& port,
    FrameMode frame_mode)
    : task(&task)
    , port(&port)
    , frame_mode(frame_mode)
{
    Frame* f = new Frame();
    f->init(0, 0, 8, frame_mode);
    frame.reset(f);
}

Common::ConfiguredInput::ConfiguredInput(Common& task,
    Common::FrameInputPort& port,
    GstElement* appsrc)
    : ConfiguredPort<Common::FrameInputPort>(task, port)
    , appsrc(appsrc)
{
}

Common::DynamicPort::DynamicPort(RTT::TaskContext* task,
    RTT::base::InputPortInterface* port,
    bool event)
    : m_task(task)
    , m_port(port)
{
    if (task->ports()->getPort(port->getName())) {
        throw std::runtime_error("port already in use " + port->getName());
    }

    if (event) {
        task->ports()->addEventPort(port->getName(), *port);
    }
    else {
        task->ports()->addPort(port->getName(), *port);
    }
}

Common::DynamicPort::DynamicPort(RTT::TaskContext* task,
    RTT::base::OutputPortInterface* port)
    : m_task(task)
    , m_port(port)
{
    if (task->ports()->getPort(port->getName())) {
        throw std::runtime_error("port already in use " + port->getName());
    }
    task->ports()->addPort(port->getName(), *port);
}

Common::DynamicPort::DynamicPort(DynamicPort&& src)
{
    m_task = src.m_task;
    m_port = src.m_port;
    src.m_task = nullptr;
    src.m_port = nullptr;
}

Common::DynamicPort::~DynamicPort()
{
    if (m_port) {
        m_task->ports()->removePort(m_port->getName());
    }
    delete m_port;
}

RTT::base::PortInterface* Common::DynamicPort::release() noexcept
{
    auto p = m_port;
    m_port = nullptr;
    return p;
}

RTT::base::PortInterface* Common::DynamicPort::get() noexcept
{
    return m_port;
}