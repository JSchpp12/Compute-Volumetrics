#pragma once


#include "StarObject.hpp"
#include "StarCamera.hpp"
#include "ConfigFile.hpp"
#include "RuntimeUpdateTexture.hpp"
#include "GeometryHelpers.hpp"
#include "Ray.hpp"
#include "VertColorMaterial.hpp"
#include "StarCommandBuffer.hpp"
#include "Vertex.hpp"
#include "virtual/ModulePlug/RenderResourceModifier.hpp"
#include "TextureMaterial.hpp"

#include <openvdb/openvdb.h>
#include <openvdb/tools/RayTracer.h>
#include <glm/ext.hpp>
#include <glm/gtx/string_cast.hpp>

#include <stdio.h>

#include <string>

class Volume :
    public star::StarObject
{
public:
    ~Volume() = default; 
    Volume(const size_t screenWidth, const size_t screenHeight) 
        : screenDimensions(screenWidth, screenHeight), StarObject()
    {
        openvdb::initialize();
        loadModel();

        std::vector<std::vector<star::Color>> colors;
        colors.resize(this->screenDimensions.y);
        for (int y = 0; y < (int)this->screenDimensions.y; y++) {
            colors[y].resize(this->screenDimensions.x);
            for (int x = 0; x < (int)this->screenDimensions.x; x++) {
                if (x == (int)this->screenDimensions.x / 2)
                    colors[y][x] = star::Color(0, 0, 255, 255);
                else
                    colors[y][x] = star::Color(255, 0, 0, 255);
            }
        }
        this->screenTexture = std::make_shared<star::RuntimeUpdateTexture>(
            this->screenDimensions.x,
            this->screenDimensions.y,
            colors
        );
    };

    void renderVolume(const float& fov_radians, const glm::vec3& camPosition, const glm::mat4& camDispMatrix, const glm::mat4& camProjMat);

    std::unique_ptr<star::StarPipeline> buildPipeline(star::StarDevice& device, vk::Extent2D swapChainExtent, vk::PipelineLayout pipelineLayout, vk::RenderPass renderPass);

protected:
    std::shared_ptr<star::RuntimeUpdateTexture> screenTexture;
    glm::vec2 screenDimensions{};
    openvdb::GridBase::Ptr baseGrid;
    
    std::unordered_map<star::Shader_Stage, star::StarShader> getShaders() override;

    void loadModel(); 

    virtual void createBoundingBox(std::vector<star::Vertex>& verts, std::vector<uint32_t>& inds) override;

    std::pair<std::unique_ptr<star::StarBuffer>, std::unique_ptr<star::StarBuffer>> loadGeometryBuffers(star::StarDevice& device) override;

    void initResources(star::StarDevice& device, const int numFramesInFlight) override;

    void destroyResources(star::StarDevice& device) override;

private:
    struct RayCamera {
        glm::vec2 dimensions{};
        glm::mat4 camDisplayMat{}, camProjMat{};
        float fov_radians = 0;

        RayCamera(const glm::vec2 dimensions, const float& fov_radians, const glm::mat4& camDisplayMat, const glm::mat4& camProjMat)
            : dimensions(dimensions), fov_radians(fov_radians), camDisplayMat(camDisplayMat)
        {
        }

        star::Ray getRayForPixel(const size_t& x, const size_t& y, const glm::vec3 camPosition) const
        {
            assert(x >= 0 && y >= 0 && "Coordinates must be positive");
            assert(x < this->dimensions.x && y < this->dimensions.y && "Coordinates must be within dimensions of screen");

            float aspectRatio = dimensions.x / dimensions.y;
            float scale = tan(this->fov_radians * 0.5);
            glm::vec4 pixelLocCamera{
                (2 * ((x + 0.5) / this->dimensions.x) - 1) * aspectRatio * scale,
                1 - 2 * ((y + 0.5) / this->dimensions.y) * scale,
                -1.0f,
                1.0f
            };

            glm::vec4 origin = this->camDisplayMat * glm::vec4{ 0,0,0,1 };
            glm::vec4 point = this->camDisplayMat * pixelLocCamera;

            return star::Ray{ origin, glm::normalize(point - origin) };
        }
    };

    bool rayBoxIntersect(const star::Ray& ray);
};

