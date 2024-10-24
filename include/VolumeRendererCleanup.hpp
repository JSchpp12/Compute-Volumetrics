#pragma once

#include "CommandBufferModifier.hpp"
#include "Texture.hpp"

class VolumeRendererCleanup : private star::CommandBufferModifier{
public:
 	VolumeRendererCleanup(std::vector<std::unique_ptr<star::Texture>>* computeOutputTextures, std::vector<std::unique_ptr<star::Texture>>* offscreenRenderTextures) : computeOutputTextures(computeOutputTextures), offscreenRenderTextures(offscreenRenderTextures) {};
	~VolumeRendererCleanup() = default; 

private:
	std::vector<std::unique_ptr<star::Texture>>* computeOutputTextures = nullptr;
	std::vector<std::unique_ptr<star::Texture>>* offscreenRenderTextures = nullptr;

	// Inherited via CommandBufferModifier
	void recordCommandBuffer(vk::CommandBuffer& commandBuffer, const int& frameInFlightIndex) override;

	star::Command_Buffer_Order getCommandBufferOrder() override;

	star::Command_Buffer_Type getCommandBufferType() override;

	vk::PipelineStageFlags getWaitStages() override;

	bool getWillBeSubmittedEachFrame() override;

	bool getWillBeRecordedOnce() override;

};