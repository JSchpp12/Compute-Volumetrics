#include <starlight/common/ConfigFile.hpp>

#include <vulkan/vulkan.hpp>
#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include <openvdb/openvdb.h>

#ifdef STAR_ENABLE_PRESENTATION
#include "InteractiveMode.hpp"

int runWindow()
{
    try
    {
        InteractiveMode interactiveInstance{};
        return interactiveInstance.run();
    }
    catch (...)
    {
        std::cerr << "Fatal exception...exiting" << std::endl;
        std::exit(EXIT_FAILURE);
    }
}

#else
#include "HeadlessMode.hpp"
int runHeadless()
{
    HeadlessMode headlessInstance{};
    return headlessInstance.run();
}

#endif

int main()
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
    return runWindow();
#else
    return runHeadless();
#endif
}