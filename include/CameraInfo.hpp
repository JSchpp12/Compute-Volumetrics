#pragma once

#include "BufferManagerRequest.hpp"
#include "BufferMemoryTransferRequest.hpp"
#include "StarCamera.hpp"

#include <glm/glm.hpp>

#include <memory>

namespace star{
	class CameraTransfer : public BufferMemoryTransferRequest{
	public:
		struct CameraData { 
			glm::mat4 inverseProjMatrix 	= glm::mat4(); 
			glm::vec2 resolution 			= glm::vec2(); 
			float aspectRatio 				= 0.0f;
			double scale 					= 0.0f; 

			CameraData() = default;

			CameraData(const glm::mat4& inverseProjMatrix, const glm::vec2& resolution, 
				const float& aspectRatio, const double& scale) : 
				inverseProjMatrix(inverseProjMatrix), resolution(resolution), 
				aspectRatio(aspectRatio), scale(scale) {}

		};

		CameraTransfer(const star::StarCamera* camera) : camera(*camera)
		{
		}

		BufferCreationArgs getCreateArgs() const override{
			return BufferCreationArgs{
				sizeof(CameraTransfer::CameraData),
				1,
				VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
				VMA_MEMORY_USAGE_AUTO,
				vk::BufferUsageFlagBits::eUniformBuffer,
				vk::SharingMode::eConcurrent
			};
		}
		
	protected:
		const StarCamera camera; 

		void writeData(star::StarBuffer& buffer) const override; 
	};

	class CameraInfo : public star::BufferManagerRequest {
		public:
			CameraInfo(const star::StarCamera* camera) : camera(camera) {};
		
		protected:
			const star::StarCamera* camera = nullptr; 

			std::unique_ptr<BufferMemoryTransferRequest> createTransferRequest() const override;
	};
}
