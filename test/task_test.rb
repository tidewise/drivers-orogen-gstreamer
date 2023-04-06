# frozen_string_literal: true

require "time"
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
                outputs: [{ name: "out", frame_mode: "MODE_RGB", window: Time.at(5),
                            period: Time.at(0), sample_loss_threshold: 2 }],
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
                outputs: [{ name: "out", frame_mode: "MODE_RGB", window: Time.at(5),
                            period: Time.at(0), sample_loss_threshold: 2 }],
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
        samples = expect_execution.to do
            [have_new_samples(cmp.generator_child.out_port, 2),
             have_new_samples(cmp.target_child.out_port, 2)]
        end
        samples = samples.map(&:last)

        expected = File.binread(File.join(__dir__, "videotestsrc_colors_320_240.bin"))
        2.times do |i|
            assert_equal 320, samples[i].size.width
            assert_equal 240, samples[i].size.height
            assert_equal :MODE_RGB, samples[i].frame_mode
            assert_equal :STATUS_VALID, samples[i].frame_status
            assert_equal 960, samples[i].row_size
            assert_equal 8, samples[i].data_depth
            assert_equal expected, samples[i].image.to_byte_array
        end
    end

    it "handles data that is not 8-bytes aligned" do
        # Having lines smaller than 8 bytes causes gstreamer to naturally pad them
        # This requires special handling inside the component
        self.expect_execution_default_timeout = 600

        source_m = task_m.specialize
        source_m.require_dynamic_service("image_source", as: "out")

        sink_m = task_m.specialize
        sink_m.require_dynamic_service("image_sink", as: "in")

        cmp_m = Syskit::Composition.new_submodel
        cmp_m
            .add(source_m, as: "generator")
            .with_arguments(
                outputs: [{ name: "out", frame_mode: "MODE_RGB", window: Time.at(5),
                            period: Time.at(0), sample_loss_threshold: 2 }],
                pipeline: <<~PIPELINE
                    videotestsrc pattern=colors ! video/x-raw,width=319,height=240 !
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
                outputs: [{ name: "out", frame_mode: "MODE_RGB", window: Time.at(5),
                            period: Time.at(0), sample_loss_threshold: 2 }],
                pipeline: <<~PIPELINE
                    udpsrc port=9384 caps = "application/x-rtp, media=(string)video,
                        clock-rate=(int)90000, encoding-name=(string)RAW,
                        width=(string)319, height=(string)240" ! rtpvrawdepay ! queue !
                        videoflip method=vertical-flip ! appsink name=out
                PIPELINE
            )
            .prefer_deployed_tasks(/target/)

        cmp_m.generator_child.connect_to cmp_m.inverter_child
        cmp = syskit_deploy_configure_and_start(cmp_m)
        samples = expect_execution.to do
            [have_new_samples(cmp.generator_child.out_port, 2),
             have_new_samples(cmp.target_child.out_port, 2)]
        end
        samples = samples.map(&:last)

        expected = File.binread(File.join(__dir__, "videotestsrc_colors_319_240.bin"))
        2.times do |i|
            File.binwrite(
                File.join(__dir__, "videotestsrc_colors_319_240_sample_#{i}.bin"),
                samples[i].image.to_byte_array
            )
            assert_equal 319, samples[i].size.width
            assert_equal 240, samples[i].size.height
            assert_equal :MODE_RGB, samples[i].frame_mode
            assert_equal :STATUS_VALID, samples[i].frame_status
            assert_equal 957, samples[i].row_size
            assert_equal 8, samples[i].data_depth
            assert(expected == samples[i].image.to_byte_array,
                   "#{i}: binary data differs")
        end
    end

    it "corrects timestamps if the period is set to a nonzero value" do
        self.expect_execution_default_timeout = 600

        cmp = syskit_deploy_and_configure(
            GstreamerPipelineTest
            .with_arguments(
                target_outputs: [
                    { name: "out", frame_mode: "MODE_RGB", window: Time.at(10),
                      period: Time.at(1.0 / 30), sample_loss_threshold: 100 }
                ]
            )
        )

        generator_out = syskit_create_reader cmp.generator_child.out_port
        inverter_in = cmp.inverter_child.orocos_task.in
        all_started = false
        plan.execution_engine.promise do
            until all_started
                while (frame = generator_out.read_new)
                    inverter_in.write(frame)
                end
                all_started = cmp.inverter_child.orocos_task.rtt_state == :RUNNING &&
                              cmp.target_child.orocos_task.rtt_state == :RUNNING
            end
        end.execute

        syskit_start(cmp)

        sample_size = 100
        samples = expect_execution.to do
            [have_new_samples(cmp.generator_child.out_port, sample_size),
             have_new_samples(cmp.target_child.out_port, sample_size)]
        end
        # We remove the first 10 values as they are subject to errors due to the first
        # value being late by more than the latency estimate
        samples = [samples[0].drop(10), samples[1].drop(10)]
        assert give_variance(samples[1]) < 10**-6
    end

    it "does not correct timestamps if the period is set to zero" do
        self.expect_execution_default_timeout = 600

        cmp = syskit_deploy_and_configure(
            GstreamerPipelineTest
            .with_arguments(
                target_outputs: [
                    { name: "out", frame_mode: "MODE_RGB", window: Time.at(10),
                      period: Time.at(0), sample_loss_threshold: 2 }
                ]
            )
        )

        generator_out = syskit_create_reader cmp.generator_child.out_port
        inverter_in = cmp.inverter_child.orocos_task.in
        all_started = false
        plan.execution_engine.promise do
            until all_started
                while (frame = generator_out.read_new)
                    inverter_in.write(frame)
                end
                all_started = cmp.inverter_child.orocos_task.rtt_state == :RUNNING &&
                              cmp.target_child.orocos_task.rtt_state == :RUNNING
            end
        end.execute

        syskit_start(cmp)
        sample_size = 100
        samples = expect_execution.to do
            [have_new_samples(cmp.generator_child.out_port, sample_size),
             have_new_samples(cmp.target_child.out_port, sample_size)]
        end
        # We remove the first 10 values as they are subject to errors due to the first
        # value being late by more than the latency estimate
        assert give_variance(samples[1]) > 10**-6
    end
