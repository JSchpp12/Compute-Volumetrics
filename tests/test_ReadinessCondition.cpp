#include <gtest/gtest.h>

#include <starlight/core/transfer/ReadinessCondition.hpp>
#include <starlight/internals/CPURecordingWaitContainer.hpp>

#include <chrono>
#include <vector>

namespace
{
star::Handle MakeHandle(uint32_t id = 1)
{
    return star::Handle{.type = 1, .id = id};
}
} // namespace

TEST(ReadinessCondition, EmptyConditionListIsReady)
{
    EXPECT_EQ(star::EvaluateReadiness({}, std::chrono::steady_clock::now()), star::ReadinessState::Ready);
    EXPECT_TRUE(star::WaitForReadiness({}, std::chrono::milliseconds(1)));
}

TEST(ReadinessCondition, MissingManagersFail)
{
    const auto handle = MakeHandle();

    EXPECT_EQ(star::ImageReadiness{nullptr}.check(handle), star::ReadinessState::Failed);
    EXPECT_EQ(star::BufferReadiness{nullptr}.check(handle), star::ReadinessState::Failed);
    EXPECT_EQ(star::ShaderReadiness{nullptr}.check(handle), star::ReadinessState::Failed);
    const auto pipelineState = star::PipelineReadiness{nullptr, nullptr}.check(handle);
    EXPECT_EQ(pipelineState, star::ReadinessState::Failed);
    EXPECT_EQ(star::RenderResourceBufferReadiness{}.check(handle), star::ReadinessState::Failed);
}

TEST(ReadinessCondition, ConditionAndWaitReportFailure)
{
    const auto handle = MakeHandle();
    const std::vector<star::ReadinessCondition> conditions{
        star::ReadinessCondition{.resource = handle, .readiness = star::PipelineReadiness{}}};

    EXPECT_EQ(conditions.front().check(), star::ReadinessState::Failed);
    EXPECT_FALSE(star::WaitForReadiness(conditions, std::chrono::milliseconds(1)));
}

TEST(CPURecordingWaitContainer, DeduplicatesByResourceAndReturnsFailures)
{
    const auto handle = MakeHandle(7);
    const std::vector<star::ReadinessCondition> waits{
        star::ReadinessCondition{.resource = handle, .readiness = star::ImageReadiness{}}};

    star::CPURecordingWaitContainer container;
    container.add(waits);
    container.add(waits);

    const auto result = container.runAll();
    ASSERT_EQ(result.failed.size(), 1u);
    EXPECT_TRUE(result.failed.front() == handle);
    EXPECT_TRUE(result.pending.empty());
    EXPECT_TRUE(container.empty());
    EXPECT_FALSE(container.hasUnrun());
}
