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

void Task::verifyNoNameCollision()
{
    set<string> names;
    for (auto const& p : _inputs.get()) {
        if (!names.insert(p.name).second) {
            throw std::runtime_error("duplicate port name " + p.name);
        }
    }

    for (auto const& p : _outputs.get()) {
        if (!names.insert(p.name).second) {
            throw std::runtime_error("duplicate port name " + p.name);
        }
    }
}

bool Task::configureHook()
{
    if (!TaskBase::configureHook())
        return false;

    verifyNoNameCollision();
    auto* pipeline = constructPipeline();
    GstUnrefGuard<GstElement> unref_guard(pipeline);

    configureInputs(pipeline);
    configureOutputs(pipeline);

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

void Task::configureInputs(GstElement* pipeline)
{
    auto config = _inputs.get();
    for (auto const& inputConfig : config) {
        unique_ptr<FrameInputPort> port(new FrameInputPort(inputConfig.name));
        configureInput(pipeline, inputConfig, true, *port);
        ports()->addEventPort(inputConfig.name, *port);
        port.release();
    }
}

void Task::configureOutputs(GstElement* pipeline)
{
    auto config = _outputs.get();
    for (auto const& outputConfig : config) {
        unique_ptr<FrameOutputPort> port(new FrameOutputPort(outputConfig.name));
        configureOutput(pipeline,
            outputConfig.name,
            outputConfig.frame_mode,
            true,
            *port);
        ports()->addPort(outputConfig.name, *port);
        port.release();
    }
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
}
void Task::cleanupHook()
{
    TaskBase::cleanupHook();
}
