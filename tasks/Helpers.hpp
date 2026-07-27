#ifndef ROCK_GSTREAMER_HELPERS_HPP
#define ROCK_GSTREAMER_HELPERS_HPP

#include <gst/gstbuffer.h>
#include <gst/gstobject.h>
#include <gst/gstsample.h>
#include <gst/video/gstvideometa.h>

#include <base/samples/Frame.hpp>

namespace gstreamer {
    inline GstVideoFormat rawModeToGSTVideoFormat(
        base::samples::frame::frame_mode_t frame_mode)
    {
        switch (frame_mode) {
            case base::samples::frame::MODE_RGB:
                return GST_VIDEO_FORMAT_RGB;
            case base::samples::frame::MODE_BGR:
                return GST_VIDEO_FORMAT_BGR;
            case base::samples::frame::MODE_RGB32:
                return GST_VIDEO_FORMAT_RGBx;
            case base::samples::frame::MODE_GRAYSCALE:
                return GST_VIDEO_FORMAT_GRAY8;
            default:
                // Should not happen, the component validates the frame mode
                // against the accepted modes
                throw std::runtime_error("unsupported base::samples::frame_mode " +
                                         std::to_string(frame_mode));
        }
    }

    inline GstCaps* rawModeToGSTCaps(base::samples::frame::frame_mode_t frame_mode)
    {
        auto format = rawModeToGSTVideoFormat(frame_mode);
        GstCaps* caps = gst_caps_new_simple("video/x-raw",
            "format",
            G_TYPE_STRING,
            gst_video_format_to_string(format),
            NULL);
        if (!caps) {
            throw std::runtime_error("failed to generate caps");
        }

        return caps;
    }

    inline std::string bayerModeToGSTCapsFormat(
        base::samples::frame::frame_mode_t frame_mode)
    {
        switch (frame_mode) {
            case base::samples::frame::MODE_BAYER_BGGR:
                return "bggr";
            case base::samples::frame::MODE_BAYER_GBRG:
                return "gbrg";
            case base::samples::frame::MODE_BAYER_GRBG:
                return "grbg";
            case base::samples::frame::MODE_BAYER_RGGB:
                return "rggb";
            default:
                // Should not happen, the component validates the frame mode
                // against the accepted modes
                throw std::runtime_error("unsupported bayer format received");
        }
    }

    inline GstCaps* bayerModeToGSTCaps(base::samples::frame::frame_mode_t frame_mode)
    {
        auto format = bayerModeToGSTCapsFormat(frame_mode);
        GstCaps* caps = gst_caps_new_simple("video/x-bayer",
            "format",
            G_TYPE_STRING,
            format.c_str(),
            NULL);
        if (!caps) {
            throw std::runtime_error("failed to generate caps");
        }

        return caps;
    }

    inline GstCaps* jpegModeToGSTCaps(base::samples::frame::frame_mode_t frame_mode)
    {
        GstCaps* caps = gst_caps_new_empty_simple("image/jpeg");
        if (!caps) {
            throw std::runtime_error("failed to generate caps");
        }

        return caps;
    }

    inline bool isFrameModeBayer(base::samples::frame::frame_mode_t frame_mode)
    {
        switch (frame_mode) {
            case base::samples::frame::MODE_BAYER_BGGR:
            case base::samples::frame::MODE_BAYER_GBRG:
            case base::samples::frame::MODE_BAYER_GRBG:
            case base::samples::frame::MODE_BAYER_RGGB:
                return true;
            default:
                return false;
        }
    }

    inline GstCaps* frameModeToGSTCaps(base::samples::frame::frame_mode_t frame_mode)
    {
        if (isFrameModeBayer(frame_mode)) {
            return bayerModeToGSTCaps(frame_mode);
        }
        else if (frame_mode == base::samples::frame::MODE_JPEG) {
            return jpegModeToGSTCaps(frame_mode);
        }
        else {
            return rawModeToGSTCaps(frame_mode);
        }
    }
}

#endif
