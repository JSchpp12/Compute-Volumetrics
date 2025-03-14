#pragma once

#include "CommandBufferModifier.hpp"
#include "RenderResourceModifier.hpp"
#include "StarComputePipeline.hpp"
#include "DescriptorModifier.hpp"
#include "FileTexture.hpp"
#include "AABBInfo.hpp"
#include "StarShaderInfo.hpp"
#include "StarCamera.hpp"
#include "StarObjectInstance.hpp"
#include "SampledVolumeTexture.hpp"

#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

#include <memory>
#include <vector>

class VolumeRenderer : public star::CommandBufferModifier, private star::RenderResourceModifier, private star::DescriptorModifier{
public:
	VolumeRenderer(star::StarCamera& camera,
		const std::vector<star::Handle>& instanceModelInfo, 
		const std::vector<star::Handle>& instanceNormalInfo,
		std::vector<std::unique_ptr<star::StarTexture>>* offscreenRenderToColors,
		std::vector<std::unique_ptr<star::StarTexture>>* offscreenRenderToDepths,
		const std::vector<star::Handle>& globalInfoBuffers, 
		const std::vector<star::Handle>& sceneLightInfoBuffers,
		const star::Handle& volumeTexture, 
		const std::array<glm::vec4, 2>& aabbBounds);

	~VolumeRenderer() = default; 

	std::vector<std::unique_ptr<star::StarTexture>>* getRenderToImages() { return &this->computeWriteToImages; }

private:
	const star::Handle volumeTexture;
	const std::vector<star::Handle>& instanceModelInfo;
	const std::vector<star::Handle>& instanceNormalInfo;
	const std::array<glm::vec4, 2>& aabbBounds; 
	const star::StarCamera& camera;
	star::Handle cameraShaderInfo; 
	std::vector<star::Handle> sceneLightInfoBuffers = std::vector<star::Handle>();
	std::unique_ptr<star::StarShaderInfo> compShaderInfo = std::unique_ptr<star::StarShaderInfo>();
	std::vector<star::Handle> globalInfoBuffers = std::vector<star::Handle>();
	std::vector<star::Handle> aabbInfoBuffers;
	std::vector<std::unique_ptr<star::StarTexture>>* offscreenRenderToColors = nullptr;
	std::vector<std::unique_ptr<star::StarTexture>>* offscreenRenderToDepths = nullptr;

	std::unique_ptr<vk::Extent2D> displaySize = std::unique_ptr<vk::Extent2D>();
	std::vector<std::unique_ptr<star::StarTexture>> computeWriteToImages = std::vector<std::unique_ptr<star::StarTexture>>();
	std::unique_ptr<vk::PipelineLayout> computePipelineLayout = std::unique_ptr<vk::PipelineLayout>();
	std::unique_ptr<star::StarComputePipeline> computePipeline = std::unique_ptr<star::StarComputePipeline>();

	// Inherited via CommandBufferModifier
	void recordCommandBuffer(vk::CommandBuffer& commandBuffer, const int& frameInFlightIndex) override;

	star::Command_Buffer_Order_Index getCommandBufferOrderIndex() override;
	
	star::Command_Buffer_Order getCommandBufferOrder() override;

	star::Queue_Type getCommandBufferType() override;

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