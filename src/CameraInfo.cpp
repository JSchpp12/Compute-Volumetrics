#include "CameraInfo.hpp"

void star::CameraTransfer::writeData(star::StarBuffer& buffer) const
{

	// data(
	// 	CameraData{

	buffer.map(); 

	auto data = CameraData{
		glm::inverse(camera.getProjectionMatrix()),
		glm::vec2(camera.getResolution()),
		camera.getResolution().x / camera.getResolution().y,
		tan(glm::radians(camera.getFieldOfView()))
	};

	buffer.writeToBuffer(&data, sizeof(CameraData));

	buffer.unmap(); 
}

std::unique_ptr<star::BufferMemoryTransferRequest> star::CameraInfo::createTransferRequest() const
{
	return std::make_unique<star::CameraTransfer>(this->camera); 
}