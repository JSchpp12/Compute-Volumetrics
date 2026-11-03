#pragma once

#include <starlight/virtual/TransferRequest_Texture.hpp>

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace transfers
{
class SkyBox : public star::TransferRequest::Texture
{
  public:
    SkyBox() = default;
    virtual ~SkyBox() = default;

    virtual void prep() override;

    virtual std::string getDebugDescription() const override;

    virtual std::unique_ptr<star::StarBuffers::Buffer> createStagingBuffer(
        star::core::device::StarDevice &device) const override;

    virtual std::unique_ptr<star::StarTextures::Texture> createFinal(
        star::core::device::StarDevice &device, const std::vector<uint32_t> &transferQueueFamilyIndex) const override;

    virtual void copyFromTransferSRCToDST(star::StarBuffers::Buffer &srcBuffer, star::StarTextures::Texture &dst,
                                          vk::CommandBuffer &commandBuffer) const override;

    virtual void writeDataToStageBuffer(star::StarBuffers::Buffer &stagingBuffer) const override;
};
} // namespace transfers
