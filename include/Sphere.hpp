#pragma once

#include "StarObject.hpp"
#include "StarCamera.hpp"
#include "ConfigFile.hpp"
#include "GeometryHelpers.hpp"
#include "VertColorMaterial.hpp"

#include <openvdb/openvdb.h>

#include <string>

class Sphere :
    public star::StarObject
{
public:
    ~Sphere() = default; 
    Sphere(); 

    std::unique_ptr<star::StarPipeline> buildPipeline(star::StarDevice& device, vk::Extent2D swapChainExtent, vk::PipelineLayout pipelineLayout, vk::RenderPass renderPass);

protected:
    // Inherited via StarObject
    std::unordered_map<star::Shader_Stage, star::StarShader> getShaders() override;

    void loadModel(); 

};

