#pragma once

#include <memory>
#include <vector>

#include "ManagerController_RenderResource_Texture.hpp"
#include "StarTexture.hpp"
#include "TransferRequest_Texture.hpp"

class SampledVolumeRequest : public star::TransferRequest::Texture
{
  public:
    SampledVolumeRequest(std::unique_ptr<std::vector<std::vector<std::vector<float>>>> sampledData)
        : sampledData(std::move(sampledData))
    {
    }

    virtual std::unique_ptr<star::StarBuffer> createStagingBuffer(vk::Device& device, VmaAllocator& allocator) const override; 

    virtual std::unique_ptr<star::StarTexture> createFinal(vk::Device& device, VmaAllocator& allocator) const override; 

    virtual void copyFromTransferSRCToDST(star::StarBuffer &srcBuffer, star::StarTexture &dst, vk::CommandBuffer &commandBuffer) const override;

    virtual void writeDataToStageBuffer(star::StarBuffer &stagingBuffer) const override; 

  private:
    std::unique_ptr<std::vector<std::vector<std::vector<float>>>> sampledData;
};

class SampledVolumeController : public star::ManagerController::RenderResource::Texture
{
  public:
    SampledVolumeController(std::unique_ptr<std::vector<std::vector<std::vector<float>>>> sampledData)
        : sampledData(std::move(sampledData))
    {
    }

    std::unique_ptr<star::TransferRequest::Texture> createTransferRequest(
        const vk::PhysicalDevice &physicalDevice) override;

  protected:
    std::unique_ptr<std::vector<std::vector<std::vector<float>>>> sampledData;
};