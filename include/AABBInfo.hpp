#pragma once 

#include "BufferModifier.hpp"

#include <glm/glm.hpp>

#include <array>

class AABBInfo : public star::BufferModifier {
public:
	AABBInfo(const std::array<glm::vec4, 2>& aabbBounds) : aabbBounds(aabbBounds),
		BufferModifier(
			VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
			VMA_MEMORY_USAGE_AUTO,
			vk::BufferUsageFlagBits::eUniformBuffer,
			sizeof(glm::vec4) * 2,
			1,
			vk::SharingMode::eConcurrent, 1) {}; 

private:
	const std::array<glm::vec4, 2 >& aabbBounds;

	void writeBufferData(star::StarBuffer& buffer) override; 
};