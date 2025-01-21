#pragma once

#include "CommandBufferModifier.hpp"
#include "FileTexture.hpp"

class VolumeRendererCleanup : private star::CommandBufferModifier{
public:
 	VolumeRendererCleanup(std::vector<std::unique_ptr<star::FileTexture>>* computeOutputTextures,
		std::vector<std::unique_ptr<star::FileTexture>>* offscreenRenderTextures, std::vector<std::unique_ptr<star::FileTexture>>* offscreenRenderDepths)
		: computeOutputTextures(computeOutputTextures), offscreenRenderTextures(offscreenRenderTextures),
		offscreenRenderDepths(offscreenRenderDepths)
	
	{};
	~VolumeRendererCleanup() = default; 

private:
	std::vector<std::unique_ptr<star::FileTexture>>* computeOutputTextures = nullptr;
	std::vector<std::unique_ptr<star::FileTexture>>* offscreenRenderTextures = nullptr;
	std::vector<std::unique_ptr<star::FileTexture>>* offscreenRenderDepths = nullptr;

	// Inherited via CommandBufferModifier
	void recordCommandBuffer(vk::CommandBuffer& commandBuffer, const int& frameInFlightIndex) override;

	star::Command_Buffer_Order getCommandBufferOrder() override;

	star::Command_Buffer_Type getCommandBufferType() override;

	vk::PipelineStageFlags getWaitStages() override;

	bool getWillBeSubmittedEachFrame() override;

	bool getWillBeRecordedOnce() override;

};