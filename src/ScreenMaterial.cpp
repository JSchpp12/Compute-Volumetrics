#include "ScreenMaterial.hpp"

std::vector<std::pair<vk::DescriptorType, const int>> ScreenMaterial::getDescriptorRequests(
    const int &numFramesInFlight) const
{
    return std::vector<std::pair<vk::DescriptorType, const int>>{
        std::make_pair<vk::DescriptorType, const int>(vk::DescriptorType::eCombinedImageSampler, int(numFramesInFlight))};
}

void ScreenMaterial::addDescriptorSetLayoutsTo(star::StarDescriptorSetLayout::Builder &constBuilder) const
{
    constBuilder.addBinding(0, vk::DescriptorType::eCombinedImageSampler,
                            vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment);
}

void ScreenMaterial::addComputeWriteToImage(std::shared_ptr<star::StarTextures::Texture> computeTexture)
{
    m_computeOutputImages.emplace_back(computeTexture);
}

std::unique_ptr<star::StarShaderInfo> ScreenMaterial::buildShaderInfo(star::core::device::DeviceContext &context, const uint8_t &numFramesInFlight, star::StarShaderInfo::Builder builder){
    assert(m_computeOutputImages.size() > 0 && "Compute images need to be assigned before shader infos be built"); 

    for (uint8_t i = 0; i < numFramesInFlight; i++){
        assert(m_computeOutputImages.at(i) && "Image is not properly initialized"); 
        
        builder.startOnFrameIndex(i); 
        builder.startSet(); 
        builder.add(*m_computeOutputImages.at(i), vk::ImageLayout::eShaderReadOnlyOptimal);
    }

    return builder.build(); 
}