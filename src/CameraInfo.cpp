#include "CameraInfo.hpp"

void CameraInfo::writeBufferData(star::StarBuffer& buffer)
{
	buffer.map(); 

	CameraData cameraData = {
		.resolution = glm::vec2(camera.getResolution()),
		.aspectRatio = camera.getResolution().x / camera.getResolution().y
	};

	buffer.writeToBuffer(&cameraData, sizeof(CameraData));

	buffer.unmap(); 
}
