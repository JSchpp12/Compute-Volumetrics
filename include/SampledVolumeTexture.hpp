#pragma once 

#include "StarTexture.hpp"
#include "ManagerController_RenderResource_Texture.hpp"
#include "TransferRequest_Memory.hpp"

#include <vector>
#include <memory>

class SampledVolumeRequest: public star::TransferRequest::Memory<star::StarTexture::TextureCreateSettings>{
	public:
	SampledVolumeRequest(std::unique_ptr<std::vector<std::vector<std::vector<float>>>> sampledData) : sampledData(std::move(sampledData)){}

	star::StarTexture::TextureCreateSettings getCreateArgs(const vk::PhysicalDeviceProperties& deviceProperties) const override; 

	void writeData(star::StarBuffer& buffer) const override; 

	private:
	std::unique_ptr<std::vector<std::vector<std::vector<float>>>> sampledData; 
};

class SampledVolumeController : public star::ManagerController::RenderResource::Texture{
	public:
	SampledVolumeController(std::unique_ptr<std::vector<std::vector<std::vector<float>>>> sampledData) : sampledData(std::move(sampledData)){}

	std::unique_ptr<star::TransferRequest::Memory<star::StarTexture::TextureCreateSettings>> createTransferRequest(const vk::PhysicalDevice& physicalDevice) override;
	protected:
	std::unique_ptr<std::vector<std::vector<std::vector<float>>>> sampledData;
};