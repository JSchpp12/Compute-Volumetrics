#pragma once

#include "ManagerController_RenderResource_Buffer.hpp"
#include "TransferRequest_Buffer.hpp"

#include "VolumeDataBase.hpp"

#include <string>

class VDBRequest : public star::TransferRequest::Buffer
{
  public:
    VDBRequest(uint8_t computeQueueIndex, std::unique_ptr<VolumeDataBase> volumeData)
        : m_computeQueueIndex(std::move(computeQueueIndex)), m_volumeData(std::move(volumeData))
    {
      assert(m_volumeData && "Sanity check to ensure volume data is valid"); 
    }

    virtual void prep() override; 

    virtual ~VDBRequest() = default;

    std::unique_ptr<star::StarBuffers::Buffer> createStagingBuffer(vk::Device &device,
                                                                   VmaAllocator &allocator) const override;

    std::unique_ptr<star::StarBuffers::Buffer> createFinal(
        vk::Device &device, VmaAllocator &allocator,
        const std::vector<uint32_t> &transferQueueFamilyIndex) const override;

    void writeDataToStageBuffer(star::StarBuffers::Buffer &buffer) const override;

  private:
    uint8_t m_computeQueueIndex;
    std::unique_ptr<VolumeDataBase> m_volumeData = nullptr; 
};

class VDBInfoController : public star::ManagerController::RenderResource::Buffer
{
  public:
    VDBInfoController(std::unique_ptr<VolumeDataBase> volumeData)
        : m_volumeData(std::move(volumeData))
    {
    }

    std::unique_ptr<star::TransferRequest::Buffer> createTransferRequest(
        star::core::device::StarDevice &device) override;

  private:
  std::unique_ptr<VolumeDataBase> m_volumeData; 

};