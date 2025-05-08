#pragma once

#include <vector>

#include "CommandBufferModifier.hpp"
#include "StarBuffer.hpp"
#include "StarTexture.hpp"

class CopyDepthTextureToBuffer : private star::CommandBufferModifier
{
  public:
    CopyDepthTextureToBuffer(std::vector<std::unique_ptr<star::StarTexture>> *offscreenRenderToDepths,
                             std::vector<std::unique_ptr<star::StarBuffer>> *buffersForDepthInfo);

  private:
    std::vector<std::unique_ptr<star::StarTexture>> *offscreenRenderToDepths = nullptr;
    std::vector<std::unique_ptr<star::StarBuffer>> *buffersForDepthInfo = nullptr;
};