#include "structs/AppConfig.hpp"
#include "util/CmdLine.hpp"
#include <starlight/common/ConfigFile.hpp>

#include <vulkan/vulkan.hpp>
#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include <openvdb/openvdb.h>

#ifdef STAR_ENABLE_PRESENTATION
#include "InteractiveMode.hpp"

static int runWindow(std::unique_ptr<AppConfig> cfg)
{
    try
    {
        InteractiveMode interactiveInstance{};
        return interactiveInstance.run(std::move(cfg));
    }
    catch (const std::exception &ex)
    {
        std::cerr << "Fatal exception encountered: " << ex.what() << std::endl;
        std::exit(EXIT_FAILURE);
    }
}

#else
#include "HeadlessMode.hpp"
static int runHeadless(std::unique_ptr<AppConfig> cfg)
{
    try
    {
        HeadlessMode headlessInstance{};
        return headlessInstance.run(std::move(cfg));
    }
    catch (const std::exception &ex)
    {
        std::cerr << "Fatal exception encountered: " << ex.what() << std::endl;
        std::exit(EXIT_FAILURE);
    }
}
#endif

int main(int argc, char **argv)
{
    openvdb::initialize();

    auto cfg = std::make_unique<AppConfig>(
        AppConfig{.volumeName = util::CmdLine::GetVolumeDirPath(argc, argv),
                  .terrainDir = util::CmdLine::GetTerrainPath(argc, argv),
                  .engineConfigFile = util::CmdLine::GetConfigFilePath(argc, argv),
                  .simControllerPath = util::CmdLine::GetSimControllerFilePath(argc, argv),
                  .overrideRenderingDevice = util::CmdLine::TryGetDeviceIndexOverride(argc, argv)});
    try
    {
        star::ConfigFile::load(cfg->engineConfigFile);
    }
    catch (...)
    {
        std::cerr << "Failed to load config file for engine";
        std::exit(EXIT_FAILURE);
    }

#ifdef STAR_ENABLE_PRESENTATION
    return runWindow(std::move(cfg));
#else
    return runHeadless(std::move(cfg));
#endif
}