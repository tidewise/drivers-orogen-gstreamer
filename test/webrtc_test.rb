# frozen_string_literal: true

require_relative "models"

WEBRTC_CONNECTED_STATES = {
    signaling_state: :GST_WEBRTC_SIGNALING_STATE_STABLE,
    peer_connection_state: :GST_WEBRTC_PEER_CONNECTION_STATE_CONNECTED,
    ice_connection_state: :GST_WEBRTC_ICE_CONNECTION_STATE_COMPLETED,
    ice_gathering_state: :GST_WEBRTC_ICE_GATHERING_STATE_COMPLETE
}.freeze

describe OroGen.gstreamer.WebRTCCommonTask do
    run_live

    before do
        generator_m = OroGen.gstreamer.Task.with_dynamic_service(
            "image_source", as: "out"
        )

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
            expect_execution.to do
                have_successful_transmission(self, cmp)
                receiver_has_connection_established(self, cmp.receive_child, "sender")
                sender_has_all_connections_established(self, cmp.send_child, ["receiver"])
            end
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
            expect_execution.to do
                have_successful_transmission(self, cmp)
                receiver_has_connection_established(self, cmp.receive_child, "sender")
                sender_has_all_connections_established(self, cmp.send_child, ["receiver"])
            end
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
            expect_execution.to do
                have_successful_transmission(self, cmp)
                receiver_has_connection_established(self, cmp.receive_child, "sender")
                sender_has_all_connections_established(self, cmp.send_child, ["receiver"])
            end
        end

        it "establishes connection if the receiver is started first" do
            receiver_m = self.receiver_m(polite: true, remote_peer_id: "sender")
            cmp_m = self.cmp_m(sender_polite: false, receiver_remote_peer_id: "sender")
            receiver = syskit_deploy(receiver_m)
            syskit_configure_and_start(receiver)

            cmp = syskit_deploy(cmp_m)
            assert_same cmp.receive_child, receiver

            syskit_configure_and_start(cmp)
            expect_execution.to do
                have_successful_transmission(self, cmp)
                receiver_has_connection_established(self, cmp.receive_child, "sender")
                sender_has_all_connections_established(self, cmp.send_child, ["receiver"])
            end
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
            cmp1, cmp2 = syskit_deploy(cmp1_m, cmp2_m)
            syskit_configure_and_start(cmp1)
            syskit_configure_and_start(cmp2)

            expect_execution.to do
                have_successful_transmission(self, cmp1)
                have_successful_transmission(self, cmp2)
                receiver_has_connection_established(self, cmp1.receive_child, "sender")
                receiver_has_connection_established(self, cmp2.receive_child, "sender")
                sender_has_all_connections_established(self, cmp1.send_child, %w[r1 r2])
            end
        end
    end

    describe "data channels" do
        it "creates a connection on an impolite sender" do
            cmp_m = self.cmp_m(
                sender_polite: false,
                sender_remote_peer_id: "receiver",
                receiver_remote_peer_id: "sender",
                data_channels: [{ label: "c", from_sender: true }]
            )
            cmp = syskit_deploy_configure_and_start(cmp_m)

            sender_msg = Types.iodrivers_base.RawPacket.new(data: [1])
            receiver_msg = Types.iodrivers_base.RawPacket.new(data: [2])
            sender_out_msg, receiver_out_msg =
                expect_execution.poll do
                    syskit_write cmp.send_child.send_child.c_in_port, sender_msg
                    syskit_write cmp.receive_child.c_in_port, receiver_msg
                end.to do
                    [have_one_new_sample(cmp.send_child.send_child.c_out_port),
                     have_one_new_sample(cmp.receive_child.c_out_port)]
                end

            assert_equal sender_msg.data, receiver_out_msg.data
            assert_equal receiver_msg.data, sender_out_msg.data
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
            have_successful_transmission(self, cmp1)
            have_successful_transmission(self, cmp2)
        end

        plan.unmark_mission_task(cmp2)
        expect_execution.garbage_collect(true).to do
            emit cmp2.receive_child.stop_event
        end

        s = sender_stats_r.read
        s1_expected = { peer_id: "r1" }.merge(WEBRTC_CONNECTED_STATES)
        assert_equal [Types.gstreamer.WebRTCPeerStats.new(s1_expected)],
                     s.peers.sort_by(&:peer_id)

        expect_execution.to { have_successful_transmission(self, cmp1) }
    end

    def sender_m(self_peer_id: "sender", remote_peer_id: nil, polite:, data_channels: [])
        data_channels = data_channels.map do |h|
            { label: h[:label], port_basename: h[:label],
              create: h[:from_sender], binary_in: h[:binary_in] || false }
        end

        send_m =
            data_channels.inject(OroGen.gstreamer.WebRTCSendTask) do |task_m, c|
                task_m.with_dynamic_service("data_channel", as: c[:label])
            end

        @sender_m.use(
            "send" =>
                send_m
                      .deployed_as("gstreamer_webrtc_test_#{self_peer_id}")
                      .with_arguments(
                          self_peer_id: self_peer_id, remote_peer_id: remote_peer_id,
                          polite: polite, data_channels: data_channels
                      )
        )
    end

    def receiver_m(self_peer_id: "receiver", remote_peer_id: nil, polite:, data_channels: [])
        data_channels = data_channels.map do |h|
            { label: h[:label], port_basename: h[:label],
              create: !h[:from_sender], binary_in: h[:binary_in] || false }
        end

        receive_m =
            data_channels.inject(OroGen.gstreamer.WebRTCReceiveTask) do |task_m, c|
                task_m.with_dynamic_service("data_channel", as: c[:label])
            end

        receive_m
            .deployed_as("gstreamer_webrtc_test_#{self_peer_id}")
            .with_arguments(
                self_peer_id: self_peer_id, remote_peer_id: remote_peer_id,
                polite: polite, data_channels: data_channels
            )
    end

    def cmp_m(
        sender: "sender", receiver: "receiver", sender_remote_peer_id: nil,
        receiver_remote_peer_id: nil, sender_polite:, data_channels: []
    )
        sender_m = self.sender_m(
            self_peer_id: sender, remote_peer_id: sender_remote_peer_id,
            polite: sender_polite, data_channels: data_channels
        )
        receiver_m = self.receiver_m(
            self_peer_id: receiver, remote_peer_id: receiver_remote_peer_id,
            polite: !sender_polite, data_channels: data_channels
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
        WEBRTC_CONNECTED_STATES.all? { |key, value| value == p[key] }
    end

    def read_expected_frame(cmp)
        expect_execution.to do
            have_one_new_sample(cmp.send_child.generator_out_port)
        end
    end

    def have_successful_transmission(expectations, cmp) # rubocop:disable Naming/PredicateName
        expected = nil
        expectations
            .have_one_new_sample(cmp.send_child.generator_out_port)
            .matching { |f| expected = f }
        expectations
            .have_one_new_sample(cmp.receive_child.video_out_port)
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
        mean_abs_sum < 10
    end
end
