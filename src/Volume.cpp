#include "Volume.hpp"

#include "ConfigFile.hpp"
#include "ManagerRenderResource.hpp"
#include "TransferRequest_IndicesInfo.hpp"
#include "TransferRequest_VertInfo.hpp"

Volume::Volume(star::core::device::DeviceContext &context, std::string vdbFilePath, const size_t &numFramesInFlight,
               std::shared_ptr<star::StarCamera> camera, const uint32_t &screenWidth, const uint32_t &screenHeight,
               OffscreenRenderer *offscreenRenderer,
               std::shared_ptr<star::ManagerController::RenderResource::Buffer> sceneCameraInfos,
               std::shared_ptr<star::ManagerController::RenderResource::Buffer> lightInfos,
               std::shared_ptr<star::ManagerController::RenderResource::Buffer> lightList)
    : star::StarObject(std::vector<std::shared_ptr<star::StarMaterial>>{std::make_shared<ScreenMaterial>()}),
      camera(camera), screenDimensions(screenWidth, screenHeight), m_offscreenRenderer(offscreenRenderer),
      m_fogControlInfo(std::make_shared<FogInfo>(FogInfo::LinearFogInfo(0.001f, 100.0f), FogInfo::ExpFogInfo(0.5f),
                                                 FogInfo::MarchedFogInfo(0.002f, 0.3f, 0.3f, 0.2f, 0.1f, 5.0f)))
{
    initVolume(context, std::move(vdbFilePath), std::move(sceneCameraInfos), std::move(lightInfos),
               std::move(lightList));
}

void Volume::init(star::core::device::DeviceContext &context, const uint8_t &numFramesInFlight)
{
    init(context);

    volumeRenderer->init(context, numFramesInFlight);
}

void Volume::init(star::core::device::DeviceContext &context)
{
    star::StarObject::init(context);
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

void Volume::recordPreRenderPassCommands(vk::CommandBuffer &commandBuffer, const uint8_t &frameInFlightIndex,
                                         const uint64_t &frameIndex)
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

void Volume::frameUpdate(star::core::device::DeviceContext &context, const uint8_t &frameInFlightIndex,
                         const star::Handle &targetCommandBuffer)
{
    star::StarObject::frameUpdate(context, frameInFlightIndex, targetCommandBuffer);

    if (isReady)
    {
        this->volumeRenderer->frameUpdate(context, frameInFlightIndex);
    }
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

    star::StarObject::prepRender(context, swapChainExtent, numSwapChainImages, fullEngineBuilder, pipelineLayout,
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
                        std::shared_ptr<star::ManagerController::RenderResource::Buffer> sceneCameraInfos,
                        std::shared_ptr<star::ManagerController::RenderResource::Buffer> lightInfos,
                        std::shared_ptr<star::ManagerController::RenderResource::Buffer> lightList)
{
    loadModel(context, vdbFilePath);

    this->volumeRenderer = std::make_unique<VolumeRenderer>(
        m_instanceInfo.getControllerModel(), m_instanceInfo.getControllerNormal(), std::move(sceneCameraInfos),
        std::move(lightList), std::move(lightInfos), m_offscreenRenderer, vdbFilePath, m_fogControlInfo, this->camera,
        this->aabbBounds);
}

void Volume::updateGridTransforms()
{
    openvdb::FloatGrid::Ptr newGrid = this->grid->copy();
    newGrid->setTransform(this->grid->transformPtr());
    openvdb::Mat4R transform = getTransform(getInstance().getDisplayMatrix());

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
    float denom = std::pow(1 + gValue * gValue - 2.0f * gValue * cosTheta, 1.5f);
    return 1.0f / (4.0f * glm::pi<float>()) * (1.0f - gValue * gValue) / denom;
}

std::vector<std::unique_ptr<star::StarMesh>> Volume::loadMeshes(star::core::device::DeviceContext &context)
{
    std::vector<std::unique_ptr<star::StarMesh>> meshes;

    auto verts = std::vector<star::Vertex>{star::Vertex{glm::vec3{-1.0f, -1.0f, 0.0f}, // position
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
                                                        glm::vec2{0.0f, 1.0f}}};

    std::vector<uint32_t> inds = std::vector<uint32_t>{0, 3, 2, 0, 2, 1};

    auto newMeshes = std::vector<std::unique_ptr<star::StarMesh>>();

    const auto vertSemaphore =
        context.getSemaphoreManager().submit(star::core::device::manager::SemaphoreRequest(false));
    star::Handle vertBuffer = star::ManagerRenderResource::addRequest(
        context.getDeviceID(), context.getSemaphoreManager().get(vertSemaphore)->semaphore,
        std::make_unique<star::TransferRequest::VertInfo>(
            context.getDevice().getDefaultQueue(star::Queue_Type::Tgraphics).getParentQueueFamilyIndex(), verts));

    const auto indSemaphore =
        context.getSemaphoreManager().submit(star::core::device::manager::SemaphoreRequest(false));

    star::Handle indBuffer = star::ManagerRenderResource::addRequest(
        context.getDeviceID(), context.getSemaphoreManager().get(indSemaphore)->semaphore,
        std::make_unique<star::TransferRequest::IndicesInfo>(
            context.getDevice().getDefaultQueue(star::Queue_Type::Tgraphics).getParentQueueFamilyIndex(), inds));

    newMeshes.emplace_back(std::make_unique<star::StarMesh>(vertBuffer, indBuffer, verts, inds, m_meshMaterials.at(0),
                                                            this->aabbBounds[0], this->aabbBounds[1], false));

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