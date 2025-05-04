#pragma once

#include "ManagerController_RenderResource_Buffer.hpp"
#include "TransferRequest_Memory.hpp"
#include "StarCamera.hpp"

#include <glm/glm.hpp>

#include <memory>

namespace star{
	namespace TransferRequest{
		class CameraInfo : public Memory<StarBuffer::BufferCreationArgs>{
			public:
				struct CameraData { 
					glm::mat4 inverseProjMatrix 	= glm::mat4(); 
					glm::vec2 resolution 			= glm::vec2(); 
					float aspectRatio 				= 0.0f;
					float farClipDist				= 0.0f;
					float nearClipDist				= 0.0f;
					double scale 					= 0.0f; 
		
					CameraData() = default;
		
					CameraData(const glm::mat4& inverseProjMatrix, const glm::vec2& resolution, 
						const float& aspectRatio, const float& farClipDist, const float& nearClipDist, 
						const double& scale) 
						: inverseProjMatrix(inverseProjMatrix), resolution(resolution), 
						aspectRatio(aspectRatio), farClipDist(farClipDist), nearClipDist(nearClipDist), 
						scale(scale) {}
				};
		
				CameraInfo(const star::StarCamera& camera) : camera(camera)
				{
				}
		
				StarBuffer::BufferCreationArgs getCreateArgs(const vk::PhysicalDeviceProperties& deviceProperties) const override{
					return StarBuffer::BufferCreationArgs{
						sizeof(CameraData),
						1,
						VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
						VMA_MEMORY_USAGE_AUTO,
						vk::BufferUsageFlagBits::eUniformBuffer,
						vk::SharingMode::eConcurrent,
						"CameraInfoBuffer"
					};
				}
				
			protected:
				const StarCamera camera; 
		
				void writeData(star::StarBuffer& buffer) const override; 
		};
	}


	class CameraInfo : public star::ManagerController::RenderResource::Buffer {
		public:
			CameraInfo(const star::StarCamera& camera) : camera(camera){};
		
		protected:
			const star::StarCamera& camera; 

			std::unique_ptr<star::TransferRequest::Memory<StarBuffer::BufferCreationArgs>> createTransferRequest(const vk::PhysicalDevice& physicalDevice) override;
	};
}
