#pragma once

#include "StarObject.hpp"
#include "StarCamera.hpp"
#include "ConfigFile.hpp"
#include "Texture.hpp"
#include "GeometryHelpers.hpp"
#include "VertColorMaterial.hpp"
#include "Vertex.hpp"
#include "virtual/ModulePlug/RenderResourceModifier.hpp"
#include "TextureMaterial.hpp"

#include <openvdb/openvdb.h>
#include <openvdb/tools/RayTracer.h>
#include <glm/glm.hpp>

#include <stdio.h>

#include <string>

class Volume :
    public star::StarObject, private star::RenderResourceModifier
{
public:
    ~Volume() = default; 
    Volume(const size_t screenWidth, const size_t screenHeight) 
        : screenDimensions(screenWidth, screenHeight)
    {
        openvdb::initialize();
        loadModel();

        std::vector<std::vector<star::Color>> colors;
        colors.resize(this->screenDimensions.y);
        for (int y = 0; y < (int)this->screenDimensions.y; y++) {
            colors[y].resize(this->screenDimensions.x);
            for (int x = 0; x < (int)this->screenDimensions.x; x++) {
                colors[y][x] = star::Color(255, 0, 0, 0);
            }
        }
        this->screenTexture = std::make_shared<star::Texture>(
            this->screenDimensions.x,
            this->screenDimensions.y,
            colors
        );
    };

    void renderVolume(const glm::vec3& cameraWorldPos, const glm::vec3& cameraRotationDegrees, 
        const double& focalLength, const double& aperature,
        const double& nearPlane, const double& farPlane,
        const float& fov_radians, const glm::mat4& invCamDispMat);

    std::unique_ptr<star::StarPipeline> buildPipeline(star::StarDevice& device, vk::Extent2D swapChainExtent, vk::PipelineLayout pipelineLayout, vk::RenderPass renderPass);

protected:
    std::shared_ptr<star::Texture> screenTexture;
    glm::vec2 screenDimensions{};
    openvdb::GridBase::Ptr baseGrid;
    
    std::unordered_map<star::Shader_Stage, star::StarShader> getShaders() override;

    void loadModel(); 

    virtual void calculateBoundingBox(std::vector<star::Vertex>& verts, std::vector<uint32_t>& inds) override;

    std::pair<std::unique_ptr<star::StarBuffer>, std::unique_ptr<star::StarBuffer>> loadGeometryBuffers(star::StarDevice& device) override;

    void initResources(star::StarDevice& device, const int numFramesInFlight) override;

    void destroyResources(star::StarDevice& device) override;

private:
    struct RayCamera {
        glm::vec2 dimensions{};
        glm::vec3 position{}, rotationDegrees{};
        glm::mat4 invCamDispMat{};
        double nearPlane = 0, farPlane = 0; 
        float fov_radians = 0;

        RayCamera(const glm::vec2 dimensions, const glm::vec3& position, const glm::vec3& rotationDegrees, const double& nearPlane, const double& farPlane, const float& fov_radians, const glm::mat4& invCamDispMat)
            : dimensions(dimensions), position(position), rotationDegrees(rotationDegrees), nearPlane(nearPlane), farPlane(farPlane), fov_radians(fov_radians), invCamDispMat(invCamDispMat)
        {
        }

        glm::vec3 getRayForPixel(const size_t& x, const size_t& y) const
        {
            assert(x >= 0 && y >= 0 && "Coordinates must be positive"); 
            assert(x < this->dimensions.x && y < this->dimensions.y && "Coordinates must be within dimensions of screen");

            float aspectRatio = dimensions.x / dimensions.y;
            float scale = tan(this->fov_radians * 0.5);
            glm::vec2 pixelLocCamera{
                (2 * ((x + 0.5) / this->dimensions.x) - 1) * aspectRatio * scale,
                1 - 2 * ((y + 0.5) / this->dimensions.y) * scale
            };

            glm::vec4 pixelLoc{
                pixelLocCamera.x,
                pixelLocCamera.y,
                -1,
                0.0f
            };

            glm::vec4 result = this->invCamDispMat * pixelLoc;

            return result; 
        }
    };
};

