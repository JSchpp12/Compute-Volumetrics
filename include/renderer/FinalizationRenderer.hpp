#pragma once

#include "VolumeRenderer.hpp"

#include <starlight/core/renderer/HeadlessRenderer.hpp>

namespace renderer
{
class FinalizationRenderer : public star::core::renderer::HeadlessRenderer
{
    uint8_t m_graphicsQueueFamilyIndex{0};
    uint8_t m_computeQueueFamilyIndex{0};
    uint8_t m_transferQueueFamilyIndex{0};
    const VolumeRenderer *m_vol{nullptr};

  public:
    FinalizationRenderer() = default;
    FinalizationRenderer(star::core::device::DeviceContext &context, const uint8_t &numFramesInFlight,
                         std::vector<std::shared_ptr<star::StarObject>> objects,
                         std::shared_ptr<std::vector<star::Light>> lights, std::shared_ptr<star::StarCamera> camera)
        : star::core::renderer::HeadlessRenderer(context, numFramesInFlight, objects, lights, camera)
    {
    }
    FinalizationRenderer(star::core::device::DeviceContext &context, const uint8_t &numFramesInFlight,
                         std::vector<std::shared_ptr<star::StarObject>> objects,
                         std::shared_ptr<std::vector<star::Light>> lights, std::shared_ptr<star::StarCamera> camera,
                         vk::PipelineStageFlags waitPoint)
        : star::core::renderer::HeadlessRenderer(context, numFramesInFlight, objects, lights, camera, waitPoint)
    {
    }
    FinalizationRenderer(star::core::device::DeviceContext &context, const uint8_t &numFramesInFlight,
                         std::vector<std::shared_ptr<star::StarObject>> objects,
                         std::shared_ptr<star::ManagerController::RenderResource::Buffer> lightData,
                         std::shared_ptr<star::ManagerController::RenderResource::Buffer> lightListData,
                         std::shared_ptr<star::ManagerController::RenderResource::Buffer> cameraData)
        : star::core::renderer::HeadlessRenderer(context, numFramesInFlight, objects, lightData, lightListData,
                                                 cameraData)
    {
    }
    FinalizationRenderer(star::core::device::DeviceContext &context, const uint8_t &numFramesInFlight,
                         std::vector<std::shared_ptr<star::StarObject>> objects,
                         std::shared_ptr<star::ManagerController::RenderResource::Buffer> lightData,
                         std::shared_ptr<star::ManagerController::RenderResource::Buffer> lightListData,
                         std::shared_ptr<star::ManagerController::RenderResource::Buffer> cameraData,
                         vk::PipelineStageFlags waitPoint)
        : star::core::renderer::HeadlessRenderer(context, numFramesInFlight, objects, lightData, lightListData,
                                                 cameraData)
    {
    }

    virtual void recordPreRenderPassCommands(vk::CommandBuffer &commandBuffer, const star::common::FrameTracker &ft) override;

    virtual void recordPostRenderingCalls(vk::CommandBuffer &commandBuffer, const star::common::FrameTracker &ft) override;

    void addMemoryBarriersPre(vk::CommandBuffer cmdBuff, const star::common::FrameTracker &ft) const;

    void addMemoryBarriersPost(vk::CommandBuffer cmdBuff, const star::common::FrameTracker &ft) const;

    virtual void prepRender(star::common::IDeviceContext &c) override;
};
} // namespace renderer