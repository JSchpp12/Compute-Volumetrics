#pragma once

#include "CommandBufferModifier.hpp"
#include "RenderResourceModifier.hpp"
#include "StarTexture.hpp"
#include "StarComputePipeline.hpp"
#include "DescriptorModifier.hpp"
#include "FileTexture.hpp"
#include "GlobalInfo.hpp"
#include "AABBInfo.hpp"
#include "LightInfo.hpp"
#include "StarShaderInfo.hpp"
#include "StarCamera.hpp"
#include "CameraInfo.hpp"
#include "StarObjectInstance.hpp"
#include "InstanceModelInfo.hpp"
#include "InstanceNormalInfo.hpp"
#include "SampledVolumeTexture.hpp"

#include <glm/glm.hpp>
#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

#include <memory>
#include <vector>

class VolumeRenderer : public star::CommandBufferModifier, private star::RenderResourceModifier, private star::DescriptorModifier{
public:
	VolumeRenderer(star::StarCamera& camera, 
		const std::vector<std::unique_ptr<star::InstanceModelInfo>>* instanceModelInfo, 
		const std::vector<std::unique_ptr<star::InstanceNormalInfo>>* instanceNormalInfo,
		std::vector<std::unique_ptr<star::StarTexture>>* offscreenRenderToColors,
		std::vector<std::unique_ptr<star::StarTexture>>* offscreenRenderToDepths,
		const std::vector<std::shared_ptr<star::GlobalInfo>>& globalInfoBuffers, 
		const std::vector<std::shared_ptr<star::LightInfo>>& sceneLightInfoBuffers,
		const SampledVolumeTexture& volumeTexture, 
		const std::array<glm::vec4, 2>& aabbBounds)
		: offscreenRenderToColors(offscreenRenderToColors), offscreenRenderToDepths(offscreenRenderToDepths),
		globalInfoBuffers(globalInfoBuffers), sceneLightInfoBuffers(sceneLightInfoBuffers), 
		aabbBounds(aabbBounds), cameraShaderInfo(std::make_unique<CameraInfo>(camera)), 
		instanceModelInfo(instanceModelInfo), volumeTexture(volumeTexture), instanceNormalInfo(instanceNormalInfo) {};

	~VolumeRenderer() = default; 

	std::vector<std::unique_ptr<star::StarTexture>>* getRenderToImages() { return &this->computeWriteToImages; }

private:
	const SampledVolumeTexture& volumeTexture;
	const std::vector<std::unique_ptr<star::InstanceModelInfo>>* instanceModelInfo = nullptr;
	const std::vector<std::unique_ptr<star::InstanceNormalInfo>>* instanceNormalInfo = nullptr;
	const std::array<glm::vec4, 2>& aabbBounds; 
	std::vector<std::shared_ptr<star::LightInfo>> sceneLightInfoBuffers;
	std::unique_ptr<star::StarShaderInfo> compShaderInfo = std::unique_ptr<star::StarShaderInfo>();
	std::unique_ptr<CameraInfo> cameraShaderInfo = std::unique_ptr<CameraInfo>(); 
	std::vector<std::shared_ptr<star::GlobalInfo>> globalInfoBuffers = std::vector<std::shared_ptr<star::GlobalInfo>>();
	std::vector<std::shared_ptr<AABBInfo>> aabbInfoBuffers;
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