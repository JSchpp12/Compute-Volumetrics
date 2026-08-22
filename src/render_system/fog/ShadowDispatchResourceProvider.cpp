#include "render_system/fog/ShadowDispatchResourceProvider.hpp"

#include "render_system/fog/DataRoles.hpp"
#include "render_system/fog/policies/ShadowResourceResolutionPolicy.hpp"

#include <starlight/core/device/DeviceContext.hpp>
#include <starlight/core/helper/command_buffer/CommandBufferHelpers.hpp>
#include <starlight/core/renderer/FrameData.hpp>
#include <starlight/wrappers/graphics/StarTextures/Texture.hpp>

#include <vector>

namespace render_system::fog
{

static std::pair<std::vector<star::StarTextures::Texture>, vk::Format> CreateTransmittanceMaps(
    star::core::device::DeviceContext &context, int height, int width, int depth, const size_t &numToCreate) noexcept
{
    std::vector<star::StarTextures::Texture> textures{numToCreate};

    const vk::Format imageFormat = vk::Format::eR8Unorm;

    auto builder =
        star::StarTextures::Texture::Builder(context.getDevice().getVulkanDevice(),
                                             context.getDevice().getAllocator().get())
            .setCreateInfo(star::Allocator::AllocationBuilder()
                               .setFlags(VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT)
                               .setUsage(VMA_MEMORY_USAGE_AUTO)
                               .build(),
                           vk::ImageCreateInfo()
                               .setExtent(vk::Extent3D().setHeight(height).setWidth(width).setDepth(depth))
                               .setSharingMode(vk::SharingMode::eExclusive)
                               .setUsage(vk::ImageUsageFlagBits::eStorage)
                               .setImageType(vk::ImageType::e3D)
                               .setArrayLayers(1)
                               .setMipLevels(1)
                               .setTiling(vk::ImageTiling::eOptimal)
                               .setInitialLayout(vk::ImageLayout::eUndefined)
                               .setSamples(vk::SampleCountFlagBits::e1),
                           "TransmittanceMap")
            .setBaseFormat(imageFormat)
            .addViewInfo(vk::ImageViewCreateInfo()
                             .setViewType(vk::ImageViewType::e3D)
                             .setFormat(imageFormat)
                             .setSubresourceRange(vk::ImageSubresourceRange()
                                                      .setAspectMask(vk::ImageAspectFlagBits::eColor)
                                                      .setBaseArrayLayer(0)
                                                      .setLayerCount(vk::RemainingArrayLayers)
                                                      .setBaseMipLevel(0)
                                                      .setLevelCount(1)));

    std::vector<vk::ImageMemoryBarrier2> barriers{numToCreate};
    for (size_t i = 0; i < numToCreate; i++)
    {
        textures[i] = builder.build();
        barriers[i] = vk::ImageMemoryBarrier2()
                          .setOldLayout(vk::ImageLayout::eUndefined)
                          .setNewLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
                          .setSrcQueueFamilyIndex(vk::QueueFamilyIgnored)
                          .setDstQueueFamilyIndex(vk::QueueFamilyIgnored)
                          .setImage(textures[i].getVulkanImage())
                          .setSrcAccessMask(vk::AccessFlagBits2::eNone)
                          .setSrcStageMask(vk::PipelineStageFlagBits2::eNone)
                          .setDstAccessMask(vk::AccessFlagBits2::eDepthStencilAttachmentRead |
                                            vk::AccessFlagBits2::eDepthStencilAttachmentWrite)
                          .setDstStageMask(vk::PipelineStageFlagBits2::eEarlyFragmentTests)
                          .setSubresourceRange(vk::ImageSubresourceRange()
                                                   .setAspectMask(vk::ImageAspectFlagBits::eDepth)
                                                   .setBaseMipLevel(0)
                                                   .setLevelCount(vk::RemainingMipLevels)
                                                   .setBaseArrayLayer(0)
                                                   .setLayerCount(vk::RemainingArrayLayers));
    }

    star::core::helper::command_buffer::SingleTimeCommands(context, star::Queue_Type::Tcompute, [&](vk::CommandBuffer cmd) {
        cmd.pipelineBarrier2(vk::DependencyInfo().setImageMemoryBarriers(barriers));
    });

    return std::make_pair(textures, imageFormat);
}

ShadowDispatchResourceProvider::ShadowDispatchResourceProvider(policies::ShadowResourceResolutionPolicy resPolicy)
    : m_resPolicy(std::move(resPolicy))
{
}

bool ShadowDispatchResourceProvider::addResourcesTo(star::core::device::DeviceContext &context,
                                                    star::core::renderer::FrameData &frameData) const noexcept
{
    if (!addTransmittanceMaps(context, frameData))
        return false;

    return true;
}

bool ShadowDispatchResourceProvider::addTransmittanceMaps(star::core::device::DeviceContext &context,
                                                          star::core::renderer::FrameData &fd) const noexcept
{
    const size_t &fi = static_cast<const size_t &>(context.frameTracker().getSetup().getNumFramesInFlight());

    auto res = m_resPolicy.getTransmittanceResolution();
    auto [transmittanceMaps, format] = CreateTransmittanceMaps(context, res[0], res[1], res[2], fi);

    std::vector<star::Handle> handles{fi};
    for (size_t i = 0; i < fi; i++)
    {
        handles[i] = context.getGraphicsManagers().imageManager.submit(
            star::core::device::manager::ImageRequest{std::move(transmittanceMaps[i])});
    }

    const star::Handle tranmittanceUse = star::core::renderer::roleHandle(data_roles::LightTransmittanceMap);
    fd.add(star::core::renderer::FrameData::TextureHandle{.handles = handles,
                                                          .layout = vk::ImageLayout::eGeneral,
                                                          .format = format},
           tranmittanceUse);

    return true;
}
} // namespace render_system::fog