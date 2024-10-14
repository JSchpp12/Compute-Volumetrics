#include "VolumeRenderer.hpp"

void VolumeRenderer::recordCommandBuffer(vk::CommandBuffer& commandBuffer, const int& frameInFlightIndex)
{
	vk::ImageMemoryBarrier prepOffscreenImages{}; 
	prepOffscreenImages.sType = vk::StructureType::eImageMemoryBarrier; 
	prepOffscreenImages.oldLayout = vk::ImageLayout::eColorAttachmentOptimal; 
	prepOffscreenImages.newLayout = vk::ImageLayout::eGeneral; 
	prepOffscreenImages.srcQueueFamilyIndex = vk::QueueFamilyIgnored; 
	prepOffscreenImages.dstQueueFamilyIndex = vk::QueueFamilyIgnored; 
	prepOffscreenImages.image = this->offscreenRenderToColors->at(frameInFlightIndex)->getImage(); 
	prepOffscreenImages.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor; 
	prepOffscreenImages.subresourceRange.baseMipLevel = 0; 
	prepOffscreenImages.subresourceRange.levelCount = 1; 
	prepOffscreenImages.subresourceRange.baseArrayLayer = 0; 
	prepOffscreenImages.subresourceRange.layerCount = 1; 
	prepOffscreenImages.srcAccessMask = vk::AccessFlagBits::eNone; 
	prepOffscreenImages.dstAccessMask = vk::AccessFlagBits::eShaderRead; 

	commandBuffer.pipelineBarrier(
		vk::PipelineStageFlagBits::eTopOfPipe,
		vk::PipelineStageFlagBits::eComputeShader,
		{},
		{},
		nullptr,
		prepOffscreenImages
	);


	//transition image layout
	vk::ImageMemoryBarrier prepWriteToImages{}; 
	prepWriteToImages.sType = vk::StructureType::eImageMemoryBarrier;
	prepWriteToImages.oldLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
	prepWriteToImages.newLayout = vk::ImageLayout::eGeneral;
	prepWriteToImages.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	prepWriteToImages.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	prepWriteToImages.image = this->computeWriteToImages.at(frameInFlightIndex)->getImage();
	prepWriteToImages.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
	prepWriteToImages.subresourceRange.baseMipLevel = 0;
	prepWriteToImages.subresourceRange.levelCount = 1;
	prepWriteToImages.subresourceRange.baseArrayLayer = 0;
	prepWriteToImages.subresourceRange.layerCount = 1;
	prepWriteToImages.srcAccessMask = vk::AccessFlagBits::eNone;
	prepWriteToImages.dstAccessMask = vk::AccessFlagBits::eShaderRead;

	commandBuffer.pipelineBarrier(
		vk::PipelineStageFlagBits::eTopOfPipe,
		vk::PipelineStageFlagBits::eComputeShader,
		{},
		{},
		nullptr,
		prepWriteToImages
	);

	//{
	//	vk::ImageMemoryBarrier cleanupWriteToImages{}; 
	//	cleanupWriteToImages.sType = vk::StructureType::eImageMemoryBarrier; 
	//	cleanupWriteToImages.oldLayout = vk::ImageLayout::eShaderReadOnlyOptimal; 
	//	cleanupWriteToImages.newLayout = vk::ImageLayout::eColorAttachmentOptimal; 
	//	cleanupWriteToImages.srcQueueFamilyIndex = vk::QueueFamilyIgnored; 
	//	cleanupWriteToImages.dstQueueFamilyIndex = vk::QueueFamilyIgnored; 
	//	cleanupWriteToImages.srcAccessMask = vk::AccessFlagBits::eShaderRead; 
	//	cleanupWriteToImages.dstAccessMask = vk::AccessFlagBits::eNone; 
	//	cleanupWriteToImages.image = this->offscreenRenderToColors->at(frameInFlightIndex)->getImage(); 
	//	cleanupWriteToImages.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor; 
	//	cleanupWriteToImages.subresourceRange.baseMipLevel = 0; 
	//	cleanupWriteToImages.subresourceRange.levelCount = 1; 
	//	cleanupWriteToImages.subresourceRange.baseArrayLayer = 0; 
	//	cleanupWriteToImages.subresourceRange.layerCount = 1; 

	//	commandBuffer.pipelineBarrier(
	//		vk::PipelineStageFlagBits::eComputeShader,
	//		vk::PipelineStageFlagBits::eBottomOfPipe,
	//		{},
	//		{},
	//		nullptr,
	//		cleanupWriteToImages
	//	);
	//}

	this->computePipeline->bind(commandBuffer);

	auto sets = std::vector{ *this->computeDescriptorSets.at(frameInFlightIndex) }; 
	commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *this->computePipelineLayout, 0, sets.size(), sets.data(), 0, VK_NULL_HANDLE);

 	commandBuffer.dispatch(80, 45, 1); 
}	

star::Command_Buffer_Order_Index VolumeRenderer::getCommandBufferOrderIndex()
{
	return star::Command_Buffer_Order_Index::second;
}

