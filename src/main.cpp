#include "config/AppConfigLoader.hpp"
#include "util/CmdLine.hpp"
#include <starlight/common/ConfigFile.hpp>

#include <cstdlib>
#include <iostream>
#include <string>

#include <vulkan/vulkan.hpp>
#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include <openvdb/openvdb.h>

#ifdef STAR_ENABLE_PRESENTATION
#include "InteractiveMode.hpp"

static int runWindow(std::unique_ptr<config::AppConfigInfo> cfg)
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
static int runHeadless(std::unique_ptr<config::AppConfigInfo> cfg)
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

std::unique_ptr<config::AppConfigInfo> LoadAppConfig(int argc, char **argv) noexcept
{
    std::optional<std::string> appConfigPath = util::CmdLine::TryGetAppConfigFilePath(argc, argv);
    auto [cfg, status] = config::AppConfigLoader::LoadFromArgs(argc, argv);
    switch (status)
    {
    case config::LoadStatus::CreatedDefault:
    case config::LoadStatus::ValidationError:
        std::exit(EXIT_FAILURE);
    }

    return std::move(cfg);
}

void initEngine(int argc, char **argv)
{
    const std::string engineConfigPath = util::CmdLine::GetConfigFilePath(argc, argv);
    try
    {
        star::ConfigFile::load(engineConfigPath);
    }
    catch (const std::exception &ex)
    {
        std::cerr << "Failed to load config file for engine: " << ex.what();
        std::exit(EXIT_FAILURE);
    }
}

int main(int argc, char **argv)
{
    openvdb::initialize();
    initEngine(argc, argv);
    auto cfg = LoadAppConfig(argc, argv);

#ifdef STAR_ENABLE_PRESENTATION
    return runWindow(std::move(cfg));
#else
    return runHeadless(std::move(cfg));
#endif
}