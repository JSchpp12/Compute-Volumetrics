#include "CameraInfo.hpp"

void CameraInfo::writeData(star::StarBuffer& buffer) const
{
	buffer.map(); 

	auto data = CameraData{
		glm::inverse(camera.getProjectionMatrix()),
		glm::vec2(camera.getResolution()),
		camera.getResolution().x / camera.getResolution().y,
		camera.getFarClippingDistance(),
		camera.getNearClippingDistance(),
		tan(camera.getVerticalFieldOfView(true))
	};

	buffer.writeToBuffer(&data, sizeof(CameraData));

	buffer.unmap(); 
}

std::unique_ptr<star::TransferRequest::Buffer> CameraInfoController::createTransferRequest(const vk::PhysicalDevice& physicalDevice)
{
	return std::make_unique<CameraInfo>(this->camera);
}