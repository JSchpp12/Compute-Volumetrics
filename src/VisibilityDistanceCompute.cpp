#include "VisibilityDistanceCompute.hpp"

#include "renderer/distance/CreatePipelines.hpp"

#include <starlight/common/ConfigFile.hpp>
#include <starlight/core/waiter/one_shot/CreateDescriptorsOnEventPolicy.hpp>
#include <starlight/event/EnginePhaseComplete.hpp>

void VisibilityDistanceCompute::cleanupRender(star::core::device::DeviceContext &context)
{
    m_dynamicShaderInfo->cleanupRender(context.getDevice()); 

    if (m_marchedPipeline.vkLayout != VK_NULL_HANDLE)
    {
        context.getDevice().getVulkanDevice().destroyPipelineLayout(m_marchedPipeline.vkLayout); 
        m_marchedPipeline.vkLayout = VK_NULL_HANDLE; 
    }
}

void VisibilityDistanceCompute::prepRender(star::core::device::DeviceContext &context,
                                           renderer::volume::ContainerRenderResourceData data,
                                           std::unique_ptr<star::StarShaderInfo> *sharedComputePipelinelayout)
{
    // submit waiter
    createBuildPipelineWaiter(context, std::move(data), sharedComputePipelinelayout);
}

void VisibilityDistanceCompute::frameUpdate(star::core::device::DeviceContext &context)
{
    updateRenderingContext(context);
}

bool VisibilityDistanceCompute::isReady(const star::core::device::DeviceContext &context)
{
    if (m_isReady)
    {
        return true; 
    }

    if (context.getPipelineManager().get(m_marchedPipeline.handle)->isReady())
    {
        m_isReady = true; 
    }

    return m_isReady; 
}

void VisibilityDistanceCompute::updateRenderingContext(const star::core::device::DeviceContext &context)
{
    const auto *record = context.getPipelineManager().get(m_marchedPipeline.handle);
    m_marchedPipeline.vkPipeline = record->request.pipeline.getVulkanPipeline();
}

void VisibilityDistanceCompute::recordCommandBuffer(vk::CommandBuffer commandBuffer,
                                                    const star::common::FrameTracker &frameTracker,
                                                    const glm::uvec2 &workgroupSize, Fog::Type type)
{
    auto sets = m_dynamicShaderInfo->getDescriptors(frameTracker.getCurrent().getFrameInFlightIndex());

    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, m_marchedPipeline.vkLayout, 2, sets.size(),
                                     sets.data(), 0, VK_NULL_HANDLE);

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, m_marchedPipeline.vkPipeline);
    commandBuffer.dispatch(workgroupSize.x, workgroupSize.y, 1);
}

void VisibilityDistanceCompute::createBuildPipelineWaiter(
    star::core::device::DeviceContext &context, renderer::volume::ContainerRenderResourceData data,
    std::unique_ptr<star::StarShaderInfo> *sharedComputePipelineLayout)
{
    star::core::waiter::one_shot::CreateDescriptorsOnEventPolicy<renderer::distance::CreatePipelines>::Builder(
        context.getEventBus())
        .setEventType(star::common::HandleTypeRegistry::instance().getTypeGuaranteedExist(
            star::event::EnginePhaseComplete::GetUniqueTypeName()))
        .setPolicy(renderer::distance::CreatePipelines{
            .inputs = {.infoManagerInstanceModel = data.inputs.instanceManagerInfo,
                       .infoManagerInstanceNormal = data.inputs.instanceNormalInfo,
                       .infoManagerGlobalCamera = data.inputs.globalInfoBuffers,
                       .infoManagerSceneLightInfo = data.inputs.globalLightInfo,
                       .infoManagerSceneLightList = data.inputs.globalLightInfo,
                       .randomValueTexture = data.inputs.randomValueTexture,
                       .fogController = data.inputs.fogController,
                       .computeRayDistBuffers = data.outputs.computeRayDistBuffers,
                       .computeRayAtCutoffBuffers = data.outputs.computeRayAtCutoffBuffer,
                       .aabbInfoBuffers = data.inputs.aabbInfoBuffers,
                       .vdbInfo = data.inputs.vdbInfoFog,
                       .staticComputeShaderInfo = sharedComputePipelineLayout},
            .outputs = {.marchedPipeline = &m_marchedPipeline.handle,
                        .dynamicShaderInfo = &m_dynamicShaderInfo,
                        .pipelineLayout = &m_marchedPipeline.vkLayout},
            .context =
                {
                    .device = &context.getDevice(),
                    .resourceManger = &context.getManagerRenderResource(),
                    .deviceID = &context.getDeviceID(),
                    .graphicsManagers = &context.getGraphicsManagers(),
                },
            .numFramesInFlight = context.getFrameTracker().getSetup().getNumFramesInFlight()})
        .buildShared();
}
