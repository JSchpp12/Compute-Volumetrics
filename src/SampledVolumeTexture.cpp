// #include "SampledVolumeTexture.hpp"

// #include <starlight/common/helper/CastHelpers.hpp>

// std::unique_ptr<star::StarBuffers::Buffer> SampledVolumeRequest::createStagingBuffer(vk::Device &device,
//                                                                             VmaAllocator &allocator) const
// {
//     uint32_t width = 0;
//     star::common::helper::SafeCast<size_t, uint32_t>(this->sampledData->size(), width);
//     uint32_t height = 0;
//     star::common::helper::SafeCast<size_t, uint32_t>(this->sampledData->at(0).size(), height);

//     const vk::DeviceSize size = width * height * 1 * 4;

//     return star::StarBuffers::Buffer::Builder(allocator)
//         .setAllocationCreateInfo(
//             star::Allocator::AllocationBuilder()
//                 .setFlags(VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT)
//                 .setUsage(VMA_MEMORY_USAGE_AUTO)
//                 .build(),
//             vk::BufferCreateInfo()
//                 .setSharingMode(vk::SharingMode::eExclusive)
//                 .setSize(size)
//                 .setUsage(vk::BufferUsageFlagBits::eTransferSrc),
//             "SampledVolume_SRC")
//         .setInstanceCount(1)
//         .setInstanceSize(size)
//         .build();
// }

// std::unique_ptr<star::StarTextures::Texture> SampledVolumeRequest::createFinal(
//     vk::Device &device, VmaAllocator &allocator, const std::vector<uint32_t> &transferQueueFamilyIndex) const
// {
//     uint32_t width = 0;
//     star::common::helper::SafeCast<size_t, uint32_t>(this->sampledData->size(), width);
//     uint32_t height = 0;
//     star::common::helper::SafeCast<size_t, uint32_t>(this->sampledData->at(0).size(), height);

//     std::vector<uint32_t> indices = {this->computeQueueFamilyIndex};
//     for (const auto &index : transferQueueFamilyIndex)
//         indices.push_back(index);

//     return star::StarTextures::Texture::Builder(device, allocator)
//         .setCreateInfo(star::Allocator::AllocationBuilder()
//                            .setFlags(VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT)
//                            .setUsage(VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO)
//                            .setPriority(1.0f)
//                            .build(),
//                        vk::ImageCreateInfo()
//                            .setExtent(vk::Extent3D().setWidth(width).setHeight(height).setDepth(1))
//                            .setPQueueFamilyIndices(indices.data())
//                            .setQueueFamilyIndexCount(indices.size())
//                            .setUsage(vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferDst |
//                                      vk::ImageUsageFlagBits::eSampled)
//                            .setImageType(vk::ImageType::e2D)
//                            .setArrayLayers(1)
//                            .setMipLevels(1)
//                            .setTiling(vk::ImageTiling::eOptimal)
//                            .setInitialLayout(vk::ImageLayout::eUndefined)
//                            .setSamples(vk::SampleCountFlagBits::e1)
//                            .setSharingMode(vk::SharingMode::eConcurrent),
//                        "SampledVolumeTexture")
//         .setBaseFormat(vk::Format::eR32Sfloat)
//         .addViewInfo(vk::ImageViewCreateInfo()
//                          .setViewType(vk::ImageViewType::e2D)
//                          .setFormat(vk::Format::eR32Sfloat)
//                          .setSubresourceRange(vk::ImageSubresourceRange()
//                                                   .setAspectMask(vk::ImageAspectFlagBits::eColor)
//                                                   .setBaseArrayLayer(0)
//                                                   .setLayerCount(1)
//                                                   .setBaseMipLevel(0)
//                                                   .setLevelCount(1)))
//         .build();
// }

// void SampledVolumeRequest::writeDataToStageBuffer(star::StarBuffers::Buffer &buffer) const
// {
//     std::vector<float> flattenedData;
//     int floatCounter = 0;
//     for (size_t i = 0; i < this->sampledData->size(); i++)
//     {
//         for (size_t j = 0; j < this->sampledData->at(i).size(); j++)
//         {
//             for (size_t k = 0; k < this->sampledData->at(i).at(j).size(); k++)
//             {
//                 flattenedData.push_back(this->sampledData->at(i).at(j).at(k));
//                 floatCounter++;
//             }
//         }
//     }

//     void *mapped = nullptr;
//     buffer.map(&mapped);

//     buffer.writeToBuffer(flattenedData.data(), mapped, sizeof(float) * floatCounter);

//     buffer.unmap();
// }

// void SampledVolumeRequest::copyFromTransferSRCToDST(star::StarBuffers::Buffer &srcBuffer, star::StarTextures::Texture &dstTexture,
//                                                     vk::CommandBuffer &commandBuffer) const
// {
//     star::StarTextures::Texture::TransitionImageLayout(dstTexture, commandBuffer, dstTexture.getBaseFormat(),
//                                              vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);

//     uint32_t width = 0, height = 0;

//     if (!star::common::helper::SafeCast<size_t, uint32_t>(this->sampledData->size(), width) ||
//         !star::common::helper::SafeCast<size_t, uint32_t>(this->sampledData->at(0).size(), height))
//     {
//     }

//     vk::BufferImageCopy region{};
//     region.bufferOffset = 0;
//     region.bufferRowLength = 0;
//     region.bufferImageHeight = 0;

//     region.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
//     region.imageSubresource.mipLevel = 0;
//     region.imageSubresource.baseArrayLayer = 0;
//     region.imageSubresource.layerCount = 1;
//     region.imageOffset = vk::Offset3D{};
//     region.imageExtent = vk::Extent3D{width, height, 1};

//     commandBuffer.copyBufferToImage(srcBuffer.getVulkanBuffer(), dstTexture.getVulkanImage(),
//                                     vk::ImageLayout::eTransferDstOptimal, region);

//     star::StarTextures::Texture::TransitionImageLayout(dstTexture, commandBuffer, dstTexture.getBaseFormat(),
//                                              vk::ImageLayout::eTransferDstOptimal,
//                                              vk::ImageLayout::eShaderReadOnlyOptimal);
// }

// std::unique_ptr<star::TransferRequest::Texture> SampledVolumeController::createTransferRequest(star::core::device::StarDevice &device)
// {
//     return std::make_unique<SampledVolumeRequest>(
//         device.getDefaultQueue(star::Queue_Type::Tcompute).getParentQueueFamilyIndex(), std::move(this->sampledData));
// }
