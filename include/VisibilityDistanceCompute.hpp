#pragma once

#include "FogType.hpp"

#include <StarShaderInfo.hpp>
#include <starlight/core/device/DeviceContext.hpp>

class VisibilityDistanceCompute
{
  public:
    void prepRender(star::core::device::DeviceContext &context);

    void recordCommandBuffer(vk::CommandBuffer commandBuffer, const glm::uvec2 &workgroupSize,
                             const star::StarShaderInfo &volumeShaderInfo, Fog::Type type);

  private:
    star::Handle m_pipelineMarched;

    void buildPipelines(star::core::device::manager::GraphicsContainer &graphicsManagers);
    // void addPreComputeMemoryBarriers(vk::CommandBuffer &cmdBuff, const star::common::FrameTracker &ft,
    //                                  const bool getBuffersBackFromTransfer) const;

    // void addPostComputeMemoryBarriers(vk::CommandBuffer &cmdBuff, const star::common::FrameTracker &ft,
    //                                   const bool giveBuffersToTransfer) const;

    // std::vector<star::StarBuffers::Buffer> createComputeWriteToBuffers(star::core::device::DeviceContext &context,
    //                                                                    const vk::Extent2D &screenSize,
    //                                                                    const size_t &dataTypeSize,
    // const std::string &debugName,
    // const size_t &numToCreate) const;
};