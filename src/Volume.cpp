#include "Volume.hpp"

#include "ManagerController_RenderResource_IndicesInfo.hpp"
#include "ManagerController_RenderResource_VertInfo.hpp"
#include "ManagerRenderResource.hpp"

Volume::Volume(star::core::device::DeviceContext &context, std::string vdbFilePath, const size_t &numFramesInFlight,
               std::shared_ptr<star::StarCamera> camera, const uint32_t &screenWidth, const uint32_t &screenHeight,
               std::vector<std::unique_ptr<star::StarTextures::Texture>> *offscreenRenderToColorImages,
               std::vector<std::unique_ptr<star::StarTextures::Texture>> *offscreenRenderToDepthImages,
               std::vector<star::Handle> sceneCameraInfos, std::vector<star::Handle> lightInfos,
               std::vector<star::Handle> lightList)
    : star::StarObject(std::vector<std::shared_ptr<star::StarMaterial>>{std::make_shared<ScreenMaterial>()}),
      camera(camera), screenDimensions(screenWidth, screenHeight),
      offscreenRenderToColorImages(offscreenRenderToColorImages),
      offscreenRenderToDepthImages(offscreenRenderToDepthImages),
      m_fogControlInfo(std::make_shared<FogInfo>(FogInfo::LinearFogInfo(0.001f, 100.0f), FogInfo::ExpFogInfo(0.5f),
                                                 FogInfo::MarchedFogInfo(0.002f, 0.3f, 0.3f, 0.2f, 0.1f, 5.0f)))
{
    initVolume(context, std::move(vdbFilePath), sceneCameraInfos, lightInfos, lightList);
}

std::unordered_map<star::Shader_Stage, star::StarShader> Volume::getShaders()
{
    std::unordered_map<star::Shader_Stage, star::StarShader> shaders;

    std::string mediaPath = star::ConfigFile::getSetting(star::Config_Settings::mediadirectory);

    const std::string vertPath = mediaPath + "shaders/screenWithTexture/screenWithTexture.vert";
    const std::string fragPath = mediaPath + "shaders/screenWithTexture/screenWithTexture.frag";

    shaders.insert(std::pair<star::Shader_Stage, star::StarShader>(
        star::Shader_Stage::vertex, star::StarShader(vertPath, star::Shader_Stage::vertex)));
    shaders.insert(std::pair<star::Shader_Stage, star::StarShader>(
        star::Shader_Stage::fragment, star::StarShader(fragPath, star::Shader_Stage::fragment)));

    return shaders;
}

// star::Handle Volume::buildPipeline(star::core::device::DeviceContext &device,
//                                                           vk::Extent2D swapChainExtent,
//                                                           vk::PipelineLayout pipelineLayout,
//                                                           star::RenderingTargetInfo renderingInfo)
// {
//     star::StarGraphicsPipeline::PipelineConfigSettings settings;
//     star::StarGraphicsPipeline::defaultPipelineConfigInfo(settings, swapChainExtent, pipelineLayout, renderingInfo);

//     // enable alpha blending
//     settings.colorBlendAttachment.blendEnable = VK_TRUE;
//     settings.colorBlendAttachment.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
//     settings.colorBlendAttachment.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
//     settings.colorBlendAttachment.colorBlendOp = vk::BlendOp::eAdd;
//     settings.colorBlendAttachment.srcAlphaBlendFactor = vk::BlendFactor::eOne;
//     settings.colorBlendAttachment.dstAlphaBlendFactor = vk::BlendFactor::eZero;
//     settings.colorBlendAttachment.alphaBlendOp = vk::BlendOp::eAdd;

//     settings.colorBlendInfo.logicOpEnable = VK_FALSE;
//     settings.colorBlendInfo.logicOp = vk::LogicOp::eCopy;

//     auto graphicsShaders = this->getShaders();

//     auto newPipeline =
//         std::make_unique<star::StarGraphicsPipeline>(graphicsShaders.at(star::Shader_Stage::vertex),
//                                                      graphicsShaders.at(star::Shader_Stage::fragment));
//     newPipeline->init(device, pipelineLayout);

//     return newPipeline;
// }

