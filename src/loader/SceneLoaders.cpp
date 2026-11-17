#include "loader/SceneLoaders.hpp"

#include "TransmittanceCellPlanner.hpp"
#include "command/image_metrics/RegisterTerrainRecordInfo.hpp"
#include "command/sim_controller/GetSimulationCameraInfo.hpp"
#include "filter/SimulationViewCoverageFilter.hpp"
#include "util/Color.hpp"
#include "util/FieldOfView.hpp"

#include <array>
#include <fstream>
#include <memory>
#include <sstream>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>
#include <star_terrain/TerrainFactory.hpp>
#include <star_terrain/command/GetAllTerrainLibraryRegistrations.hpp>
#include <star_terrain/command/GetLibraryCartForTerrain.hpp>
#include <star_terrain/file_data/CoverageInfo.hpp>
#include <star_terrain/file_data/CoverageInfo_json.hpp>
#include <star_terrain/io/IFilter.hpp>
#include <star_terrain/io/LibraryCart.hpp>
#include <star_terrain/rendering/TerrainGeometryDefinition.hpp>

#include <star_skybox/factory/SkyboxFactory.hpp>
#include <star_skybox/maker/MakeSkybox.hpp>
#include <starlight/ShaderResolver.hpp>
#include <starlight/command/CreateObject.hpp>
#include <starlight/command/detail/create_object/FromObjFileLoader.hpp>
#include <starlight/common/materials/TextureMaterial.hpp>
#include <starlight/core/Exceptions.hpp>
#include <starlight/core/logging/LoggingFactory.hpp>
#include <starlight/debug/DebugPrimitives.hpp>
#include <starlight/material/MaterialProvider.hpp>
#include <starlight/object/BasicObject.hpp>
#include <starlight/primitive/SquareObject.hpp>
#include <starlight/virtual/StarCamera.hpp>

