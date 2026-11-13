#pragma once

#include <starlight/policy/event/ListenForMaterialSwapCommitted.hpp>
#include <starlight/policy/event/ListenForMaterialSwapRolledBack.hpp>

#include <star_common/Handle.hpp>

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace star
{
class StarObject;
}
namespace star::core::device
{
class DeviceContext;
}
namespace star::skybox
{
class SkyboxContext;
}

/// @brief App-owned runtime skybox selection and creation: cached library registrations, the initial object, and
/// pending selections.
/// @details The library scan and sidecar reads live in `star::skybox::service::SkyboxFileService`; the service is
/// passive. This controller creates the initial skybox object, caches the registration view, and asks the service for
/// one entry's info when applying a later swap. Not thread-safe: all methods run on the render/main thread.
class SkyboxController
{
  public:
    /// @brief GUI-facing view of one library entry (handle + label/path).
    struct EntryView
    {
        star::Handle handle{};
        std::string displayName;
        std::filesystem::path texturePath;
    };

    SkyboxController() : m_listenForRollback(*this), m_listenForCommitted(*this)
    {
    }
    ~SkyboxController() = default;
    SkyboxController(const SkyboxController &) = delete;
    SkyboxController &operator=(const SkyboxController &) = delete;
    SkyboxController(SkyboxController &&) = delete;
    SkyboxController &operator=(SkyboxController &&) = delete;

    /// @brief Creates the initial skybox object and caches the library registrations.
    /// @details Resolves the registration matching @p configuredTexturePath, builds its resource through
    /// loader::LoadSkybox, and returns the renderable object. Must run before the scene's render phases are built so
    /// the object joins the offscreen phase. This controller is the only caller that creates the initial skybox; later
    /// selections swap the same object's material. The first call blocks on the library scan and sidecar reads.
    /// @throws When the library is empty or the configured entry cannot be resolved.
    std::shared_ptr<star::StarObject> createSkybox(star::core::device::DeviceContext &context,
                                                   star::skybox::SkyboxContext &skyboxContext,
                                                   const std::filesystem::path &configuredTexturePath);

    /// @brief Stores the pass that renders the skybox and subscribes to swap-rollback notifications.
    /// @details Later swaps register their transfer waits and one-shot setup on this pass. The controller must be
    /// detached before the device's event bus is destroyed.
    void attachSkybox(star::core::device::DeviceContext &context, const star::Handle &passCommandBuffer);

    /// @brief Unsubscribe from the device event bus (call before the DeviceContext is cleaned up).
    void detachSkybox(star::common::EventBus &eventBus);

    /// @brief Applies a pending selection, if one exists.
    /// @details Must run after the owning pass has been triggered for this frame. Resolves the requested entry's info
    /// through the service, builds the material, and submits the swap. Updates activeHandle() only when
    /// MaterialService accepts the swap. Rejects the selection while an earlier swap is still awaiting its recording
    /// result, so a single in-flight record is always enough to restore the active handle on rollback.
    void apply(star::core::device::DeviceContext &context, star::skybox::SkyboxContext &skyboxContext);

    /// @brief Requests a selection by handle. The GUI calls this; the latest request wins.
    void request(star::Handle handle);

    /// @brief Restores the active entry when MaterialService rolls a swap back.
    void onMaterialSwapRolledBack(const star::event::MaterialSwapRolledBack &event, bool &keepAlive);

    /// @brief Drops a completed in-flight swap when MaterialService reports it committed.
    void onMaterialSwapCommitted(const star::event::MaterialSwapCommitted &event, bool &keepAlive);

    /// @brief Cached library entries, in stable library order.
    [[nodiscard]] const std::vector<EntryView> &getRegistrations() const noexcept
    {
        return m_entries;
    }

    [[nodiscard]] std::optional<star::Handle> activeHandle() const noexcept
    {
        return m_active;
    }

    [[nodiscard]] const std::optional<std::string> &scanError() const noexcept
    {
        return m_scanError;
    }

  private:
    void cacheRegistrations(star::core::device::DeviceContext &context);

    /// @brief A committed swap whose pass-readiness result has not been reported yet.
    struct InFlightSwap
    {
        star::Handle material;
        star::Handle requested;
        std::optional<star::Handle> previous;
    };

    std::vector<EntryView> m_entries;
    std::optional<star::Handle> m_active;
    std::optional<star::Handle> m_pending;
    std::optional<std::string> m_scanError;

    std::shared_ptr<star::StarObject> m_skybox;
    star::Handle m_passCommandBuffer;
    star::common::EventBus *m_evtBus{nullptr};
    std::vector<InFlightSwap> m_inFlightSwaps;
    star::policy::event::ListenForMaterialSwapRolledBack<SkyboxController> m_listenForRollback;
    star::policy::event::ListenForMaterialSwapCommitted<SkyboxController> m_listenForCommitted;
};
