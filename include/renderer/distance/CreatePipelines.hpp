#pragma once

#include "FogControlInfo.hpp"

#include "renderer/volume/ContainerRenderResourceData.hpp"

#include <ManagerController_RenderResource_Buffer.hpp>
#include <ManagerRenderResource.hpp>
#include <StarBuffers/Buffer.hpp>
#include <StarShaderInfo.hpp>
#include <cstdint>
#include <device/StarDevice.hpp>
#include <device/managers/GraphicsContainer.hpp>
#include <memory>
#include <star_common/Handle.hpp>
#include <vector>
#include <vulkan/vulkan_handles.hpp>

namespace renderer::distance
{

struct CreatePipelines
{
    struct Inputs
    {
        star::ManagerController::RenderResource::Buffer *infoManagerInstanceModel{nullptr};
        star::ManagerController::RenderResource::Buffer *infoManagerInstanceNormal{nullptr};
        star::ManagerController::RenderResource::Buffer *infoManagerGlobalCamera{nullptr};
        star::ManagerController::RenderResource::Buffer *infoManagerSceneLightInfo{nullptr};
        star::ManagerController::RenderResource::Buffer *infoManagerSceneLightList{nullptr};
        star::Handle *randomValueTexture{nullptr};
        FogInfoController *fogController{nullptr};
        std::vector<star::StarBuffers::Buffer> *computeRayDistBuffers{nullptr};
        std::vector<star::StarBuffers::Buffer> *computeRayAtCutoffBuffers{nullptr};
        std::vector<star::Handle> *aabbInfoBuffers{nullptr};
        star::Handle *vdbInfo{nullptr};
        std::unique_ptr<star::StarShaderInfo> *staticComputeShaderInfo{nullptr};
    };

    struct Outputs
    {
        star::Handle *marchedPipeline{nullptr};
        std::unique_ptr<star::StarShaderInfo> *dynamicShaderInfo{nullptr};
        vk::PipelineLayout *pipelineLayout{nullptr};
    };

    struct DeviceContext
    {
        star::core::device::StarDevice *device{nullptr};
        star::ManagerRenderResource *resourceManger{nullptr};
        const star::Handle *deviceID{nullptr};
        star::core::device::manager::GraphicsContainer *graphicsManagers{nullptr};
    };

    Inputs inputs;
    Outputs outputs;
    DeviceContext context;
    uint8_t numFramesInFlight;

    int operator()()
    {
        create();
        return 0;
    }

  private:
    void create();

    std::unique_ptr<star::StarShaderInfo> buildShaderInfo() const;
};
} // namespace renderer::distance
