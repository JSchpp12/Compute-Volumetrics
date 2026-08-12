#pragma once

#include "FogType.hpp"
#include "StarCamera.hpp"
#include "core/renderer/FrameData.hpp"
#include "structs/FogInfo.hpp"

#include <starlight/core/renderer/IRenderPhaseProvider.hpp>

#include <star_common/Handle.hpp>

#include <array>
#include <memory>
#include <optional>
#include <string>

namespace star::ManagerController::RenderResource
{
class Buffer;
}

/// Builds a VolumeRenderPhase. Holds the volume setup recipe -- vdb path,
/// camera, aabb, the shared offscreen FrameData, the offscreen render phase
/// whose render-to images the volume samples, and the instance info controllers
/// -- plus the initial fog config the application threads in before the phase
/// is built. build() runs the cold-path setup (the old VolumeRenderer init() +
/// prepRender) on the phase and returns it. The offscreen is reached by its
/// scene phase handle; build() looks it up via the RenderPhaseRegistry to read
/// the offscreen phase's render-to images.
class VolumeRenderPhaseProvider : public star::core::renderer::IRenderPhaseProvider
{
  public:
    VolumeRenderPhaseProvider(star::ManagerController::RenderResource::Buffer *instanceManagerInfo,
                              star::ManagerController::RenderResource::Buffer *instanceNormalInfo,
                              std::shared_ptr<star::core::renderer::FrameData> frameData,
                              star::Handle offscreenPhaseHandle, std::string vdbFilePath,
                              std::shared_ptr<star::StarCamera> camera, const std::array<glm::vec4, 2> &aabbBounds,
                              bool enableCutoffHighlighting);

    virtual ~VolumeRenderPhaseProvider() = default;

    VolumeRenderPhaseProvider(const VolumeRenderPhaseProvider &) = delete;
    VolumeRenderPhaseProvider &operator=(const VolumeRenderPhaseProvider &) = delete;
    VolumeRenderPhaseProvider(VolumeRenderPhaseProvider &&) = default;
    VolumeRenderPhaseProvider &operator=(VolumeRenderPhaseProvider &&) = default;

    /// Initial fog configuration, set by the application before the provider is
    /// handed to the scene (the phase is not built yet). build() applies these
    /// to the phase's fog controller.
    FogInfo &getFogInfo()
    {
        return m_fogInfo;
    }
    void setFogType(Fog::Type type)
    {
        m_fogType = std::move(type);
    }
    void setTransferNeighborHandle(star::Handle handle)
    {
        m_transferNeighborHandle = std::move(handle);
    }
    void setOffscreenPhaseHandle(star::Handle handle)
    {
        m_offscreenPhaseHandle = std::move(handle);
    }
    void setShadowTerrainPhaseHandle(star::Handle handle)
    {
        m_shadowTerrainPhaseHandle = std::move(handle);
    }

    virtual std::unique_ptr<star::core::renderer::RenderPhase> build(
        star::core::device::DeviceContext &context, star::core::renderer::RenderPhaseRegistry &phases) override;

  private:
    star::ManagerController::RenderResource::Buffer *m_infoManagerInstanceModel{nullptr};
    star::ManagerController::RenderResource::Buffer *m_infoManagerInstanceNormal{nullptr};
    std::shared_ptr<star::core::renderer::FrameData> m_frameData;
    star::Handle m_offscreenPhaseHandle{};
    star::Handle m_shadowTerrainPhaseHandle{};
    std::string m_vdbFilePath;
    std::shared_ptr<star::StarCamera> m_camera;
    std::array<glm::vec4, 2> m_aabbBounds;
    bool m_enableCutoffHighlighting = false;

    FogInfo m_fogInfo;
    Fog::Type m_fogType = Fog::Type::sMarched;
    std::optional<star::Handle> m_transferNeighborHandle{std::nullopt};
};