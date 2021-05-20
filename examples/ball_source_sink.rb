# frozen_string_literal: true

using_task_library "gstreamer"

module Services
    data_service_type "ImageSource" do
        output_port "image", ro_ptr("/base/samples/frame/Frame")
    end

    data_service_type "ImageSink" do
        input_port "image", ro_ptr("/base/samples/frame/Frame")
    end
end

Syskit.extend_model OroGen.gstreamer.Task do
    argument :mode

    dynamic_service Services::ImageSink, as: "image_sink" do
        provides Services::ImageSink, as: name, "image" => name
    end

    dynamic_service Services::ImageSource, as: "image_source" do
        provides Services::ImageSource, as: name, "image" => name
    end

    def configure
        super
        if mode == 1
            properties.pipeline = <<~PIPELINE
                videotestsrc pattern=ball ! queue ! videoconvert ! appsink name=ball
            PIPELINE
            properties.outputs = [{ name: "ball", frame_mode: "MODE_RGB" }]
        elsif mode == 2
            properties.pipeline = <<~PIPELINE
                appsrc name=ball ! queue ! videoconvert ! xvimagesink
            PIPELINE
            properties.inputs = [{ name: "ball" }]
        else
            raise "mode: #{mode}"
        end
    end
end

class SourceSink < Syskit::Composition
    add(Services::ImageSource, as: "source")
    add(Services::ImageSink, as: "sink")

    source_child.connect_to sink_child
end

Syskit.conf.use_deployment OroGen.gstreamer.Task => "ball_source"
Syskit.conf.use_deployment OroGen.gstreamer.Task => "ball_sink"

Robot.actions do
    profile do
        source_m = OroGen.gstreamer.Task.specialize
        source_m.require_dynamic_service("image_source", as: "ball")

        sink_m = OroGen.gstreamer.Task.specialize
        sink_m.require_dynamic_service("image_sink", as: "ball")

        define("ball_source", source_m)
            .with_arguments(mode: 1)
            .prefer_deployed_tasks(/source/)
        define("ball_sink", sink_m)
            .with_arguments(mode: 2)
            .prefer_deployed_tasks(/sink/)
        define("ball_source_sink", SourceSink)
            .use("source" => ball_source_def,
                 "sink" => ball_sink_def)
    end
end

Robot.controller do
    Robot.ball_source_def!
end
