/* Generated from orogen/lib/orogen/templates/tasks/Task.cpp */

#include <gst/gstcaps.h>
#include <set>

#include "Helpers.hpp"
#include "Task.hpp"

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
    mPipeline = unref_guard.release();
    return true;
}

GstElement* Task::constructPipeline()
{
    GError* error = NULL;
    gchar* descr = g_strdup(_pipeline.get().c_str());

    GstElement* element = gst_parse_launch(descr, &error);
    if (error != NULL) {
        string errorMessage = "could not construct pipeline: " + string(error->message);
        g_clear_error(&error);
        throw std::runtime_error(errorMessage);
    }

    return element;
}

std::vector<Task::DynamicPort> Task::configureInputs(GstElement* pipeline)
{
    auto config = _inputs.get();
    std::vector<DynamicPort> ports;
    for (auto const& inputConfig : config) {
        if (provides()->hasService(inputConfig.name)) {
            throw std::runtime_error("name collision in input port creation: " +
                                     inputConfig.name + " already exists");
        }
        unique_ptr<FrameInputPort> port(new FrameInputPort(inputConfig.name));
        configureInput(pipeline, inputConfig.name, true, *port);
        ports.emplace_back(DynamicPort(this, port.release(), true));
    }

    return ports;
}

std::vector<Task::DynamicPort> Task::configureOutputs(GstElement* pipeline)
{
    auto config = _outputs.get();
    std::vector<DynamicPort> ports;
    for (auto const& outputConfig : config) {
        if (provides()->hasService(outputConfig.name)) {
            throw std::runtime_error("name collision in output port creation: " +
                                     outputConfig.name + " already exists");
        }
        unique_ptr<FrameOutputPort> port(new FrameOutputPort(outputConfig.name));
        configureOutput(pipeline,
            outputConfig.name,
            outputConfig.frame_mode,
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

    return startPipeline();
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
    TaskBase::stopHook();
    gst_element_set_state(GST_ELEMENT(mPipeline), GST_STATE_PAUSED);
}
void Task::cleanupHook()
{
    destroyPipeline();

    TaskBase::cleanupHook();
}
