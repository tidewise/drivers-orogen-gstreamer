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

        @sender_m = Syskit::Composition.new_submodel do
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

            generator_child.connect_to send_child
            export generator_child.out_port, as: "generator_out"
            export send_child.signalling_in_port
            export send_child.signalling_out_port
            export send_child.stats_port
        end

        sender_m = @sender_m
        @cmp_m = Syskit::Composition.new_submodel do
            add(sender_m, as: "send")
            add(OroGen.gstreamer.WebRTCReceiveTask, as: "receive")

            send_child.signalling_out_port.connect_to \
                receive_child.signalling_in_port, type: :buffer, size: 100
            receive_child.signalling_out_port.connect_to \
                send_child.signalling_in_port, type: :buffer, size: 100
        end

    end

    after do
    end

    describe "connections with both peer_ids set and the receiver as polite peer" do
        it "establishes connection if the sender is started first" do
            sender_m = self.sender_m(polite: false, remote_peer_id: "receiver")
            cmp_m = self.cmp_m(
                sender_polite: false,
                receiver_remote_peer_id: "sender",
                sender_remote_peer_id: "receiver"
            )
            sender = syskit_deploy(sender_m)
            syskit_configure_and_start(sender)

            cmp = syskit_deploy(cmp_m)
            assert_same cmp.send_child, sender

            syskit_configure_and_start(cmp)
            frames = expect_execution.to do
                [have_one_new_sample(cmp.send_child.generator_out_port),
                 have_one_new_sample(cmp.receive_child.video_out_port),
                 receiver_has_connection_established(self, cmp.receive_child, "sender"),
                 sender_has_all_connections_established(self, cmp.send_child, ["receiver"])]
            end

            assert_has_transmission_succeeded(frames[0], frames[1])
        end

        it "establishes connection if the receiver is started first" do
            receiver_m = self.receiver_m(polite: true, remote_peer_id: "sender")
            cmp_m = self.cmp_m(
                sender_polite: false,
                receiver_remote_peer_id: "sender",
                sender_remote_peer_id: "receiver"
            )
            receiver = syskit_deploy(receiver_m)
            syskit_configure_and_start(receiver)

            cmp = syskit_deploy(cmp_m)
            assert_same cmp.receive_child, receiver

            syskit_configure_and_start(cmp)
            frames = expect_execution.to do
                [have_one_new_sample(cmp.send_child.generator_out_port),
                 have_one_new_sample(cmp.receive_child.video_out_port),
                 receiver_has_connection_established(self, cmp.receive_child, "sender"),
                 sender_has_all_connections_established(self, cmp.send_child, ["receiver"])]
            end

            assert_has_transmission_succeeded(frames[0], frames[1])
        end
    end

    describe "connections with the peer_id only set on the receiver, which is polite" do
        it "establishes connection if the sender is started first" do
            sender_m = self.sender_m(polite: false)
            cmp_m = self.cmp_m(sender_polite: false, receiver_remote_peer_id: "sender")
            sender = syskit_deploy(sender_m)
            syskit_configure_and_start(sender)

            cmp = syskit_deploy(cmp_m)
            assert_same cmp.send_child, sender

            syskit_configure_and_start(cmp)
            frames = expect_execution.to do
                [have_one_new_sample(cmp.send_child.generator_out_port),
                 have_one_new_sample(cmp.receive_child.video_out_port),
                 receiver_has_connection_established(self, cmp.receive_child, "sender"),
                 sender_has_all_connections_established(self, cmp.send_child, ["receiver"])]
            end

            assert_has_transmission_succeeded(frames[0], frames[1])
        end

        it "establishes connection if the receiver is started first" do
            receiver_m = self.receiver_m(polite: true, remote_peer_id: "sender")
            cmp_m = self.cmp_m(sender_polite: false, receiver_remote_peer_id: "sender")
            receiver = syskit_deploy(receiver_m)
            syskit_configure_and_start(receiver)

            cmp = syskit_deploy(cmp_m)
            assert_same cmp.receive_child, receiver

            syskit_configure_and_start(cmp)
            frames = expect_execution.to do
                [have_one_new_sample(cmp.send_child.generator_out_port),
                 have_one_new_sample(cmp.receive_child.video_out_port),
                 receiver_has_connection_established(self, cmp.receive_child, "sender"),
                 sender_has_all_connections_established(self, cmp.send_child, ["receiver"])]
            end

            assert_has_transmission_succeeded(frames[0], frames[1])
        end

        it "establishes connection with two receivers, startup sandwiched" do
            r1_m = self.receiver_m(
                polite: true, self_peer_id: "r1", remote_peer_id: "sender"
            )
            syskit_deploy_configure_and_start(r1_m)

            cmp1_m = cmp_m(
                receiver: "r1", sender_polite: false, receiver_remote_peer_id: "sender"
            )
            cmp2_m = cmp_m(
                receiver: "r2", sender_polite: false, receiver_remote_peer_id: "sender"
            )
            r2_m = self.receiver_m(
                polite: true, self_peer_id: "r1", remote_peer_id: "sender"
            )
            cmp1, cmp2 = syskit_deploy(cmp1_m, cmp2_m)
            syskit_configure_and_start(cmp1)
            syskit_configure_and_start(cmp2)

            frames = expect_execution.to do
                [have_one_new_sample(cmp1.send_child.generator_out_port),
                 have_one_new_sample(cmp1.receive_child.video_out_port),
                 have_one_new_sample(cmp2.receive_child.video_out_port),
                 receiver_has_connection_established(self, cmp1.receive_child, "sender"),
                 receiver_has_connection_established(self, cmp2.receive_child, "sender"),
                 sender_has_all_connections_established(self, cmp1.send_child, %w[r1 r2])]
            end

            assert_has_transmission_succeeded(frames[0], frames[1])
            assert_has_transmission_succeeded(frames[0], frames[2])
        end
    end

    it "dynamically detects a lost peer on the send side" do
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
        syskit_start(cmp1)
        syskit_start(cmp2)

        expect_execution.to do
            [have_one_new_sample(cmp1.send_child.generator_out_port),
             have_one_new_sample(cmp1.receive_child.video_out_port),
             have_one_new_sample(cmp2.receive_child.video_out_port)]
        end

        plan.unmark_mission_task(cmp2)
        expect_execution.garbage_collect(true).to do
            emit cmp2.receive_child.stop_event
        end

        s = sender_stats_r.read
        s1_expected = { "peer_id" => "r1" }.merge(WEBRTC_CONNECTED_STATES)
        assert_equal [s1_expected],
                     s.peers.sort_by(&:peer_id).map(&:to_simple_value)

        frames = expect_execution.to do
            [have_one_new_sample(cmp1.send_child.generator_out_port),
             have_one_new_sample(cmp1.receive_child.video_out_port)]
        end
        assert_has_transmission_succeeded(frames[0], frames[1])
    end

    def sender_m(self_peer_id: "sender", remote_peer_id: nil, polite:)
        @sender_m.use(
            "send" =>
                OroGen.gstreamer.WebRTCSendTask
                      .deployed_as("gstreamer_webrtc_test_#{self_peer_id}")
                      .with_arguments(
                          self_peer_id: self_peer_id, remote_peer_id: remote_peer_id,
                          polite: polite
                      )
        )
    end

    def receiver_m(self_peer_id: "receiver", remote_peer_id: nil, polite:)
        OroGen.gstreamer.WebRTCReceiveTask
              .deployed_as("gstreamer_webrtc_test_#{self_peer_id}")
              .with_arguments(
                  self_peer_id: self_peer_id, remote_peer_id: remote_peer_id,
                  polite: polite
              )
    end

    def cmp_m(sender: "sender", receiver: "receiver", sender_remote_peer_id: nil,
        receiver_remote_peer_id: nil, sender_polite:)
        sender_m = self.sender_m(
            self_peer_id: sender, remote_peer_id: sender_remote_peer_id,
            polite: sender_polite
        )
        receiver_m = self.receiver_m(
            self_peer_id: receiver, remote_peer_id: receiver_remote_peer_id,
            polite: !sender_polite
        )
        @cmp_m.use("send" => sender_m, "receive" => receiver_m)
    end

    def receiver_has_connection_established(expectations, task, peer_id)
        expectations.have_one_new_sample(task.stats_port).matching do |p|
            p.peer_id == peer_id && connected_peer?(p)
        end
    end

    def sender_has_all_connections_established(expectations, task, peer_ids)
        expectations.have_one_new_sample(task.stats_port).matching do |stats|
            stats.peers.map(&:peer_id).sort == peer_ids.sort &&
                stats.peers.all? { |p| connected_peer?(p) }
        end
    end

    def connected_peer?(p)
        p.peer_connection_state == :GST_WEBRTC_PEER_CONNECTION_STATE_NEW &&
            p.ice_connection_state == :GST_WEBRTC_ICE_CONNECTION_STATE_CONNECTED
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
