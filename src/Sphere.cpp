#include "Sphere.hpp"

Sphere::Sphere()
{
    loadModel(); 
}

std::unordered_map<star::Shader_Stage, star::StarShader> Sphere::getShaders()
{
    std::unordered_map<star::Shader_Stage, star::StarShader> shaders; 

    std::string mediaPath = star::ConfigFile::getSetting(star::Config_Settings::mediadirectory); 

    std::string vertPath = mediaPath + "shaders/boundingBox/bounding.vert"; 
    std::string fragPath = mediaPath + "shaders/boundingBox/bounding.frag"; 

    shaders.insert(std::pair<star::Shader_Stage, star::StarShader>(star::Shader_Stage::vertex, star::StarShader(vertPath, star::Shader_Stage::vertex))); 
    shaders.insert(std::pair<star::Shader_Stage, star::StarShader>(star::Shader_Stage::fragment, star::StarShader(fragPath, star::Shader_Stage::fragment)));

    return shaders;
}

std::unique_ptr<star::StarPipeline> Sphere::buildPipeline(star::StarDevice& device, vk::Extent2D swapChainExtent,
    vk::PipelineLayout pipelineLayout, vk::RenderPass renderPass)
{
    star::StarGraphicsPipeline::PipelineConfigSettings settings;
    star::StarGraphicsPipeline::defaultPipelineConfigInfo(settings, swapChainExtent, renderPass, pipelineLayout);

    auto graphicsShaders = this->getShaders();

    auto newPipeline = std::make_unique<star::StarGraphicsPipeline>(device, settings, graphicsShaders.at(star::Shader_Stage::vertex), graphicsShaders.at(star::Shader_Stage::fragment));
    newPipeline->init();

    return std::move(newPipeline);
}

void Sphere::loadModel()
{
    const std::string filePath(star::ConfigFile::getSetting(star::Config_Settings::mediadirectory) + "volumes/sphere.vdb");

    openvdb::initialize();

    openvdb::io::File file(filePath); 
    file.open(); 

    openvdb::GridBase::Ptr baseGrid; 
    for (openvdb::io::File::NameIterator nameIter = file.beginName(); nameIter != file.endName(); ++nameIter) {
        std::cout << nameIter.gridName() << std::endl;

        if (nameIter.gridName() == "ls_sphere") {
            baseGrid = file.readGrid(nameIter.gridName()); 
        }
        else {
            std::cout << "Skipping extra grid: " << nameIter.gridName(); 
        }
    }

    openvdb::math::CoordBBox bbox = baseGrid.get()->evalActiveVoxelBoundingBox(); 
    openvdb::math::Coord& bmin = bbox.min(); 
    openvdb::math::Coord& bmax = bbox.max(); 

    glm::vec3 min{ bmin.x(), bmin.y(), bmin.z()};
    glm::vec3 max{ bmax.x(), bmax.y(), bmax.z() }; 

    std::unique_ptr<std::vector<star::Vertex>> bbVerts = std::make_unique<std::vector<star::Vertex>>(); 
    std::unique_ptr<std::vector<uint32_t>> bbInds = std::make_unique<std::vector<uint32_t>>(); 

    star::GeometryHelpers::calculateAxisAlignedBoundingBox(min, max, *bbVerts, *bbInds); 

    this->meshes.push_back(std::make_unique<star::StarMesh>(*bbVerts, *bbInds, std::make_unique<star::VertColorMaterial>(), false));

    file.close();
}

std::pair<std::unique_ptr<star::StarBuffer>, std::unique_ptr<star::StarBuffer>> Sphere::loadGeometryBuffers(star::StarDevice& device)
{
    return std::pair<std::unique_ptr<star::StarBuffer>, std::unique_ptr<star::StarBuffer>>();
}
