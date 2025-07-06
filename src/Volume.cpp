#include "Volume.hpp"

#include "ManagerController_RenderResource_IndicesInfo.hpp"
#include "ManagerController_RenderResource_VertInfo.hpp"
#include "ManagerRenderResource.hpp"
#include "SampledVolumeTexture.hpp"

Volume::Volume(std::shared_ptr<star::StarCamera> camera, const uint32_t &screenWidth, const uint32_t &screenHeight,
               std::vector<std::unique_ptr<star::Light>> &lightList,
               std::vector<std::unique_ptr<star::StarTexture>> *offscreenRenderToColorImages,
               std::vector<std::unique_ptr<star::StarTexture>> *offscreenRenderToDepthImages,
               std::vector<star::Handle> &globalInfos, std::vector<star::Handle> &lightInfos)
    : camera(camera), screenDimensions(screenWidth, screenHeight), lightList(lightList),
      offscreenRenderToColorImages(offscreenRenderToColorImages),
      offscreenRenderToDepthImages(offscreenRenderToDepthImages), StarObject()
{
    openvdb::initialize();
    loadModel();

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
        this->camera, this->instanceModelInfos, offscreenRenderToColorImages, offscreenRenderToDepthImages, globalInfos,
        lightInfos, this->sampledTexture, this->aabbBounds);

    loadGeometry();
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

void Volume::renderVolume(const double &fov_radians, const glm::vec3 &camPosition, const glm::mat4 &camDispMatrix,
                          const glm::mat4 &camProjMat)
{
    RayCamera camera(glm::vec2{1280, 720}, fov_radians, camDispMatrix, camProjMat);

    std::array<glm::vec3, 2> bbounds = this->meshes.front()->getBoundingBoxCoords();
    {
        const auto position = this->instances.front()->getPosition();
        const auto scale = this->instances.front()->getScale();

        bbounds[0] = bbounds[0] * scale + position;
        bbounds[1] = bbounds[1] * scale + position;
    }

    {
        int curX = 0, curY = 0;
        std::array<std::unique_ptr<std::jthread>, NUM_THREADS> threads;
        int numPerThread = std::floor((this->screenDimensions.x * this->screenDimensions.y) / NUM_THREADS);

        for (auto &thread : threads)
        {
            // create work for each thread
            std::vector<std::optional<std::pair<std::pair<size_t, size_t>, star::Color *>>> coordWork(numPerThread);

            int curIndex = 0;

            for (int i = 0; i < numPerThread; i++)
            {
                coordWork[curIndex] = std::make_optional(std::make_pair(std::pair<int, int>(curX, curY), nullptr));

                if (curX == this->screenDimensions.x - 1 && curY < this->screenDimensions.y - 1)
                {
                    curX = 0;
                    curY++;
                }
                else if (curX == this->screenDimensions.x - 1)
                {
                    break;
                }
                else
                {
                    curX++;
                }

                curIndex++;
            }

            // create thread
            thread = std::make_unique<std::jthread>(
                Volume::calculateColor, std::ref(this->lightList), std::ref(this->stepSize),
                std::ref(this->stepSize_light), std::ref(this->russianRouletteCutoff), std::ref(this->sigma_absorbtion),
                std::ref(this->sigma_scattering), std::ref(this->lightPropertyDir_g), std::ref(this->volDensity),
                std::ref(bbounds), this->grid, camera, coordWork, this->rayMarchToAABB, this->rayMarchToVolumeBoundry);
        }
    }
    std::cout << "Done" << std::endl;

    // this->screenTexture->updateGPU();

    this->udpdateVolumeRender = false;
    this->isVisible = true;
}

