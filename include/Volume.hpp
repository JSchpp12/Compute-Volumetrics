#pragma once

#include "Color.hpp"
#include "ConfigFile.hpp"
#include "FileHelpers.hpp"
#include "FogInfo.hpp"
#include "GeometryHelpers.hpp"
#include "Light.hpp"
#include "Ray.hpp"
#include "RenderResourceModifier.hpp"
#include "RuntimeUpdateTexture.hpp"
#include "ScreenMaterial.hpp"
#include "StarCamera.hpp"
#include "StarCommandBuffer.hpp"
#include "StarObject.hpp"
#include "VertColorMaterial.hpp"
#include "Vertex.hpp"
#include "VolumeRenderer.hpp"

#include <openvdb/Grid.h>
#include <openvdb/openvdb.h>
#include <openvdb/tools/GridTransformer.h>
#include <openvdb/tools/Interpolation.h>
#include <openvdb/tools/LevelSetSphere.h>
#include <openvdb/tools/RayTracer.h>
#include <stdio.h>
#include <tbb/tbb.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/ext.hpp>
#include <glm/gtx/string_cast.hpp>
#include <random>
#include <string>
#include <thread>

constexpr auto NUM_THREADS = 20;

enum Phase_Function
{
    Henyey_Greenstein
};

class Volume : public star::StarObject
{
  public:
    // class ProcessVolume
    // {
    //     std::vector<std::vector<std::vector<float>>> &sampledGridData;
    //     openvdb::FloatGrid::ConstAccessor myAccessor;
    //     const size_t &myStepSize = 0;
    //     const size_t &halfTotalSteps = 0;

    //   public:
    //     void operator()(const openvdb::CoordBBox &it) const
    //     {
    //         for (auto &coord : it)
    //         {
    //             openvdb::Vec3R finalCoord =
    //                 openvdb::Vec3R(coord.x() * myStepSize, coord.y() * myStepSize, coord.z() * myStepSize);
    //             float result = openvdb::tools::BoxSampler::sample(this->myAccessor, finalCoord);

    //             size_t sampledLocX = coord.x() + halfTotalSteps;
    //             size_t sampledLocY = coord.y() + halfTotalSteps;
    //             size_t sampledLocZ = coord.z() + halfTotalSteps;
    //             sampledGridData[sampledLocX][sampledLocY][sampledLocZ] = result;
    //         }
    //     }

    //     ProcessVolume(openvdb::FloatGrid *volume, std::vector<std::vector<std::vector<float>>> &gridData,
    //                   const size_t &myStepSize, const size_t &halfTotalSteps)
    //         : myAccessor(volume->getConstAccessor()), sampledGridData(gridData), myStepSize(myStepSize),
    //           halfTotalSteps(halfTotalSteps)
    //     {
    //     }
    // };

    bool udpdateVolumeRender = false;
    bool rayMarchToVolumeBoundry = false;
    bool rayMarchToAABB = false;

    virtual ~Volume() = default;
    Volume(star::core::device::DeviceContext &context, std::string vdbPath, const size_t &numFramesInFlight,
           std::shared_ptr<star::StarCamera> camera, const uint32_t &screenWidth, const uint32_t &screenHeight,
           std::vector<std::unique_ptr<star::StarTextures::Texture>> *offscreenRenderToColorImages,
           std::vector<std::unique_ptr<star::StarTextures::Texture>> *offscreenRenderToDepthImages,
           const star::ManagerController::RenderResource::Buffer &sceneCameraInfos,
           const star::ManagerController::RenderResource::Buffer &lightInfos,
           const star::ManagerController::RenderResource::Buffer &lightList);

    // star::Handle buildPipeline(star::core::device::DeviceContext &device,
    //                                                   vk::Extent2D swapChainExtent, vk::PipelineLayout
    //                                                   pipelineLayout, star::core::renderer::RenderingTargetInfo
    //                                                   renderInfo) override;

    // star::core::renderer::RenderingContext buildRenderingContext(star::core::device::DeviceContext &context) override;
    /// <summary>
    /// Expensive, only call when necessary.
    /// </summary>
    void updateGridTransforms();

    virtual void prepRender(star::core::device::DeviceContext &context, const vk::Extent2D &swapChainExtent,
                            const uint8_t &numSwapChainImages, star::StarShaderInfo::Builder fullEngineBuilder,
                            vk::PipelineLayout pipelineLayout,
                            star::core::renderer::RenderingTargetInfo renderingInfo) override;

    virtual void prepRender(star::core::device::DeviceContext &context, const vk::Extent2D &swapChainExtent,
                            const uint8_t &numSwapChainImages, star::StarShaderInfo::Builder fullEngineBuilder,
                            star::Handle sharedPipeline) override;

    virtual void cleanupRender(star::core::device::DeviceContext &context) override;

    virtual bool isRenderReady(star::core::device::DeviceContext &context) override;

    virtual void recordPreRenderPassCommands(vk::CommandBuffer &commandBuffer, const uint8_t &frameInFlightIndex, const uint64_t &frameIndex) override;

    virtual void recordPostRenderPassCommands(vk::CommandBuffer &commandBuffer, const int &frameInFlightIndex) override;

    virtual void frameUpdate(star::core::device::DeviceContext &context, const uint8_t &frameInFlightIndex,
                             const star::Handle &targetCommandBuffer) override;

    void setFogType(const VolumeRenderer::FogType &fogType)
    {
        this->volumeRenderer->setFogType(fogType);
    }

