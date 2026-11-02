#include "config/AppConfigLoader.hpp"
#include "util/CmdLine.hpp"

#include <starlight/core/Exceptions.hpp>

#include <cstdlib>
#include <string>

#include <vulkan/vulkan.hpp>
#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include <openvdb/openvdb.h>

#ifdef STAR_ENABLE_PRESENTATION
#include "InteractiveMode.hpp"
#else
#include "HeadlessMode.hpp"
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
    star::core::install_terminate_handler();

    openvdb::initialize();

#ifdef STAR_ENABLE_PRESENTATION
    InteractiveMode interactiveInstance{};
    return interactiveInstance.run(LoadAppConfig(argc, argv));
#else
    HeadlessMode headlessInstance{};
    return headlessInstance.run(LoadAppConfig(argc, argv));
#endif
}