#pragma once

#include "SwapChainRenderer.hpp"

class FinalPresentationRenderer : public star::SwapChainRenderer{
public:
    FinalPresentationRenderer(const star::StarWindow &window, std::shared_ptr<star::StarScene> scene, star::StarDevice &device, const int &numFramesInFlight); 

    virtual void recordCommandBuffer(vk::CommandBuffer &buffer, const int &frameIndexToBeDrawn) override;
protected:

};