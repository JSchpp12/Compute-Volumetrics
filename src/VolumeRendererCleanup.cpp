#include "VolumeRendererCleanup.hpp"

void VolumeRendererCleanup::recordCommandBuffer(vk::CommandBuffer &commandBuffer, const int &frameInFlightIndex)
{
    {
        vk::ImageMemoryBarrier barrier{};
        barrier.sType = vk::StructureType::eImageMemoryBarrier;
        barrier.oldLayout = vk::ImageLayout::eGeneral;
        barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        barrier.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
        barrier.dstQueueFamilyIndex = vk::QueueFamilyIgnored;

        barrier.image = this->computeOutputTextures->at(frameInFlightIndex)->getImage();
        barrier.srcAccessMask = vk::AccessFlagBits::eShaderWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

        barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        barrier.subresourceRange.baseMipLevel = 0; // image does not have any mipmap levels
        barrier.subresourceRange.levelCount = 1;   // image is not an array
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader,  // which pipeline stages
                                                                                  // should occurr before
                                                                                  // barrier
                                      vk::PipelineStageFlagBits::eFragmentShader, // pipeline stage in which
                                                                                  // operations will wait on
                                                                                  // the barrier
                                      {}, {}, nullptr, barrier);
    }
    {
        vk::ImageMemoryBarrier barrier{};
        barrier.sType = vk::StructureType::eImageMemoryBarrier;
        barrier.oldLayout = vk::ImageLayout::eGeneral;
        barrier.newLayout = vk::ImageLayout::eDepthAttachmentOptimal;
        barrier.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
        barrier.dstQueueFamilyIndex = vk::QueueFamilyIgnored;

        barrier.image = this->offscreenRenderDepths->at(frameInFlightIndex)->getImage();
        barrier.srcAccessMask = vk::AccessFlagBits::eNone;
        barrier.dstAccessMask = vk::AccessFlagBits::eNone;
        barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eBottomOfPipe, vk::PipelineStageFlagBits::eTopOfPipe,
                                      {}, {}, nullptr, barrier);
    }

    {
        vk::ImageMemoryBarrier barrier{};
        barrier.sType = vk::StructureType::eImageMemoryBarrier;
        barrier.oldLayout = vk::ImageLayout::eGeneral;
        barrier.newLayout = vk::ImageLayout::eColorAttachmentOptimal;
        barrier.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
        barrier.dstQueueFamilyIndex = vk::QueueFamilyIgnored;

        barrier.image = this->offscreenRenderTextures->at(frameInFlightIndex)->getImage();
        barrier.srcAccessMask = vk::AccessFlagBits::eNone;
        barrier.dstAccessMask = vk::AccessFlagBits::eNone;
        barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eBottomOfPipe, vk::PipelineStageFlagBits::eTopOfPipe,
                                      {}, {}, nullptr, barrier);
    }
}

star::Command_Buffer_Order VolumeRendererCleanup::getCommandBufferOrder()
{
    return star::Command_Buffer_Order::before_render_pass;
}

star::Queue_Type VolumeRendererCleanup::getCommandBufferType()
{
    return star::Queue_Type::Tgraphics;
}

vk::PipelineStageFlags VolumeRendererCleanup::getWaitStages()
{
    return vk::PipelineStageFlags();
}

bool VolumeRendererCleanup::getWillBeSubmittedEachFrame()
{
    return true;
}

bool VolumeRendererCleanup::getWillBeRecordedOnce()
{
    return true;
}
