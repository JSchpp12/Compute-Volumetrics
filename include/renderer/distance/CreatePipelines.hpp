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
    std::unique_ptr<star::StarShaderInfo> *m_volumeShaderInfo{nullptr};
    FogInfoController *fogController;
    std::vector<star::Handle> *aabbInfoBuffers;
    std::vector<star::StarBuffers::Buffer> *computeRayDistBuffers;
    std::vector<star::StarBuffers::Buffer> *computeRayAtCutoffBuffers;
    star::Handle *vdbInfo;
    star::Handle *randomValueTexture;
    star::Handle *m_resultMarchedPipeline;

    star::core::device::manager::GraphicsContainer *m_graphicsManagers{nullptr};
    star::core::device::StarDevice *m_device{nullptr};
    star::ManagerRenderResource *m_resourceManager{nullptr};

    int operator()()
    {
        create();
        return 0;
    }

  private:
    void create();
};
} // namespace renderer::distance
