#include "render_system/fog/FogRenderResourceFactory.hpp"

#include "ManagerRenderResource.hpp"
#include "RandomValueTexture.hpp"

#include "Allocator.hpp"
#include "render_system/fog/ConstantsFactory.hpp"
#include "render_system/fog/DataRoles.hpp"
#include "render_system/fog/ShaderFactory.hpp"

#include <starlight/core/Exceptions.hpp>
#include <starlight/core/device/DeviceContext.hpp>
#include <starlight/core/helper/command_buffer/CommandBufferHelpers.hpp>
#include <starlight/core/helper/queue/QueueHelpers.hpp>
#include <starlight/core/logging/LoggingFactory.hpp>
#include <starlight/core/renderer/FrameData.hpp>
#include <starlight/maker/texture/MakeTexture.hpp>
#include <starlight/wrappers/graphics/StarTextures/Texture.hpp>

#include <vk_mem_alloc.h>

#include <memory>
#include <sstream>
#include <utility>
#include <vector>

#include <cassert>

namespace render_system::fog
{

static std::pair<std::vector<star::StarTextures::Texture>, vk::Format> CreateTransmittanceMaps(
    star::core::device::DeviceContext &context, int width, int height, int depth, const size_t &numToCreate)
{
    std::vector<star::StarTextures::Texture> textures{numToCreate};

    const vk::Format imageFormat = vk::Format::eR8Unorm;

    auto builder =
        star::maker::texture::MakeTexture(context.getDevice())
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
            .addViewInfo(vk::ImageViewCreateInfo()
                             .setViewType(vk::ImageViewType::e3D)
                             .setFormat(imageFormat)
                             .setSubresourceRange(vk::ImageSubresourceRange()
                                                      .setAspectMask(vk::ImageAspectFlagBits::eColor)
                                                      .setBaseArrayLayer(0)
                                                      .setLayerCount(vk::RemainingArrayLayers)
                                                      .setBaseMipLevel(0)
                                                      .setLevelCount(1)));

    const auto makeResult = builder.make(textures.data(), numToCreate);
    if (!makeResult)
    {
        STAR_THROW("Failed to create transmittance maps: " + makeResult.error().detail);
    }

    std::vector<vk::ImageMemoryBarrier2> barriers{numToCreate};
    for (size_t i = 0; i < numToCreate; i++)
    {
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

static void LogTransmittanceResourceDetails(const policies::TransmittancePlan &plan, const size_t framesInFlight,
                                            const vk::Format format)
{
    std::ostringstream oss;
    oss << "Transmittance resources:" << std::endl;
    oss << "\tmap resolution: " << plan.transmittanceMapResolution[0] << " x " << plan.transmittanceMapResolution[1]
        << " x " << plan.transmittanceMapResolution[2] << std::endl;
    oss << "\tnoise resolution: " << plan.noiseResolution[0] << " x " << plan.noiseResolution[1] << std::endl;
    oss << "\tnoise breadth (max ray samples): " << plan.maxSamples << std::endl;
    oss << "\tmap format: " << vk::to_string(format) << std::endl;

    star::core::logging::debug(oss.str());
}

FogRenderResourceFactory::TransmittanceResources FogRenderResourceFactory::createTransmittanceResources(
    star::core::device::DeviceContext &context, star::core::renderer::FrameData &frameData) const
{
    const auto &transmittance = resourcePolicies.transmittance;
    const size_t framesInFlight = static_cast<size_t>(context.frameTracker().getSetup().getNumFramesInFlight());

    // storage view for the precompute pass, sampled view for the color pass
    auto [transmittanceMaps, format] = CreateTransmittanceMaps(
        context, transmittance.transmittanceMapResolution[0], transmittance.transmittanceMapResolution[1],
        transmittance.transmittanceMapResolution[2], framesInFlight);

    LogTransmittanceResourceDetails(transmittance, framesInFlight, format);

    std::vector<star::Handle> handles{framesInFlight};
    for (size_t i = 0; i < framesInFlight; i++)
    {
        handles[i] = context.getGraphicsManagers().imageManager.submit(
            star::core::device::manager::ImageRequest{std::move(transmittanceMaps[i])});
    }

    std::vector<const star::StarTextures::Texture *> textures{framesInFlight};
    for (size_t i = 0; i < framesInFlight; i++)
    {
        textures[i] = &context.getGraphicsManagers().imageManager.get(handles[i])->texture;
    }

    const star::Handle mapRole = star::core::renderer::roleHandle(data_roles::LightTransmittanceMap);
    frameData.add(star::core::renderer::FrameData::BorrowedTexture{.textures = textures,
                                                                   .layout = vk::ImageLayout::eGeneral,
                                                                   .format = format},
                  mapRole);

    const star::Handle mapSampledRole = star::core::renderer::roleHandle(data_roles::LightTransmittanceMapSampled);
    frameData.add(star::core::renderer::FrameData::BorrowedTexture{.textures = textures,
                                                                   .layout = vk::ImageLayout::eShaderReadOnlyOptimal,
                                                                   .format = format},
                  mapSampledRole);

    // one random value per column per sample, so the precompute can jitter within each transmittance texel
    const uint32_t computeQueueFamilyIndex =
        star::core::helper::GetEngineDefaultQueue(context.getEventBus(), context.getGraphicsManagers().queueManager,
                                                  star::Queue_Type::Tcompute)
            ->getParentQueueFamilyIndex();
    const star::Handle noiseRole = star::core::renderer::roleHandle(data_roles::LightTransmittanceNoise);
    frameData.add(
        star::core::renderer::FrameData::FixedTextureHandle{
            .handle = star::ManagerRenderResource::addRequest(
                context.getDeviceID(),
                std::make_unique<RandomValueTexture<4>>(transmittance.noiseResolution[0],
                                                        transmittance.noiseResolution[1], computeQueueFamilyIndex,
                                                        context.getDevice().getPhysicalDevice().getProperties())),
            .layout = vk::ImageLayout::eGeneral,
            .format = RandomValueTexture<4>::SelectFormat()},
        noiseRole);

    return TransmittanceResources{mapRole, mapSampledRole, noiseRole};
}

std::vector<PipelineVariant> FogRenderResourceFactory::transmittancePipelineVariants(
    star::core::CommandBus &commandBus) const
{
    const star::Handle shader =
        render_system::fog::CreateVolumeComputeShader("volume_precomputeLightTransmittance.comp", commandBus);

    const auto &variants = resourcePolicies.transmittance.variants;
    std::vector<PipelineVariant> pipelineVariants;
    pipelineVariants.reserve(variants.size());

    star::core::logging::debug("Preparing ", variants.size(),
                               " transmittance pipeline variants from shader "
                               "'volume_precomputeLightTransmittance.comp'");

    // Log every variant's position and constants: headless runs bind the first variant by default, so the index is
    // what makes the pipeline actually used verifiable from the log files after a run.
    for (std::size_t i = 0; i < variants.size(); i++)
    {
        const TransmittanceVariant &variant = variants[i];
        const TransmittanceConstants constants{.numRaySamples = variant.samples,
                                               .randTextureBreath = resourcePolicies.transmittance.maxSamples};

        star::core::logging::debug("Transmittance pipeline variant index ", i, " of ", variants.size(), " (name: '",
                                   variant.name, "'", i == 0 ? ", default" : "", ")");
        LogDetails(constants);

        pipelineVariants.push_back(PipelineVariant{
            .name = variant.name, .shader = shader, .constants = MakePrecomputeTransmittanceConstants(constants)});
    }

    return pipelineVariants;
}
} // namespace render_system::fog
