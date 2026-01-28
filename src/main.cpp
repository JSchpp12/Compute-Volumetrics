#include <starlight/common/ConfigFile.hpp>

#include <vulkan/vulkan.hpp>
#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include <openvdb/openvdb.h>

#ifdef STAR_ENABLE_PRESENTATION
#include "InteractiveMode.hpp"

int runWindow()
{
    InteractiveMode interactiveInstance{};
    return interactiveInstance.run();
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
    star::ConfigFile::load("./StarEngine.cfg");

    openvdb::initialize();

#ifdef STAR_ENABLE_PRESENTATION
    return runWindow();
#else
    return runHeadless();
#endif
}