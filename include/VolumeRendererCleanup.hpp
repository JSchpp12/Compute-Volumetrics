#pragma once

#include "CommandBufferModifier.hpp"
#include "StarImage.hpp"

class VolumeRendererCleanup : private star::CommandBufferModifier{
public:
 	VolumeRendererCleanup(std::vector<std::unique_ptr<star::StarImage>>* computeOutputTextures,
		std::vector<std::unique_ptr<star::StarImage>>* offscreenRenderTextures, std::vector<std::unique_ptr<star::StarImage>>* offscreenRenderDepths)
		: computeOutputTextures(computeOutputTextures), offscreenRenderTextures(offscreenRenderTextures),
		offscreenRenderDepths(offscreenRenderDepths)
	
	{};
	~VolumeRendererCleanup() = default; 

private:
	std::vector<std::unique_ptr<star::StarImage>>* computeOutputTextures = nullptr;
	std::vector<std::unique_ptr<star::StarImage>>* offscreenRenderTextures = nullptr;
	std::vector<std::unique_ptr<star::StarImage>>* offscreenRenderDepths = nullptr;

	// Inherited via CommandBufferModifier
	void recordCommandBuffer(vk::CommandBuffer& commandBuffer, const int& frameInFlightIndex) override;

	star::Command_Buffer_Order getCommandBufferOrder() override;

	star::Queue_Type getCommandBufferType() override;

	vk::PipelineStageFlags getWaitStages() override;

	bool getWillBeSubmittedEachFrame() override;

	bool getWillBeRecordedOnce() override;

};