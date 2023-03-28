# frozen_string_literal: true

using_task_library "gstreamer"

WEBRTC_CONNECTED_STATES = {
    "signaling_state" => "GST_WEBRTC_SIGNALING_STATE_STABLE",
    "peer_connection_state" => "GST_WEBRTC_PEER_CONNECTION_STATE_NEW",
    "ice_connection_state" => "GST_WEBRTC_ICE_CONNECTION_STATE_CONNECTED",
    "ice_gathering_state" => "GST_WEBRTC_ICE_GATHERING_STATE_COMPLETE"
}.freeze

describe OroGen.gstreamer.WebRTCCommonTask do
    run_live

    before do
        generator_m = OroGen.gstreamer.Task.specialize
        generator_m.require_dynamic_service("image_source", as: "out")

        @cmp_m = Syskit::Composition.new_submodel do
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
            add(OroGen.gstreamer.WebRTCReceiveTask, as: "receive")

            generator_child.connect_to send_child
            send_child.signalling_out_port.connect_to \
                receive_child.signalling_in_port, type: :buffer, size: 100
            receive_child.signalling_out_port.connect_to \
                send_child.signalling_in_port, type: :buffer, size: 100
        end

    end

    after do
    end

    it "successfully establishes connection and transmits video when the receiver is "\
       "polite and both of them have each other's peer ID" do
        cmp = syskit_deploy(
            cmp_m(
                sender_polite: false,
                receiver_remote_peer_id: "sender",
                sender_remote_peer_id: "receiver"
            )
        )

        syskit_configure(cmp)
        sender_stats_r = cmp.send_child.stats_port.reader
        receiver_stats_r = cmp.receive_child.stats_port.reader
        syskit_start(cmp)

        frames = expect_execution.to do
            [have_one_new_sample(cmp.generator_child.out_port),
             have_one_new_sample(cmp.receive_child.video_out_port)]
        end

        s = sender_stats_r.read
        s_expected = { "peer_id" => "receiver" }.merge(WEBRTC_CONNECTED_STATES)
        r = receiver_stats_r.read
        r_expected = { "peer_id" => "sender" }.merge(WEBRTC_CONNECTED_STATES)
        assert_equal [s_expected], s.peers.map(&:to_simple_value)
        assert_equal r_expected, r.to_simple_value
        assert_has_transmission_succeeded(frames[0], frames[1])
    end

    it "successfully sends data to two receivers with different peer IDs when the "\
       "receivers are polite" do
        cmp1_m = cmp_m(
            receiver: "r1", sender_polite: false, receiver_remote_peer_id: "sender"
        )
        cmp2_m = cmp_m(
            receiver: "r2", sender_polite: false, receiver_remote_peer_id: "sender"
        )
        cmp1, cmp2 = syskit_deploy(cmp1_m, cmp2_m)

        syskit_configure(cmp1)
        syskit_configure(cmp2)
        sender_stats_r = cmp1.send_child.stats_port.reader
        r1_stats_r = cmp1.receive_child.stats_port.reader
        r2_stats_r = cmp2.receive_child.stats_port.reader
        syskit_start(cmp1)
        syskit_start(cmp2)

        frames = expect_execution.to do
            [have_one_new_sample(cmp1.generator_child.out_port),
             have_one_new_sample(cmp1.receive_child.video_out_port),
             have_one_new_sample(cmp2.receive_child.video_out_port)]
        end

        s = sender_stats_r.read
        s1_expected = { "peer_id" => "r1" }.merge(WEBRTC_CONNECTED_STATES)
        s2_expected = { "peer_id" => "r2" }.merge(WEBRTC_CONNECTED_STATES)
        r1 = r1_stats_r.read
        r2 = r2_stats_r.read
        r_expected = { "peer_id" => "sender" }.merge(WEBRTC_CONNECTED_STATES)
        assert_equal [s1_expected, s2_expected],
                     s.peers.sort_by(&:peer_id).map(&:to_simple_value)
        assert_equal r_expected, r1.to_simple_value
        assert_equal r_expected, r2.to_simple_value
        assert_has_transmission_succeeded(frames[0], frames[1])
        assert_has_transmission_succeeded(frames[0], frames[2])
    end

    def cmp_m(sender: "sender", receiver: "receiver", sender_remote_peer_id: nil, receiver_remote_peer_id: nil, sender_polite:)
        @cmp_m.use(
            "send" =>
                OroGen.gstreamer.WebRTCSendTask
                      .deployed_as("gstreamer_webrtc_test_#{sender}")
                      .with_arguments(
                          self_peer_id: sender, remote_peer_id: sender_remote_peer_id,
                          polite: sender_polite
                      ),
            "receive" =>
                OroGen.gstreamer.WebRTCReceiveTask
                      .deployed_as("gstreamer_webrtc_test_#{receiver}")
                      .with_arguments(
                          self_peer_id: receiver, remote_peer_id: receiver_remote_peer_id,
                          polite: !sender_polite
                      )
        )
    end

    def assert_has_transmission_succeeded(generator_frame, received_frame)
        abs_sum =
            generator_frame
            .image
            .zip(received_frame.image)
            .map { |a, b| (a - b).abs }
            .inject(&:+)
        mean_abs_sum = Float(abs_sum) / generator_frame.image.size
        assert(mean_abs_sum < 10,
               "expected the mean abs sum of source and received images to be less "\
               "than 10, but is #{mean_abs_sum}")
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

Syskit.extend_model OroGen.gstreamer.WebRTCCommonTask do
    argument :self_peer_id
    argument :remote_peer_id, default: nil
    argument :polite

    def update_properties
        super

        properties.signalling_config do |s|
            s.self_peer_id = self_peer_id
            s.remote_peer_id = remote_peer_id if remote_peer_id
            s.polite = polite
            s
        end
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
