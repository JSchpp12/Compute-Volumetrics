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
        star::StarTextures::Texture::Builder(context.getDevice())
            .setCreateInfo(star::Allocator::AllocationBuilder()
                               .setFlags(VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT)
                               .setUsage(VMA_MEMORY_USAGE_AUTO)
                               .build(),
                           vk::ImageCreateInfo()
                               .setExtent(vk::Extent3D().setHeight(height).setWidth(width).setDepth(depth))
                               .setSharingMode(vk::SharingMode::eExclusive)
                               .setUsage(vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled |
                                         vk::ImageUsageFlagBits::eTransferDst)
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
                          .setNewLayout(vk::ImageLayout::eGeneral)
                          .setSrcQueueFamilyIndex(vk::QueueFamilyIgnored)
                          .setDstQueueFamilyIndex(vk::QueueFamilyIgnored)
                          .setImage(textures[i].getVulkanImage())
                          .setSrcAccessMask(vk::AccessFlagBits2::eNone)
                          .setSrcStageMask(vk::PipelineStageFlagBits2::eNone)
                          .setDstAccessMask(vk::AccessFlagBits2::eShaderWrite)
                          .setDstStageMask(vk::PipelineStageFlagBits2::eComputeShader)
                          .setSubresourceRange(vk::ImageSubresourceRange()
                                                   .setAspectMask(vk::ImageAspectFlagBits::eColor)
                                                   .setBaseMipLevel(0)
                                                   .setLevelCount(vk::RemainingMipLevels)
                                                   .setBaseArrayLayer(0)
                                                   .setLayerCount(vk::RemainingArrayLayers));
    }

    star::core::helper::command_buffer::SingleTimeCommands(
        context, star::Queue_Type::Tcompute,
        [&](vk::CommandBuffer cmd) { cmd.pipelineBarrier2(vk::DependencyInfo().setImageMemoryBarriers(barriers)); });

    return std::make_pair(textures, imageFormat);
}

/// Create sampled wrapper textures that share the same underlying vk::Image as the transmittance maps but add their own
/// ImageView + Sampler.
static std::vector<std::shared_ptr<star::StarTextures::Texture>> CreateTransmittanceMapSampledWrappers(
    star::core::device::DeviceContext &context,
    const std::vector<const star::StarTextures::Texture *> &transmittanceTextures, const vk::Format format) noexcept
{
    const size_t num = transmittanceTextures.size();
    std::vector<std::shared_ptr<star::StarTextures::Texture>> wrappers{num};

    for (size_t i = 0; i < num; i++)
    {
        const auto &src = *transmittanceTextures[i];
        const vk::Extent3D extent = src.getBaseExtent();
        const vk::DeviceSize size =
            star::StarTextures::Texture::CalculateSize(format, extent, /*arrayLayers=*/1, vk::ImageType::e3D,
                                                       /*mipLevels=*/1);

        wrappers[i] = star::StarTextures::Texture::Builder(context.getDevice(), src.getVulkanImage())
                          .setBaseFormat(format)
                          .addViewInfo(vk::ImageViewCreateInfo()
                                           .setViewType(vk::ImageViewType::e3D)
                                           .setFormat(format)
                                           .setSubresourceRange(vk::ImageSubresourceRange()
                                                                    .setAspectMask(vk::ImageAspectFlagBits::eColor)
                                                                    .setBaseArrayLayer(0)
                                                                    .setLayerCount(vk::RemainingArrayLayers)
                                                                    .setBaseMipLevel(0)
                                                                    .setLevelCount(1)))
                          .setSamplerInfo(vk::SamplerCreateInfo()
                                              .setMagFilter(vk::Filter::eLinear)
                                              .setMinFilter(vk::Filter::eLinear)
                                              .setAddressModeU(vk::SamplerAddressMode::eClampToEdge)
                                              .setAddressModeV(vk::SamplerAddressMode::eClampToEdge)
                                              .setAddressModeW(vk::SamplerAddressMode::eClampToEdge)
                                              .setBorderColor(vk::BorderColor::eIntOpaqueBlack)
                                              .setUnnormalizedCoordinates(VK_FALSE)
                                              .setCompareEnable(VK_FALSE)
                                              .setCompareOp(vk::CompareOp::eAlways)
                                              .setMipmapMode(vk::SamplerMipmapMode::eLinear)
                                              .setMipLodBias(0.0f)
                                              .setMinLod(0.0f)
                                              .setMaxLod(0.0f))
                          .setSizeInfo(size, extent)
                          .buildShared();
    }

    return wrappers;
}

ShadowDispatchResourceProvider::ShadowDispatchResourceProvider(policies::ShadowResourceResolutionPolicy resPolicy)
    : m_resPolicy(std::move(resPolicy))
{
}

ShadowDispatchResourceProvider::AdditionalResourcesInfo ShadowDispatchResourceProvider::addResourcesTo(
    star::core::device::DeviceContext &context, star::core::renderer::FrameData &frameData) const noexcept
{
    std::vector<std::pair<star::Handle, ShadowDispatchResourceProvider::AdditionalResourcesInfo::Info>> addResources{1};

    addResources[0] = addTransmittanceMaps(context, frameData);

    return ShadowDispatchResourceProvider::AdditionalResourcesInfo(addResources);
}

std::pair<star::Handle, ShadowDispatchResourceProvider::AdditionalResourcesInfo::Info> ShadowDispatchResourceProvider::
    addTransmittanceMaps(star::core::device::DeviceContext &context, star::core::renderer::FrameData &fd) const noexcept
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

    // borrow textures for different view
    std::vector<const star::StarTextures::Texture *> textures{fi};
    for (size_t i = 0; i < fi; i++)
    {
        textures[i] = &context.getGraphicsManagers().imageManager.get(handles[i])->texture;
    }

    const star::Handle transmittanceMapRole = star::core::renderer::roleHandle(data_roles::LightTransmittanceMap);
    fd.add(star::core::renderer::FrameData::BorrowedTexture{.textures = std::move(textures),
                                                            .layout = vk::ImageLayout::eGeneral,
                                                            .format = format},
           transmittanceMapRole);

    // Create sampled wrappers (same vk::Image, with sampler) so the transmittance map can also be bound as a
    // combined-image-sampler (sampler3D in volume_color.comp)
    std::vector<const star::StarTextures::Texture *> transmittanceTexturePtrs{fi};
    for (size_t i = 0; i < fi; i++)
    {
        transmittanceTexturePtrs[i] = &context.getGraphicsManagers().imageManager.get(handles[i])->texture;
    }
    auto sampledWrappers = CreateTransmittanceMapSampledWrappers(context, transmittanceTexturePtrs, format);

    const star::Handle sampledUse = star::core::renderer::roleHandle(data_roles::LightTransmittanceMapSampled);
    fd.add(star::core::renderer::FrameData::OwnedTexture{.textures = std::move(sampledWrappers),
                                                         .layout = vk::ImageLayout::eShaderReadOnlyOptimal,
                                                         .format = format},
           sampledUse);

    return std::make_pair(transmittanceMapRole,
                          ShadowDispatchResourceProvider::AdditionalResourcesInfo::Info{.resolution = res});
}
} // namespace render_system::fog
