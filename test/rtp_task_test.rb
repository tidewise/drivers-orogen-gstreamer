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

        sender_sample = expect_execution.to do
            have_one_new_sample(rtp_sender.rtp_statistics_port)
        end

        receiver_sample = expect_execution.to do
            have_one_new_sample(rtp_receiver.rtp_statistics_port)
        end

        pp sender_sample
        pp receiver_sample
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
              .with_arguments(rtp_monitored_sessions:
                  { rtpbin_name: "rtptransmit", sessions: [{ session_id: 0 }] })
              .deployed_as("rtptransmit")
            #   .deployed_as_unmanaged("rtptransmit")
    end

    def rtp_receiver_m(port)
        OroGen.gstreamer.RTPTask
              .with_arguments(
                  pipeline: <<~PIPELINE
                      rtpbin name=receive
                      udpsrc port=#{port}
                             caps="application/x-rtp,media=(string)video,clock-rate=(int)90000,
                             encoding-name=(string)H264,payload=(int)96"
                          ! receive.recv_rtp_sink_0
                      receive.recv_rtp_src_0_0_96
                          ! filesink location=/dev/null
                  PIPELINE
              )
              .with_arguments(rtp_monitored_sessions:
                  { rtpbin_name: "receive", sessions: [{ session_id: 0 }] })
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
