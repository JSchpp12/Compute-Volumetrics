#pragma once

#include "FogControlInfo.hpp"

#include <starlight/virtual/ManagerController_RenderResource_Buffer.hpp>
#include <starlight/wrappers/graphics/StarShaderInfo.hpp>

namespace renderer::volume

{
struct ContainerRenderResourceData
{
    struct Inputs
    {
        FogInfoController *fogController;
        std::vector<star::Handle> *aabbInfoBuffers;
        const std::vector<star::Handle> *offscreenRenderToColors;
        const std::vector<star::Handle> *offscreenRenderToDepths;
        std::shared_ptr<star::ManagerController::RenderResource::Buffer> instanceManagerInfo;
        std::shared_ptr<star::ManagerController::RenderResource::Buffer> instanceNormalInfo;
        std::shared_ptr<star::ManagerController::RenderResource::Buffer> globalInfoBuffers;
        std::shared_ptr<star::ManagerController::RenderResource::Buffer> globalLightList;
        std::shared_ptr<star::ManagerController::RenderResource::Buffer> globalLightInfo;
        star::Handle *cameraShaderInfo;
        star::Handle *vdbInfoSDF;
        star::Handle *vdbInfoFog;
        star::Handle *randomValueTexture;
    };

    struct Outputs
    {
        std::vector<std::shared_ptr<star::StarTextures::Texture>> *computeWriteToImages;
        std::vector<star::StarBuffers::Buffer> *computeRayDistBuffers{nullptr};
        std::vector<star::StarBuffers::Buffer> *computeRayAtCutoffBuffer;
    };

    Inputs inputs; 
    Outputs outputs;
};
} // namespace renderer::volume