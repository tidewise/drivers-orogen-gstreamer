#include "Helpers.hpp"

gboolean gstreamer::busWatchCallback(GstBus* bus, GstMessage* msg, gpointer pipeline)
{
    switch (GST_MESSAGE_TYPE(msg)) {
        case GST_MESSAGE_LATENCY:
            gst_bin_recalculate_latency(GST_BIN(pipeline));
            break;
        default:
            return TRUE;
    }

    return TRUE;
}