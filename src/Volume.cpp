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

void Volume::renderVolume(const float& fov_radians, const glm::vec3& camPosition, const glm::mat4& camDispMatrix, const glm::mat4& camProjMat)
{
    RayCamera camera(this->screenDimensions, fov_radians, camDispMatrix, camProjMat);

    std::array<glm::vec3, 2> bbounds = this->meshes.front()->getBoundingBoxCoords();
    {
        auto position = this->instances.front()->getPosition();
        auto scale = this->instances.front()->getScale();

        bbounds[0] = bbounds[0] * scale + position;
        bbounds[1] = bbounds[1] * scale + position;
    }
    //float tt0, tt1;
    //auto tRay = star::Ray{
    //    glm::vec3{0.0, 0.0, 1.0},
    //    glm::vec3{0.0f, 0.0f, -1.0f}
    //};
    //auto test = rayBoxIntersect(tRay, bbounds, tt0, tt1);

    //if (test)
    //    std::cout << "TEST";

    //glm::vec3 hitPoint = tRay.org + (tt0 * tRay.dir);

    for (size_t x = 0; x < this->screenDimensions.x; x++) {
        for (size_t y = 0; y < this->screenDimensions.y; y++) {
            float t0 = 0, t1 = 0;
            auto ray = camera.getRayForPixel(x, y, camPosition);
            star::Color newCol{};

            if (x == this->screenDimensions.x / 2 && y == this->screenDimensions.y / 2)
                std::cout << "test" << std::endl;

            if (rayBoxIntersect(ray, bbounds, t0, t1))
                newCol = this->backMarch(ray, bbounds, t0, t1);
                //newCol = star::Color(0.0f, 1.0f, 0.0f, 1.0f);
            else
                newCol = star::Color(0.0f, 0.0f, 0.0f, 0.0f);
            this->screenTexture->getRawData()->at(y).at(x) = newCol;
        }
    }

    this->screenTexture->updateGPU();
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
    const std::string filePath(star::ConfigFile::getSetting(star::Config_Settings::mediadirectory) + "volumes/Sphere.vdb");

    openvdb::io::File file(filePath);

    file.open();

    for (openvdb::io::File::NameIterator nameIter = file.beginName(); nameIter != file.endName(); ++nameIter) {
        std::cout << nameIter.gridName() << std::endl;

        if (nameIter.gridName() == "ls_sphere") {
            baseGrid = file.readGrid(nameIter.gridName());
    
        }
        else {
            std::cout << "Skipping extra grid: " << nameIter.gridName();
        }
    }

    file.close();
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

    openvdb::math::CoordBBox bbox = baseGrid.get()->evalActiveVoxelBoundingBox();
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

    auto testBB = aabbBounds[ray.sign[0]].x;
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

    if (tmax >= std::max(0.0f, tmin)) {
        int t = 0;
        t += 1;
    }
    
    t0 = tmin;
    t1 = tmax;

    return tmax >= std::max(0.0f, tmin);
}

star::Color Volume::backMarch(const star::Ray& ray, const std::array<glm::vec3, 2>& aabbHit, const float& t0, const float& t1)
{
    float fittedStepSize = (t1 - t0) / this->numSteps;
    float beerExpTrans = this->calcExp(fittedStepSize, this->sigma);
    float transparency = 1.0f;

    star::Color resultingColor{};

    for (int i = 0; i < this->numSteps; i++) {
        transparency *= beerExpTrans;
        float t = t1 - fittedStepSize * (i + 0.5); 
        glm::vec3 position = ray.org + (t / ray.invDir);

        for (const auto& light : this->lightList) {
            //in-scattering 
            star::Ray lightRay{
                position,
                glm::normalize(light->getPosition() - position)
            };

            float lt0 = 0, lt1 = 0;
            if (rayBoxIntersect(lightRay, aabbHit, lt0, lt1)) {
                assert(lt0 != 0 && "Unknown ray error occurred. The sample step should always be inside hit volume");
                float lightAtten = calcExp(lt1, this->sigma);

                resultingColor.setR(resultingColor.r() + (light->getAmbient().r * lightAtten * fittedStepSize));
                resultingColor.setG(resultingColor.g() + (light->getAmbient().g * lightAtten * fittedStepSize));
                resultingColor.setB(resultingColor.b() + (light->getAmbient().b * lightAtten * fittedStepSize));
                resultingColor.setA(resultingColor.a() + (light->getAmbient().a * lightAtten * fittedStepSize));
            }
        }

        resultingColor.setR(resultingColor.r() * transparency);
        resultingColor.setG(resultingColor.g() * transparency);
        resultingColor.setB(resultingColor.b() * transparency);
        resultingColor.setA(resultingColor.a() * transparency);
    }

    return resultingColor;
}