void Volume::loadModel(star::core::device::DeviceContext &context, const std::string &filePath)
{

    if (!star::file_helpers::FileExists(filePath))
    {
        throw std::runtime_error("Provided file does not exist" + filePath);
    }

    openvdb::initialize();

    openvdb::GridBase::Ptr baseGrid{};

    openvdb::io::File file(filePath);

    file.open();

    std::cout << "OpenVDB File Info:" << std::endl;
    for (openvdb::io::File::NameIterator nameIter = file.beginName(); nameIter != file.endName(); ++nameIter)
    {
        std::cout << "Available Grids in file:" << std::endl;
        std::cout << nameIter.gridName() << std::endl;

        if (!baseGrid)
        {
            baseGrid = file.readGrid(nameIter.gridName());
        }
        else
        {
            std::cout << "Skipping extra grid: " << nameIter.gridName();
        }
    }
    file.close();

    this->grid = openvdb::gridPtrCast<openvdb::FloatGrid>(baseGrid);

    openvdb::GridClass gridClass = baseGrid->getGridClass();
    switch (gridClass)
    {
    case openvdb::GridClass::GRID_LEVEL_SET: {
        std::cout << "Grid type: Level_Set" << std::endl;
        break;
    }
    case openvdb::GridClass::GRID_FOG_VOLUME: {
        std::cout << "Grid type: Fog " << std::endl;
        break;
    }
    default:
        throw std::runtime_error("Unsupported OpenVDB volume class type");
    }

    // std::cout << "Type: " << baseGrid->type() << std::endl;

    // if (this->grid->getGridClass() == openvdb::GridClass::GRID_LEVEL_SET)
    // {
    //     // need to convert to fog volume
    //     convertToFog(this->grid);
    // }

    openvdb::math::CoordBBox bbox = this->grid->evalActiveVoxelBoundingBox();
    openvdb::math::Coord &bmin = bbox.min();
    openvdb::math::Coord &bmax = bbox.max();

    this->aabbBounds[0] = glm::vec4{bmin.x(), bmin.y(), bmin.z(), 1.0};
    this->aabbBounds[1] = glm::vec4{bmax.x(), bmax.y(), bmax.z(), 1.0};
}

void Volume::convertToFog(openvdb::FloatGrid::Ptr &grid)
{
    const float outside = grid->background();
    const float width = 2.0f * outside;

    // Visit and update all of the grid's active values, which correspond to
    // voxels on the narrow band.
    for (openvdb::FloatGrid::ValueOnIter iter = grid->beginValueOn(); iter; ++iter)
    {
        float dist = iter.getValue();
        iter.setValue((outside - dist) / width);
    }

    // Visit all of the grid's inactive tile and voxel values and update the
    // values that correspond to the interior region.
    for (openvdb::FloatGrid::ValueOffIter iter = grid->beginValueOff(); iter; ++iter)
    {
        if (iter.getValue() < 0.0)
        {
            iter.setValue(1.0);
            iter.setValueOff();
        }
    }
    // Set exterior voxels to 0.
    openvdb::tools::changeBackground(grid->tree(), 0.0f);

    grid->setGridClass(openvdb::GridClass::GRID_FOG_VOLUME);
}

void Volume::recordPreRenderPassCommands(vk::CommandBuffer &commandBuffer, const int &frameInFlightIndex)
{
    commandBuffer.pipelineBarrier2(
        vk::DependencyInfo().setImageMemoryBarriers(vk::ArrayProxyNoTemporaries<const vk::ImageMemoryBarrier2>{
            vk::ImageMemoryBarrier2()
                .setImage(this->volumeRenderer->getRenderToImages().at(frameInFlightIndex)->getVulkanImage())
                .setOldLayout(vk::ImageLayout::eGeneral)
                .setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
                .setSrcStageMask(vk::PipelineStageFlagBits2::eTopOfPipe)
                .setSrcAccessMask(vk::AccessFlagBits2::eNone)
                .setDstStageMask(vk::PipelineStageFlagBits2::eFragmentShader)
                .setDstAccessMask(vk::AccessFlagBits2::eShaderRead)
                .setSrcQueueFamilyIndex(this->computeQueueFamily)
                .setDstQueueFamilyIndex(this->graphicsQueueFamily)
                .setSubresourceRange(vk::ImageSubresourceRange()
                                         .setAspectMask(vk::ImageAspectFlagBits::eColor)
                                         .setBaseMipLevel(0)
                                         .setLevelCount(1)
                                         .setBaseArrayLayer(0)
                                         .setLayerCount(1))}));
}

