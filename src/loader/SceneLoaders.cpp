#include "loader/SceneLoaders.hpp"

#include "Terrain.hpp"
#include "command/image_metrics/RegisterTerrainRecordInfo.hpp"
#include "util/Color.hpp"

#include <starlight/command/CreateObject.hpp>
#include <starlight/command/detail/create_object/DirectObjCreation.hpp>
#include <starlight/command/detail/create_object/FromObjFileLoader.hpp>
#include <starlight/debug/DebugPrimitives.hpp>
#include <starlight/primitive/SquareObject.hpp>

namespace loader
{
static std::shared_ptr<star::StarObject> LoadTerrain(star::core::device::DeviceContext &ctx,
                                                     const std::filesystem::path &terrainPath)
{
    auto cmd = star::command::CreateObject::Builder()
                   .setLoader(std::make_unique<star::command::create_object::DirectObjCreation>(
                       std::make_shared<Terrain>(ctx, terrainPath.string())))
                   .setUniqueName("terrain")
                   .build();
    ctx.begin().set(cmd).submit();
    cmd.getReply().get()->init(ctx);

    // register the terrain information with the image metric manager for cache

    const auto *terrain = static_cast<const Terrain *>(cmd.getReply().get().get());
    ctx.getCmdBus().submit(image_metrics::RegisterTerrainRecordInfo{}
                               .setTerrainHeightFilePath(terrain->getShapeFilePath())
                               .setTerrainRenderingType(terrain->getTerrainRenderingType()));
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

    return DebugCubeComponent{.cubeInfos = std::move(cubeDesc), .numberOfDebugSquares = static_cast<uint8_t>(colors.size())};
}

static std::shared_ptr<star::StarObject> LoadHorse(star::core::device::DeviceContext &ctx,
                                                   const std::filesystem::path &mediaPath)
{

    auto horsePath = mediaPath / "models" / "horse" / "WildHorse.obj";
    auto cmd = star::command::CreateObject::Builder()
                   .setLoader(std::make_unique<star::command::create_object::FromObjFileLoader>(horsePath.string()))
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
    desc.addObject(LoadTerrain(ctx, terrainPath));
    // desc.addObject(LoadHorse(ctx, mediaDirPath));
    desc.addDebugCube(LoadCube(ctx, numCubes));
    return desc;
}

SceneDescription ReleaseSceneLoader(star::core::device::DeviceContext &ctx, const std::filesystem::path &mediaDirPath,
                                    const std::filesystem::path &terrainPath)
{
    SceneDescription desc;
    desc.addObject(LoadTerrain(ctx, terrainPath));
    return desc;
}
} // namespace loader