#include "Volume.hpp"

Volume::Volume()
{
    openvdb::initialize();
    loadModel(); 
}

std::unordered_map<star::Shader_Stage, star::StarShader> Volume::getShaders()
{
    std::unordered_map<star::Shader_Stage, star::StarShader> shaders; 

    std::string mediaPath = star::ConfigFile::getSetting(star::Config_Settings::mediadirectory); 

    std::string vertPath = mediaPath + "shaders/boundingBox/bounding.vert"; 
    std::string fragPath = mediaPath + "shaders/boundingBox/bounding.frag"; 

    shaders.insert(std::pair<star::Shader_Stage, star::StarShader>(star::Shader_Stage::vertex, star::StarShader(vertPath, star::Shader_Stage::vertex))); 
    shaders.insert(std::pair<star::Shader_Stage, star::StarShader>(star::Shader_Stage::fragment, star::StarShader(fragPath, star::Shader_Stage::fragment)));

    return shaders;
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
    const std::string filePath(star::ConfigFile::getSetting(star::Config_Settings::mediadirectory) + "volumes/Volume.vdb");

    openvdb::io::File file(filePath);

    file.open();

    for (openvdb::io::File::NameIterator nameIter = file.beginName(); nameIter != file.endName(); ++nameIter) {
        std::cout << nameIter.gridName() << std::endl;

        if (nameIter.gridName() == "ls_Volume") {
            baseGrid = file.readGrid(nameIter.gridName());
        }
        else {
            std::cout << "Skipping extra grid: " << nameIter.gridName();
        }
    }


    file.close();
}

void Volume::calculateBoundingBox(std::vector<star::Vertex>& verts, std::vector<uint32_t>& inds)
{

    openvdb::math::CoordBBox bbox = baseGrid.get()->evalActiveVoxelBoundingBox();
    openvdb::math::Coord& bmin = bbox.min();
    openvdb::math::Coord& bmax = bbox.max();

    glm::vec3 min{ bmin.x(), bmin.y(), bmin.z() };
    glm::vec3 max{ bmax.x(), bmax.y(), bmax.z() };

    star::GeometryHelpers::calculateAxisAlignedBoundingBox(min, max, verts, inds);
}

std::pair<std::unique_ptr<star::StarBuffer>, std::unique_ptr<star::StarBuffer>> Volume::loadGeometryBuffers(star::StarDevice& device)
{
    return std::pair<std::unique_ptr<star::StarBuffer>, std::unique_ptr<star::StarBuffer>>();
}

void Volume::initResources(star::StarDevice& device, const int numFramesInFlight)
{

}

void Volume::destroyResources(star::StarDevice& device)
{
}
