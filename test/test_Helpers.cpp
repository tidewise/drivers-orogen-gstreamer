#include "../tasks/Helpers.hpp"
#include <gtest/gtest.h>

using namespace gstreamer;
using namespace base;
using namespace std;

struct HelpersTest : public ::testing::Test {
    HelpersTest()
    {
    }

    inline static bool unreferred{false};
    static void unref(GstElement* e)
    {
        unreferred = true;
    }
};

TEST_F(HelpersTest, it_implements_move_constructor)
{
    GstElement obj;
    GstUnrefGuard<GstElement> first_guard(&obj, unref);
    GstUnrefGuard<GstElement> second_guard(std::move(first_guard));
    ASSERT_EQ(nullptr, first_guard.get());
    ASSERT_EQ(&obj, second_guard.get());
}

TEST_F(HelpersTest, it_implements_move_assignment)
{
    GstElement obj;
    GstUnrefGuard<GstElement> second_guard(nullptr);
    unreferred = false;
    {
        GstUnrefGuard<GstElement> first_guard(&obj, unref);
        second_guard = std::move(first_guard);
        ASSERT_EQ(nullptr, first_guard.get());
    }
    ASSERT_EQ(&obj, second_guard.get());
    ASSERT_EQ(false, unreferred);
}