std::unique_ptr<star::StarPipeline> Volume::buildPipeline(star::StarDevice &device, vk::Extent2D swapChainExtent,
                                                          vk::PipelineLayout pipelineLayout,
                                                          star::RenderingTargetInfo renderingInfo)
{
    star::StarGraphicsPipeline::PipelineConfigSettings settings;
    star::StarGraphicsPipeline::defaultPipelineConfigInfo(settings, swapChainExtent, pipelineLayout, renderingInfo);

    // enable alpha blending
    settings.colorBlendAttachment.blendEnable = VK_TRUE;
    settings.colorBlendAttachment.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
    settings.colorBlendAttachment.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
    settings.colorBlendAttachment.colorBlendOp = vk::BlendOp::eAdd;
    settings.colorBlendAttachment.srcAlphaBlendFactor = vk::BlendFactor::eOne;
    settings.colorBlendAttachment.dstAlphaBlendFactor = vk::BlendFactor::eZero;
    settings.colorBlendAttachment.alphaBlendOp = vk::BlendOp::eAdd;

    settings.colorBlendInfo.logicOpEnable = VK_FALSE;
    settings.colorBlendInfo.logicOp = vk::LogicOp::eCopy;

    auto graphicsShaders = this->getShaders();

    auto newPipeline =
        std::make_unique<star::StarGraphicsPipeline>(device, settings, graphicsShaders.at(star::Shader_Stage::vertex),
                                                     graphicsShaders.at(star::Shader_Stage::fragment));
    newPipeline->init();

    return newPipeline;
}

