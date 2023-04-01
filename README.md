# gstreamer::Task, a generic GStreamer processing step for Rock

WARNING: this package requires GStreamer 1.20 or later

This package implements a single component, gstreamer::Task that allows to either
inject or retrieve images from a string-defined GStreamer pipeline.

The pipeline is described in GStreamer's text format in the 'pipeline' property.
Inputs and outputs are defined in the 'inputs' and 'outputs' properties. The
gist of the concept is that the names set in these properties must match the
name of an appsrc (resp. appsink) element in the pipeline.

For instance, with the following setup:

~~~
pipeline="appsrc name=in ! videorate ! video/x-raw,framerate=25/2 ! appsink name=out"
inputs=[{ name: "in" }]
outputs=[{ name: "out", frame_mode: "RGB" }]
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

Run Syskit with GST_DEBUG to 2 to see errors in the text log, up to 6 for a whole trace.
Since Syskit redirects to file, we recommend setting GST_DEBUG_COLOR_MODE to off.
