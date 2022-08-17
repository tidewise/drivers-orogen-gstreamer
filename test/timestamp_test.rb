# frozen_string_literal: true

using_task_library "gstreamer"

task_m = OroGen.gstreamer.Task
describe OroGen.gstreamer.Task do
    run_live

    it "attaches ports to source and sinks in the pipeline" do
        self.expect_execution_default_timeout = 600

        source_m = task_m.specialize
        source_m.require_dynamic_service("image_source", as: "out")

        sink_m = task_m.specialize
        sink_m.require_dynamic_service("image_sink", as: "in")

        cmp_m = Syskit::Composition.new_submodel
        cmp_m
            .add(source_m, as: "generator")
            .with_arguments(
                outputs: [{ name: "out", frame_mode: "MODE_RGB", window: Time.at(5), period: Time.at(1.0/30), sample_loss_threshold: 2 }],
                pipeline: <<~PIPELINE
                    videotestsrc pattern=colors ! queue ! videoconvert !
                        appsink name=out
                PIPELINE
            )
            .prefer_deployed_tasks(/generator/)

        cmp_m
            .add(sink_m, as: "inverter")
            .with_arguments(
                inputs: [{ name: "in" }],
                pipeline: <<~PIPELINE
                    appsrc name=in ! queue ! videoflip method=vertical-flip !
                        rtpvrawpay ! udpsink host=127.0.0.1 port=9384
                PIPELINE
            )
            .prefer_deployed_tasks(/inverter/)

        cmp_m
            .add(source_m, as: "target")
            .with_arguments(
                outputs: [{ name: "out", frame_mode: "MODE_RGB", window: Time.at(5), period: Time.at(1.0/30), sample_loss_threshold: 2 }],
                pipeline: <<~PIPELINE
                    udpsrc port=9384 caps = "application/x-rtp, media=(string)video,
                        clock-rate=(int)90000, encoding-name=(string)RAW,
                        width=(string)320, height=(string)240" ! rtpvrawdepay ! queue !
                        videoflip method=vertical-flip ! appsink name=out
                PIPELINE
            )
            .prefer_deployed_tasks(/target/)
        cmp_m.generator_child.connect_to cmp_m.inverter_child
        cmp = syskit_deploy_configure_and_start(cmp_m)
        sample_size = 100
        samples = expect_execution.to do
            [have_new_samples(cmp.generator_child.out_port, sample_size),
             have_new_samples(cmp.target_child.out_port, sample_size)]
        end
        # We remove the first 10 values as they are subject to errors due to the first value
        # being late by more than the latency estimate
        samples = [samples[0].drop(10),samples[1].drop(10)]
        expected = File.binread(File.join(__dir__, "videotestsrc_colors_320_240.bin"))
        samples.each_with_index do |array, i|
            array.each_cons(2) do |a, b|
                puts "#{i} T #{a.time.strftime("%H:%M:%s.%N")} #{b.time.strftime("%H:%M:%s.%N")} #{b.time - a.time}"
            end
        end
    end
end


module Services
    data_service_type "ImageSource" do
        output_port "image", ro_ptr("/base/samples/frame/Frame")
    end

    data_service_type "ImageSink" do
        input_port "image", ro_ptr("/base/samples/frame/Frame")
    end
end

task_m.class_eval do
    argument :pipeline
    argument :inputs, default: []
    argument :outputs, default: []

    dynamic_service Services::ImageSink, as: "image_sink" do
        provides Services::ImageSink, as: name, "image" => name
    end

    dynamic_service Services::ImageSource, as: "image_source" do
        provides Services::ImageSource, as: name, "image" => name
    end

    def update_properties
        super

        properties.pipeline_initialization_timeout = Time.at(600)
        properties.pipeline = pipeline
        properties.inputs = inputs
        properties.outputs = outputs
    end
end

Syskit.conf.use_deployment OroGen.gstreamer.Task => "generator"
Syskit.conf.use_deployment OroGen.gstreamer.Task => "inverter"
Syskit.conf.use_deployment OroGen.gstreamer.Task => "target"