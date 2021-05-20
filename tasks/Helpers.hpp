#ifndef ROCK_GSTREAMER_HELPERS_HPP
#define ROCK_GSTREAMER_HELPERS_HPP

#include <gst/gstobject.h>
#include <gst/gstsample.h>
#include <gst/gstbuffer.h>
#include <gst/video/gstvideometa.h>

namespace gstreamer {
    template<typename T>
    struct GstUnref;
    #define ROCK_GSTREAMER_UNREF(GstStruct, gst_unref) \
        template<> struct GstUnref<GstStruct> { \
            static void unref(GstStruct* obj) { gst_unref(obj); } \
        };

    ROCK_GSTREAMER_UNREF(GstElement, gst_object_unref);
    ROCK_GSTREAMER_UNREF(GstCaps, gst_caps_unref);
    ROCK_GSTREAMER_UNREF(GstSample, gst_sample_unref);
    ROCK_GSTREAMER_UNREF(GstMemory, gst_memory_unref);

    template<typename T>
    struct GstUnrefGuard {
        T* object;
        typedef void (*Unref)(T*);
        Unref unref = nullptr;

        explicit GstUnrefGuard(T* object, Unref unref = GstUnref<T>::unref)
            : object(object)
            , unref(unref) {}
        ~GstUnrefGuard() {
            if (object) {
                unref(object);
            }
        }
        T* release() {
            T* ret = object;
            object = nullptr;
            return ret;
        }
    };

    struct GstMemoryUnmapGuard {
        GstMemory* memory;
        GstMapInfo& mapInfo;
        GstMemoryUnmapGuard(GstMemory* memory, GstMapInfo& mapInfo)
            : memory(memory), mapInfo(mapInfo) {}
        ~GstMemoryUnmapGuard() { gst_memory_unmap(memory, &mapInfo); }
    };

    static GstVideoFormat frameModeToGSTFormat(base::samples::frame::frame_mode_t format) {
        switch(format) {
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
                throw std::runtime_error("unsupported frame format received");
        }
    }
}

#endif
