#pragma once

#include <starlight/core/device/DeviceContext.hpp>
#include <starlight/wrappers/graphics/StarShaderInfo.hpp>

#include <star_common/Handle.hpp>

#include <vulkan/vulkan.hpp>

#include <memory>

class VolumePipelineBuilder
{
  public:
    VolumePipelineBuilder() = default;

    VolumePipelineBuilder(star::core::device::DeviceContext *context,
                          std::unique_ptr<star::StarShaderInfo> *staticShaderInfo,
                          std::unique_ptr<star::StarShaderInfo> *dynamicShaderInfo,
                          std::unique_ptr<vk::PipelineLayout> *computePipelineLayout,
                          star::Handle *marchedHomogenousPipeline, star::Handle *nanoVDBPipeline_hitBoundingBox,
                          star::Handle *nanoVDBPipeline_surface, star::Handle *marchedPipeline,
                          star::Handle *linearPipeline, star::Handle *expPipeline, star::Handle *initPipeline,
                          vk::Pipeline *cachedInitPipeline, star::Handle *dispatchCmdPipeline,
                          vk::Pipeline *cachedDispatchCmdPipeline);

    int operator()()
    {
        create();
        return 0;
    }

  private:
    void create();

    star::core::device::DeviceContext *m_context{nullptr};
    std::unique_ptr<star::StarShaderInfo> *m_staticShaderInfo{nullptr};
    std::unique_ptr<star::StarShaderInfo> *m_dynamicShaderInfo{nullptr};
    std::unique_ptr<vk::PipelineLayout> *m_computePipelineLayout{nullptr};
    star::Handle *m_marchedHomogenousPipeline{nullptr};
    star::Handle *m_nanoVDBPipeline_hitBoundingBox{nullptr};
    star::Handle *m_nanoVDBPipeline_surface{nullptr};
    star::Handle *m_marchedPipeline{nullptr};
    star::Handle *m_linearPipeline{nullptr};
    star::Handle *m_expPipeline{nullptr};
    star::Handle *m_initPipeline{nullptr};
    vk::Pipeline *m_cachedInitPipeline{nullptr};
    star::Handle *m_dispatchCmdPipeline{nullptr};
    vk::Pipeline *m_cachedDispatchPipeline{nullptr};
};
