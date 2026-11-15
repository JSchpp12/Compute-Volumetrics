// Provides the Vulkan Memory Allocator implementation for the unit-test executable.
// The application defines it in src/main.cpp, which the test target does not link; without this,
// any test that pulls in starlight's buffer/allocator objects fails to link.
#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>
