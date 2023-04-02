# gstreamer::Task, a generic GStreamer processing step for Rock

WARNING: this package requires GStreamer 1.20 or later

This package implements components that expose gstreamer's functionality to
receive, send or process images within a Rock system.

## gstreamer::Task

The gstreamer::Task is a generic task that allows to inject and/or retrieve images
from a GStreamer pipeline defined by a string.

The pipeline is described in GStreamer's text format in the 'pipeline' property.
Inputs and outputs are defined in the 'inputs' and 'outputs' properties. The
gist of the concept is that the names set in these properties must match the
name of an appsrc (resp. appsink) element in the pipeline.

For instance, with the following setup:

~~~
pipeline="appsrc name=in ! videorate ! video/x-raw,framerate=25/2 ! appsink name=out"
inputs=[{ name: "in" }]
outputs=[{ name: "out", frame_mode: "MODE_RGB" }]
~~~

the task would change the video rate from its input to its output. Note that
components may have no inputs, no outputs or neither (to only use the Rock
interface to control the GStreamer's pipeline state). Possibilities are
limitless.

A GStreamer pipeline needs data to start. As such, a component that has inputs configured
will block in the 'start' command until it receives data on its ports. It will timeout
after 'pipeline_initialization_timeout' if no data is received. This is also valid
for GStreamer sources (e.g. network sources)

Caps (format and width/height) for inputs are auto-detected on the first
received frame. Output configuration must contain the desired frame format,
which is then announced to the GStreamer pipeline. One may have to add a
`videoconvert` node to convert to the desired format.

## gstreamer::WebRTCSendTask and gstreamer::WebRTCReceiveTask

These two components provide send-only (resp. receive-only) functionality based on
GStreamer. In the case of the send task, an encoding pipeline property shall provide
the pipeline definition from the appsrc (which is created inside the component itself)
to a RTP stream. See the property's default value for inspiration.

They do not assume a specific signalling server, instead relying on the signalling
interface and protocol as described by `drivers/orogen/webrtc_base`. They must be
coupled with a signalling component to actually provide WebRTC functionality.

**Limitations**

When doing tests connecting the WebRTCSendTask with a WebRTCReceiveTask, we noticed
the following limitations. They are not enforced by the code itself, as it may be
that things would work with other WebRTC peers.

These are treated as bugs, and will be fixed in future versions of the package.

- The sender task must be impolite. It crashes when receiving an offer
  from the sender task
- Data channels must be created by the impolite peer. There seem to be an issue
  with renegotiation if the polite peer (e.g. the ReceiveTask in tests) creates
  a channel.

## Debugging the Components

Run Syskit with GST_DEBUG to 2 to see errors in the text log, up to 6 for a whole trace.
Since Syskit redirects to file, we recommend setting GST_DEBUG_COLOR_MODE to off.
