# frozen_string_literal: true

using_task_library "gstreamer"

gstreamer_Task_test_m = OroGen.gstreamer.Task
describe OroGen.gstreamer.Task do
    run_live

    attr_reader :cmp

    before do
        self.expect_execution_default_timeout = 600

        source_m = gstreamer_Task_test_m.specialize
        source_m.require_dynamic_service("image_source", as: "out")

        sink_m = gstreamer_Task_test_m.specialize
        sink_m.require_dynamic_service("image_sink", as: "in")

        cmp_m = Syskit::Composition.new_submodel
        cmp_m.add(source_m, as: "generator")
            .with_arguments(mode: "generator")
            .prefer_deployed_tasks(/generator/)
        cmp_m.add(sink_m, as: "inverter")
            .with_arguments(mode: "inverter")
            .prefer_deployed_tasks(/inverter/)
        cmp_m.add(source_m, as: "target")
            .with_arguments(mode: "target")
            .prefer_deployed_tasks(/target/)

        cmp_m.generator_child.connect_to cmp_m.inverter_child
        @cmp = syskit_deploy_configure_and_start(cmp_m)
    end

    it "successfully transmits the data from one pipeline to the next" do
        samples = expect_execution.to do
            [have_one_new_sample(cmp.generator_child.out_port),
             have_one_new_sample(cmp.target_child.out_port)]
        end
        assert_equal samples[0].size, samples[1].size
        assert_equal samples[0].image, samples[1].image
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

gstreamer_Task_test_m.class_eval do
    argument :mode

    dynamic_service Services::ImageSink, as: "image_sink" do
        provides Services::ImageSink, as: name, "image" => name
    end

    dynamic_service Services::ImageSource, as: "image_source" do
        provides Services::ImageSource, as: name, "image" => name
    end

    def configure
        super

        properties.pipeline_initialization_timeout = Time.at(600)

        if mode == "generator"
            properties.pipeline = <<~PIPELINE
                videotestsrc pattern=pinwheel ! queue ! videoconvert ! appsink name=out
            PIPELINE
            properties.outputs = [{ name: "out", frame_mode: "MODE_RGB" }]
        elsif mode == "inverter"
            properties.pipeline = <<~PIPELINE
                appsrc name=in ! queue ! videoflip method=vertical-flip !
                    rtpvrawpay ! udpsink host=127.0.0.1 port=9384
            PIPELINE
            properties.inputs = [{ name: "in" }]
        elsif mode == "target"
            properties.pipeline = <<~PIPELINE
                udpsrc port=9384 caps = "application/x-rtp, media=(string)video, clock-rate=(int)90000,
                    encoding-name=(string)RAW, width=(string)320, height=(string)240" ! rtpvrawdepay ! queue !
                    videoflip method=vertical-flip ! appsink name=out
            PIPELINE
            properties.outputs = [{ name: "out", frame_mode: "MODE_RGB" }]
        else
            raise "mode: #{mode}"
        end
    end
end

Syskit.conf.use_deployment OroGen.gstreamer.Task => "generator"
Syskit.conf.use_deployment OroGen.gstreamer.Task => "inverter"
Syskit.conf.use_deployment OroGen.gstreamer.Task => "target"