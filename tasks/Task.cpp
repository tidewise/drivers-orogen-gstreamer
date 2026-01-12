/* Generated from orogen/lib/orogen/templates/tasks/Task.cpp */

#include <gst/gstcaps.h>

#include "Helpers.hpp"
#include "Task.hpp"

#include <chrono>
#include <thread>

using namespace std;
using namespace gstreamer;
using namespace base::samples::frame;
using iodrivers_base::RawPacket;

void movePortsToRegistry(vector<Common::DynamicPort>& ports,
    vector<Common::DynamicPort>& registry);

GstElement* fetchElement(GstElement& pipeline, string const& name);

Task::Task(string const& name)
    : TaskBase(name)
{
}

Task::~Task()
{
}

/// The following lines are template definitions for the various state machine
// hooks defined by Orocos::RTT. See Task.hpp for more detailed
// documentation about them.

bool Task::configureHook()
{
    if (!TaskBase::configureHook())
        return false;

    auto* pipeline = constructPipeline();
    GstUnrefGuard<GstElement> unref_guard(pipeline);

    auto in_ports = configureInputs(pipeline);
    auto out_ports = configureOutputs(pipeline);

    for (auto& p : in_ports) {
        m_dynamic_ports.emplace_back(move(p));
    }
    for (auto& p : out_ports) {
        m_dynamic_ports.emplace_back(move(p));
    }

    configureRawIO(*pipeline);
    gst_debug_bin_to_dot_file_with_ts(GST_BIN(pipeline),
        GST_DEBUG_GRAPH_SHOW_VERBOSE,
        getName().c_str());

    m_pipeline = unref_guard.release();
    return true;
}

void Task::configureRawIO(GstElement& pipeline)
{
    vector<DynamicPort> in_ports;
    tie(in_ports, m_bound_raw_in) =
        validatePortsWithElements<RTT::InputPort, RawPacket, true>(pipeline,
            _raw_inputs.get());
    for (auto& binding : m_bound_raw_in) {
        g_object_set(binding.app_element, "is-live", TRUE, NULL);
    }

    vector<DynamicPort> out_ports;
    tie(out_ports, m_bound_raw_out) =
        validatePortsWithElements<RTT::OutputPort, RawPacket, false>(pipeline,
            _raw_outputs.get());
    setupRawOutputs();

    // stores the new ports on the m_dynamic_ports registry
    m_dynamic_ports.reserve(m_dynamic_ports.size() + in_ports.size() + out_ports.size());
    movePortsToRegistry(in_ports, m_dynamic_ports);
    movePortsToRegistry(out_ports, m_dynamic_ports);
}

template <template <typename> class Port, typename T, bool input>
pair<std::vector<Task::DynamicPort>, std::vector<Task::ElementPortBinding<Port, T>>> Task::
    validatePortsWithElements(GstElement& pipeline, vector<std::string> const& port_names)
{
    vector<DynamicPort> ports;
    ports.reserve(port_names.size());
    vector<ElementPortBinding<Port, T>> bindings;
    bindings.reserve(port_names.size());
    for (auto& port_name : port_names) {
        if (provides()->hasService(port_name)) {
            throw runtime_error("name collision in input port creation: " + port_name +
                                " already exists");
        }
        unique_ptr<Port<T>> port(new Port<T>(port_name));
        GstElement* el = fetchElement(pipeline, port_name);
        bindings.emplace_back(port.get(), el);

        if constexpr (input) {
            ports.emplace_back(this, port.release(), true);
        }
        else {
            ports.emplace_back(this, port.release());
        }
    }

    return {move(ports), std::move(bindings)};
}

GstElement* fetchElement(GstElement& pipeline, string const& name)
{
    GstUnrefGuard app_element(gst_bin_get_by_name(GST_BIN(&pipeline), name.c_str()));
    if (!app_element.get()) {
        throw runtime_error("cannot find element named " + name + " in pipeline");
    }

    return app_element.get();
}

void Task::setupRawOutputs()
{
    for (auto& bound_out : m_bound_raw_out) {
        g_object_set(bound_out.app_element, "emit-signals", TRUE, NULL);
        g_signal_connect(bound_out.app_element,
            "new-sample",
            G_CALLBACK(processAppSinkNewRawSample),
            &bound_out);
    }
}

