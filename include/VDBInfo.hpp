#pragma once

#include "ManagerController_RenderResource_Buffer.hpp"
#include "TransferRequest_Buffer.hpp"

#include <nanovdb/HostBuffer.h>
#include <nanovdb/GridHandle.h>

#include <string>

class VDBRequest : public star::TransferRequest::Buffer
{
  public:
    VDBRequest(std::string vdbPath, uint8_t computeQueueIndex)
        : m_vdbPath(std::move(vdbPath)), m_computeQueueIndex(std::move(computeQueueIndex))
    {
    }

    virtual ~VDBRequest() = default;

    virtual void prep() override;

    std::unique_ptr<star::StarBuffers::Buffer> createStagingBuffer(vk::Device &device,
                                                                   VmaAllocator &allocator) const override;

    std::unique_ptr<star::StarBuffers::Buffer> createFinal(
        vk::Device &device, VmaAllocator &allocator,
        const std::vector<uint32_t> &transferQueueFamilyIndex) const override;

    void writeDataToStageBuffer(star::StarBuffers::Buffer &buffer) const override;

  private:
    std::string m_vdbPath;
    uint8_t m_computeQueueIndex;
    std::unique_ptr<nanovdb::GridHandle<nanovdb::HostBuffer>> m_gridData = nullptr; 
    // std::unique_ptr<nanovdb::GridHandle<nanovdb::>> m_rawGridDataHandle = nullptr;

    std::unique_ptr<nanovdb::GridHandle<nanovdb::HostBuffer>> loadNanoVolume();
};

class VDBInfoController : public star::ManagerController::RenderResource::Buffer
{
  public:
    VDBInfoController(std::string vdbPath) : m_vdbPath(std::move(vdbPath))
    {
    }

    std::unique_ptr<star::TransferRequest::Buffer> createTransferRequest(
        star::core::device::StarDevice &device) override;

  private:
    std::string m_vdbPath;
};