void Volume::recordPostRenderPassCommands(vk::CommandBuffer &commandBuffer, const int &frameInFlightIndex)
{
    commandBuffer.pipelineBarrier2(
        vk::DependencyInfo().setImageMemoryBarriers(vk::ArrayProxyNoTemporaries<const vk::ImageMemoryBarrier2>{
            vk::ImageMemoryBarrier2()
                .setImage(this->volumeRenderer->getRenderToImages().at(frameInFlightIndex)->getVulkanImage())
                .setOldLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
                .setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
                .setSrcStageMask(vk::PipelineStageFlagBits2::eFragmentShader)
                .setSrcAccessMask(vk::AccessFlagBits2::eShaderRead)
                .setDstStageMask(vk::PipelineStageFlagBits2::eBottomOfPipe)
                .setDstAccessMask(vk::AccessFlagBits2::eNone)
                .setSrcQueueFamilyIndex(this->graphicsQueueFamily)
                .setDstQueueFamilyIndex(this->computeQueueFamily)
                .setSubresourceRange(vk::ImageSubresourceRange()
                                         .setAspectMask(vk::ImageAspectFlagBits::eColor)
                                         .setBaseMipLevel(0)
                                         .setLevelCount(1)
                                         .setBaseArrayLayer(0)
                                         .setLayerCount(1))}));
}

void Volume::frameUpdate(star::core::device::DeviceContext &context)
{
    this->volumeRenderer->frameUpdate(context);

    star::StarObject::frameUpdate(context);
}

void Volume::prepRender(star::core::device::DeviceContext &context, const vk::Extent2D &swapChainExtent,
                        const uint8_t &numSwapChainImages, star::StarShaderInfo::Builder fullEngineBuilder,
                        vk::PipelineLayout pipelineLayout, star::core::renderer::RenderingTargetInfo renderingInfo)
{
    volumeRenderer->prepRender(context, swapChainExtent, numSwapChainImages);

    for (size_t i = 0; i < this->volumeRenderer->getRenderToImages().size(); i++)
    {
        static_cast<ScreenMaterial *>(m_meshMaterials[0].get())
            ->addComputeWriteToImage(this->volumeRenderer->getRenderToImages()[i]);
    }

    RecordQueueFamilyInfo(context, this->computeQueueFamily, this->graphicsQueueFamily);

    this->star::StarObject::prepRender(context, swapChainExtent, numSwapChainImages, fullEngineBuilder, pipelineLayout,
                                       renderingInfo);
}

void Volume::prepRender(star::core::device::DeviceContext &context, const vk::Extent2D &swapChainExtent,
                        const uint8_t &numSwapChainImages, star::StarShaderInfo::Builder fullEngineBuilder,
                        star::Handle sharedPipeline)
{
    volumeRenderer->prepRender(context, swapChainExtent, numSwapChainImages);

    for (size_t i = 0; i < this->volumeRenderer->getRenderToImages().size(); i++)
    {
        static_cast<ScreenMaterial *>(m_meshMaterials[0].get())
            ->addComputeWriteToImage(this->volumeRenderer->getRenderToImages()[i]);
    }

    RecordQueueFamilyInfo(context, this->computeQueueFamily, this->graphicsQueueFamily);

    this->star::StarObject::prepRender(context, swapChainExtent, numSwapChainImages, fullEngineBuilder, sharedPipeline);
}

void Volume::cleanupRender(star::core::device::DeviceContext &context)
{
    volumeRenderer->cleanupRender(context);

    star::StarObject::cleanupRender(context);
}

bool Volume::isRenderReady(star::core::device::DeviceContext &context)
{
    return star::StarObject::isRenderReady(context) && this->volumeRenderer->isRenderReady(context);
}

void Volume::initVolume(star::core::device::DeviceContext &context, std::string vdbFilePath,
                        std::vector<star::Handle> &globalInfos, std::vector<star::Handle> &lightInfos,
                        std::vector<star::Handle> &lightList)
{
    loadModel(context, vdbFilePath);

    std::vector<std::vector<star::Color>> colors;
    colors.resize(this->screenDimensions.y);
    for (int y = 0; y < this->screenDimensions.y; y++)
    {
        colors[y].resize(this->screenDimensions.x);
        for (int x = 0; x < this->screenDimensions.x; x++)
        {
            if (x == this->screenDimensions.x / 2)
                colors[y][x] = star::Color(1.0f, 1.0f, 1.0f, 1.0f);
        }
    }

    this->volumeRenderer = std::make_unique<VolumeRenderer>(
        vdbFilePath, m_fogControlInfo, this->camera, this->instanceModelInfos, offscreenRenderToColorImages,
        offscreenRenderToDepthImages, globalInfos, lightInfos, lightList, this->aabbBounds);
}

