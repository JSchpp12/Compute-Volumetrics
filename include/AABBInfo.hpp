#pragma once 

#include "ManagerController_RenderResource_Buffer.hpp"
#include "TransferRequest_Memory.hpp"
#include "StarBuffer.hpp"

#include <glm/glm.hpp>

#include <array>


class AABBTransfer : public star::TransferRequest::Memory<star::StarBuffer::BufferCreationArgs>{
	public:
	AABBTransfer(const std::array<glm::vec4, 2>& aabbBounds) : aabbBounds(aabbBounds){}

	star::StarBuffer::BufferCreationArgs getCreateArgs(const vk::PhysicalDeviceProperties& deviceProperties) const override{
		return star::StarBuffer::BufferCreationArgs(
			sizeof(glm::vec4),
			2,
			VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
			VMA_MEMORY_USAGE_AUTO,
			vk::BufferUsageFlagBits::eUniformBuffer,
			vk::SharingMode::eConcurrent,
			"AABBInfoBuffer"
		);
	}

	void writeData(star::StarBuffer& buffer) const override; 

	protected:
	const std::array<glm::vec4, 2> aabbBounds;
};

class AABBController : public star::ManagerController::RenderResource::Buffer {
	public:
	AABBController(const std::array<glm::vec4, 2>& aabbBounds) : aabbBounds(aabbBounds){} 

	std::unique_ptr<star::TransferRequest::Memory<star::StarBuffer::BufferCreationArgs>> createTransferRequest() override;

	private:
	const std::array<glm::vec4, 2 > aabbBounds;
};