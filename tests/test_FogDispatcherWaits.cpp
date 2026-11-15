#include <gtest/gtest.h>

#include "render_system/fog/FogDispatcher.hpp"
#include "render_system/fog/struct/SyncInfo.hpp"

#include <starlight/wrappers/graphics/StarCommandBuffer.hpp>

#include <cstdint>
#include <optional>
#include <vector>

namespace
{
using render_system::fog::FogDispatcher;
using render_system::fog::WaitInfo;
using star::StarCommandBuffer;

StarCommandBuffer::Data MakeData(std::optional<uint64_t> timelineValue)
{
    StarCommandBuffer::Data data{};
    data.signaledValue = timelineValue;
    return data;
}

vk::SemaphoreSubmitInfo MakeEdgeWait(uint64_t value)
{
    return vk::SemaphoreSubmitInfo().setSemaphore(vk::Semaphore{}).setValue(value).setStageMask(
        vk::PipelineStageFlagBits2::eAllCommands);
}
} // namespace

TEST(FogDispatcherWaits, PreservesTimelineValue)
{
    WaitInfo edges{};
    const std::vector<StarCommandBuffer::Data> dataWaits{MakeData(uint64_t{7})};

    std::vector<vk::SemaphoreSubmitInfo> out;
    FogDispatcher::AppendWaitInfos(out, edges, dataWaits);

    ASSERT_EQ(out.size(), 1u);
    EXPECT_EQ(out[0].value, 7u);
}

TEST(FogDispatcherWaits, BinaryWaitUsesZero)
{
    WaitInfo edges{};
    const std::vector<StarCommandBuffer::Data> dataWaits{MakeData(std::nullopt)};

    std::vector<vk::SemaphoreSubmitInfo> out;
    FogDispatcher::AppendWaitInfos(out, edges, dataWaits);

    ASSERT_EQ(out.size(), 1u);
    EXPECT_EQ(out[0].value, 0u);
}

TEST(FogDispatcherWaits, AppendsBeyondOldFixedCapacity)
{
    WaitInfo edges{};
    for (uint32_t i{0}; i < 5; ++i)
        edges.info[i] = MakeEdgeWait(i + 1);
    edges.count = 5;

    const std::vector<StarCommandBuffer::Data> dataWaits{MakeData(uint64_t{6}), MakeData(uint64_t{7}),
                                                         MakeData(uint64_t{8})};

    std::vector<vk::SemaphoreSubmitInfo> out;
    FogDispatcher::AppendWaitInfos(out, edges, dataWaits);

    // The previous fixed-size buffer held 6 entries, so 5 edge + 3 data waits would have overflowed it.
    ASSERT_EQ(out.size(), 8u);
    for (uint32_t i{0}; i < 5; ++i)
        EXPECT_EQ(out[i].value, i + 1) << "edge wait " << i;
    EXPECT_EQ(out[5].value, 6u);
    EXPECT_EQ(out[6].value, 7u);
    EXPECT_EQ(out[7].value, 8u);
}

TEST(FogDispatcherWaits, EdgeWaitsPrecedeDataWaits)
{
    WaitInfo edges{};
    edges.info[0] = MakeEdgeWait(11);
    edges.count = 1;

    const std::vector<StarCommandBuffer::Data> dataWaits{MakeData(uint64_t{22})};

    std::vector<vk::SemaphoreSubmitInfo> out;
    FogDispatcher::AppendWaitInfos(out, edges, dataWaits);

    ASSERT_EQ(out.size(), 2u);
    EXPECT_EQ(out[0].value, 11u);
    EXPECT_EQ(out[1].value, 22u);
}