void Volume::updateGridTransforms()
{
    openvdb::FloatGrid::Ptr newGrid = this->grid->copy();
    newGrid->setTransform(this->grid->transformPtr());
    openvdb::Mat4R transform = getTransform(this->instances.front()->getDisplayMatrix());

    openvdb::tools::GridTransformer transformer(transform);

    transformer.transformGrid<openvdb::tools::BoxSampler, openvdb::FloatGrid>(*this->grid, *newGrid);

    this->grid = newGrid;
    this->grid->pruneGrid();
}

float Volume::calcExp(const float &stepSize, const float &sigma)
{
    return std::exp(-stepSize * sigma);
}

float Volume::henyeyGreensteinPhase(const glm::vec3 &viewDirection, const glm::vec3 &lightDirection,
                                    const float &gValue)
{
    float cosTheta = glm::dot(viewDirection, lightDirection);
    float denom = std::powf(1 + gValue * gValue - 2.0f * gValue * cosTheta, 1.5f);
    return 1.0f / (4.0f * glm::pi<float>()) * (1.0f - gValue * gValue) / denom;
}

std::vector<std::unique_ptr<star::StarMesh>> Volume::loadMeshes(star::core::device::DeviceContext &context)
{
    std::vector<std::unique_ptr<star::StarMesh>> meshes;

    std::unique_ptr<std::vector<star::Vertex>> verts =
        std::unique_ptr<std::vector<star::Vertex>>(new std::vector<star::Vertex>{
            star::Vertex{glm::vec3{-1.0f, -1.0f, 0.0f}, // position
                         glm::vec3{0.0f, 1.0f, 0.0f},   // normal - posy
                         glm::vec3{0.0f, 1.0f, 0.0f},   // color
                         glm::vec2{0.0f, 0.0f}},
            star::Vertex{glm::vec3{1.0f, -1.0f, 0.0f}, // position
                         glm::vec3{0.0f, 1.0f, 0.0f},  // normal - posy
                         glm::vec3{0.0f, 1.0f, 0.0f},  // color
                         glm::vec2{1.0f, 0.0f}},
            star::Vertex{glm::vec3{1.0f, 1.0f, 0.0f}, // position
                         glm::vec3{0.0f, 1.0f, 0.0f}, // normal - posy
                         glm::vec3{1.0f, 0.0f, 0.0f}, // color
                         glm::vec2{1.0f, 1.0f}},
            star::Vertex{glm::vec3{-1.0f, 1.0f, 0.0f}, // position
                         glm::vec3{0.0f, 1.0f, 0.0f},  // normal - posy
                         glm::vec3{0.0f, 1.0f, 0.0f},  // color
                         glm::vec2{0.0f, 1.0f}},
        });

    std::unique_ptr<std::vector<uint32_t>> inds =
        std::unique_ptr<std::vector<uint32_t>>(new std::vector<uint32_t>{0, 3, 2, 0, 2, 1});

    auto newMeshes = std::vector<std::unique_ptr<star::StarMesh>>();

    star::Handle vertBuffer = star::ManagerRenderResource::addRequest(
        context.getDeviceID(), std::make_unique<star::ManagerController::RenderResource::VertInfo>(*verts));
    star::Handle indBuffer = star::ManagerRenderResource::addRequest(
        context.getDeviceID(), std::make_unique<star::ManagerController::RenderResource::IndicesInfo>(*inds));

    newMeshes.emplace_back(std::unique_ptr<star::StarMesh>(new star::StarMesh(
        vertBuffer, indBuffer, *verts, *inds, m_meshMaterials.at(0), this->aabbBounds[0], this->aabbBounds[1], false)));

    return newMeshes;
}

openvdb::Mat4R Volume::getTransform(const glm::mat4 &objectDisplayMat)
{
    std::unique_ptr<float> rawData = std::unique_ptr<float>(new float[16]);

    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            rawData.get()[(i * 4) + j] = objectDisplayMat[j][i];
        }
    }

    return openvdb::Mat4R(rawData.get());
}

void Volume::RecordQueueFamilyInfo(star::core::device::DeviceContext &device, uint32_t &computeQueueFamilyIndex,
                                   uint32_t &graphicsQueueFamilyIndex)
{
    computeQueueFamilyIndex =
        device.getDevice().getDefaultQueue(star::Queue_Type::Tcompute).getParentQueueFamilyIndex();
    graphicsQueueFamilyIndex =
        device.getDevice().getDefaultQueue(star::Queue_Type::Tgraphics).getParentQueueFamilyIndex();
}