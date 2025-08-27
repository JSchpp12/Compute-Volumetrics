#pragma once

#include <glm/glm.hpp>
#include <memory>

#include "ManagerController_RenderResource_Buffer.hpp"
#include "StarCamera.hpp"
#include "TransferRequest_Buffer.hpp"

class CameraInfo : public star::TransferRequest::Buffer
{
  public:
    struct CameraData
    {
        glm::mat4 inverseProjMatrix = glm::mat4();
        glm::vec2 resolution = glm::vec2();
        float aspectRatio = 0.0f;
        float farClipDist = 0.0f;
        float nearClipDist = 0.0f;
        double scale = 0.0f;

        CameraData() = default;

        CameraData(const glm::mat4 &inverseProjMatrix, const glm::vec2 &resolution, const float &aspectRatio,
                   const float &farClipDist, const float &nearClipDist, const double &scale)
            : inverseProjMatrix(inverseProjMatrix), resolution(resolution), aspectRatio(aspectRatio),
              farClipDist(farClipDist), nearClipDist(nearClipDist), scale(scale)
        {
        }
    };

    CameraInfo(const std::shared_ptr<star::StarCamera> camera, const uint32_t &computeQueueFamilyIndex,
               const vk::DeviceSize &minUniformBufferOffsetAlignment)
        : camera(camera), computeQueueFamilyIndex(computeQueueFamilyIndex),
          minUniformBufferOffsetAlignment(minUniformBufferOffsetAlignment)
    {
    }

    std::unique_ptr<star::StarBuffers::Buffer> createStagingBuffer(vk::Device &device, VmaAllocator &allocator) const override;

    std::unique_ptr<star::StarBuffers::Buffer> createFinal(vk::Device &device, VmaAllocator &allocator,
                                                  const std::vector<uint32_t> &transferQueueFamilyIndex) const override;

    void writeDataToStageBuffer(star::StarBuffers::Buffer &buffer) const override;

  protected:
    const uint32_t computeQueueFamilyIndex;
    const vk::DeviceSize minUniformBufferOffsetAlignment;
    const std::shared_ptr<star::StarCamera> camera = nullptr;

    // void writeData(star::StarBuffer &buffer) const override;
};

class CameraInfoController : public star::ManagerController::RenderResource::Buffer
{
  public:
    CameraInfoController(const std::shared_ptr<star::StarCamera> camera) : camera(camera) {};

  protected:
    const std::shared_ptr<star::StarCamera> camera = nullptr;

    std::unique_ptr<star::TransferRequest::Buffer> createTransferRequest(star::core::device::StarDevice &device) override;
};