GstFlowReturn Task::processAppSinkNewRawSample(GstElement* appsink,
    BoundRawOutput* binding)
{
    // pull sample -> retrieve the buffer -> map the buffer
    GstUnrefGuard sample(gst_app_sink_pull_sample(GST_APP_SINK(appsink)));

    // buffer should not be unrefered when returned from gst_sample_get_buffer
    GstBuffer* buffer{gst_sample_get_buffer(sample.get())};
    if (buffer == NULL) {
        return GST_FLOW_OK;
    }

    GstUnrefGuard memory(gst_buffer_get_memory(buffer, 0));
    GstMapInfo map_info = GST_MAP_INFO_INIT;
    if (!gst_memory_map(memory.get(), &map_info, GST_MAP_READ)) {
        return GST_FLOW_OK;
    }
    GstMemoryUnmapGuard memory_map(memory.get(), map_info);

    RawPacket& out = binding->memory;
    out.data.resize(map_info.size);
    copy(map_info.data, map_info.data + map_info.size, out.data.begin());
    // TODO: take the buffer timestamp (if it is there)
    out.time = base::Time::now();
    binding->port->write(out);

    return GST_FLOW_OK;
}

void movePortsToRegistry(vector<Common::DynamicPort>& ports,
    vector<Common::DynamicPort>& registry)
{
    move(ports.begin(), ports.end(), std::back_insert_iterator(registry));
}

GstElement* Task::constructPipeline()
{
    GError* error = NULL;
    gchar* descr = g_strdup(_pipeline.get().c_str());

    GstElement* element = gst_parse_launch(descr, &error);
    if (error != NULL) {
        string error_message = "could not construct pipeline: " + string(error->message);
        g_clear_error(&error);
        throw runtime_error(error_message);
    }

    return element;
}

vector<Task::DynamicPort> Task::configureInputs(GstElement* pipeline)
{
    auto config = _inputs.get();
    vector<DynamicPort> ports;
    for (auto const& input_config : config) {
        if (provides()->hasService(input_config.name)) {
            throw runtime_error("name collision in input port creation: " +
                                input_config.name + " already exists");
        }
        unique_ptr<FrameInputPort> port(new FrameInputPort(input_config.name));
        configureInput(pipeline, input_config.name, true, *port);
        ports.emplace_back(DynamicPort(this, port.release(), true));
    }

    return ports;
}

vector<Task::DynamicPort> Task::configureOutputs(GstElement* pipeline)
{
    auto config = _outputs.get();
    vector<DynamicPort> ports;
    for (auto const& output_config : config) {
        if (provides()->hasService(output_config.name)) {
            throw runtime_error("name collision in output port creation: " +
                                output_config.name + " already exists");
        }
        unique_ptr<FrameOutputPort> port(new FrameOutputPort(output_config.name));
        configureOutput(pipeline,
            output_config.name,
            output_config.frame_mode,
            true,
            *port);
        ports.emplace_back(DynamicPort(this, port.release()));
    }

    return ports;
}

bool Task::startHook()
{
    if (!TaskBase::startHook())
        return false;

    startPipeline();

    gst_debug_bin_to_dot_file_with_ts(GST_BIN(m_pipeline),
        GST_DEBUG_GRAPH_SHOW_VERBOSE,
        getName().c_str());

    return true;
}

void Task::waitForInitialData(base::Time const& deadline)
{
    waitFirstFrames(deadline);
    waitFirstRawData(deadline);
}

void Task::waitFirstRawData(base::Time const& deadline)
{
    bool all = false;
    while (!all) {
        all = true;
        for (auto& binding : m_bound_raw_in) {
            if (!binding.memory.data.empty()) {
                continue;
            }

            all = binding.port->read(binding.memory, false) != RTT::NoData;
        }

        if (!all && base::Time::now() > deadline) {
            throw runtime_error("timed out while waiting for the first frames");
        }

        this_thread::sleep_for(10ms);
    }

    for (auto& binding : m_bound_raw_in) {
        pushRawData(*binding.app_element, binding.memory.data);
    }
}

void Task::updateHook()
{
    TaskBase::updateHook();
}

void Task::processInputs()
{
    processFrameInputs();
    processRawInputs();
}

void Task::processRawInputs()
{
    for (auto& binding : m_bound_raw_in) {
        GstElement* appsrc = binding.app_element;
        while (binding.port->read(binding.memory, false) == RTT::NewData) {
            pushRawData(*appsrc, binding.memory.data);
        }
    }
}

void Task::pushRawData(GstElement& appsrc, vector<std::uint8_t> const& data)
{
    GstUnrefGuard buffer(gst_buffer_new_and_alloc(data.size()));
    gst_buffer_fill(buffer.get(), 0, data.data(), data.size());
    GstFlowReturn ret;
    g_signal_emit_by_name(&appsrc, "push-buffer", buffer.get(), &ret);

    if (ret != GST_FLOW_OK) {
        throw runtime_error("failed to push raw data to buffer");
    }
}

void Task::errorHook()
{
    TaskBase::errorHook();
}
void Task::stopHook()
{
    TaskBase::stopHook();
    gst_element_set_state(GST_ELEMENT(m_pipeline), GST_STATE_PAUSED);
}
void Task::cleanupHook()
{
    destroyPipeline();

    TaskBase::cleanupHook();
}
