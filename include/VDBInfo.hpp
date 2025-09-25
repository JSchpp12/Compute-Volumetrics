#pragma once

#include "ManagerController_RenderResource_Buffer.hpp"
#include "TransferRequest_Buffer.hpp"

#include "VolumeData.hpp"

#include <string>

class VDBRequest : public star::TransferRequest::Buffer
{
  public:
    VDBRequest(uint8_t computeQueueIndex, std::string vdbPath, openvdb::GridClass ensureThisType)
        : m_computeQueueIndex(std::move(computeQueueIndex)), m_volumeData(std::move(vdbPath), std::move(ensureThisType))
    {
    }

    virtual ~VDBRequest() = default;

    std::unique_ptr<star::StarBuffers::Buffer> createStagingBuffer(vk::Device &device,
                                                                   VmaAllocator &allocator) const override;

    std::unique_ptr<star::StarBuffers::Buffer> createFinal(
        vk::Device &device, VmaAllocator &allocator,
        const std::vector<uint32_t> &transferQueueFamilyIndex) const override;

    void writeDataToStageBuffer(star::StarBuffers::Buffer &buffer) const override;

  private:
    uint8_t m_computeQueueIndex;
    VolumeData m_volumeData; 
};

class VDBInfoController : public star::ManagerController::RenderResource::Buffer
{
  public:
    VDBInfoController(std::string vdbPath, openvdb::GridClass ensureThisType)
        : m_vdbPath(std::move(vdbPath)), m_ensureThisType(std::move(ensureThisType))
    {
    }

    std::unique_ptr<star::TransferRequest::Buffer> createTransferRequest(
        star::core::device::StarDevice &device) override;

  private:
    // nanovdb::GridType type;
    std::string m_vdbPath;
    openvdb::GridClass m_ensureThisType;
};