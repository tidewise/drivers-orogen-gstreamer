# frozen_string_literal: true

require_relative "models"

describe OroGen.gstreamer.Task do
    run_live

    it "attaches ports to source and sinks in the pipeline" do
        self.expect_execution_default_timeout = 600

        cmp = syskit_deploy_configure_and_start(cmp_m)
        samples = expect_execution.to do
            [have_new_samples(cmp.generator_child.out_port, 2),
             have_new_samples(cmp.target_child.out_port, 2)]
        end
        samples = samples.map(&:last)

        expected = File.binread(File.join(__dir__, "videotestsrc_colors_320_240.bin"))
        assert_frame_ok(samples[0], expected, 320, 240, 960, "generated frame")
        assert_frame_ok(samples[1], expected, 320, 240, 960, "transferred frame")
    end

    it "handles data that is not 8-bytes aligned" do
        # Having lines smaller than 8 bytes causes gstreamer to naturally pad them
        # This requires special handling inside the component
        cmp = syskit_deploy_configure_and_start(cmp_m(319, 240))
        samples = expect_execution.to do
            [have_new_samples(cmp.generator_child.out_port, 2),
             have_new_samples(cmp.target_child.out_port, 2)]
        end
        samples = samples.map(&:last)

        expected = File.binread(File.join(__dir__, "videotestsrc_colors_319_240.bin"))
        assert_frame_ok(samples[0], expected, 319, 240, 957, "generated frame")
        assert_frame_ok(samples[1], expected, 319, 240, 957, "transferred frame")
    end

    it "attaches ports to a jpeg source and sinks in the pipeline" do
        self.expect_execution_default_timeout = 15

        jpeg_cmp = syskit_deploy_configure_and_start(jpeg_cmp_m)
        expect_execution.to do
            have_successful_transmission(self, jpeg_cmp)
        end
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
        OroGen.gstreamer.Task.with_dynamic_service("image_sink", as: "in").with_dynamic_service("image_source", as: "inverted_out")
              .with_arguments(
                  inputs: [{ name: "in" }],
                  outputs: [{ name: "inverted_out", frame_mode: "MODE_RGB" }],
                  pipeline: <<~PIPELINE
                      appsrc format=3 name=in ! queue ! videoflip method=vertical-flip !
                          appsink name=inverted_out
                  PIPELINE
              )
              .deployed_as("inverter")
    end

    def target_m(width = 320, height = 240)
        OroGen.gstreamer.Task.with_dynamic_service("image_source", as: "out").with_dynamic_service("image_sink", as: "inverted_in")
              .with_arguments(
                  inputs: [{ name: "inverted_in" }],
                  outputs: [{ name: "out", frame_mode: "MODE_RGB" }],
                  pipeline: <<~PIPELINE
                      appsrc format=3 name=inverted_in ! queue ! videoflip method=vertical-flip !
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
        cmp_m.inverter_child.connect_to cmp_m.target_child
        cmp_m
    end

    def jpeg_generator_m(width = 320, height = 240)
        OroGen.gstreamer.Task.with_dynamic_service("image_source", as: "out")
              .with_arguments(
                  outputs: [{ name: "out", frame_mode: "MODE_RGB" }],
                  pipeline: <<~PIPELINE
                      videotestsrc pattern=colors num-buffers=5 !
                      video/x-raw,width=#{width},height=#{height} !
                      appsink name=out
                  PIPELINE
              )
              .deployed_as("generator")
    end

    def jpeg_converter_m
        OroGen.gstreamer.Task
              .with_dynamic_service("image_sink", as: "in")
              .with_dynamic_service("image_source", as: "out")
              .with_arguments(
                  inputs: [{ name: "in" }],
                  outputs: [{ name: "out", frame_mode: "MODE_JPEG" }],
                  pipeline: <<~PIPELINE
                      appsrc name=in ! videoconvert ! video/x-raw,format=(string)I420 !
                      jpegenc ! appsink name=out
                  PIPELINE
              )
              .deployed_as("jpeg_converter")
    end

    def jpeg_target_m
        OroGen.gstreamer.Task
              .with_dynamic_service("image_sink", as: "in")
              .with_dynamic_service("image_source", as: "out")
              .with_arguments(
                  inputs: [{ name: "in" }],
                  outputs: [{ name: "out", frame_mode: "MODE_RGB" }],
                  pipeline: <<~PIPELINE
                      appsrc name=in ! jpegdec ! video/x-raw,format=(string)I420 !
                      videoconvert ! appsink name=out
                  PIPELINE
              )
              .deployed_as("target")
    end

    def jpeg_cmp_m(width = 320, height = 240)
        jpeg_cmp_m = Syskit::Composition.new_submodel
        jpeg_cmp_m.add jpeg_generator_m(width, height), as: "jpeg_generator"
        jpeg_cmp_m.add jpeg_converter_m, as: "jpeg_converter"
        jpeg_cmp_m.add jpeg_target_m, as: "jpeg_target"
        jpeg_cmp_m.jpeg_generator_child.connect_to jpeg_cmp_m.jpeg_converter_child
        jpeg_cmp_m.jpeg_converter_child.connect_to jpeg_cmp_m.jpeg_target_child
        jpeg_cmp_m
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

    def have_successful_transmission(expectations, cmp) # rubocop:disable Naming/PredicateName
        expected = nil
        pp "Getting generator buffer"
        expectations
            .have_one_new_sample(cmp.jpeg_generator_child.out_port)
            .matching { |f| expected = f }
        pp "Getting target buffer"
        expectations
            .have_one_new_sample(cmp.jpeg_target_child.out_port)
            .matching { |f| expected && transmission_succeeded?(expected, f) }
    end

    def transmission_succeeded?(generator_frame, received_frame)
        abs_sum =
            generator_frame
            .image
            .zip(received_frame.image)
            .map { |a, b| (a - b).abs }
            .inject(&:+)
        mean_abs_sum = Float(abs_sum) / generator_frame.image.size
        mean_abs_sum < 12
    end
end
