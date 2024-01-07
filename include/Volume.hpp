#pragma once

#include "StarObject.hpp"
#include "StarCamera.hpp"
#include "ConfigFile.hpp"
#include "Texture.hpp"
#include "GeometryHelpers.hpp"
#include "VertColorMaterial.hpp"
#include "Vertex.hpp"
#include "virtual/ModulePlug/RenderResourceModifier.hpp"
#include <openvdb/openvdb.h>

#include <string>

class Volume :
    public star::StarObject, private star::RenderResourceModifier
{
public:
    ~Volume() = default; 
    Volume(); 

    std::unique_ptr<star::StarPipeline> buildPipeline(star::StarDevice& device, vk::Extent2D swapChainExtent, vk::PipelineLayout pipelineLayout, vk::RenderPass renderPass);

protected:
    openvdb::GridBase::Ptr baseGrid;

    std::unordered_map<star::Shader_Stage, star::StarShader> getShaders() override;

    void loadModel(); 

    virtual void calculateBoundingBox(std::vector<star::Vertex>& verts, std::vector<uint32_t>& inds) override;

    std::pair<std::unique_ptr<star::StarBuffer>, std::unique_ptr<star::StarBuffer>> loadGeometryBuffers(star::StarDevice& device) override;


    // Inherited via RenderResourceModifier
    void initResources(star::StarDevice& device, const int numFramesInFlight) override;

    void destroyResources(star::StarDevice& device) override;

};

