#include "loader/SceneLoaders.hpp"

#include "TransmittanceCellPlanner.hpp"
#include "command/image_metrics/RegisterTerrainRecordInfo.hpp"
#include "util/Color.hpp"

#include <array>

#include <star_terrain/TerrainFactory.hpp>
#include <star_terrain/rendering/TerrainGeometryDefinition.hpp>

#include <starlight/ShaderResolver.hpp>
#include <starlight/command/CreateObject.hpp>
#include <starlight/command/FileIO/GetPath.hpp>
#include <starlight/command/detail/create_object/FromObjFileLoader.hpp>
#include <starlight/core/Exceptions.hpp>
#include <starlight/debug/DebugPrimitives.hpp>
#include <starlight/object/BasicObject.hpp>
#include <starlight/primitive/SquareObject.hpp>

namespace loader
{

static std::pair<std::shared_ptr<star::StarObject>, std::shared_ptr<star::StarObject>> LoadTerrain(
    star::core::device::DeviceContext &ctx, const std::filesystem::path &mediaDirPath,
    const std::filesystem::path &terrainPath, bool aggressiveTerrainTextureLoading, bool forceFlatTerrains)
{
    const std::filesystem::path terrainShaderDir = std::filesystem::path{"shaders"} / "terrain" / "standard_geometry";

    star::command::file_io::GetPath terrainPathCmd;
    terrainPathCmd.setCandidate(terrainPath).setKind(star::io::PathKind::directory);
    ctx.getCmdBus().submit(terrainPathCmd);
    const auto resolvedTerrainPath = terrainPathCmd.getReply().get();
    if (!resolvedTerrainPath)
    {
        STAR_THROW("Failed to resolve terrain directory: " + resolvedTerrainPath.error().toString());
    }

    auto geometry = star::terrain::rendering::TerrainGeometryDefinition::Builder(ctx)
                        .setTerrainDir(resolvedTerrainPath.value())
                        .setShapeRepresentation(star::terrain::rendering::Representation::Geometry)
                        .setDefaultRenderType(forceFlatTerrains ? star::terrain::rendering::ElevationType::Flat
                                                                : star::terrain::rendering::ElevationType::Real)
                        .build();

    auto terrains =
        star::terrain::TerrainFactory::buildTerrains(ctx, geometry, terrainShaderDir, aggressiveTerrainTextureLoading);

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
    std::vector<star::primitive::CubeDesc> cubeDesc{colors.size()};
    for (size_t i{0}; i < colors.size(); i++)
    {
        cubeDesc[i] = {.color = colors[i]};
    }

    return DebugCubeComponent{.cubeInfos = std::move(cubeDesc),
                              .numberOfDebugSquares = static_cast<uint8_t>(colors.size())};
}

static TransmittanceVizComponent LoadTransmittanceViz()
{
    const std::array<int, 3> windowSize = transmittance_viz::kDefaultWindowSize;

    std::vector<star::primitive::CubeDesc> cubeDesc{transmittance_viz::WindowCellCount(windowSize)};
    for (size_t i = 0; i < transmittance_viz::WindowCellCount(windowSize); ++i)
    {
        const std::array<int, 3> offset = transmittance_viz::WindowOffsetAt(i, windowSize);
        const glm::vec3 rgb =
            util::HSVToRGB(static_cast<float>(offset[2]) / static_cast<float>(windowSize[2]), 0.85f, 0.9f);

        cubeDesc[i] =
            star::primitive::CubeDesc{.size = glm::vec3{1.0f}, .color = star::Color{rgb.r, rgb.g, rgb.b, 1.0f}};
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

SceneDescription DebugSceneLoader(star::core::device::DeviceContext &ctx, const std::filesystem::path &mediaDirPath,
                                  const std::filesystem::path &terrainPath, bool enableTransmittanceMapDebug,
                                  bool aggressiveTerrainTextureLoading, bool forceFlatTerrains)
{
    constexpr uint8_t numCubes{15};

    SceneDescription desc;
    auto [colorTerrain, shadowMapTerrain] =
        LoadTerrain(ctx, mediaDirPath, terrainPath, aggressiveTerrainTextureLoading, forceFlatTerrains);
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
                                    bool aggressiveTerrainTextureLoading, bool forceFlatTerrains)
{
    SceneDescription desc;
    auto [colorTerrain, shadowMapTerrain] =
        LoadTerrain(ctx, mediaDirPath, terrainPath, aggressiveTerrainTextureLoading, forceFlatTerrains);
    desc.addObject(std::move(colorTerrain));
    // desc.addObject(LoadHorse(ctx, mediaDirPath));
    desc.addShadowObject(std::move(shadowMapTerrain));

    if (enableTransmittanceMapDebug)
        desc.addTransmittanceViz(LoadTransmittanceViz());

    return desc;
}
} // namespace loader