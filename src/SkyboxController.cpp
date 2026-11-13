#include "SkyboxController.hpp"

#include "loader/SceneLoaders.hpp"

#include <star_skybox/command/GetAllSkyboxRegistrations.hpp>
#include <star_skybox/command/GetSkyboxInfo.hpp>
#include <star_skybox/context/SkyboxContext.hpp>
#include <star_skybox/maker/MakeSkybox.hpp>

#include <starlight/command/material/SetMaterialResource.hpp>
#include <starlight/core/Exceptions.hpp>
#include <starlight/core/device/DeviceContext.hpp>
#include <starlight/core/logging/LoggingFactory.hpp>
#include <starlight/core/transfer/RegisterTransferWait.hpp>
#include <starlight/object/StarObject.hpp>

#include <algorithm>
#include <exception>
#include <expected>
#include <iterator>
#include <utility>

using EntryView = SkyboxController::EntryView;

/// @brief Finds the registration for the skybox the scene was configured with.
/// @details Matches the exact path first, then the filename, mirroring SkyboxFileService::findHandleByPath so the
/// controller and any caller holding the configured path agree on which entry is meant.
static const EntryView *FindRegistrationForPath(const std::vector<EntryView> &entries,
                                                const std::filesystem::path &texturePath)
{
    for (const auto &entry : entries)
    {
        if (entry.texturePath == texturePath)
            return &entry;
    }

    const std::string filename = texturePath.filename().string();
    for (const auto &entry : entries)
    {
        if (entry.texturePath.filename().string() == filename)
            return &entry;
    }

    return nullptr;
}

/// @brief Resolves one entry's description through the file service.
static std::expected<star::skybox::SkyboxInfo, std::string> ResolveEntryInfo(star::core::device::DeviceContext &context,
                                                                             star::Handle handle)
{
    star::skybox::command::GetSkyboxInfo command{handle};
    context.getCmdBus().submit(command);
    auto info = command.getReply().get();
    if (!info)
        return std::unexpected(info.error().what());

    return std::move(*info);
}

void SkyboxController::cacheRegistrations(star::core::device::DeviceContext &context)
{
    star::skybox::command::GetAllSkyboxRegistrations command;
    context.getCmdBus().submit(command);
    const auto reply = command.getReply().get();

    m_entries.clear();
    m_entries.reserve(reply.registrations.size());
    for (const auto &registration : reply.registrations)
    {
        m_entries.push_back({.handle = registration.handle,
                             .displayName = registration.displayName,
                             .texturePath = registration.texturePath});
    }

    m_scanError = reply.scanError;
    if (m_scanError.has_value())
        star::core::logging::warning("Skybox library scan failed: ", *m_scanError);
}

std::shared_ptr<star::StarObject> SkyboxController::createSkybox(star::core::device::DeviceContext &context,
                                                                 star::skybox::SkyboxContext &skyboxContext,
                                                                 const std::filesystem::path &configuredTexturePath)
{
    m_skybox.reset();
    m_active.reset();
    m_pending.reset();
    m_inFlightSwaps.clear();

    cacheRegistrations(context);

    if (m_entries.empty())
        STAR_THROW("Skybox library is empty; cannot create the initial skybox");

    const EntryView *initial = FindRegistrationForPath(m_entries, configuredTexturePath);
    if (initial == nullptr)
        STAR_THROW("Configured skybox '" + configuredTexturePath.string() + "' is not in the skybox library");

    auto info = ResolveEntryInfo(context, initial->handle);
    if (!info)
        STAR_THROW("Failed to resolve skybox '" + initial->displayName + "': " + info.error());

    // The single skybox creator: the object and its initial resource are built together. Later selections swap the
    // same object's material resource through apply().
    m_skybox = loader::LoadSkybox(context, skyboxContext, initial->texturePath, std::move(*info));
    m_active = initial->handle;
    return m_skybox;
}

void SkyboxController::attachSkybox(star::core::device::DeviceContext &context, const star::Handle &passCommandBuffer)
{
    m_passCommandBuffer = passCommandBuffer;

    if (m_evtBus == nullptr)
    {
        m_evtBus = &context.getEventBus();
        m_listenForRollback.init(context.getEventBus());
        m_listenForCommitted.init(context.getEventBus());
    }
}

void SkyboxController::detachSkybox(star::common::EventBus &eventBus)
{
    if (m_evtBus == nullptr)
        return;

    m_listenForRollback.cleanup(eventBus);
    m_listenForCommitted.cleanup(eventBus);
    m_evtBus = nullptr;
}