star::Command_Buffer_Order VolumeRenderer::getCommandBufferOrder()
{
	return star::Command_Buffer_Order::before_render_pass;
}

star::Command_Buffer_Type VolumeRenderer::getCommandBufferType()
{
	return star::Command_Buffer_Type::Tcompute;
}

vk::PipelineStageFlags VolumeRenderer::getWaitStages()
{
	return vk::PipelineStageFlagBits::eComputeShader; 
}

bool VolumeRenderer::getWillBeSubmittedEachFrame()
{
	return true;
}

bool VolumeRenderer::getWillBeRecordedOnce()
{
	return false;
}

void VolumeRenderer::initResources(star::StarDevice& device, const int& numFramesInFlight, const vk::Extent2D& screensize)
{
	std::string compShaderPath = star::ConfigFile::getSetting(star::Config_Settings::mediadirectory) + "shaders/volumeRenderer/volume.comp";
	auto compShader = star::StarShader(compShaderPath, star::Shader_Stage::compute);

	this->displaySize = std::make_unique<vk::Extent2D>(screensize);
	{
		auto settings = star::StarTexture::TextureCreateSettings{
			vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled, 
			vk::Format::eR8G8B8A8Unorm,
			VMA_MEMORY_USAGE_GPU_ONLY, 
			VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT, 
			false, 
			true
		};

		this->computeWriteToImages.resize(numFramesInFlight);

		for (int i = 0; i < numFramesInFlight; i++) {
			this->computeWriteToImages[i] = std::make_unique<star::Texture>(screensize.width, screensize.height, settings);
			this->computeWriteToImages[i]->prepRender(device);

			//set the layout to general for compute shader use
			auto oneTime = device.beginSingleTimeCommands();
			this->computeWriteToImages[i]->transitionLayout(oneTime, vk::ImageLayout::eShaderReadOnlyOptimal, vk::AccessFlagBits::eNone, vk::AccessFlagBits::eShaderRead, vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eComputeShader);
			device.endSingleTimeCommands(oneTime);
		}
 	}

	this->computeDescriptorSetLayout = star::StarDescriptorSetLayout::Builder(device)
		.addBinding(0, vk::DescriptorType::eStorageImage, vk::ShaderStageFlagBits::eCompute)
		.addBinding(1, vk::DescriptorType::eStorageImage, vk::ShaderStageFlagBits::eCompute)
		.build();
	
	{
		auto sets = std::vector{
			this->computeDescriptorSetLayout->getDescriptorSetLayout() 
		};

		vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = vk::StructureType::ePipelineLayoutCreateInfo;
		pipelineLayoutInfo.pSetLayouts = sets.data();
		pipelineLayoutInfo.setLayoutCount = sets.size();
		pipelineLayoutInfo.pushConstantRangeCount = 0;
		pipelineLayoutInfo.pPushConstantRanges = nullptr;

		this->computePipelineLayout = std::make_unique<vk::PipelineLayout>(device.getDevice().createPipelineLayout(pipelineLayoutInfo));
	}

	this->computePipeline = std::make_unique<star::StarComputePipeline>(device, *this->computePipelineLayout, compShader);
	this->computePipeline->init(); 
}

void VolumeRenderer::destroyResources(star::StarDevice& device)
{
	this->computeDescriptorSetLayout.reset(); 

	for (auto& computeWriteToImage : this->computeWriteToImages) {
		computeWriteToImage->cleanupRender(device); 
	}

	this->computePipeline.reset();
	device.getDevice().destroyPipelineLayout(*this->computePipelineLayout);	
}

std::vector<std::pair<vk::DescriptorType, const int>> VolumeRenderer::getDescriptorRequests(const int& numFramesInFlight)
{
	return std::vector<std::pair<vk::DescriptorType, const int>>{
		std::make_pair(vk::DescriptorType::eStorageImage, 1),
		std::make_pair(vk::DescriptorType::eCombinedImageSampler, 1)
	};
}

void VolumeRenderer::createDescriptors(star::StarDevice& device, const int& numFramesInFlight)
{
	this->computeDescriptorSets.resize(numFramesInFlight);

	for (int i = 0; i < numFramesInFlight; i++) {
		auto writeToTexInfo = vk::DescriptorImageInfo{
			VK_NULL_HANDLE,
			this->computeWriteToImages[i]->getImageView(),
			vk::ImageLayout::eGeneral
		};

		auto readFromTexInfo = vk::DescriptorImageInfo{
			this->offscreenRenderToColors->at(i)->getSampler(),
			this->offscreenRenderToColors->at(i)->getImageView(),
			vk::ImageLayout::eGeneral
		};

		this->computeDescriptorSets[i] = std::make_unique<vk::DescriptorSet>(
			star::StarDescriptorWriter(device, *this->computeDescriptorSetLayout, star::ManagerDescriptorPool::getPool())
			.writeImage(0, readFromTexInfo)
			.writeImage(1, writeToTexInfo)
			.build()
		);
	}
}
