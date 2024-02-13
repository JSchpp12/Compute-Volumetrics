#include "Volume.hpp"

std::unordered_map<star::Shader_Stage, star::StarShader> Volume::getShaders()
{
    std::unordered_map<star::Shader_Stage, star::StarShader> shaders; 

    std::string mediaPath = star::ConfigFile::getSetting(star::Config_Settings::mediadirectory); 

    std::string vertPath = mediaPath + "shaders/screenWithTexture/screenWithTexture.vert"; 
    std::string fragPath = mediaPath + "shaders/screenWithTexture/screenWithTexture.frag";

    shaders.insert(std::pair<star::Shader_Stage, star::StarShader>(star::Shader_Stage::vertex, star::StarShader(vertPath, star::Shader_Stage::vertex))); 
    shaders.insert(std::pair<star::Shader_Stage, star::StarShader>(star::Shader_Stage::fragment, star::StarShader(fragPath, star::Shader_Stage::fragment)));

    return shaders;
}

void Volume::renderVolume(const double& fov_radians, const glm::vec3& camPosition, const glm::mat4& camDispMatrix, const glm::mat4& camProjMat)
{
    RayCamera camera(this->screenDimensions, fov_radians, camDispMatrix, camProjMat);

    std::array<glm::vec3, 2> bbounds = this->meshes.front()->getBoundingBoxCoords();
    {
        auto position = this->instances.front()->getPosition();
        auto scale = this->instances.front()->getScale();

        bbounds[0] = bbounds[0] * scale + position;
        bbounds[1] = bbounds[1] * scale + position;
    }

    auto acc = this->grid->getConstAccessor();

    std::cout << "Beginning Volume Render..." << std::endl;
    if (this->rayMarchToVolumeBoundry)
        std::cout << "Marching to active boundry" << std::endl;
    else
        std::cout << "Normal march" << std::endl;

    for (size_t x = 0; x < this->screenDimensions.x; x++) {
        for (size_t y = 0; y < this->screenDimensions.y; y++) {
            float t0 = 0, t1 = 0;
            auto ray = camera.getRayForPixel(x, y, camPosition);
            star::Color newCol{};

            if (rayBoxIntersect(ray, bbounds, t0, t1))
                if (this->rayMarchToAABB)
                    newCol = star::Color(1.0f, 0.0f, 0.0f, 1.0f);
                else if (this->rayMarchToVolumeBoundry)
                    newCol = this->forwardMarchToVolumeActiveBoundry(acc, ray, bbounds, t0, t1);
                else
                    newCol = this->forwardMarch(acc, ray, bbounds, t0, t1);
            else
                newCol = star::Color(0.0f, 0.0f, 0.0f, 0.0f);
            this->screenTexture->getRawData()->at(y).at(x) = newCol;
        }
    }
    std::cout << "Done" << std::endl; 

    this->screenTexture->updateGPU();

    this->udpdateVolumeRender = false;
    this->isVisible = true;
}

std::unique_ptr<star::StarPipeline> Volume::buildPipeline(star::StarDevice& device, vk::Extent2D swapChainExtent,
    vk::PipelineLayout pipelineLayout, vk::RenderPass renderPass)
{
    star::StarGraphicsPipeline::PipelineConfigSettings settings;
    star::StarGraphicsPipeline::defaultPipelineConfigInfo(settings, swapChainExtent, renderPass, pipelineLayout);

    //enable alpha blending
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

    auto newPipeline = std::make_unique<star::StarGraphicsPipeline>(device, settings, graphicsShaders.at(star::Shader_Stage::vertex), graphicsShaders.at(star::Shader_Stage::fragment));
    newPipeline->init();

    return std::move(newPipeline);
}

void Volume::loadModel()
{
    const std::string filePath(star::ConfigFile::getSetting(star::Config_Settings::mediadirectory) + "volumes/sphere.vdb");
    openvdb::GridBase::Ptr baseGrid{};

    openvdb::io::File file(filePath);

    file.open();

    std::cout << "OpenVDB File Info:" << std::endl;
    for (openvdb::io::File::NameIterator nameIter = file.beginName(); nameIter != file.endName(); ++nameIter) {
        std::cout << "Available Grids in file:" << std::endl;
        std::cout << nameIter.gridName() << std::endl;

        if (nameIter.gridName() == "ls_sphere") {
            baseGrid = file.readGrid(nameIter.gridName());
        }
        else {
            std::cout << "Skipping extra grid: " << nameIter.gridName();
        }
    }
    file.close();

    this->grid = openvdb::gridPtrCast<openvdb::FloatGrid>(baseGrid);

    std::cout << "Type: " << baseGrid->type() << std::endl;



    if (this->grid->getGridClass() == openvdb::GridClass::GRID_LEVEL_SET){
        //need to convert to fog volume
        convertToFog(this->grid);
    }
}

