#include <gtest/gtest.h>

#include <star_skybox/buffers/TransferRequest_SkyboxBuffer.hpp>
#include <star_skybox/struct/SkyboxInfo.hpp>

#include <filesystem>
#include <future>
#include <string>

namespace
{
using star::skybox::SkyboxInfo;
using star::skybox::TransferRequest::SkyboxBuffer;
} // namespace

TEST(SkyboxBufferHooks, FailureRejectsInputsPromise)
{
    std::promise<SkyboxBuffer::ConversionResult> promise;
    auto future = promise.get_future();
    SkyboxBuffer request(SkyboxInfo{}, 0u, std::filesystem::path{"dummy.png"}, std::move(promise));

    request.onTransferFailed("boom");

    auto result = future.get();
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), "boom");
}

TEST(SkyboxBufferHooks, RepeatedFailureAfterResolutionIsNoOp)
{
    std::promise<SkyboxBuffer::ConversionResult> promise;
    auto future = promise.get_future();
    SkyboxBuffer request(SkyboxInfo{}, 0u, std::filesystem::path{"dummy.png"}, std::move(promise));

    request.onTransferFailed("first");
    // The promise is already resolved; a second failure must not throw or overwrite the result.
    request.onTransferFailed("second");
    request.onTransferComplete();

    auto result = future.get();
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), "first");
}
