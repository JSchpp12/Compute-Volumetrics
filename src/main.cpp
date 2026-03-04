#include "util/CmdLine.hpp"

#include <starlight/common/ConfigFile.hpp>

#include <vulkan/vulkan.hpp>
#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include <openvdb/openvdb.h>

#ifdef STAR_ENABLE_PRESENTATION
#include "InteractiveMode.hpp"

int runWindow(std::string &&terrainPath, std::string &&simControllerPath)
{
    try
    {
        InteractiveMode interactiveInstance{};
        return interactiveInstance.run(std::move(terrainPath));
    }
    catch (...)
    {
        std::cerr << "Fatal exception...exiting" << std::endl;
        std::exit(EXIT_FAILURE);
    }
}

#else
#include "HeadlessMode.hpp"
int runHeadless(std::string &&terrainPath, std::string &&simControllerPath)
{
    HeadlessMode headlessInstance{};
    return headlessInstance.run(std::move(terrainPath));
}

#endif

static 

int main(int argc, char **argv)
{
    try
    {
        star::ConfigFile::load("./StarEngine.cfg");
    }
    catch (...)
    {
        std::cerr << "Failed to load config file for engine";
        std::exit(EXIT_FAILURE);
    }

    openvdb::initialize();

#ifdef STAR_ENABLE_PRESENTATION
    return runWindow(util::CmdLine::GetTerrainPath(argc, argv), util::CmdLine::GetSimControllerFilePath(argc, argv));
#else
    return runHeadless(util::CmdLine::GetTerrainPath(argc, argv), util::CmdLine::GetSimControllerFilePath(argc, argv));
#endif
}