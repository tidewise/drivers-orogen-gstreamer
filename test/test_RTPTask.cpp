#include <gst/gststructure.h>
#include <gtest/gtest.h>

#include <gstreamer/RTPTask.hpp>

using namespace gstreamer;
using namespace std;

struct RTPTaskTest : public ::testing::Test {
    RTPTaskTest()
    {
    }

    // Create empty RTPTask
    RTPTask getTask()
    {
        return RTPTask("gstreamer::RTPTask");
    }
};

TEST_F(RTPTaskTest, it_correcly_parses_a_gst_structure)
{

    // Create nested structure (source-stats)
    GstStructure* source_stats = gst_structure_new("application/x-rtp-source-stats",
        "ssrc",
        G_TYPE_UINT,
        0,
        "internal",
        G_TYPE_BOOLEAN,
        TRUE,
        "validated",
        G_TYPE_BOOLEAN,
        TRUE,
        "received-bye",
        G_TYPE_BOOLEAN,
        FALSE,
        "is-csrc",
        G_TYPE_BOOLEAN,
        FALSE,
        "is-sender",
        G_TYPE_BOOLEAN,
        TRUE,
        "seqnum-base",
        G_TYPE_INT,
        9258,
        "clock-rate",
        G_TYPE_INT,
        90000,
        "octets-sent",
        G_TYPE_UINT64,
        8513,
        "packets-sent",
        G_TYPE_UINT64,
        7,
        "octets-received",
        G_TYPE_UINT64,
        8513,
        "packets-received",
        G_TYPE_UINT64,
        7,
        "bytes-received",
        G_TYPE_UINT64,
        8793,
        "bitrate",
        G_TYPE_UINT64,
        0,
        "packets-lost",
        G_TYPE_INT,
        0,
        "jitter",
        G_TYPE_UINT,
        0,
        "sent-pli-count",
        G_TYPE_UINT,
        0,
        "recv-pli-count",
        G_TYPE_UINT,
        0,
        "sent-fir-count",
        G_TYPE_UINT,
        0,
        "recv-fir-count",
        G_TYPE_UINT,
        0,
        "sent-nack-count",
        G_TYPE_UINT,
        0,
        "recv-nack-count",
        G_TYPE_UINT,
        0,
        "recv-packet-rate",
        G_TYPE_UINT,
        0,
        "have-sr",
        G_TYPE_BOOLEAN,
        FALSE,
        "sr-ntptime",
        G_TYPE_UINT64,
        0,
        "sr-rtptime",
        G_TYPE_UINT,
        0,
        "sr-octet-count",
        G_TYPE_UINT,
        0,
        "sr-packet-count",
        G_TYPE_UINT,
        0,
        NULL);

    RTPSourceStatistics bla;

    // Static method testing
    auto bla = RTPTask::fetchBoolean(source_stats, "have_sr");
    
    // get RTPTask
    // auto task = getTask();
    // auto always_present = task.extractRTPSourceStats(source_stats);

    // Clean-up
    // gst_structure_free(source_stats);
}