end

module Services # :nodoc:
    data_service_type "ImageSource" do
        output_port "image", ro_ptr("/base/samples/frame/Frame")
    end

    data_service_type "ImageSink" do
        input_port "image", ro_ptr("/base/samples/frame/Frame")
    end
end

Syskit.extend_model OroGen.gstreamer.Task do
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

class GstreamerPipelineTest < Syskit::Composition # :nodoc:
    argument :target_outputs

    source_m = OroGen.gstreamer.Task.with_dynamic_service("image_source", as: "out")
    sink_m = OroGen.gstreamer.Task.with_dynamic_service("image_sink", as: "in")

    add(source_m, as: "generator")
        .with_arguments(
            outputs: [{ name: "out", frame_mode: "MODE_RGB", window: Time.at(5),
                        period: Time.at(1.0 / 30), sample_loss_threshold: 100 }],
            pipeline: <<~PIPELINE
                videotestsrc pattern=colors ! queue ! videoconvert !
                    appsink name=out
            PIPELINE
        )
        .prefer_deployed_tasks(/generator/)

    add(sink_m, as: "inverter")
        .with_arguments(
            inputs: [{ name: "in" }],
            pipeline: <<~PIPELINE
                appsrc name=in ! queue ! videoflip method=vertical-flip !
                    rtpvrawpay ! udpsink host=127.0.0.1 port=9384
            PIPELINE
        )
        .prefer_deployed_tasks(/inverter/)

    add(source_m, as: "target")
        .with_arguments(
            outputs: from(:parent_task).target_outputs,
            pipeline: <<~PIPELINE
                udpsrc port=9384 caps = "application/x-rtp, media=(string)video,
                    clock-rate=(int)90000, encoding-name=(string)RAW,
                    width=(string)320, height=(string)240" ! rtpvrawdepay ! queue !
                    videoflip method=vertical-flip ! appsink name=out
            PIPELINE
        )
        .prefer_deployed_tasks(/target/)

    data_reader generator_child.out_port, as: "out"
    data_writer inverter_child.in_port, as: "in"
    poll do
        super()

        if (frame = out_reader.read_new)
            sleep rand * 0.1
            in_writer.write(frame)
        end
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

def give_variance(list)
    diffs = list.each_cons(2).map do |a, b|
        (b.time - a.time)
    end
    m = diffs.sum(0.0) / diffs.size
    sum = diffs.inject(0) { |acc, v| acc + (v - m)**2 }
    sum / diffs.size
end

Syskit.conf.use_deployment OroGen.gstreamer.Task => "generator"
Syskit.conf.use_deployment OroGen.gstreamer.Task => "inverter"
Syskit.conf.use_deployment OroGen.gstreamer.Task => "target"
