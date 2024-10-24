#pragma once

#include "BufferModifier.hpp"
#include "StarCamera.hpp"

#include <glm/glm.hpp>

#include <memory>

class CameraInfo : public star::BufferModifier {
public:
	CameraInfo(star::StarCamera& camera) : camera(camera), BufferModifier(
		VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
		VMA_MEMORY_USAGE_AUTO,
		vk::BufferUsageFlagBits::eUniformBuffer,
		sizeof(CameraData),
		1,
		vk::SharingMode::eConcurrent, 1) {};

protected:
	star::StarCamera& camera; 

	struct CameraData {
		glm::vec2 resolution; 
		float aspectRatio; 
	};

	// Inherited via BufferModifier
	void writeBufferData(star::StarBuffer& buffer) override;

};