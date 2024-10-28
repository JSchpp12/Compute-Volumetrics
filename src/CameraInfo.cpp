#include "CameraInfo.hpp"

void CameraInfo::writeBufferData(star::StarBuffer& buffer)
{
	buffer.map(); 

	CameraData cameraData = {
		.displayMatrix = glm::inverse(camera.getDisplayMatrix()),
		.inverseProjMatrix = glm::inverse(camera.getProjectionMatrix()),
		.resolution = glm::vec2(camera.getResolution()),
		.aspectRatio = camera.getResolution().x / camera.getResolution().y,
		.scale = tan(glm::radians(camera.getFieldOfView()))
	};

	buffer.writeToBuffer(&cameraData, sizeof(CameraData));

	buffer.unmap(); 
}