void SkyboxController::onMaterialSwapRolledBack(const star::event::MaterialSwapRolledBack &event, bool &keepAlive)
{
    keepAlive = true;

    const auto it = std::find_if(m_inFlightSwaps.rbegin(), m_inFlightSwaps.rend(),
                                 [&event](const InFlightSwap &swap) { return swap.material == event.getMaterial(); });
    if (it == m_inFlightSwaps.rend())
        return;

    if (m_active.has_value() && *m_active == it->requested)
        m_active = it->previous;

    m_inFlightSwaps.erase(std::next(it).base());
}

void SkyboxController::onMaterialSwapCommitted(const star::event::MaterialSwapCommitted &event, bool &keepAlive)
{
    keepAlive = true;

    event.getResults().forEach([this](const star::MaterialSwapCommitInfo &commit) {
        const auto it = std::find_if(m_inFlightSwaps.begin(), m_inFlightSwaps.end(),
                                     [&commit](const InFlightSwap &swap) { return swap.material == commit.material; });
        if (it != m_inFlightSwaps.end())
            m_inFlightSwaps.erase(it);
    });
}

void SkyboxController::request(star::Handle handle)
{
    m_pending = std::move(handle);
}

void SkyboxController::apply(star::core::device::DeviceContext &context, star::skybox::SkyboxContext &skyboxContext)
{
    if (!m_pending.has_value())
        return;

    const star::Handle requested = *m_pending;

    if (!m_skybox || !m_passCommandBuffer.isInitialized())
    {
        star::core::logging::warning("Skybox swap requested before the skybox or its render pass was attached; "
                                     "ignoring");
        m_pending.reset();
        return;
    }

    if (m_active.has_value() && *m_active == requested)
    {
        m_pending.reset();
        return;
    }

    const auto entry = std::find_if(m_entries.begin(), m_entries.end(),
                                    [&requested](const EntryView &view) { return view.handle == requested; });
    if (entry == m_entries.end())
    {
        star::core::logging::warning("Skybox swap requested for an unknown handle; ignoring");
        m_pending.reset();
        return;
    }

    const auto &materials = m_skybox->getMaterialHandles();
    if (materials.empty())
    {
        // The scene's prepRender has not created the materials yet; retry on the next frame.
        return;
    }

    // Only one swap per material may be in flight. Submitting a second would discard the older swap's rollback
    // record, so a late rollback could restore the wrong active handle or rebind an already-retired image.
    const bool swapPending =
        std::any_of(m_inFlightSwaps.begin(), m_inFlightSwaps.end(),
                    [&materials](const InFlightSwap &swap) { return swap.material == materials.front(); });
    if (swapPending)
    {
        star::core::logging::warning("Skybox swap to '", entry->displayName,
                                     "' rejected: a previous swap is still pending");
        m_pending.reset();
        return;
    }

    auto info = ResolveEntryInfo(context, requested);
    if (!info)
    {
        star::core::logging::warning("Skybox info for '", entry->displayName, "' failed: ", info.error());
        m_pending.reset();
        return;
    }

    try
    {
        auto made = star::skybox::maker::MakeSkybox(context, skyboxContext)
                        .setTexture(entry->texturePath.string())
                        .setInfo(std::move(*info))
                        .useGPUSampling()
                        .make();

        std::vector<star::ReadinessCondition> readiness;
        if (made.condition.has_value())
            readiness.push_back(std::move(*made.condition));
        if (made.wait.has_value())
            readiness.push_back(star::core::transfer::MakeTransferReadinessCondition(context, *made.wait));

        // Submitted by named lvalue: the reply is read immediately after the synchronous command dispatch.
        star::command::material::SetMaterialResource setResourceCommand{
            star::command::material::MaterialTarget{.material = materials.front()}, m_passCommandBuffer,
            star::command::material::set_material_resource::IncomingResource{.handle = made.resource,
                                                                             .readiness = std::move(readiness)},
            std::move(made.prep)};
        context.getCmdBus().submit(setResourceCommand);

        const auto result = setResourceCommand.getReply().get();
        if (!result.has_value())
        {
            star::core::logging::warning("Skybox swap to '", entry->displayName,
                                         "' was rejected: ", result.error().what(), " (", result.error().detail, ")");
        }
        else
        {
            // The pending check above guarantees this material has no other in-flight swap, so the record is
            // unambiguous for a rollback.
            m_inFlightSwaps.push_back({.material = materials.front(), .requested = requested, .previous = m_active});

            m_active = requested;
            star::core::logging::info("Skybox swap to '", entry->displayName,
                                      "' accepted; material commits when ready");
        }
    }
    catch (const std::exception &e)
    {
        star::core::logging::warning("Skybox swap to '", entry->displayName, "' failed: ", e.what());
    }

    m_pending.reset();
}
