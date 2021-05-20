# frozen_string_literal: true

using_task_library "gstreamer"

Syskit.extend_model OroGen.gstreamer.Task do
    def configure
        super
        properties.pipeline = <<~PIPELINE
            videotestsrc pattern=ball ! queue ! videoconvert ! appsink name=ball
        PIPELINE
        properties.outputs = [{ name: "ball", frameMode: "MODE_RGB" }]
    end
end

Robot.controller do
    Roby.plan.add_mission_task(
        OroGen.gstreamer.Task
              .deployed_as("ball_source")
    )
end
