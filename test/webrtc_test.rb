# frozen_string_literal: true

using_task_library "gstreamer"

describe OroGen.gstreamer.WebRTCCommonTask do
    run_live

    attr_reader :cmp

    before do
        generator_m = OroGen.gstreamer.Task.specialize
        generator_m.require_dynamic_service("image_source", as: "out")

        cmp_m = Syskit::Composition.new_submodel do
            add(generator_m, as: "generator")
                .with_arguments(
                    outputs: [{ name: "out", frame_mode: "MODE_RGB" }],
                    pipeline: <<~PIPELINE
                        videotestsrc pattern=colors ! video/x-raw,width=320,height=240 !
                            appsink name=out
                    PIPELINE
                )
                .deployed_as("gstreamer_webrtc_test_generator")
            add(OroGen.gstreamer.WebRTCSendTask, as: "send")
                .deployed_as("gstreamer_webrtc_test_send")
            add(OroGen.gstreamer.WebRTCReceiveTask, as: "receive")
                .deployed_as("gstreamer_webrtc_test_receive")

            generator_child.connect_to send_child
            send_child.signalling_out_port.connect_to \
                receive_child.signalling_in_port, type: :buffer, size: 100
            receive_child.signalling_out_port.connect_to \
                send_child.signalling_in_port, type: :buffer, size: 100
        end

        @cmp = syskit_deploy(cmp_m)
    end

    after do
    end

    it "successfully establishes connection and transmits video when the sender is "\
       "polite and both of them have each other's peer ID" do
        cmp.send_child.properties.signalling_config = {
            polite: false, self_peer_id: "sender", remote_peer_id: "receiver"
        }
        cmp.receive_child.properties.signalling_config = {
            polite: true, self_peer_id: "receiver", remote_peer_id: "sender"
        }

        syskit_configure_and_start(cmp)
        frames = expect_execution.to do
            [have_one_new_sample(cmp.generator_child.out_port),
             have_one_new_sample(cmp.receive_child.video_out_port)]
        end
        abs_sum =
            frames[0]
            .image
            .zip(frames[1].image)
            .map { |a, b| (a - b).abs }
            .inject(&:+)
        mean_abs_sum = Float(abs_sum) / frames[0].image.size
        assert(mean_abs_sum < 10,
               "expected the mean abs sum of source and received images to be less "\
               "than 1, but is #{mean_abs_sum}")
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

OroGen.gstreamer.Task.class_eval do
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
