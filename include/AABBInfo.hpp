#pragma once 

#include "BufferManagerRequest.hpp"
#include "BufferMemoryTransferRequest.hpp"

#include <glm/glm.hpp>

#include <array>

class AABBTransfer : public star::BufferMemoryTransferRequest{
	public:
	AABBTransfer(const std::array<glm::vec4, 2>& aabbBounds) : aabbBounds(aabbBounds){}

	BufferCreationArgs getCreateArgs() const override{
		return BufferCreationArgs(
			sizeof(glm::vec4),
			2,
			VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
			VMA_MEMORY_USAGE_AUTO,
			vk::BufferUsageFlagBits::eUniformBuffer,
			vk::SharingMode::eConcurrent);
	}

	void writeData(star::StarBuffer& buffer) const override; 

	protected:
	const std::array<glm::vec4, 2> aabbBounds;
};

class AABBInfo : public star::BufferManagerRequest {
	public:
	AABBInfo(const std::array<glm::vec4, 2>& aabbBounds) : aabbBounds(aabbBounds){} 

	std::unique_ptr<star::BufferMemoryTransferRequest> createTransferRequest() const override;

	private:
	const std::array<glm::vec4, 2 > aabbBounds;
};