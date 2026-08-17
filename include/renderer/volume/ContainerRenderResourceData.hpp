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
        const star::ManagerController::RenderResource::Buffer *instanceManagerInfo;
        const star::ManagerController::RenderResource::Buffer *instanceNormalInfo;
        const star::ManagerController::RenderResource::Buffer *globalInfoBuffers;
        const star::ManagerController::RenderResource::Buffer *globalLightInfo;
        const star::ManagerController::RenderResource::Buffer *globalLightList;
        star::Handle *cameraShaderInfo;
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
