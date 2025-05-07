#pragma once 

#include "StarTexture.hpp"
#include "ManagerController_RenderResource_Texture.hpp"
#include "TransferRequest_Texture.hpp"

#include <vector>
#include <memory>

class SampledVolumeRequest: public star::TransferRequest::Texture{
	public:
	SampledVolumeRequest(std::unique_ptr<std::vector<std::vector<std::vector<float>>>> sampledData) : sampledData(std::move(sampledData)){}

	star::StarTexture::RawTextureCreateSettings getCreateArgs() const override; 

	void writeData(star::StarBuffer& buffer) const override; 

	void copyFromTransferSRCToDST(star::StarBuffer& srcBuffer, star::StarTexture& dstTexture, vk::CommandBuffer& commandBuffer) const override; 

	private:
	std::unique_ptr<std::vector<std::vector<std::vector<float>>>> sampledData; 
};

class SampledVolumeController : public star::ManagerController::RenderResource::Texture{
	public:
	SampledVolumeController(std::unique_ptr<std::vector<std::vector<std::vector<float>>>> sampledData) : sampledData(std::move(sampledData)){}

	std::unique_ptr<star::TransferRequest::Texture> createTransferRequest(const vk::PhysicalDevice& physicalDevice) override;
	protected:
	std::unique_ptr<std::vector<std::vector<std::vector<float>>>> sampledData;
};