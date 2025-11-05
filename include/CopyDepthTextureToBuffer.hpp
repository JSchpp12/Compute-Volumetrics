// #pragma once

// #include <vector>

// #include "CommandBufferModifier.hpp"
// #include "StarBuffer.hpp"
// #include "StarTextures/Texture.hpp"

// class CopyDepthTextureToBuffer : private star::CommandBufferModifier
// {
//   public:
//     CopyDepthTextureToBuffer(std::vector<std::unique_ptr<star::StarTextures::Texture>> *offscreenRenderToDepths,
//                              std::vector<std::unique_ptr<star::StarBuffer>> *buffersForDepthInfo);

//   private:
//     std::vector<std::unique_ptr<star::StarTextures::Texture>> *offscreenRenderToDepths = nullptr;
//     std::vector<std::unique_ptr<star::StarBuffer>> *buffersForDepthInfo = nullptr;
// };