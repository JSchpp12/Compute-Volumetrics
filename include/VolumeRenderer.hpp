#pragma once

#include "CommandBufferModifier.hpp"
#include "RenderResourceModifier.hpp"
#include "StarTexture.hpp"
#include "StarComputePipeline.hpp"
#include "DescriptorModifier.hpp"
#include "Texture.hpp"

#include <glm/glm.hpp>
#include <vma/vk_mem_alloc.h>
#include <memory>
#include <vulkan/vulkan.hpp>
#include <vector>

class VolumeRenderer : public star::CommandBufferModifier, private star::RenderResourceModifier, private star::DescriptorModifier{
public:
	VolumeRenderer(std::vector<std::unique_ptr<star::Texture>>* offscreenRenderToColors) : offscreenRenderToColors(offscreenRenderToColors) {};
	~VolumeRenderer() = default; 

	std::vector<std::unique_ptr<star::Texture>>* getRenderToImages() { return &this->computeWriteToImages; }

private:
	std::vector<std::unique_ptr<star::Texture>>* offscreenRenderToColors = nullptr;

	std::unique_ptr<vk::Extent2D> displaySize = std::unique_ptr<vk::Extent2D>();
	std::vector<std::unique_ptr<vk::DescriptorSet>> computeDescriptorSets = std::vector<std::unique_ptr<vk::DescriptorSet>>();
	std::unique_ptr<star::StarDescriptorSetLayout> computeDescriptorSetLayout = std::unique_ptr<star::StarDescriptorSetLayout>();
	std::vector<std::unique_ptr<star::Texture>> computeWriteToImages = std::vector<std::unique_ptr<star::Texture>>();
	std::unique_ptr<vk::PipelineLayout> computePipelineLayout = std::unique_ptr<vk::PipelineLayout>();
	std::unique_ptr<star::StarComputePipeline> computePipeline = std::unique_ptr<star::StarComputePipeline>();

	// Inherited via CommandBufferModifier
	void recordCommandBuffer(vk::CommandBuffer& commandBuffer, const int& frameInFlightIndex) override;

	star::Command_Buffer_Order_Index getCommandBufferOrderIndex() override;
	
	star::Command_Buffer_Order getCommandBufferOrder() override;

	star::Command_Buffer_Type getCommandBufferType() override;

	vk::PipelineStageFlags getWaitStages() override;

	bool getWillBeSubmittedEachFrame() override;

	bool getWillBeRecordedOnce() override;

	// Inherited via RenderResourceModifier
	void initResources(star::StarDevice& device, const int& numFramesInFlight, 
		const vk::Extent2D& screensize) override;

	void destroyResources(star::StarDevice& device) override;

	std::vector<std::pair<vk::DescriptorType, const int>> getDescriptorRequests(const int& numFramesInFlight) override;

	void createDescriptors(star::StarDevice& device, const int& numFramesInFlight) override;

};