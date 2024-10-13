#pragma once 

#include "SceneRenderer.hpp"

class OffscreenRenderer : public star::SceneRenderer {
public:
	OffscreenRenderer(std::vector<std::unique_ptr<star::Light>>& lightList,
		std::vector<std::reference_wrapper<star::StarObject>> objectList, 
		star::StarCamera& camera); 

protected:
	std::vector<std::unique_ptr<star::Texture>> createRenderToImages(star::StarDevice& device, const int& numFramesInFlight) override;

private:
	// Inherited via SceneRenderer
	star::Command_Buffer_Order_Index getCommandBufferOrderIndex() override; 

	star::Command_Buffer_Order getCommandBufferOrder() override;

	vk::PipelineStageFlags getWaitStages() override;

	bool getWillBeSubmittedEachFrame() override;

	bool getWillBeRecordedOnce() override;

	// Inherited via SceneRenderer
	vk::Format getCurrentRenderToImageFormat() override;
};