    FogInfo &getFogControlInfo()
    {
        assert(m_fogControlInfo && "Fog control info must be proper");
        return *this->m_fogControlInfo;
    }

  protected:
    std::shared_ptr<star::StarCamera> camera = nullptr;
    std::unique_ptr<VolumeRenderer> volumeRenderer = nullptr;
    std::array<glm::vec4, 2> aabbBounds;
    std::vector<std::unique_ptr<star::StarTextures::Texture>> *offscreenRenderToColorImages = nullptr;
    std::vector<std::unique_ptr<star::StarTextures::Texture>> *offscreenRenderToDepthImages = nullptr;
    std::shared_ptr<FogInfo> m_fogControlInfo = nullptr;

    glm::vec2 screenDimensions{};
    openvdb::FloatGrid::Ptr grid{};

    uint32_t computeQueueFamily = 0;
    uint32_t graphicsQueueFamily = 0;

    std::unordered_map<star::Shader_Stage, star::StarShader> getShaders() override;

    void initVolume(star::core::device::DeviceContext &context, std::string vdbFilePath,
           const star::ManagerController::RenderResource::Buffer &sceneCameraInfos,
           const star::ManagerController::RenderResource::Buffer &lightInfos,
           const star::ManagerController::RenderResource::Buffer &lightList);

    void loadModel(star::core::device::DeviceContext &context, const std::string &filePath);

    void loadGeometry(star::core::device::DeviceContext &context);

    void convertToFog(openvdb::FloatGrid::Ptr &grid);

    std::vector<std::unique_ptr<star::StarMesh>> loadMeshes(star::core::device::DeviceContext &context) override;

    // virtual void recordRenderPassCommands(vk::CommandBuffer &commandBuffer, vk::PipelineLayout &pipelineLayout,
    //                                       int swapChainIndexNum) override;

  private:
    struct RayCamera
    {
        glm::vec2 dimensions{};
        glm::mat4 camDisplayMat{}, camProjMat{};
        double fov_radians = 0;

        RayCamera(const glm::vec2 dimensions, const double &fov_radians, const glm::mat4 &camDisplayMat,
                  const glm::mat4 &camProjMat)
            : dimensions(dimensions), fov_radians(fov_radians), camDisplayMat(camDisplayMat)
        {
        }

        star::Ray getRayForPixel(const size_t &x, const size_t &y) const
        {
            assert(x < this->dimensions.x && y < this->dimensions.y &&
                   "Coordinates must be within dimensions of screen");

            float aspectRatio = dimensions.x / dimensions.y;
            double scale = tan(this->fov_radians);
            glm::vec3 pixelLocCamera{(2 * ((x + 0.5) / this->dimensions.x) - 1) * aspectRatio * scale,
                                     (1 - 2 * ((y + 0.5) / this->dimensions.y)) * scale, -1.0f};

            glm::vec3 origin = this->camDisplayMat * glm::vec4{0, 0, 0, 1};
            glm::vec3 point = this->camDisplayMat * glm::vec4(glm::normalize(pixelLocCamera), 1.0f);
            glm::vec3 direction = point - origin;
            auto normDir = glm::normalize(direction);
            return star::Ray{origin, normDir};
        }
    };

    void updateDependentData(star::core::device::DeviceContext &context, const uint8_t &frameInFlightIndex, const star::Handle &targetCommandBuffer) override;

    static float calcExp(const float &stepSize, const float &sigma);

    static float henyeyGreensteinPhase(const glm::vec3 &viewDirection, const glm::vec3 &lightDirection,
                                       const float &gValue);

    // static void calculateColor(
    //     const std::vector<std::unique_ptr<star::Light>> &lightList, const float &stepSize, const float
    //     &stepSize_light, const int &russianRouletteCutoff, const float &sigma_absorbtion, const float
    //     &sigma_scattering, const float &lightPropertyDir_g, const float &volDensity, const std::array<glm::vec3, 2>
    //     &aabbBounds, openvdb::FloatGrid::Ptr grid, RayCamera camera,
    //     std::vector<std::optional<std::pair<std::pair<size_t, size_t>, star::Color *>>> coordColorWork,
    //     bool marchToaabbIntersection, bool marchToVolBoundry);

    // static bool rayBoxIntersect(const star::Ray &ray, const std::array<glm::vec3, 2> &aabbBounds, float &t0, float
    // &t1);

    // static star::Color forwardMarch(const std::vector<std::unique_ptr<star::Light>> &lightList, const float
    // &stepSize,
    //                                 const float &stepSize_light, const int &russianRouletteCutoff,
    //                                 const float &sigma_absorbtion, const float &sigma_scattering,
    //                                 const float &lightPropertyDir_g, const float &volDensity, const star::Ray &ray,
    //                                 openvdb::FloatGrid::Ptr grid, const std::array<glm::vec3, 2> &aabbHit,
    //                                 const float &t0, const float &t1);

    // static star::Color forwardMarchToVolumeActiveBoundry(const float &stepSize, const star::Ray &ray,
    //                                                      const std::array<glm::vec3, 2> &aabbHit,
    //                                                      openvdb::FloatGrid::Ptr grid, const float &t0,
    //                                                      const float &t1);

    static openvdb::Mat4R getTransform(const glm::mat4 &objectDisplayMat);

    static void RecordQueueFamilyInfo(star::core::device::DeviceContext &context, uint32_t &computeQueueFamilyIndex,
                                      uint32_t &graphicsQueueFamilyIndex);
};
