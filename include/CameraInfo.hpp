#pragma once

#include <glm/glm.hpp>
#include <starlight/virtual/ManagerController_RenderResource_Buffer.hpp>
#include <starlight/virtual/StarCamera.hpp>
#include <starlight/virtual/TransferRequest_Buffer.hpp>

#include <memory>

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

        CameraData(glm::mat4 inverseProjMatrix, glm::vec2 resolution, float aspectRatio, float farClipDist,
                   float nearClipDist, double scale)
            : inverseProjMatrix(std::move(inverseProjMatrix)), resolution(std::move(resolution)),
              aspectRatio(std::move(aspectRatio)), farClipDist(std::move(farClipDist)),
              nearClipDist(std::move(nearClipDist)), scale(std::move(scale))
        {
        }
    };

    CameraInfo(const std::shared_ptr<star::StarCamera> camera, const uint32_t &computeQueueFamilyIndex,
               const vk::DeviceSize &minUniformBufferOffsetAlignment)
        : camera(camera), computeQueueFamilyIndex(computeQueueFamilyIndex),
          minUniformBufferOffsetAlignment(minUniformBufferOffsetAlignment)
    {
    }

    std::unique_ptr<star::StarBuffers::Buffer> createStagingBuffer(vk::Device &device,
                                                                   VmaAllocator &allocator) const override;

    std::unique_ptr<star::StarBuffers::Buffer> createFinal(
        vk::Device &device, VmaAllocator &allocator,
        const std::vector<uint32_t> &transferQueueFamilyIndex) const override;

    void writeDataToStageBuffer(star::StarBuffers::Buffer &buffer) const override;

  protected:
    const std::shared_ptr<star::StarCamera> camera = nullptr;
    const uint32_t computeQueueFamilyIndex;
    const vk::DeviceSize minUniformBufferOffsetAlignment;
};
