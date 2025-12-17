#pragma once

#include "FogControlInfo.hpp"
#include "ManagerController_RenderResource_Buffer.hpp"
#include "StarShaderInfo.hpp"
#include "core/device/StarDevice.hpp"
#include "core/device/managers/GraphicsContainer.hpp"
#include "managers/ManagerRenderResource.hpp"

#include <star_common/Handle.hpp>

class VolumeRendererCreateDescriptorsPolicy
{
  public:
    VolumeRendererCreateDescriptorsPolicy() = default;

    VolumeRendererCreateDescriptorsPolicy(
        star::Handle *deviceID, FogInfoController *fogController, std::vector<star::Handle> *aabbInfoBuffers,
        std::vector<star::Handle> *offscreenRenderToColors, std::vector<star::Handle> *offscreenRenderToDepths,
        std::vector<std::shared_ptr<star::StarTextures::Texture>> *computeWriteToImages,
        star::Handle *nanoVDBPipeline_hitBoundingBox, star::Handle *nanoVDBPipeline_surface,
        star::Handle *marchedPipeline, star::Handle *linearPipeline, star::Handle *expPipeline,
        star::Handle *cameraShaderInfo, star::Handle *vdbInfoSDF, star::Handle *vdbInfoFog,
        star::Handle *randomValueTexture, std::unique_ptr<star::StarShaderInfo> *SDFShaderInfo,
        std::unique_ptr<star::StarShaderInfo> *VolumeShaderInfo,
        std::unique_ptr<vk::PipelineLayout> *computePipelineLayout,
        std::shared_ptr<star::ManagerController::RenderResource::Buffer> infoManagerInstanceModel,
        std::shared_ptr<star::ManagerController::RenderResource::Buffer> infoManagerInstanceNormal,
        std::shared_ptr<star::ManagerController::RenderResource::Buffer> infoManagerGlobalCamera,
        std::shared_ptr<star::ManagerController::RenderResource::Buffer> infoManagerSceneLightInfo,
        std::shared_ptr<star::ManagerController::RenderResource::Buffer> infoManagerSceneLightList,
        star::core::device::StarDevice *device, star::core::device::manager::GraphicsContainer *graphicsManagers,
        star::ManagerRenderResource *resourceManager, uint8_t numFramesInFlight)
        : m_deviceID(deviceID), m_fogController(fogController), m_aabbInfoBuffers(aabbInfoBuffers),
          m_offscreenRenderToColors(offscreenRenderToColors), m_offscreenRenderToDepths(offscreenRenderToDepths),
          m_computeWriteToImages(computeWriteToImages),
          m_nanoVDBPipeline_hitBoundingBox(nanoVDBPipeline_hitBoundingBox),
          m_nanoVDBPipeline_surface(nanoVDBPipeline_surface), m_marchedPipeline(marchedPipeline),
          m_linearPipeline(linearPipeline), m_expPipeline(expPipeline), m_cameraShaderInfo(cameraShaderInfo),
          m_vdbInfoSDF(vdbInfoSDF), m_vdbInfoFog(vdbInfoFog), m_randomValueTexture(randomValueTexture),
          m_SDFShaderInfo(SDFShaderInfo), m_VolumeShaderInfo(VolumeShaderInfo),
          m_computePipelineLayout(computePipelineLayout),
          m_infoManagerInstanceModel(std::move(infoManagerInstanceModel)),
          m_infoManagerInstanceNormal(std::move(infoManagerInstanceNormal)),
          m_infoManagerGlobalCamera(std::move(infoManagerGlobalCamera)),
          m_infoManagerSceneLightInfo(std::move(infoManagerSceneLightInfo)),
          m_infoManagerSceneLightList(std::move(infoManagerSceneLightList)), m_device(device),
          m_graphicsManagers(graphicsManagers), m_resourceManager(resourceManager),
          m_numFramesInFlight(numFramesInFlight){};

    void create();

  private:
    star::Handle *m_deviceID;
    FogInfoController *m_fogController;
    std::vector<star::Handle> *m_aabbInfoBuffers;
    std::vector<star::Handle> *m_offscreenRenderToColors, *m_offscreenRenderToDepths;
    std::vector<std::shared_ptr<star::StarTextures::Texture>> *m_computeWriteToImages;
    star::Handle *m_nanoVDBPipeline_hitBoundingBox;
    star::Handle *m_nanoVDBPipeline_surface;
    star::Handle *m_marchedPipeline;
    star::Handle *m_linearPipeline;
    star::Handle *m_expPipeline;
    star::Handle *m_cameraShaderInfo;
    star::Handle *m_vdbInfoSDF;
    star::Handle *m_vdbInfoFog;
    star::Handle *m_randomValueTexture;
    std::unique_ptr<star::StarShaderInfo> *m_SDFShaderInfo;
    std::unique_ptr<star::StarShaderInfo> *m_VolumeShaderInfo;
    std::unique_ptr<vk::PipelineLayout> *m_computePipelineLayout;
    std::shared_ptr<star::ManagerController::RenderResource::Buffer> m_infoManagerInstanceModel,
        m_infoManagerInstanceNormal, m_infoManagerGlobalCamera, m_infoManagerSceneLightInfo,
        m_infoManagerSceneLightList;
    star::core::device::StarDevice *m_device;
    star::core::device::manager::GraphicsContainer *m_graphicsManagers;
    star::ManagerRenderResource *m_resourceManager;
    uint8_t m_numFramesInFlight;

    std::unique_ptr<star::StarShaderInfo> buildShaderInfo(bool useSDF);

    void createDescriptors();

    void buildPipelines();
};