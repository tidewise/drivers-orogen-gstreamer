#ifndef ROCK_GSTREAMER_HELPERS_HPP
#define ROCK_GSTREAMER_HELPERS_HPP

#include <gst/gstbuffer.h>
#include <gst/gstobject.h>
#include <gst/gstsample.h>
#include <gst/video/gstvideometa.h>

#include <base/samples/Frame.hpp>

namespace gstreamer {
    template <typename T> struct GstUnref;
#define ROCK_GSTREAMER_UNREF(GstStruct, gst_unref)                                       \
    template <> struct GstUnref<GstStruct> {                                             \
        static void unref(GstStruct* obj)                                                \
        {                                                                                \
            gst_unref(obj);                                                              \
        }                                                                                \
    };

    ROCK_GSTREAMER_UNREF(GstElement, gst_object_unref);
    ROCK_GSTREAMER_UNREF(GstCaps, gst_caps_unref);
    ROCK_GSTREAMER_UNREF(GstSample, gst_sample_unref);
    ROCK_GSTREAMER_UNREF(GstBuffer, gst_buffer_unref);
    ROCK_GSTREAMER_UNREF(GstMemory, gst_memory_unref);
    ROCK_GSTREAMER_UNREF(GstVideoFrame, gst_video_frame_unmap);
    ROCK_GSTREAMER_UNREF(GstPad, gst_object_unref);

    template <typename T> struct GstUnrefGuard {
        T* object;
        typedef void (*Unref)(T*);
        Unref unref = nullptr;

        explicit GstUnrefGuard(T* object, Unref unref = GstUnref<T>::unref)
            : object(object)
            , unref(unref)
        {
        }
        ~GstUnrefGuard()
        {
            if (object) {
                unref(object);
            }
        }
        T* get()
        {
            return object;
        }
        T* release()
        {
            T* ret = object;
            object = nullptr;
            return ret;
        }
    };

    struct GstMemoryUnmapGuard {
        GstMemory* memory;
        GstMapInfo& map_info;
        GstMemoryUnmapGuard(GstMemory* memory, GstMapInfo& map_info)
            : memory(memory)
            , map_info(map_info)
        {
        }
        ~GstMemoryUnmapGuard()
        {
            gst_memory_unmap(memory, &map_info);
        }
    };

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
        GstCaps* caps =
            gst_caps_new_simple("image/jpeg", NULL);
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