std::pair<std::unique_ptr<star::StarBuffer>, std::unique_ptr<star::StarBuffer>> Volume::loadGeometryBuffers(star::StarDevice& device)
{
    std::unique_ptr<std::vector<star::Vertex>> verts = std::unique_ptr<std::vector<star::Vertex>>(new std::vector<star::Vertex>{
        star::Vertex{
            glm::vec3{-1.0f, -1.0f, 0.0f},	//position
            glm::vec3{0.0f, 1.0f, 0.0f},	//normal - posy
            glm::vec3{0.0f, 1.0f, 0.0f},		//color
            glm::vec2{0.0f, 0.0f}
        },
        star::Vertex{
            glm::vec3{1.0f, -1.0f, 0.0f},	//position
            glm::vec3{0.0f, 1.0f, 0.0f},	//normal - posy
            glm::vec3{0.0f, 1.0f, 0.0f},		//color
            glm::vec2{1.0f, 0.0f}
        },
        star::Vertex{
            glm::vec3{1.0f, 1.0f, 0.0f},	//position
            glm::vec3{0.0f, 1.0f, 0.0f},	//normal - posy
            glm::vec3{1.0f, 0.0f, 0.0f},		//color
            glm::vec2{1.0f, 1.0f}
        },
        star::Vertex{
            glm::vec3{-1.0f, 1.0f, 0.0f},	//position
            glm::vec3{0.0f, 1.0f, 0.0f},	//normal - posy
            glm::vec3{0.0f, 1.0f, 0.0f},		//color
            glm::vec2{0.0f, 1.0f}
        },
    });

    std::unique_ptr<std::vector<uint32_t>> inds = std::unique_ptr<std::vector<uint32_t>>(new std::vector<uint32_t>{
        0,3,2,0,2,1
    });

    std::unique_ptr<star::TextureMaterial> material = std::unique_ptr<star::TextureMaterial>(new star::TextureMaterial(this->screenTexture));
    auto newMeshs = std::vector<std::unique_ptr<star::StarMesh>>();

    openvdb::math::CoordBBox bbox = this->grid->evalActiveVoxelBoundingBox();
    openvdb::math::Coord& bmin = bbox.min();
    openvdb::math::Coord& bmax = bbox.max();

    glm::vec3 min{ bmin.x(), bmin.y(), bmin.z() };
    glm::vec3 max{ bmax.x(), bmax.y(), bmax.z() };

    newMeshs.emplace_back(std::unique_ptr<star::StarMesh>(new star::StarMesh(*verts, *inds, std::move(material), min, max, false)));

    this->meshes = std::move(newMeshs);

    auto stagingVert = std::make_unique<star::StarBuffer>(
        device,
        vk::DeviceSize{ sizeof(star::Vertex) },
        vk::DeviceSize{ verts->size() },
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
    );
    stagingVert->map();
    stagingVert->writeToBuffer(verts->data(), VK_WHOLE_SIZE);
    stagingVert->unmap();

    auto stagingIndex = std::make_unique<star::StarBuffer>(
        device,
        vk::DeviceSize{ sizeof(uint32_t) },
        vk::DeviceSize{ inds->size() },
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
    );
    stagingIndex->map();
    stagingIndex->writeToBuffer(inds->data(), VK_WHOLE_SIZE);
    stagingIndex->unmap();

    return std::pair<std::unique_ptr<star::StarBuffer>, std::unique_ptr<star::StarBuffer>>(std::move(stagingVert), std::move(stagingIndex));
}

void Volume::initResources(star::StarDevice& device, const int numFramesInFlight)
{
    this->StarObject::initResources(device, numFramesInFlight);
}

