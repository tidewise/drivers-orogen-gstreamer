# frozen_string_literal: true

require_relative "models"

describe OroGen.gstreamer.Task do
    run_live

    it "attaches ports to source and sinks in the pipeline" do
        self.expect_execution_default_timeout = 600

        cmp = syskit_deploy_configure_and_start(cmp_m)
        samples = expect_execution.to do
            [have_one_new_sample(cmp.generator_child.out_port),
             have_one_new_sample(cmp.target_child.out_port)]
        end

        expected = File.binread(File.join(__dir__, "videotestsrc_colors_320_240.bin"))
        assert_frame_ok(samples[0], expected, 320, 240, 960, "generated frame")
        assert_frame_ok(samples[1], expected, 320, 240, 960, "transferred frame")
    end

    it "handles data that is not 8-bytes aligned" do
        # Having lines smaller than 8 bytes causes gstreamer to naturally pad them
        # This requires special handling inside the component
        cmp = syskit_deploy_configure_and_start(cmp_m(319, 240))
        samples = expect_execution.to do
            [have_one_new_sample(cmp.generator_child.out_port),
             have_one_new_sample(cmp.target_child.out_port)]
        end

        expected = File.binread(File.join(__dir__, "videotestsrc_colors_319_240.bin"))
        assert_frame_ok(samples[0], expected, 319, 240, 957, "generated frame")
        assert_frame_ok(samples[1], expected, 319, 240, 957, "transferred frame")
    end

    def generator_m(width = 320, height = 240)
        OroGen.gstreamer.Task.with_dynamic_service("image_source", as: "out")
              .with_arguments(
                  outputs: [{ name: "out", frame_mode: "MODE_RGB" }],
                  pipeline: <<~PIPELINE
                      videotestsrc pattern=colors !
                      video/x-raw,width=#{width},height=#{height} ! queue !
                          videoconvert ! appsink name=out
                  PIPELINE
              )
              .deployed_as("generator")
    end

    def inverter_m
        OroGen.gstreamer.Task.with_dynamic_service("image_sink", as: "in")
              .with_arguments(
                  inputs: [{ name: "in" }],
                  pipeline: <<~PIPELINE
                      appsrc format=3 name=in ! queue ! videoflip method=vertical-flip !
                          rtpvrawpay ! udpsink host=127.0.0.1 port=9384
                  PIPELINE
              )
              .deployed_as("inverter")
    end

    def target_m(width = 320, height = 240)
        OroGen.gstreamer.Task.with_dynamic_service("image_source", as: "out")
              .with_arguments(
                  outputs: [{ name: "out", frame_mode: "MODE_RGB" }],
                  pipeline: <<~PIPELINE
                      udpsrc port=9384 caps="application/x-rtp, media=(string)video,
                          clock-rate=(int)90000, encoding-name=(string)RAW,
                          width=(string)#{width}, height=(string)#{height}" !
                          rtpvrawdepay ! queue ! videoflip method=vertical-flip !
                          appsink name=out
                  PIPELINE
              )
              .deployed_as("target")
    end

    def cmp_m(width = 320, height = 240)
        cmp_m = Syskit::Composition.new_submodel
        cmp_m.add generator_m(width, height), as: "generator"
        cmp_m.add inverter_m, as: "inverter"
        cmp_m.add target_m(width, height), as: "target"
        cmp_m.generator_child.connect_to cmp_m.inverter_child
        cmp_m
    end

    def assert_frame_ok(frame, expected_data, width, height, row_size, description)
        assert_equal width, frame.size.width, "unexpected width for #{description}"
        assert_equal height, frame.size.height, "unexpected height for #{description}"
        assert_equal :MODE_RGB, frame.frame_mode,
                     "unexpected frame mode for #{description}"
        assert_equal :STATUS_VALID, frame.frame_status,
                     "unexpected frame status for #{description}"
        assert_equal row_size, frame.row_size, "unexpected row size for #{description}"
        assert_equal 8, frame.data_depth, "unexpected data depth for #{description}"
        assert(expected_data == frame.image.to_byte_array,
               "data bitmap differs in #{description}")
    end
end
