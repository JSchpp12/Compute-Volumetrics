#include "CopyDepthTextureToBuffer.hpp"

CopyDepthTextureToBuffer::CopyDepthTextureToBuffer(
    std::vector<std::unique_ptr<star::StarTexture>> *offscreenRenderToDepths,
    std::vector<std::unique_ptr<star::StarBuffer>> *buffersForDepthInfo)
    : offscreenRenderToDepths(offscreenRenderToDepths), buffersForDepthInfo(buffersForDepthInfo)
{
    assert(this->offscreenRenderToDepths != nullptr && this->buffersForDepthInfo != nullptr);
    assert(this->offscreenRenderToDepths->size() == this->offscreenRenderToDepths->size() &&
           "There must be a buffer for each depth");
}