# frozen_string_literal: true

require_relative "models"

describe OroGen.gstreamer.RTPTask do
    run_live

    before do
        @rtp_port = allocate_available_port
    end

    it "publishes sender statistics" do
        rtp_sender = syskit_deploy_configure_and_start(rtp_sender_m(@rtp_port))
        rtp_receiver = syskit_deploy_configure_and_start(rtp_receiver_m(@rtp_port))

        expect_execution.to do
            have_one_new_sample(rtp_sender.rtp_statistics_port)
        end

        expect_execution.to do
            have_one_new_sample(rtp_receiver.rtp_statistics_port)
        end
    end

    it "does not raise on re-configuration cycle" do
        m = rtp_sender_m(@rtp_port)
        sender = syskit_deploy_configure_and_start(m)
        sender.needs_reconfiguration!
        syskit_stop(sender)
        syskit_deploy_configure_and_start(m)
    end

    describe "retransmission" do
        attr_reader :ports
        before do
            @ports = 6.times.map { |_| allocate_available_port }
        end

        it "can setup rtpbin with pipeline mapping properties" do
            receiver_m =
                OroGen.gstreamer.RTPTask
                .with_arguments(
                    pipeline: <<~PIPELINE
                        rtpbin name=receive rtp-profile=avpf latency=250 do-retransmission=true
                               fec-decoders=fec,0="rtpst2022-1-fecdec\\ size-time\\=1000000000";
                        udpsrc name="rtp_src" port=#{ports[0]}
                               caps="application/x-rtp,media=(string)video,clock-rate=(int)90000,
                               encoding-name=(string)H264,payload=(int)96"
                        queue name="rtp_sink"
                        ! udpsink host=127.0.0.1 port=#{ports[5]}
                        udpsrc name="rtcp_src" port=#{ports[1]}
                        udpsink name="rtcp_feedback_sink" host=127.0.0.1 port=#{ports[2]}
                        udpsrc name="row_fec" port=#{ports[3]}
                        udpsrc name="col_fec" port=#{ports[4]}
                    PIPELINE
                )
                .with_arguments(rtp_monitoring_config:
                    { rtpbin_name: "receive", sessions_id: [0] })
                .deployed_as("rtpreceive")

            transmit_m =
                OroGen.gstreamer.RTPTask
                .with_arguments(
                    pipeline: <<~PIPELINE
                        rtpbin name=transmit rtp-profile=avpf
                               fec-encoders=fec,0="rtpst2022-1-fecenc\\ rows\\=10\\ columns\\=10";
                        videotestsrc
                        ! x264enc speed-preset=ultrafast
                        ! rtph264pay ssrc=0 aggregate-mode=zero-latency config-interval=-1
                        ! capsfilter name="rtp_src"
                                     caps=application/x-rtp,media=video,clock-rate=90000,encoding-name=H264,payload=96
                        udpsink name="rtp_sink" host=127.0.0.1 port=#{ports[0]}
                        udpsink name="rtcp_feedback_sink" host=127.0.0.1 port=#{ports[1]} sync=false async=false
                        udpsrc name="rtcp_src" port=#{ports[2]}
                        udpsink name="row_fec" host=127.0.0.1 port=#{ports[3]} async=false
                        udpsink name="col_fec" host=127.0.0.1 port=#{ports[4]} async=false
                    PIPELINE
                )
                .with_arguments(rtp_monitoring_config:
                    { rtpbin_name: "transmit", sessions_id: [0] })
                .deployed_as("rtptransmit")

            sender_t = syskit_deploy(transmit_m)
            sender_t.property_overrides.sender_map =
                { session_id: 0, rtp_source: "rtp_src", rtp_sink: "rtp_sink",
                  rtcp_source: "rtcp_src", rtcp_feedback_sink: "rtcp_feedback_sink",
                  fec_sink_0: "row_fec", fec_sink_1: "col_fec" }
            sender_t.needs_reconfiguration!
            syskit_configure_and_start(sender_t)

            receiver_t = syskit_deploy(receiver_m)
            receiver_t.property_overrides.receiver_map =
                { session_id: 0, rtp_source: "rtp_src",
                  rtp_sink: "rtp_sink", rtcp_source: "rtcp_src",
                  rtcp_feedback_sink: "rtcp_feedback_sink",
                  fec_source_0: "row_fec", fec_source_1: "col_fec" }
            receiver_t.needs_reconfiguration!

            syskit_configure_and_start(receiver_t)
        end
    end

    def rtp_sender_m(port)
        OroGen.gstreamer.RTPTask
              .with_arguments(
                  pipeline: <<~PIPELINE
                      rtpbin name=rtptransmit
                      videotestsrc
                          ! x264enc speed-preset=ultrafast
                          ! rtph264pay ssrc=0 aggregate-mode=zero-latency config-interval=-1
                          ! application/x-rtp,media=video,clock-rate=90000,encoding-name=H264,payload=96
                          ! rtptransmit.send_rtp_sink_0
                      rtptransmit.send_rtp_src_0
                          ! udpsink host=127.0.0.1 port=#{port}
                  PIPELINE
              )
              .with_arguments(rtp_monitoring_config:
                  { rtpbin_name: "rtptransmit", sessions_id: [0] })
              .deployed_as("rtptransmit")
    end

    def rtp_receiver_m(port)
        OroGen.gstreamer.RTPTask
              .with_arguments(
                  pipeline: <<~PIPELINE
                      rtpbin name=rtpreceive
                      udpsrc port=#{port}
                             caps="application/x-rtp,media=(string)video,clock-rate=(int)90000,
                             encoding-name=(string)H264,payload=(int)96"
                          ! rtpreceive.recv_rtp_sink_0
                      rtpreceive.recv_rtp_src_0_0_96
                          ! filesink location=/dev/null
                  PIPELINE
              )
              .with_arguments(rtp_monitoring_config:
                  { rtpbin_name: "rtpreceive", sessions_id: [0] })
              .deployed_as("rtpreceive")
    end

    # Poor man's dynamic port allocation
    #
    # Get a free port from Linux and free it, hoping that in our test setups
    # noone will be able to grab the same port between this call and the actual
    # gstreamer task
    def allocate_available_port
        server = TCPServer.new("127.0.0.1", 0)
        server.local_address.ip_port
    ensure
        server.close
    end
end
