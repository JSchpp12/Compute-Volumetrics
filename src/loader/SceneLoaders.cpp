#include "loader/SceneLoaders.hpp"

#include "command/image_metrics/RegisterTerrainRecordInfo.hpp"
#include "util/Color.hpp"

#include <star_terrain/rendering/FromTerrainDirLoader.hpp>
#include <star_terrain/rendering/TerrainObject.hpp>

#include <starlight/ShaderResolver.hpp>
#include <starlight/command/CreateObject.hpp>
#include <starlight/command/detail/create_object/FromObjFileLoader.hpp>
#include <starlight/debug/DebugPrimitives.hpp>
#include <starlight/object/BasicObject.hpp>
#include <starlight/primitive/SquareObject.hpp>

namespace loader
{

static std::shared_ptr<star::StarObject> SubmitInitTerrain(star::core::device::DeviceContext &ctx,
                                                           star::terrain::TerrainGeometryDefinition geomDef,
                                                           std::filesystem::path vertShader,
                                                           std::filesystem::path fragShader, std::string name,
                                                           bool useGreyscale)
{
    star::terrain::TerrainObjectDefinition def{.geometry = std::move(geomDef),
                                               .vertShaderPath = std::move(vertShader),
                                               .fragShaderPath = std::move(fragShader),
                                               .colorMode = useGreyscale ? star::terrain::ColoringMode::greyscale
                                                                         : star::terrain::ColoringMode::color};

    star::ShaderResolver terrainResolver = star::ShaderResolver::Builder{ctx.getCmdBus()}
                                               .setShader(star::Shader_Stage::vertex, def.vertShaderPath.string())
                                               .setShader(star::Shader_Stage::fragment, def.fragShaderPath.string())
                                               .build();

    auto cmd = star::command::CreateObject::Builder()
                   .setLoader(std::make_unique<star::terrain::FromTerrainDirLoader>(ctx, std::move(def)))
                   .setShaderResolver(std::move(terrainResolver))
                   .setUniqueName(std::move(name))
                   .build();
    ctx.begin().set(cmd).submit();
    cmd.getReply().get()->init(ctx);
    return cmd.getReply().get();
}

static std::pair<std::shared_ptr<star::StarObject>, std::shared_ptr<star::StarObject>> LoadTerrain(
    star::core::device::DeviceContext &ctx, const std::filesystem::path &mediaDirPath,
    const std::filesystem::path &terrainPath)
{
    // create terrain geometry definition
    auto geometry = star::terrain::TerrainGeometryDefinition::Builder(ctx)
                        .setTerrainDir(terrainPath)
                        .setRenderType(star::terrain::rendering::Type::Real)
                        .build();

    const std::filesystem::path terrainShaderDir = mediaDirPath / "shaders" / "terrain";
    // share between them
    auto colorTerrain = SubmitInitTerrain(ctx, geometry, terrainShaderDir / "color.vert",
                                          terrainShaderDir / "color.frag", "terrain_color", false);
    auto shadowTerrain = SubmitInitTerrain(ctx, geometry, terrainShaderDir / "shadow_cast.vert",
                                           terrainShaderDir / "shadow_cast.frag", "terrain_depth", true);

    // register the terrain information with the image metric manager for cache
    const auto *terrain = static_cast<const star::terrain::TerrainObject *>(colorTerrain.get());
    ctx.getCmdBus().submit(image_metrics::RegisterTerrainRecordInfo{}
                               .setTerrainHeightFilePath(terrain->getShapeFilePath())
                               .setTerrainRenderingType(terrain->getRenderingType()));

    return std::make_pair(std::move(colorTerrain), std::move(shadowTerrain));
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
    std::vector<star::primitive::CubeDesc> cubeDesc;
    std::vector<star::Color> colors = CreateNeonColors(numToCreate);
    cubeDesc.reserve(colors.size());
    for (const auto &color : colors)
    {
        cubeDesc.push_back({.color = color});
    }

    return DebugCubeComponent{.cubeInfos = std::move(cubeDesc),
                              .numberOfDebugSquares = static_cast<uint8_t>(colors.size())};
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
                                  const std::filesystem::path &terrainPath)
{
    constexpr uint8_t numCubes{15};

    SceneDescription desc;
    auto [colorTerrain, shadowMapTerrain] = LoadTerrain(ctx, mediaDirPath, terrainPath);
    desc.addObject(std::move(colorTerrain));
    desc.addObject(LoadHorse(ctx, mediaDirPath));
    desc.addShadowObject(std::move(shadowMapTerrain));
    desc.addDebugCube(LoadCube(ctx, numCubes));
    return desc;
}

SceneDescription ReleaseSceneLoader(star::core::device::DeviceContext &ctx, const std::filesystem::path &mediaDirPath,
                                    const std::filesystem::path &terrainPath)
{
    SceneDescription desc;
    auto [colorTerrain, shadowMapTerrain] = LoadTerrain(ctx, mediaDirPath, terrainPath);
    desc.addObject(std::move(colorTerrain));
    desc.addObject(LoadHorse(ctx, mediaDirPath));
    desc.addShadowObject(std::move(shadowMapTerrain));
    return desc;
}
} // namespace loader