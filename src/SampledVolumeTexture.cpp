#include "SampledVolumeTexture.hpp"

#include "CastHelpers.hpp"

std::unique_ptr<star::StarBuffer> SampledVolumeRequest::createStagingBuffer(vk::Device &device, VmaAllocator &allocator, const uint32_t& transferQueueFamilyIndex) const{
    int width = this->sampledData->size(); 
    int height = this->sampledData->at(0).size(); 
    int depth = 0; 

    return std::make_unique<star::StarBuffer>(
        allocator, 
        (width * height * 1 * 4),
        1,
        VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
        VMA_MEMORY_USAGE_AUTO,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::SharingMode::eConcurrent,
        "SampledVolumeTexture_SRC"
    );
}

std::unique_ptr<star::StarTexture> SampledVolumeRequest::createFinal(vk::Device &device, VmaAllocator &allocator, const uint32_t& transferQueueFamilyIndex) const
{
    uint32_t indices[] = {
        this->computeQueueFamilyIndex,
        transferQueueFamilyIndex
    };

    return star::StarTexture::Builder(device, allocator)
        .setCreateInfo(
            star::Allocator::AllocationBuilder()
                .setFlags(VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT)
                .setUsage(VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO)
                .setPriority(1.0f)
                .build(),
            vk::ImageCreateInfo()
                .setExtent(
                    vk::Extent3D()
                        .setWidth(this->sampledData->size())
                        .setHeight(this->sampledData->at(0).size())
                        .setDepth(1)
                )
                .setPQueueFamilyIndices(&indices[0])
                .setQueueFamilyIndexCount(2)
                .setUsage(vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled)
                .setImageType(vk::ImageType::e2D)
                .setArrayLayers(1)
                .setMipLevels(1)
                .setTiling(vk::ImageTiling::eOptimal)
                .setInitialLayout(vk::ImageLayout::eUndefined)
                .setSamples(vk::SampleCountFlagBits::e1)
                .setSharingMode(vk::SharingMode::eConcurrent),
            "SampledVolumeTexture"
        )
        .setBaseFormat(vk::Format::eR32Sfloat)
        .addViewInfo(
            vk::ImageViewCreateInfo()
                .setViewType(vk::ImageViewType::e2D)
                .setFormat(vk::Format::eR32Sfloat)
                .setSubresourceRange(
                    vk::ImageSubresourceRange()
                        .setAspectMask(vk::ImageAspectFlagBits::eColor)
                        .setBaseArrayLayer(0)
                        .setLayerCount(1)
                        .setBaseMipLevel(0)
                        .setLevelCount(1)
                )
        )
        .build();
}

void SampledVolumeRequest::writeDataToStageBuffer(star::StarBuffer &buffer) const
{
    std::vector<float> flattenedData;
    int floatCounter = 0;
    for (int i = 0; i < this->sampledData->size(); i++)
    {
        for (int j = 0; j < this->sampledData->at(i).size(); j++)
        {
            for (int k = 0; k < this->sampledData->at(i).at(j).size(); k++)
            {
                flattenedData.push_back(this->sampledData->at(i).at(j).at(k));
                floatCounter++;
            }
        }
    }

    buffer.map();

    buffer.writeToBuffer(flattenedData.data(), sizeof(float) * floatCounter);

    buffer.unmap();
}

void SampledVolumeRequest::copyFromTransferSRCToDST(star::StarBuffer &srcBuffer, star::StarTexture &dstTexture,
                                                    vk::CommandBuffer &commandBuffer) const
{
    star::StarTexture::TransitionImageLayout(dstTexture, commandBuffer, dstTexture.getBaseFormat(), vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);

    uint32_t width = star::CastHelpers::int_to_unsigned_int(this->sampledData->size());
    uint32_t height = star::CastHelpers::int_to_unsigned_int(this->sampledData->at(0).size());

    vk::BufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = vk::Offset3D{};
    region.imageExtent = vk::Extent3D{width, height, 1};

    commandBuffer.copyBufferToImage(srcBuffer.getVulkanBuffer(), dstTexture.getVulkanImage(),
                                    vk::ImageLayout::eTransferDstOptimal, region);

    star::StarTexture::TransitionImageLayout(dstTexture, commandBuffer, dstTexture.getBaseFormat(), vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);
}

std::unique_ptr<star::TransferRequest::Texture> SampledVolumeController::createTransferRequest(
    star::StarDevice &device)
{
    return std::make_unique<SampledVolumeRequest>(device.getQueueFamily(star::Queue_Type::Tcompute).getQueueFamilyIndex(), std::move(this->sampledData));
}
