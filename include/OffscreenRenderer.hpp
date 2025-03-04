#pragma once 

#include "SceneRenderer.hpp"
#include "GlobalInfo.hpp"

class OffscreenRenderer : public star::SceneRenderer {
public:
	OffscreenRenderer(star::StarScene& scene);

private:
	std::vector<std::unique_ptr<star::GlobalInfo>> globalInfoBuffers = std::vector<std::unique_ptr<star::GlobalInfo>>();

	std::vector<std::unique_ptr<star::StarImage>> createRenderToImages(star::StarDevice& device, const int& numFramesInFlight) override;

	std::vector<std::unique_ptr<star::StarImage>> createRenderToDepthImages(star::StarDevice& device, const int& numFramesInFlight) override;

	// Inherited via SceneRenderer
	star::Command_Buffer_Order_Index getCommandBufferOrderIndex() override; 

	star::Command_Buffer_Order getCommandBufferOrder() override;

	vk::PipelineStageFlags getWaitStages() override;

	bool getWillBeSubmittedEachFrame() override;

	bool getWillBeRecordedOnce() override;

	vk::Format getCurrentRenderToImageFormat() override;
};