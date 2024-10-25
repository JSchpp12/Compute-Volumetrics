#pragma once

#include "RenderingTargetInfo.hpp"
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
#include "VolumeRenderer.hpp"
#include "VolumeRendererCleanup.hpp"
#include "Light.hpp"
#include "Texture.hpp"

#include "ScreenMaterial.hpp"

#include <openvdb/openvdb.h>
#include <openvdb/Grid.h>
#include <openvdb/tools/RayTracer.h>
#include <openvdb/tools/Interpolation.h>
#include <openvdb/tools/GridTransformer.h>
#include <openvdb/tools/LevelSetSphere.h>

#include <glm/ext.hpp>
#include <glm/gtx/string_cast.hpp>

#include <stdio.h>

#include <random>
#include <string>
#include <thread>

constexpr auto NUM_THREADS = 20;

enum Phase_Function {
    Henyey_Greenstein
};

class Volume :
    public star::StarObject
{
public:
    bool udpdateVolumeRender = false;
    bool rayMarchToVolumeBoundry = false;
    bool rayMarchToAABB = false;

    ~Volume() = default;
    Volume(star::StarCamera& camera, const size_t screenWidth, const size_t screenHeight, 
        std::vector<std::unique_ptr<star::Light>>& lightList, 
        std::vector<std::unique_ptr<star::Texture>>* offscreenRenderToColorImages, 
        std::vector<std::shared_ptr<star::GlobalInfo>>& globalInfos, 
        std::vector<std::shared_ptr<star::LightInfo>>& lightInfos)
		: screenDimensions(screenWidth, screenHeight), lightList(lightList), 
		StarObject()
    {
        openvdb::initialize();
        loadModel();

        std::vector<std::vector<star::Color>> colors;
        colors.resize(this->screenDimensions.y);
        for (int y = 0; y < (int)this->screenDimensions.y; y++) {
            colors[y].resize(this->screenDimensions.x);
            for (int x = 0; x < (int)this->screenDimensions.x; x++) {
                if (x == (int)this->screenDimensions.x / 2)
                    colors[y][x] = star::Color(1.0f, 1.0f, 1.0f, 1.0f);
            }
        }

        this->volumeRenderer = std::make_unique<VolumeRenderer>(camera, &this->instanceModelInfos, &this->instanceNormalInfos, offscreenRenderToColorImages, globalInfos, lightInfos, this->aabbBounds);
        this->volumeRendererCleanup = std::make_unique<VolumeRendererCleanup>(this->volumeRenderer->getRenderToImages(), offscreenRenderToColorImages);
    };

    void renderVolume(const double& fov_radians, const glm::vec3& camPosition, const glm::mat4& camDispMatrix, const glm::mat4& camProjMat);

    std::unique_ptr<star::StarPipeline> buildPipeline(star::StarDevice& device, vk::Extent2D swapChainExtent, vk::PipelineLayout pipelineLayout, star::RenderingTargetInfo renderInfo);

    /// <summary>
    /// Expensive, only call when necessary.
    /// </summary>
    void updateGridTransforms();

    void setAbsorbCoef(const float& newCoef) {
        assert(newCoef > 0 && "Coeff must be greater than 0");
        this->sigma_absorbtion = newCoef;
    }
protected:
    std::unique_ptr<VolumeRenderer> volumeRenderer = nullptr; 
	std::unique_ptr<VolumeRendererCleanup> volumeRendererCleanup = nullptr;
    std::array<glm::vec4, 2> aabbBounds; 

    std::vector<std::unique_ptr<star::Light>>& lightList;
    float stepSize = 0.05f, stepSize_light = 0.4f;
    float sigma_absorbtion = 0.001f, sigma_scattering = 0.001f, lightPropertyDir_g = 0.2f;
    float volDensity = 1.0f;
    int russianRouletteCutoff = 4;
    //std::shared_ptr<star::RuntimeUpdateTexture> screenTexture;
    glm::u64vec2 screenDimensions{};
    openvdb::FloatGrid::Ptr grid{};

    std::unordered_map<star::Shader_Stage, star::StarShader> getShaders() override;

    void loadModel();

    virtual std::pair<std::unique_ptr<star::StarBuffer>, std::unique_ptr<star::StarBuffer>> loadGeometryBuffers(star::StarDevice& device) override;

    void convertToFog(openvdb::FloatGrid::Ptr& grid);

    virtual void recordRenderPassCommands(vk::CommandBuffer& commandBuffer, vk::PipelineLayout& pipelineLayout, int swapChainIndexNum) override;
private:
    struct RayCamera {
        glm::vec2 dimensions{};
        glm::mat4 camDisplayMat{}, camProjMat{};
        double fov_radians = 0;

        RayCamera(const glm::vec2 dimensions, const double& fov_radians, const glm::mat4& camDisplayMat, const glm::mat4& camProjMat)
            : dimensions(dimensions), fov_radians(fov_radians), camDisplayMat(camDisplayMat)
        {
        }

        star::Ray getRayForPixel(const size_t& x, const size_t& y) const
        {
            assert(x >= 0 && y >= 0 && "Coordinates must be positive");
            assert(x < this->dimensions.x && y < this->dimensions.y && "Coordinates must be within dimensions of screen");

            float aspectRatio = dimensions.x / dimensions.y;
            double scale = tan(this->fov_radians);
            glm::vec3 pixelLocCamera{
                (2 * ((x + 0.5) / this->dimensions.x) - 1) * aspectRatio * scale,
                (1 - 2 * ((y + 0.5) / this->dimensions.y)) * scale,
                -1.0f
            };

            glm::vec3 origin = this->camDisplayMat * glm::vec4{ 0, 0, 0, 1 };
            auto normPix = glm::normalize(pixelLocCamera); 
            glm::vec3 point = this->camDisplayMat * glm::vec4(glm::normalize(pixelLocCamera), 1.0f);
            glm::vec3 direction = point - origin;
            auto normDir = glm::normalize(direction);
            return star::Ray{ origin, normDir };
        }
    };

    static float calcExp(const float& stepSize, const float& sigma);

    static float henyeyGreensteinPhase(const glm::vec3& viewDirection,
        const glm::vec3& lightDirection, const float& gValue);

    static void calculateColor(const std::vector<std::unique_ptr<star::Light>>& lightList,
        const float& stepSize, const float& stepSize_light, const int& russianRouletteCutoff,
        const float& sigma_absorbtion, const float& sigma_scattering,
        const float& lightPropertyDir_g, const float& volDensity,
        const std::array<glm::vec3, 2>& aabbBounds, openvdb::FloatGrid::Ptr grid,
        RayCamera camera, std::vector<std::optional<std::pair<std::pair<size_t, size_t>, star::Color*>>> coordColorWork,
        bool marchToaabbIntersection, bool marchToVolBoundry);

    static bool rayBoxIntersect(const star::Ray& ray, const std::array<glm::vec3, 2>& aabbBounds,
        float& t0, float& t1);

    static star::Color forwardMarch(const std::vector<std::unique_ptr<star::Light>>& lightList,
        const float& stepSize, const float& stepSize_light, const int& russianRouletteCutoff,
        const float& sigma_absorbtion, const float& sigma_scattering,
        const float& lightPropertyDir_g, const float& volDensity,
        const star::Ray& ray, openvdb::FloatGrid::Ptr grid,
        const std::array<glm::vec3, 2>& aabbHit,
        const float& t0, const float& t1);

    static star::Color forwardMarchToVolumeActiveBoundry(const float& stepSize,
        const star::Ray& ray, const std::array<glm::vec3, 2>& aabbHit,
        openvdb::FloatGrid::Ptr grid, const float& t0, const float& t1);

    static openvdb::Mat4R getTransform(const glm::mat4& objectDisplayMat);
};
