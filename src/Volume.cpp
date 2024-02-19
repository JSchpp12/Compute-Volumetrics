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

    {
        size_t curX = 0, curY = 0;
        std::array<std::unique_ptr<std::jthread>, NUM_THREADS> threads;
        int numPerThread = std::floor((this->screenDimensions.x * this->screenDimensions.y) / NUM_THREADS);

        for (auto& thread : threads) {
            //create work for each thread
            std::vector<std::optional<std::pair<std::pair<size_t, size_t>, star::Color*>>> coordWork(numPerThread);

            int curIndex = 0;

            for (int i = 0; i < numPerThread; i++) {
                coordWork[curIndex] = std::make_optional(std::make_pair(std::pair<int,int>(curX, curY), &this->screenTexture->getRawData()->at(curY).at(curX)));

                if (curX == this->screenDimensions.x - 1 && curY < this->screenDimensions.y - 1) {
                    curX = 0;
                    curY++;
                }else if (curX == this->screenDimensions.x - 1){
                    break;
                }
                else {
                    curX++;
                }
                
                curIndex++;
            }

            //create thread
            thread = std::make_unique<std::jthread>(Volume::calculateColor,
                std::ref(this->lightList),std::ref(this->numSteps), std::ref(this->numStepsLight),
                std::ref(this->russianRouletteCutoff),
                std::ref(this->sigma_absorbtion), std::ref(this->sigma_scattering), std::ref(this->lightPropertyDir_g), 
                std::ref(this->volDensity), std::ref(bbounds),
                this->grid, camera, coordWork);
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

float Volume::calcExp(const float& stepSize, const float& sigma)
{
    return std::exp(-stepSize * sigma);
}

float Volume::henyeyGreensteinPhase(const float& g, const float& cos_theta)
{
    float denom = 1 + g * g - 2 * g * cos_theta;
    return 1 / (4 * glm::pi<float>()) * (1 - g * g) / (denom * std::sqrtf(denom));
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

void Volume::calculateColor(const std::vector<std::unique_ptr<star::Light>>& lightList, const int& numSteps, const int& numSteps_light, const int& russianRouletteCutoff, const float& sigma_absorbtion, const float& sigma_scattering, const float& lightPropertyDir_g, const float& volDensity, const std::array<glm::vec3, 2>& aabbBounds, openvdb::FloatGrid::Ptr grid, RayCamera camera, std::vector<std::optional<std::pair<std::pair<size_t, size_t>, star::Color*>>> coordColorWork)
{
    for (auto& work : coordColorWork) {
        if (work.has_value()) {
            float t0 = 0, t1 = 0;
            auto ray = camera.getRayForPixel(work->first.first, work->first.second);
            star::Color newCol{};

            if (rayBoxIntersect(ray, aabbBounds, t0, t1))
                newCol = forwardMarch(lightList, numSteps, numSteps_light, russianRouletteCutoff, sigma_absorbtion, sigma_scattering, lightPropertyDir_g, volDensity, ray, grid, aabbBounds, t0, t1);
            else
                newCol = star::Color(0.0f, 0.0f, 0.0f, 0.0f);

            *work->second = newCol;
        }
    }
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

star::Color Volume::forwardMarch(const std::vector<std::unique_ptr<star::Light>>& lightList, const int& numSteps, const int& numSteps_light, const int& russianRouletteCutoff, const float& sigma_absorbtion, const float& sigma_scattering, const float& lightPropertyDir_g, const float& volDensity, const star::Ray& ray, openvdb::FloatGrid::Ptr grid, const std::array<glm::vec3, 2>& aabbHit, const float& t0, const float& t1)
{
    float fittedStepSize = (t1 - t0) / numSteps;
    float transparency = 1.0f;
    star::Color backColor{};
    star::Color resultingColor{};

    openvdb::tools::GridSampler<openvdb::FloatGrid::ConstAccessor, openvdb::tools::BoxSampler> sampler(gridAccessor, this->grid->transform());
    auto gridAccessor = grid->getConstAccessor();
    openvdb::tools::GridSampler<openvdb::FloatGrid::ConstAccessor, openvdb::tools::BoxSampler> sampler(gridAccessor, grid->transform());

    for (int i = 0; i < numSteps; ++i) {
        float randJitter = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
        float t = t0 + fittedStepSize * (i + 0.5f);
        glm::vec3 position = ray.org + (t / ray.invDir);
        openvdb::Vec3R oPosition(position.x, position.y, position.z);
        openvdb::FloatGrid::ValueType sampledDensity = sampler.wsSample(oPosition);
        float beerExpTrans = std::exp(-fittedStepSize * sampledDensity * (sigma_absorbtion + sigma_scattering));
        transparency *= beerExpTrans;

        for (const auto& light : lightList) {
            //in-scattering 
            star::Ray lightRay{
                position,
                glm::normalize(light->getPosition() - position)
            };

            float lt0 = 0, lt1 = 0;
            if (rayBoxIntersect(lightRay, aabbHit, lt0, lt1)) {
                float cosTheta = glm::dot(ray.dir, lightRay.dir);
                float fittedLightStepSize = lt1 / numSteps_light;

                float sumLightSampleDensities = 0.0f;
                for (int lightStep = 0; lightStep < numSteps_light; lightStep++) {
                    float tLight = fittedLightStepSize * (lightStep + 0.5);
                    glm::vec3 lightTracePosition = lightRay.org + (tLight / lightRay.invDir);

                    openvdb::FloatGrid::ValueType sampledValue = sampler.wsSample(openvdb::Vec3R(lightTracePosition.x, lightTracePosition.y, lightTracePosition.z));
                    sumLightSampleDensities += sampledValue;
                }

                float lightAtten = std::exp(-sumLightSampleDensities * -lt1 * (sigma_absorbtion + sigma_scattering));
                float phaseResult = lightAtten * henyeyGreensteinPhase(lightPropertyDir_g, cosTheta) * volDensity * fittedStepSize;

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
        if ((transparency < 1e-3) && (randJitter > 1.0f / russianRouletteCutoff))
            break;
        else if (transparency < 1e-3)
            transparency *= russianRouletteCutoff;
    }

    resultingColor.setR(backColor.r() * transparency + resultingColor.r());
    resultingColor.setG(backColor.g() * transparency + resultingColor.g());
    resultingColor.setB(backColor.b() * transparency + resultingColor.b());
    resultingColor.setA(backColor.a() * transparency + resultingColor.a());
    return resultingColor;
}