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

    for (size_t x = 0; x < this->screenDimensions.x; x++) {
        for (size_t y = 0; y < this->screenDimensions.y; y++) {
            if (x == this->screenDimensions.x / 2 && y == this->screenDimensions.y / 2)
                std::cout << "test" << std::endl;

            auto ray = camera.getRayForPixel(x, y, camPosition);
            star::Color newCol{};

            if (rayBoxIntersect(ray))
                newCol = star::Color(0, 255, 0, 255);
            else
                newCol = star::Color(255, 0, 0, 255);
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

void Volume::createBoundingBox(std::vector<star::Vertex>& verts, std::vector<uint32_t>& inds)
{
    openvdb::math::CoordBBox bbox = baseGrid.get()->evalActiveVoxelBoundingBox();
    openvdb::math::Coord& bmin = bbox.min();
    openvdb::math::Coord& bmax = bbox.max();

    glm::vec3 min{ bmin.x(), bmin.y(), bmin.z() };
    glm::vec3 max{ bmax.x(), bmax.y(), bmax.z() };

    star::GeometryHelpers::calculateAxisAlignedBoundingBox(min, max, verts, inds, true);
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
    newMeshs.emplace_back(std::unique_ptr<star::StarMesh>(new star::StarMesh(*verts, *inds, std::move(material), false)));

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

bool Volume::rayBoxIntersect(const star::Ray& ray)
{
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

    std::array<glm::vec3, 2> bbounds = this->meshes.front()->getBoundingBoxCoords();

    float tmin = 0, tmax = 0, tymin = 0, tymax = 0, tzmin = 0, tzmax = 0;

    tmin = (bbounds[ray.sign[0]].x - ray.org.x) * ray.invDir.x; 
    tmax = (bbounds[!ray.sign[0]].x - ray.org.x) * ray.invDir.x;

    tymin = (bbounds[ray.sign[1]].y - ray.org.y) * ray.invDir.y; 
    tymax = (bbounds[!ray.sign[1]].y - ray.org.y) * ray.invDir.y;

    if ((tmin > tymax) || (tymin > tmax))
        return false;

    if (tymin > tmin)
        tmin = tymin;
    if (tymax < tmax)
        tmax = tymax;

    tzmin = (bbounds[ray.sign[2]].z - ray.org.z) * ray.invDir.z;
    tzmax = (bbounds[!ray.sign[2]].z - ray.org.z) * ray.invDir.z;

    if ((tmin > tzmax) || (tzmin > tmax))
        return false; 

    if (tzmin > tmin)
        tmin = tzmin;
    if (tzmax < tmax)
        tmax = tzmax;

    return tmin <= tmax && tmax >=0; 
}