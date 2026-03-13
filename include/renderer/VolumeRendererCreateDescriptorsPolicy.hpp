#pragma once

#include "renderer/volume/ContainerRenderResourceData.hpp"

#include "core/device/StarDevice.hpp"
#include "core/device/managers/GraphicsContainer.hpp"
#include "managers/ManagerRenderResource.hpp"
#include "renderer/volume/ContainerRenderResourceData.hpp"

#include <star_common/Handle.hpp>

class VolumeRendererCreateDescriptorsPolicy
{
  public:
    VolumeRendererCreateDescriptorsPolicy() = default;

    VolumeRendererCreateDescriptorsPolicy(
        star::Handle *deviceID, renderer::volume::ContainerRenderResourceData data,
        std::unique_ptr<star::StarShaderInfo> *SDFShaderInfo, std::unique_ptr<star::StarShaderInfo> *volumeShaderInfo,
        star::Handle *marchedHomogenousPipeline, star::Handle *nanoVDBPipeline_hitBoundingBox,
        star::Handle *nanoVDBPipeline_surface, star::Handle *marchedPipeline, star::Handle *linearPipeline,
        star::Handle *expPipeline, std::unique_ptr<vk::PipelineLayout> *computePipelineLayout,
        star::core::device::StarDevice *device, star::core::device::manager::GraphicsContainer *graphicsManagers,
        star::ManagerRenderResource *resourceManager, uint8_t numFramesInFlight)
        : m_deviceID(deviceID), m_data(std::move(data)), m_SDFShaderInfo(SDFShaderInfo),
          m_volumeShaderInfo(volumeShaderInfo), m_marchedHomogenousPipeline(marchedHomogenousPipeline),
          m_nanoVDBPipeline_hitBoundingBox(nanoVDBPipeline_hitBoundingBox),
          m_nanoVDBPipeline_surface(nanoVDBPipeline_surface), m_marchedPipeline(marchedPipeline),
          m_linearPipeline(linearPipeline), m_expPipeline(expPipeline), m_computePipelineLayout(computePipelineLayout),
          m_device(device), m_graphicsManagers(graphicsManagers), m_resourceManager(resourceManager),
          m_numFramesInFlight(numFramesInFlight) {};

    void create();

  private:
    star::Handle *m_deviceID{nullptr};
    renderer::volume::ContainerRenderResourceData m_data;
    std::unique_ptr<star::StarShaderInfo> *m_SDFShaderInfo;
    std::unique_ptr<star::StarShaderInfo> *m_volumeShaderInfo;
    star::Handle *m_marchedHomogenousPipeline{nullptr};
    star::Handle *m_nanoVDBPipeline_hitBoundingBox{nullptr};
    star::Handle *m_nanoVDBPipeline_surface{nullptr};
    star::Handle *m_marchedPipeline{nullptr};
    star::Handle *m_linearPipeline{nullptr};
    star::Handle *m_expPipeline{nullptr};
    std::unique_ptr<vk::PipelineLayout> *m_computePipelineLayout{nullptr};
    star::core::device::StarDevice *m_device{nullptr};
    star::core::device::manager::GraphicsContainer *m_graphicsManagers{nullptr};
    star::ManagerRenderResource *m_resourceManager{nullptr};
    uint8_t m_numFramesInFlight;

    std::unique_ptr<star::StarShaderInfo> buildShaderInfo(bool useSDF);

    void createDescriptors();
};