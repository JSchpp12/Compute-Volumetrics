#include "transfers/SkyBox.hpp"

namespace transfers
{
void SkyBox::prep()
{
}

std::string SkyBox::getDebugDescription() const
{
    return "skybox";
}

std::unique_ptr<star::StarBuffers::Buffer> SkyBox::createStagingBuffer(star::core::device::StarDevice &) const
{
    return nullptr;
}

std::unique_ptr<star::StarTextures::Texture> SkyBox::createFinal(star::core::device::StarDevice &,
                                                                 const std::vector<uint32_t> &) const
{
    return nullptr;
}

void SkyBox::copyFromTransferSRCToDST(star::StarBuffers::Buffer &, star::StarTextures::Texture &,
                                      vk::CommandBuffer &) const
{
}

void SkyBox::writeDataToStageBuffer(star::StarBuffers::Buffer &) const
{
}
} // namespace transfers
