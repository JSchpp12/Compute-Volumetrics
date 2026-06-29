#include "loader/SceneLoaders.hpp"

#include "command/image_metrics/RegisterTerrainRecordInfo.hpp"
#include "util/Color.hpp"

#include <star_terrain/rendering/FromTerrainDirLoader.hpp>
#include <star_terrain/rendering/TerrainObject.hpp>

#include <starlight/command/CreateObject.hpp>
#include <starlight/command/detail/create_object/FromObjFileLoader.hpp>
#include <starlight/debug/DebugPrimitives.hpp>
#include <starlight/object/BasicObject.hpp>
#include <starlight/primitive/SquareObject.hpp>
#include <starlight/ShaderResolver.hpp>

namespace loader
{
static std::shared_ptr<star::StarObject> LoadTerrain(star::core::device::DeviceContext &ctx,
                                                     const std::filesystem::path &mediaDirPath,
                                                     const std::filesystem::path &terrainPath)
{
    star::terrain::TerrainObjectDefinition def{
        .terrainDir = terrainPath,
        .vertShaderPath = mediaDirPath / "shaders" / "default.vert",
        .fragShaderPath = mediaDirPath / "shaders" / "default.frag",
        .renderType = star::terrain::rendering::Type::Real,
    };

    star::ShaderResolver terrainResolver = star::ShaderResolver::Builder{ctx.getCmdBus()}
        .setShader(star::Shader_Stage::vertex, def.vertShaderPath.string())
        .setShader(star::Shader_Stage::fragment, def.fragShaderPath.string())
        .build();

    auto cmd = star::command::CreateObject::Builder()
                   .setLoader(std::make_unique<star::terrain::FromTerrainDirLoader>(ctx, std::move(def)))
                   .setShaderResolver(std::move(terrainResolver))
                   .setUniqueName("terrain")
                   .build();
    ctx.begin().set(cmd).submit();
    cmd.getReply().get()->init(ctx);

    // register the terrain information with the image metric manager for cache
    const auto *terrain = static_cast<const star::terrain::TerrainObject *>(cmd.getReply().get().get());
    ctx.getCmdBus().submit(image_metrics::RegisterTerrainRecordInfo{}
                               .setTerrainHeightFilePath(terrain->getShapeFilePath())
                               .setTerrainRenderingType(terrain->getRenderingType()));
    return cmd.getReply().get();
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
    desc.addObject(LoadTerrain(ctx, mediaDirPath, terrainPath));
    // desc.addObject(LoadHorse(ctx, mediaDirPath));
    desc.addDebugCube(LoadCube(ctx, numCubes));
    return desc;
}

SceneDescription ReleaseSceneLoader(star::core::device::DeviceContext &ctx, const std::filesystem::path &mediaDirPath,
                                    const std::filesystem::path &terrainPath)
{
    SceneDescription desc;
    desc.addObject(LoadTerrain(ctx, mediaDirPath, terrainPath));
    return desc;
}
} // namespace loader