#include "CameraInfo.hpp"

void CameraInfo::writeBufferData(star::StarBuffer& buffer)
{
	buffer.map(); 

	double camRads = glm::radians(camera.getFieldOfView());
	double scale = (camRads);
	CameraData cameraData = {
		.displayMatrix = glm::inverse(camera.getDisplayMatrix()),
		.resolution = glm::vec2(camera.getResolution()),
		.aspectRatio = camera.getResolution().x / camera.getResolution().y,
		.scale = tan(glm::radians(camera.getFieldOfView()))
	};

	//fov radians - should be -> 0.78539818525314331
	buffer.writeToBuffer(&cameraData, sizeof(CameraData));

	buffer.unmap(); 
}
