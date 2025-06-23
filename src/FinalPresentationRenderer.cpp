#include "FinalPresentationRenderer.hpp"

FinalPresentationRenderer::FinalPresentationRenderer(const star::StarWindow &window,
                                                     std::shared_ptr<star::StarScene> scene, star::StarDevice &device,
                                                     const int &numFramesInFlight)
    : star::SwapChainRenderer(window, scene, device, numFramesInFlight)
{
}

void FinalPresentationRenderer::recordCommandBuffer(vk::CommandBuffer &buffer, const int &frameIndexToBeDrawn)
{
    this->star::SwapChainRenderer::recordCommandBuffer(buffer, frameIndexToBeDrawn); 
}