void Volume::loadModel()
{
    const std::string filePath(star::ConfigFile::getSetting(star::Config_Settings::mediadirectory) +
                               "volumes/sphere.vdb");

    if (!star::FileHelpers::FileExists(filePath))
    {
        throw std::runtime_error("Provided file does not exist" + filePath);
    }

    openvdb::GridBase::Ptr baseGrid{};

    openvdb::io::File file(filePath);

    file.open();

    std::cout << "OpenVDB File Info:" << std::endl;
    for (openvdb::io::File::NameIterator nameIter = file.beginName(); nameIter != file.endName(); ++nameIter)
    {
        std::cout << "Available Grids in file:" << std::endl;
        std::cout << nameIter.gridName() << std::endl;

        if (nameIter.gridName() == "ls_sphere")
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

    std::cout << "Type: " << baseGrid->type() << std::endl;

    if (this->grid->getGridClass() == openvdb::GridClass::GRID_LEVEL_SET)
    {
        // need to convert to fog volume
        convertToFog(this->grid);
    }

    openvdb::math::CoordBBox bbox = this->grid->evalActiveVoxelBoundingBox();
    openvdb::math::Coord &bmin = bbox.min();
    openvdb::math::Coord &bmax = bbox.max();

    this->aabbBounds[0] = glm::vec4{bmin.x(), bmin.y(), bmin.z(), 1.0};
    this->aabbBounds[1] = glm::vec4{bmax.x(), bmax.y(), bmax.z(), 1.0};

    auto gridAccessor = grid->getConstAccessor();
    openvdb::math::CoordBBox bounds;
    gridAccessor.tree().getIndexRange(bounds);

    size_t step_size = 200;
    {
        openvdb::math::Coord min = bounds.min();
        {
            int x = static_cast<int>(min.x()) / static_cast<int>(step_size);
            int y = static_cast<int>(min.y()) / static_cast<int>(step_size);
            int z = static_cast<int>(min.z()) / static_cast<int>(step_size);
            min = openvdb::math::Coord(x, y, z);
        }

        openvdb::math::Coord max = bounds.max();
        {
            int x = static_cast<int>(max.x()) / static_cast<int>(step_size);
            int y = static_cast<int>(max.y()) / static_cast<int>(step_size);
            int z = static_cast<int>(max.z()) / static_cast<int>(step_size);
            max = openvdb::math::Coord(x, y, z);
        }

        bounds = openvdb::math::CoordBBox(min, max);
    }

    auto sampledBoundrySize = bounds.extents().asVec3i();
    std::unique_ptr<std::vector<std::vector<std::vector<float>>>> sampledGridData =
        std::unique_ptr<std::vector<std::vector<std::vector<float>>>>(new std::vector(
            sampledBoundrySize.x(),
            std::vector<std::vector<float>>(sampledBoundrySize.y(), std::vector<float>(sampledBoundrySize.z(), 0.0f))));

    std::cout << "Sampling grid with step size of: " << step_size << std::endl;
    size_t halfTotalSteps = sampledGridData->size() / 2;
    ProcessVolume processor = ProcessVolume(this->grid.get(), *sampledGridData, step_size, halfTotalSteps);
    oneapi::tbb::parallel_for(bounds, processor);

    this->sampledTexture =
        star::ManagerRenderResource::addRequest(std::make_unique<SampledVolumeController>(std::move(sampledGridData)));
    std::cout << "Done" << std::endl;
}

void Volume::loadGeometry()
{
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

    std::unique_ptr<ScreenMaterial> material =
        std::unique_ptr<ScreenMaterial>(std::make_unique<ScreenMaterial>(this->volumeRenderer->getRenderToImages()));
    auto newMeshes = std::vector<std::unique_ptr<star::StarMesh>>();

    star::Handle vertBuffer = star::ManagerRenderResource::addRequest(
        std::make_unique<star::ManagerController::RenderResource::VertInfo>(*verts));
    star::Handle indBuffer = star::ManagerRenderResource::addRequest(
        std::make_unique<star::ManagerController::RenderResource::IndicesInfo>(*inds));

    newMeshes.emplace_back(std::unique_ptr<star::StarMesh>(new star::StarMesh(
        vertBuffer, indBuffer, *verts, *inds, std::move(material), this->aabbBounds[0], this->aabbBounds[1], false)));

    this->meshes = std::move(newMeshes);
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

void Volume::prepRender(star::StarDevice &device, vk::Extent2D swapChainExtent, vk::PipelineLayout pipelineLayout,
                        star::RenderingTargetInfo renderingInfo, int numSwapChainImages,
                        star::StarShaderInfo::Builder fullEngineBuilder)
{
    RecordQueueFamilyInfo(device, this->computeQueueFamily, this->graphicsQueueFamily);

    this->star::StarObject::prepRender(device, swapChainExtent, pipelineLayout, renderingInfo, numSwapChainImages,
                                       fullEngineBuilder);
}

void Volume::prepRender(star::StarDevice &device, int numSwapChainImages, star::StarPipeline &sharedPipeline,
                        star::StarShaderInfo::Builder fullEngineBuilder)
{
    RecordQueueFamilyInfo(device, this->computeQueueFamily, this->graphicsQueueFamily);

    this->star::StarObject::prepRender(device, numSwapChainImages, sharedPipeline, fullEngineBuilder);
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

void Volume::calculateColor(
    const std::vector<std::unique_ptr<star::Light>> &lightList, const float &stepSize, const float &stepSize_light,
    const int &russianRouletteCutoff, const float &sigma_absorbtion, const float &sigma_scattering,
    const float &lightPropertyDir_g, const float &volDensity, const std::array<glm::vec3, 2> &aabbBounds,
    openvdb::FloatGrid::Ptr grid, RayCamera camera,
    std::vector<std::optional<std::pair<std::pair<size_t, size_t>, star::Color *>>> coordColorWork,
    bool marchToaabbIntersection, bool marchToVolBoundry)
{
    for (auto &work : coordColorWork)
    {
        if (work.has_value())
        {
            float t0 = 0, t1 = 0;
            auto ray = camera.getRayForPixel(work->first.first, work->first.second);
            star::Color newCol{};

            if (rayBoxIntersect(ray, aabbBounds, t0, t1))
                if (marchToaabbIntersection)
                    newCol = star::Color(1.0f, 0.0f, 0.0f, 1.0f);
                else if (marchToVolBoundry)
                    newCol = forwardMarchToVolumeActiveBoundry(stepSize, ray, aabbBounds, grid, t0, t1);
                else
                    newCol =
                        forwardMarch(lightList, stepSize, stepSize_light, russianRouletteCutoff, sigma_absorbtion,
                                     sigma_scattering, lightPropertyDir_g, volDensity, ray, grid, aabbBounds, t0, t1);
            else
                newCol = star::Color(0.0f, 0.0f, 0.0f, 0.0f);

            *work->second = newCol;
        }
    }
}

bool Volume::rayBoxIntersect(const star::Ray &ray, const std::array<glm::vec3, 2> &aabbBounds, float &t0, float &t1)
{
    float tmin = -INFINITY, tmax = INFINITY, txmin = 0, txmax = 0, tymin = 0, tymax = 0, tzmin = 0, tzmax = 0;

    txmin = (aabbBounds[ray.sign[0]].x - ray.org.x) * ray.invDir.x;
    txmax = (aabbBounds[!ray.sign[0]].x - ray.org.x) * ray.invDir.x;

    tmin = std::min(txmin, txmax);
    tmax = std::max(txmin, txmax);

    tymin = (aabbBounds[ray.sign[1]].y - ray.org.y) * ray.invDir.y;
    tymax = (aabbBounds[!ray.sign[1]].y - ray.org.y) * ray.invDir.y;

    tmin = std::max(tmin, std::min(tymin, tymax));
    tmax = std::min(tmax, std::max(tymin, tymax));

    tzmin = (aabbBounds[ray.sign[2]].z - ray.org.z) * ray.invDir.z;
    tzmax = (aabbBounds[!ray.sign[2]].z - ray.org.z) * ray.invDir.z;

    tmin = std::max(tmin, std::min(tzmin, tzmax));
    tmax = std::min(tmax, std::max(tzmin, tzmax));

    t0 = tmin;
    t1 = tmax;

    return tmax >= std::max(0.0f, tmin);
}

star::Color Volume::forwardMarch(const std::vector<std::unique_ptr<star::Light>> &lightList, const float &stepSize,
                                 const float &stepSize_light, const int &russianRouletteCutoff,
                                 const float &sigma_absorbtion, const float &sigma_scattering,
                                 const float &lightPropertyDir_g, const float &volDensity, const star::Ray &ray,
                                 openvdb::FloatGrid::Ptr grid, const std::array<glm::vec3, 2> &aabbHit, const float &t0,
                                 const float &t1)
{
    int numSteps = static_cast<int>(std::ceil((t1 - t0) / stepSize));
    float fittedStepSize = (t1 - t0) / numSteps;
    float transparency = 1.0f;
    star::Color backColor{};
    star::Color resultingColor{};

    auto gridAccessor = grid->getConstAccessor();
    openvdb::tools::GridSampler<openvdb::FloatGrid::ConstAccessor, openvdb::tools::BoxSampler> sampler(
        gridAccessor, grid->transform());

    for (int i = 0; i < numSteps; ++i)
    {
        float randJitter = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
        float t = t0 + fittedStepSize * (i + 0.5f);
        glm::vec3 position = ray.org + (t / ray.invDir);
        openvdb::Vec3R oPosition(position.x, position.y, position.z);
        openvdb::FloatGrid::ValueType sampledDensity = sampler.wsSample(oPosition);
        float beerExpTrans = std::exp(-fittedStepSize * sampledDensity * (sigma_absorbtion + sigma_scattering));
        transparency *= beerExpTrans;

        for (const auto &light : lightList)
        {
            // in-scattering
            star::Ray lightRay{position, glm::normalize(light->getPosition() - position)};

            float lt0 = 0, lt1 = 0;
            if (rayBoxIntersect(lightRay, aabbHit, lt0, lt1))
            {
                int numLightSteps = static_cast<int>(std::ceil((lt1 - lt0) / stepSize_light));
                float fittedLightStepSize = (lt1 - lt0) / static_cast<float>(numLightSteps);

                float sumLightSampleDensities = 0.0f;
                for (int lightStep = 0; lightStep < numLightSteps; lightStep++)
                {
                    float tLight = fittedLightStepSize * (lightStep + 0.5);
                    glm::vec3 lightTracePosition = lightRay.org + (tLight / lightRay.invDir);

                    openvdb::FloatGrid::ValueType sampledValue = sampler.wsSample(
                        openvdb::Vec3R(lightTracePosition.x, lightTracePosition.y, lightTracePosition.z));
                    sumLightSampleDensities += sampledValue;
                }

                float lightAtten = std::exp(-sumLightSampleDensities * -lt1 * (sigma_absorbtion + sigma_scattering));
                float phaseResult = lightAtten * henyeyGreensteinPhase(position, lightRay.dir, lightPropertyDir_g) *
                                    volDensity * fittedLightStepSize;

                resultingColor.setR(resultingColor.getR() + (light->getAmbient().r * phaseResult));
                resultingColor.setG(resultingColor.getG() + (light->getAmbient().g * phaseResult));
                resultingColor.setB(resultingColor.getB() + (light->getAmbient().b * phaseResult));
                resultingColor.setA(resultingColor.getA() + (light->getAmbient().a * phaseResult));
            }
        }

        resultingColor.setR(resultingColor.getR() * transparency);
        resultingColor.setG(resultingColor.getG() * transparency);
        resultingColor.setB(resultingColor.getB() * transparency);
        resultingColor.setA(resultingColor.getA() * transparency);

        // russian roulette cutoff
        if ((transparency < 1e-3) && (randJitter > 1.0f / russianRouletteCutoff))
            break;
        else if (transparency < 1e-3)
            transparency *= russianRouletteCutoff;
    }

    resultingColor.setR(backColor.getR() * transparency + resultingColor.getR());
    resultingColor.setG(backColor.getG() * transparency + resultingColor.getG());
    resultingColor.setB(backColor.getB() * transparency + resultingColor.getB());
    resultingColor.setA(backColor.getA() * transparency + resultingColor.getA());
    return resultingColor;
}

star::Color Volume::forwardMarchToVolumeActiveBoundry(const float &stepSize, const star::Ray &ray,
                                                      const std::array<glm::vec3, 2> &aabbHit,
                                                      openvdb::FloatGrid::Ptr grid, const float &t0, const float &t1)
{
    int numSteps = static_cast<int>(std::ceil((t1 - t0) / stepSize));
    float fittedStepSize = (t1 - t0) / numSteps;
    star::Color resultingColor{};

    auto gridAccessor = grid->getConstAccessor();
    openvdb::tools::GridSampler<openvdb::FloatGrid::ConstAccessor, openvdb::tools::BoxSampler> sampler(
        gridAccessor, grid->transform());

    for (int i = 0; i < numSteps; ++i)
    {
        float t = t0 + fittedStepSize * (i + 0.5f);
        glm::vec3 position = ray.org + (t / ray.invDir);
        openvdb::Vec3R oPosition(position.x, position.y, position.z);
        openvdb::FloatGrid::ValueType sampledDensity = sampler.wsSample(oPosition);

        if (sampledDensity > 0.0f && sampledDensity < 1.0f)
        {
            resultingColor.setR(1.0f);
            resultingColor.setA(1.0f);
            break;
        }
    }

    return resultingColor;
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

void Volume::RecordQueueFamilyInfo(star::StarDevice &device, uint32_t &computeQueueFamilyIndex,
                                   uint32_t &graphicsQueueFamilyIndex)
{
    computeQueueFamilyIndex = device.getDefaultQueue(star::Queue_Type::Tcompute).getParentQueueFamilyIndex();
    graphicsQueueFamilyIndex = device.getDefaultQueue(star::Queue_Type::Tgraphics).getParentQueueFamilyIndex();
}