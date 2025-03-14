#include "CameraInfo.hpp"

void star::TransferRequest::CameraInfo::writeData(star::StarBuffer& buffer) const
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

std::unique_ptr<star::TransferRequest::Memory<star::StarBuffer::BufferCreationArgs>> star::CameraInfo::createTransferRequest()
{
	return std::make_unique<TransferRequest::CameraInfo>(this->camera); 
}