void Volume::destroyResources(star::StarDevice& device)
{
    this->StarObject::destroyResources(device);

    this->screenTexture.reset(); 
}

void Volume::updateGridTransforms()
{
    openvdb::FloatGrid::Ptr newGrid = this->grid->copy();
    newGrid->setTransform(this->grid->transformPtr());
    openvdb::Mat4R transform = getTransform(this->instances.front()->getDisplayMatrix());

    auto test = this->instances.front()->getScale();
    openvdb::tools::GridTransformer transformer(transform);

    transformer.transformGrid<openvdb::tools::BoxSampler,openvdb::FloatGrid>(*this->grid, *newGrid);

    this->grid = newGrid;
    this->grid->pruneGrid();
}

bool Volume::rayBoxIntersect(const star::Ray& ray, const std::array<glm::vec3, 2>& aabbBounds, float& t0, float& t1)
{
    //Debug ray-plane intersection
    //{
    //    //glm::vec3 r0 = glm::vec3{ 0.0, 0.0, 0.0 }; 
    //    //glm::vec3 rd = glm::vec3{ 0.0, 1.0, 0.0 };

    //    glm::vec3 normal = glm::vec3{ 0.0, -1.0, 0.0 };
    //    glm::vec3 p0 = glm::vec3{ 0.0, 0.0, 0.0 };

    //    float denom = glm::dot(normal, ray.dir);
    //    if (denom > 1e-6) {
    //        glm::vec3 p0l0 = p0 - ray.org;
    //        float t = glm::dot(p0l0, normal) / denom;
    //        return (t >= 0);
    //    }
    //    return false;

    //}

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

star::Color Volume::forwardMarch(openvdb::FloatGrid::ConstAccessor& gridAccessor, const star::Ray& ray, const std::array<glm::vec3, 2>& aabbHit, const float& t0, const float& t1)
{
    int numSteps = static_cast<int>(std::ceil((t1 - t0) / this->stepSize));
    float fittedStepSize = (t1 - t0) / numSteps;
    float transparency = 1.0f;
    star::Color backColor{};
    star::Color resultingColor{};

    openvdb::tools::GridSampler<openvdb::FloatGrid::ConstAccessor, openvdb::tools::BoxSampler> sampler(gridAccessor, this->grid->transform());

    for (int i = 0; i < numSteps; ++i) {
        float randJitter = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
        float t = t0 + fittedStepSize * (i + 0.5f);
        glm::vec3 position = ray.org + (t / ray.invDir);
        openvdb::Vec3R oPosition(position.x, position.y, position.z);
        openvdb::FloatGrid::ValueType sampledDensity = sampler.wsSample(oPosition);

        float beerExpTrans = std::exp(-fittedStepSize * sampledDensity * (this->sigma_absorbtion + this->sigma_scattering));
        transparency *= beerExpTrans;

        for (const auto& light : this->lightList) {
            //in-scattering 
            star::Ray lightRay{
                position,
                glm::normalize(light->getPosition() - position)
            };

            float lt0 = 0, lt1 = 0;
            if (rayBoxIntersect(lightRay, aabbHit, lt0, lt1)) {
                int numLightSteps = static_cast<int>(std::ceil((lt1 - lt0) / this->stepSize_light));
                float fittedLightStepSize = (lt1 - lt0) / static_cast<float>(numLightSteps);

                float sumLightSampleDensities = 0.0f;
                for (int lightStep = 0; lightStep < numLightSteps; lightStep++) {
                    float tLight = fittedLightStepSize * (lightStep + 0.5);
                    glm::vec3 lightTracePosition = lightRay.org + (tLight / lightRay.invDir);

                    openvdb::FloatGrid::ValueType sampledValue = sampler.wsSample(openvdb::Vec3R(lightTracePosition.x, lightTracePosition.y, lightTracePosition.z));
                    sumLightSampleDensities += sampledValue;
                }

                float lightAtten = std::exp(-sumLightSampleDensities * -lt1 * (this->sigma_absorbtion + this->sigma_scattering));
                float phaseResult = lightAtten * henyeyGreensteinPhase(position, lightRay.dir, this->lightPropertyDir_g) * this->volDensity * fittedLightStepSize;

                resultingColor.setR(resultingColor.r() + (light->getAmbient().r * phaseResult));
                resultingColor.setG(resultingColor.g() + (light->getAmbient().g * phaseResult));
                resultingColor.setB(resultingColor.b() + (light->getAmbient().b * phaseResult));
                resultingColor.setA(resultingColor.a() + (light->getAmbient().a * phaseResult));
            }
        }

        resultingColor.setR(resultingColor.r() * transparency);
        resultingColor.setG(resultingColor.g() * transparency);
        resultingColor.setB(resultingColor.b() * transparency);
        resultingColor.setA(resultingColor.a() * transparency);

        //russian roulette cutoff
        if ((transparency < 1e-3) && (randJitter > 1.0f / this->russianRouletteCutoff))
            break;
        else if (transparency < 1e-3)
            transparency *= this->russianRouletteCutoff;  
    }

    resultingColor.setR(backColor.r() * transparency + resultingColor.r());
    resultingColor.setG(backColor.g() * transparency + resultingColor.g());
    resultingColor.setB(backColor.b() * transparency + resultingColor.b());
    resultingColor.setA(backColor.a() * transparency + resultingColor.a());
    return resultingColor;
}

star::Color Volume::forwardMarchToVolumeActiveBoundry(openvdb::FloatGrid::ConstAccessor& gridAccessor, const star::Ray& ray, const std::array<glm::vec3, 2>& aabbHit, const float& t0, const float& t1)
{
    int numSteps = static_cast<int>(std::ceil((t1 - t0) / this->stepSize));
    float fittedStepSize = (t1 - t0) / numSteps;
    float transparency = 1.0f;
    star::Color backColor{};
    star::Color resultingColor{};

    openvdb::tools::GridSampler<openvdb::FloatGrid::ConstAccessor, openvdb::tools::BoxSampler> sampler(gridAccessor, this->grid->transform());

    for (int i = 0; i < numSteps; ++i) {
        float randJitter = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
        float t = t0 + fittedStepSize * (i + 0.5f);
        glm::vec3 position = ray.org + (t / ray.invDir);
        openvdb::Vec3R oPosition(position.x, position.y, position.z);
        openvdb::FloatGrid::ValueType sampledDensity = sampler.wsSample(oPosition);

        float beerExpTrans = std::exp(-fittedStepSize * sampledDensity * (this->sigma_absorbtion + this->sigma_scattering));
        transparency *= beerExpTrans;

        if (sampledDensity > 0.0f && sampledDensity < 1.0f) {
            resultingColor.setR(1.0f);
            resultingColor.setA(1.0f);
            break;
        }
    }

    return resultingColor;
}

void Volume::convertToFog(openvdb::FloatGrid::Ptr& grid)
{
    const float outside = grid->background();
    const float width = 2.0f * outside;

    // Visit and update all of the grid's active values, which correspond to
    // voxels on the narrow band.
    for (openvdb::FloatGrid::ValueOnIter iter = grid->beginValueOn(); iter; ++iter) {
        float dist = iter.getValue();
        iter.setValue((outside - dist) / width);
    }

    // Visit all of the grid's inactive tile and voxel values and update the values
    // that correspond to the interior region.
    for (openvdb::FloatGrid::ValueOffIter iter = grid->beginValueOff(); iter; ++iter) {
        if (iter.getValue() < 0.0) {
            iter.setValue(1.0);
            iter.setValueOff();
        }
    }
    // Set exterior voxels to 0.
    openvdb::tools::changeBackground(grid->tree(), 0.0f);

    grid->setGridClass(openvdb::GridClass::GRID_FOG_VOLUME);
}

float Volume::henyeyGreensteinPhase(const glm::vec3& viewDirection, const glm::vec3& lightDirection, const float& gValue)
{
    float cosTheta = glm::dot(viewDirection, lightDirection); 
    float denom = std::powf(1 + gValue * gValue - 2.0f * gValue * cosTheta, 1.5f);
    return 1.0f / (4.0f * glm::pi<float>()) * (1.0f - gValue * gValue) / denom;
}

openvdb::Mat4R Volume::getTransform(const glm::mat4& objectDisplayMat)
{
    std::unique_ptr<float> rawData = std::unique_ptr<float>(new float[16]);

    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            rawData.get()[(i*4) + j] = objectDisplayMat[j][i];
        }
    }

    return openvdb::Mat4R(rawData.get());
}
