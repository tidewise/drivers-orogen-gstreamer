# frozen_string_literal: true

using_task_library "gstreamer"

module Services
    data_service_type "ImageSource" do
        output_port "image", ro_ptr("/base/samples/frame/Frame")
    end

    data_service_type "ImageSink" do
        input_port "image", ro_ptr("/base/samples/frame/Frame")
    end

    data_service_type "DataChannel" do
        input_port "in", "/iodrivers_base/RawPacket"
        output_port "out", "/iodrivers_base/RawPacket"
    end
end

Syskit.extend_model OroGen.gstreamer.WebRTCCommonTask do
    argument :self_peer_id
    argument :remote_peer_id, default: nil
    argument :polite
    argument :data_channels, default: []

    dynamic_service Services::DataChannel, as: "data_channel" do
        provides Services::DataChannel,
                 as: name, "out" => "#{name}_out", "in" => "#{name}_in"
    end

    def update_properties
        super

        properties.signalling do |s|
            s.self_peer_id = self_peer_id
            s.remote_peer_id = remote_peer_id if remote_peer_id
            s.polite = polite
            s
        end
        properties.data_channels = data_channels
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

Syskit.extend_model OroGen.gstreamer.RTPTask do
    argument :rtp_monitoring_config

    def update_properties
        super

        properties.rtp_monitoring_config = rtp_monitoring_config
    end
end
