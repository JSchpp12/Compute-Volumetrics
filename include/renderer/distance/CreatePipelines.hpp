#pragma once

#include "FogControlInfo.hpp"

#include "renderer/volume/ContainerRenderResourceData.hpp"

#include <memory>
#include <cstdint>
#include <vector>
#include <vulkan/vulkan_handles.hpp>
#include <star_common/Handle.hpp>
#include <device/managers/GraphicsContainer.hpp>
#include <device/StarDevice.hpp>
#include <ManagerRenderResource.hpp>
#include <ManagerController_RenderResource_Buffer.hpp>
#include <StarBuffers/Buffer.hpp>
#include <StarShaderInfo.hpp>

namespace renderer::distance
{

struct CreatePipelines
{
    struct Inputs
    {
        std::shared_ptr<star::ManagerController::RenderResource::Buffer> infoManagerInstanceModel{nullptr};
        std::shared_ptr<star::ManagerController::RenderResource::Buffer> infoManagerInstanceNormal{nullptr};
        std::shared_ptr<star::ManagerController::RenderResource::Buffer> infoManagerGlobalCamera{nullptr};
        std::shared_ptr<star::ManagerController::RenderResource::Buffer> infoManagerSceneLightInfo{nullptr};
        std::shared_ptr<star::ManagerController::RenderResource::Buffer> infoManagerSceneLightList{nullptr};
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
