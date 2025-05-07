#pragma once 

#include "SceneRenderer.hpp"

class OffscreenRenderer : public star::SceneRenderer {
public:
	OffscreenRenderer(star::StarScene& scene);

	virtual void recordCommandBuffer(vk::CommandBuffer& commandBuffer, const int& frameInFlightIndex) override;

private:
	std::vector<std::shared_ptr<star::StarBuffer>> depthInfoStorageBuffers; 
	
	std::vector<std::unique_ptr<star::StarTexture>> createRenderToImages(star::StarDevice& device, const int& numFramesInFlight) override;

	std::vector<std::unique_ptr<star::StarTexture>> createRenderToDepthImages(star::StarDevice& device, const int& numFramesInFlight) override;

	std::vector<std::shared_ptr<star::StarBuffer>> createDepthBufferContainers(star::StarDevice& device); 
	// Inherited via SceneRenderer
	star::Command_Buffer_Order_Index getCommandBufferOrderIndex() override; 

	star::Command_Buffer_Order getCommandBufferOrder() override;

	vk::PipelineStageFlags getWaitStages() override;

	bool getWillBeSubmittedEachFrame() override;

	bool getWillBeRecordedOnce() override;

	vk::Format getCurrentRenderToImageFormat() override;

	static vk::ImageMemoryBarrier2 createMemoryBarrierPrepForDepthCopy(const vk::Image& depthImage);
};