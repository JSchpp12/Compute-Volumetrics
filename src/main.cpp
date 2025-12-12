#include "Application.hpp"
#include "DefaultFinalRenderControlPolicy.hpp"
#include "StarEngine.hpp"

#include <vulkan/vulkan.hpp>
#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include "ConfigFile.hpp"

int main()
{
    star::ConfigFile::load("./StarEngine.cfg");

    auto application = Application();
    auto engine =
        star::StarEngine<star::DefaultFinalRenderControlPolicy>(star::DefaultFinalRenderControlPolicy{}, application);

    engine.run();
}