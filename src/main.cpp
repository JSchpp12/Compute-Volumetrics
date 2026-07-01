#include "config/AppConfigLoader.hpp"
#include "util/CmdLine.hpp"

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
    auto [cfg, status] = config::AppConfigLoader::LoadFromArgs(argc, argv);
    if (status != config::LoadStatus::Loaded)
        std::exit(EXIT_FAILURE);

    return std::move(cfg);
}

int main(int argc, char **argv)
{
    openvdb::initialize();
    auto cfg = LoadAppConfig(argc, argv);

#ifdef STAR_ENABLE_PRESENTATION
    return runWindow(std::move(cfg));
#else
    return runHeadless(std::move(cfg));
#endif
}