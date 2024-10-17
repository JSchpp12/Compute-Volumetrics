#include "OffscreenRenderer.hpp"

OffscreenRenderer::OffscreenRenderer(star::StarScene& scene)
	: star::SceneRenderer(scene)
{
}

std::vector<std::unique_ptr<star::Texture>> OffscreenRenderer::createRenderToImages(star::StarDevice& device, const int& numFramesInFlight)
{
	std::vector<std::unique_ptr<star::Texture>> newRenderToImages = std::vector<std::unique_ptr<star::Texture>>();

	auto imageCreateSettings = star::StarTexture::TextureCreateSettings::createDefault(false);
	imageCreateSettings.imageFormat = this->getCurrentRenderToImageFormat();
	imageCreateSettings.imageUsage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eStorage;
	imageCreateSettings.allocationCreateFlags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT & VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT;
	imageCreateSettings.memoryUsage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
	imageCreateSettings.imageFormat = this->getCurrentRenderToImageFormat();

	for (int i = 0; i < numFramesInFlight; i++) {
		newRenderToImages.push_back(std::make_unique<star::Texture>(this->swapChainExtent->width, this->swapChainExtent->height, imageCreateSettings));
		newRenderToImages.back()->prepRender(device);

		auto oneTimeSetup = device.beginSingleTimeCommands();
		newRenderToImages.back()->transitionLayout(oneTimeSetup, vk::ImageLayout::eColorAttachmentOptimal, vk::AccessFlagBits::eNone, vk::AccessFlagBits::eColorAttachmentWrite, vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eColorAttachmentOutput);
		device.endSingleTimeCommands(oneTimeSetup);
	}

	return newRenderToImages;
}

star::Command_Buffer_Order_Index OffscreenRenderer::getCommandBufferOrderIndex()
{
	return star::Command_Buffer_Order_Index::first; 
}

star::Command_Buffer_Order OffscreenRenderer::getCommandBufferOrder()
{
	return star::Command_Buffer_Order::before_render_pass;
}

vk::PipelineStageFlags OffscreenRenderer::getWaitStages()
{
	//should be able to wait until the fragment shader where the image produced from the compute shader will be used
	return vk::PipelineStageFlagBits::eFragmentShader; 
}

bool OffscreenRenderer::getWillBeSubmittedEachFrame()
{
	return true;
}

bool OffscreenRenderer::getWillBeRecordedOnce()
{
	return false;
}

vk::Format OffscreenRenderer::getCurrentRenderToImageFormat()
{
	return vk::Format::eR8G8B8A8Snorm;
}
