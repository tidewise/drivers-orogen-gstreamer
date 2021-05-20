# frozen_string_literal: true

using_task_library "gstreamer"

Syskit.extend_model OroGen.gstreamer.Task do
    def configure
        super
        properties.pipeline = <<~PIPELINE
            videotestsrc name=test pattern=ball ! tee name=t
            t. ! queue ! videoconvert ! appsink name=ball
            t. ! queue ! xvimagesink
        PIPELINE
        properties.outputs = [{ name: "ball", frameMode: "MODE_RGB" }]
    end
end

Robot.controller do
    Roby.plan.add_mission_task(
        OroGen.gstreamer.Task
              .deployed_as_unmanaged("gstreamer_ball")
    )
end
