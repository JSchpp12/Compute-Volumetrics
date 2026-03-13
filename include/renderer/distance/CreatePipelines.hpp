#pragma once

#include "FogControlInfo.hpp"

#include "renderer/volume/ContainerRenderResourceData.hpp"

#include <memory>

namespace renderer::distance
{

struct CreatePipelines
{
    std::shared_ptr<star::ManagerController::RenderResource::Buffer> infoManagerInstanceModel;
    std::shared_ptr<star::ManagerController::RenderResource::Buffer> infoManagerInstanceNormal;
    std::shared_ptr<star::ManagerController::RenderResource::Buffer> infoManagerGlobalCamera;
    std::shared_ptr<star::ManagerController::RenderResource::Buffer> infoManagerSceneLightInfo;
    std::shared_ptr<star::ManagerController::RenderResource::Buffer> infoManagerSceneLightList;
    std::unique_ptr<star::StarShaderInfo> *SDFShaderInfo;
    std::unique_ptr<star::StarShaderInfo> *VolumeShaderInfo;
    FogInfoController *fogController;
    std::vector<star::Handle> *aabbInfoBuffers;
    std::vector<star::StarBuffers::Buffer> *computeRayDistBuffers;
    std::vector<star::StarBuffers::Buffer> *computeRayAtCutoffBuffers;
    star::Handle *vdbInfo;
    star::Handle *randomValueTexture;
    star::Handle *m_resultMarchedPipeline;

    void create();

  private:
};
} // namespace renderer::distance