namespace loader
{

struct ResolvedTerrainCart
{
    std::filesystem::path terrainRoot;
    std::filesystem::path shapeFilePath;
    std::unique_ptr<star::terrain::LibraryCart> cart;
};

/// @brief Finds the registration for the terrain the scene was configured with.
static const star::terrain::command::TerrainLibraryRegistration *FindTerrainRegistrationForPath(
    const std::vector<star::terrain::command::TerrainLibraryRegistration> &registrations,
    const std::filesystem::path &terrainPath)
{
    for (const auto &registration : registrations)
    {
        if (registration.terrainRootPath == terrainPath)
            return &registration;
    }

    const std::string filename = terrainPath.filename().string();
    for (const auto &registration : registrations)
    {
        if (registration.terrainRootPath.filename().string() == filename)
            return &registration;
    }

    return nullptr;
}

static star::terrain::command::GetLibraryCartForTerrainResult GetPendingLibraryCart(
    star::core::device::DeviceContext &ctx, const star::Handle &registration,
    std::vector<std::unique_ptr<star::terrain::IFilter>> filters)
{
    using namespace star::terrain::command;

    GetLibraryCartForTerrain cartCommand;
    cartCommand.setHandle(registration).setFilters(std::move(filters));
    ctx.getCmdBus().submit(cartCommand);
    return cartCommand.getReply().get();
}

/// @brief Reads the geographic center (lat, lon) of the terrain from its shape descriptor.
static glm::dvec2 ReadTerrainOriginLatLon(const std::filesystem::path &shapeFilePath)
{
    std::ifstream shapeStream(shapeFilePath);
    if (!shapeStream.is_open())
    {
        STAR_THROW("Failed to open terrain shape file: " + shapeFilePath.string());
    }

    nlohmann::json shapeJson;
    shapeStream >> shapeJson;
    return shapeJson.get<star::terrain::CoverageInfo>().center;
}

/// @brief Builds the filters limiting terrain chunks to the area the simulation views.
static std::vector<std::unique_ptr<star::terrain::IFilter>> BuildTerrainFilters(
    star::core::device::DeviceContext &ctx, const std::filesystem::path &shapeFilePath, const star::StarCamera &camera)
{
    std::vector<std::unique_ptr<star::terrain::IFilter>> filters;

    sim_controller::GetSimulationCameraInfo sweepInfoCommand;
    ctx.getCmdBus().submit(sweepInfoCommand);
    const auto sweepInfo = sweepInfoCommand.getReply().get();
    if (!sweepInfo.has_value())
    {
        star::core::logging::warning(
            "Simulation controller did not provide camera sweep info; loading terrain without view filtering");
        return filters;
    }

    // The simulation drives the camera along its configured start direction when one is given; otherwise it keeps the
    // camera looking where the camera already looks.
    const glm::vec3 viewDirection = sweepInfo->startViewDirection.has_value() ? sweepInfo->startViewDirection.value()
                                                                              : glm::vec3(camera.getForwardVector());
    const glm::vec3 cameraPosition =
        sweepInfo->startCameraPosition.has_value() ? sweepInfo->startCameraPosition.value() : camera.getPosition();
    const float horizontalFieldOfViewDegrees = camera.getHorizontalFieldOfView();

    const double viewAzimuth = util::GetViewAzimuthRadians(viewDirection);
    const double viewSpanDegrees =
        util::GetViewSpanDegrees(sweepInfo->rotationSpanDegrees, horizontalFieldOfViewDegrees);

    filters.push_back(std::make_unique<filter::SimulationViewCoverageFilter>(
        filter::FilterRange{
            .center = {cameraPosition.x, cameraPosition.z},
            .viewDistance = camera.getFarClippingDistance(),
            .startAzimuth = util::GetLeadingEdgeAzimuthRadians(viewAzimuth, horizontalFieldOfViewDegrees),
            .sweptAngle = glm::radians(viewSpanDegrees),
        },
        ReadTerrainOriginLatLon(shapeFilePath)));

    return filters;
}

static star::terrain::LibraryCartReadResult GetLibraryCart(star::terrain::PendingLibraryCart &pendingCart)
{
    return pendingCart.getLibraryCart();
}

/// @brief Queries the terrain library for the configured terrain and blocks for its cart.
static ResolvedTerrainCart ResolveTerrainCart(star::core::device::DeviceContext &ctx,
                                              const std::filesystem::path &terrainPath, const star::StarCamera &camera,
                                              bool enableTerrainFilteringToSimulationBounds)
{
    using namespace star::terrain::command;

    GetAllTerrainLibraryRegistrations listCommand;
    ctx.getCmdBus().submit(listCommand);
    const auto registrations = listCommand.getReply().get();

    if (registrations.scanError.has_value())
    {
        std::ostringstream oss;
        oss << "Terrain library scan failed: " << *registrations.scanError;
        STAR_THROW(oss.str());
    }

    const auto *registration = FindTerrainRegistrationForPath(registrations.registrations, terrainPath);
    if (registration == nullptr)
    {
        std::ostringstream oss;
        oss << "Configured terrain '" << terrainPath.string() << "' is not in the terrain library";
        STAR_THROW(oss.str());
    }
    if (!registration->error.empty())
    {
        std::ostringstream oss;
        oss << "Terrain library entry '" << registration->displayName << "' failed: " << registration->error;
        STAR_THROW(oss.str());
    }

    const std::filesystem::path shapeFilePath = registration->terrainRootPath / registration->terrainShapeFile;

    std::vector<std::unique_ptr<star::terrain::IFilter>> terrainFilters;
    if (enableTerrainFilteringToSimulationBounds)
        terrainFilters = BuildTerrainFilters(ctx, shapeFilePath, camera);

    auto pendingCart = GetPendingLibraryCart(ctx, registration->handle, std::move(terrainFilters));
    if (!pendingCart)
    {
        std::ostringstream oss;
        oss << "Failed to create library cart for '" << registration->displayName
            << "': " << pendingCart.error().what();
        STAR_THROW(oss.str());
    }

    auto cart = pendingCart.value().getLibraryCart();
    if (!cart)
    {
        std::ostringstream oss;
        oss << "Failed to read library cart for '" << registration->displayName << "': " << cart.error().what();
        STAR_THROW(oss.str());
    }

    return ResolvedTerrainCart{
        .terrainRoot = registration->terrainRootPath, .shapeFilePath = shapeFilePath, .cart = std::move(cart.value())};
}

static std::pair<std::shared_ptr<star::StarObject>, std::shared_ptr<star::StarObject>> LoadTerrain(
    star::core::device::DeviceContext &ctx, const std::filesystem::path &terrainPath,
    bool aggressiveTerrainTextureLoading, bool forceFlatTerrains, bool enableTerrainFilteringToSimulationBounds,
    const star::StarCamera &camera)
{
    const std::filesystem::path terrainShaderDir = std::filesystem::path{"shaders"} / "terrain" / "standard_geometry";

    ResolvedTerrainCart resolved =
        ResolveTerrainCart(ctx, terrainPath, camera, enableTerrainFilteringToSimulationBounds);

    const star::terrain::rendering::TerrainGeometryDefinition geometry{
        .terrainDir = resolved.terrainRoot,
        .shapeRepresentation = star::terrain::rendering::Representation::Geometry,
        .defaultRenderType = forceFlatTerrains ? star::terrain::rendering::ElevationType::Flat
                                               : star::terrain::rendering::ElevationType::Real,
    };

    auto terrains = star::terrain::TerrainFactory::buildTerrains(ctx, geometry, resolved.cart->info(),
                                                                 resolved.cart->chunks(), resolved.shapeFilePath,
                                                                 terrainShaderDir, aggressiveTerrainTextureLoading);

    // register the terrain information with the image metric manager for cache
    ctx.getCmdBus().submit(image_metrics::RegisterTerrainRecordInfo{}
                               .setTerrainHeightFilePath(terrains.shapeFilePath)
                               .setTerrainRenderingType(terrains.defaultRenderingType));

    return std::make_pair(std::move(terrains.colorTerrain), std::move(terrains.greyscaleTerrain));
}

static std::vector<star::Color> CreateNeonColors(std::size_t count)
{
    std::vector<star::Color> colors;
    colors.reserve(count);

    if (count == 0)
        return colors;

    constexpr float saturation = 1.0f;
    constexpr float value = 1.0f;

    for (std::size_t i = 0; i < count; ++i)
    {
        float hue = static_cast<float>(i) / static_cast<float>(count);
        glm::vec3 rgb = util::HSVToRGB(hue, saturation, value);

        colors.push_back({rgb.r, rgb.g, rgb.b, 1.0f});
    }

    return colors;
}

static DebugCubeComponent LoadCube(star::core::device::DeviceContext &ctx, size_t numToCreate)
{
    std::vector<star::Color> colors = CreateNeonColors(numToCreate);
    std::vector<star::primitive::cube::CubeDesc> cubeDesc{colors.size()};
    for (size_t i{0}; i < colors.size(); i++)
    {
        cubeDesc[i] = {.coloringApproach = star::primitive::cube::SolidColor{colors[i]}};
    }

    return DebugCubeComponent{.cubeInfos = std::move(cubeDesc),
                              .numberOfDebugSquares = static_cast<uint8_t>(colors.size())};
}

static TransmittanceVizComponent LoadTransmittanceViz()
{
    const std::array<int, 3> windowSize = transmittance_viz::kDefaultWindowSize;

    std::vector<star::primitive::cube::CubeDesc> cubeDesc{transmittance_viz::WindowCellCount(windowSize)};
    for (size_t i = 0; i < transmittance_viz::WindowCellCount(windowSize); ++i)
    {
        const std::array<int, 3> offset = transmittance_viz::WindowOffsetAt(i, windowSize);
        const glm::vec3 rgb =
            util::HSVToRGB(static_cast<float>(offset[2]) / static_cast<float>(windowSize[2]), 0.85f, 0.9f);

        cubeDesc[i] = star::primitive::cube::CubeDesc{
            .coloringApproach = star::primitive::cube::SolidColor{star::Color{rgb.r, rgb.g, rgb.b, 1.0f}}};
    }

    return TransmittanceVizComponent{.cubeInfos = std::move(cubeDesc), .windowSize = windowSize};
}

static std::shared_ptr<star::StarObject> LoadHorse(star::core::device::DeviceContext &ctx,
                                                   const std::filesystem::path &mediaPath)
{
    auto horsePath = mediaPath / "models" / "horse" / "WildHorse.obj";
    star::ShaderResolver horseResolver = star::BasicObject::PrepareResolver(horsePath.string(), ctx.getCmdBus());
    auto cmd = star::command::CreateObject::Builder()
                   .setLoader(std::make_unique<star::command::create_object::FromObjFileLoader>(horsePath.string()))
                   .setShaderResolver(std::move(horseResolver))
                   .setUniqueName("horse")
                   .build();
    ctx.begin().set(cmd).submit();
    cmd.getReply().get()->init(ctx);
    return cmd.getReply().get();
}

std::shared_ptr<star::StarObject> LoadSkybox(star::core::device::DeviceContext &ctx,
                                             star::skybox::SkyboxContext &skyboxContext,
                                             const std::filesystem::path &skyboxTexturePath,
                                             star::skybox::SkyboxInfo info)
{
    using namespace star::primitive;

    auto skybox = star::skybox::maker::MakeSkybox(ctx, skyboxContext)
                      .setTexture(skyboxTexturePath.string())
                      .setInfo(std::move(info))
                      .useGPUSampling()
                      .make();

    std::vector<cube::CubeDesc> desc{cube::CubeDesc{.coloringApproach = cube::TextureColor{skybox.resource}}};

    const auto skyShadersPath = std::filesystem::path("shaders") / "skybox";
    const auto vertShader = skyShadersPath / "skybox.vert";
    const auto fragShader = skyShadersPath / "skybox.frag";

    std::vector<star::setup::TransferWait> waits;
    if (skybox.wait.has_value())
        waits.push_back(std::move(*skybox.wait));

    std::vector<star::ReadinessCondition> conditions;
    if (skybox.condition.has_value())
        conditions.push_back(std::move(*skybox.condition));

    std::vector<std::unique_ptr<star::IMaterialProvider>> providers;
    providers.push_back(
        std::make_unique<star::MaterialProvider>(std::make_unique<star::TextureMaterial>(skybox.resource),
                                                 std::move(waits), std::move(skybox.prep), std::move(conditions)));

    return std::make_shared<CubeObject>(desc, std::move(providers),
                                        star::ShaderResolver::Builder(ctx.getCmdBus())
                                            .setShader(star::Shader_Stage::vertex, vertShader.string())
                                            .setShader(star::Shader_Stage::fragment, fragShader.string())
                                            .build(),
                                        star::skybox::factory::CreateSkyboxRenderOverrides());
}

SceneDescription DebugSceneLoader(star::core::device::DeviceContext &ctx, const std::filesystem::path &mediaDirPath,
                                  const std::filesystem::path &terrainPath, bool enableTransmittanceMapDebug,
                                  bool aggressiveTerrainTextureLoading, bool forceFlatTerrains,
                                  bool enableTerrainFilteringToSimulationBounds, const star::StarCamera &camera)
{
    constexpr uint8_t numCubes{15};

    SceneDescription desc;
    auto [colorTerrain, shadowMapTerrain] =
        LoadTerrain(ctx, terrainPath, aggressiveTerrainTextureLoading, forceFlatTerrains,
                    enableTerrainFilteringToSimulationBounds, camera);
    desc.addObject(std::move(colorTerrain));
    // desc.addObject(LoadHorse(ctx, mediaDirPath));
    desc.addShadowObject(std::move(shadowMapTerrain));
    desc.addDebugCube(LoadCube(ctx, numCubes));

    if (enableTransmittanceMapDebug)
        desc.addTransmittanceViz(LoadTransmittanceViz());

    return desc;
}

SceneDescription ReleaseSceneLoader(star::core::device::DeviceContext &ctx, const std::filesystem::path &mediaDirPath,
                                    const std::filesystem::path &terrainPath, bool enableTransmittanceMapDebug,
                                    bool aggressiveTerrainTextureLoading, bool forceFlatTerrains,
                                    bool enableTerrainFilteringToSimulationBounds, const star::StarCamera &camera)
{
    SceneDescription desc;
    auto [colorTerrain, shadowMapTerrain] =
        LoadTerrain(ctx, terrainPath, aggressiveTerrainTextureLoading, forceFlatTerrains,
                    enableTerrainFilteringToSimulationBounds, camera);
    desc.addObject(std::move(colorTerrain));
    desc.addShadowObject(std::move(shadowMapTerrain));
    // desc.addObject(LoadHorse(ctx, mediaDirPath));

    if (enableTransmittanceMapDebug)
        desc.addTransmittanceViz(LoadTransmittanceViz());

    return desc;
}
} // namespace loader