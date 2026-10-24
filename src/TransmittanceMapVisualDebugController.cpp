#include "TransmittanceMapVisualDebugController.hpp"

#include "TransmittanceCellPlanner.hpp"
#include "render_system/fog/TransmittanceMapConfig.hpp"

#include <starlight/object/StarObject.hpp>
#include <starlight/virtual/StarCamera.hpp>

#include <cassert>
#include <utility>

TransmittanceMapVisualDebugController::PlacementInputs TransmittanceMapVisualDebugController::PlacementInputs::
    FromCamera(const star::StarCamera &camera, const glm::vec3 &lightDirection,
               const star::terrain::rendering::BoundsDefinition &volumeWorldBounds)
{
    return PlacementInputs{.cameraPosition = camera.getPosition(),
                           .cameraForward = glm::vec3{camera.getForwardVector()},
                           .lightDirection = lightDirection,
                           .volumeWorldBounds = volumeWorldBounds};
}

TransmittanceMapVisualDebugController::TransmittanceMapVisualDebugController(std::shared_ptr<star::StarObject> cells,
                                                                             std::array<int, 3> windowSize)
    : m_cells(std::move(cells)), m_windowSize(std::move(windowSize))
{
}

bool TransmittanceMapVisualDebugController::update(const star::StarCamera &camera, const PlacementInputs &newInputs)
{
    if (m_appliedInputs.has_value() && m_appliedInputs.value() == newInputs)
        return false;

    place(camera, newInputs);
    m_appliedInputs = newInputs;
    return true;
}

void TransmittanceMapVisualDebugController::refresh(const star::StarCamera &camera, const PlacementInputs &newInputs)
{
    place(camera, newInputs);
    m_appliedInputs = newInputs;
}

void TransmittanceMapVisualDebugController::place(const star::StarCamera &camera, const PlacementInputs &inputs)
{
    assert(m_cells != nullptr && "Transmittance debug cells were never provided");

    // Same light projection the volume precompute receives (see ComputeLightViewProj), so the
    // cells line up with the texels the precompute actually writes.
    const glm::mat4 worldToLightViewProj =
        transmittance_viz::ComputeLightViewProj(camera, inputs.lightDirection, inputs.volumeWorldBounds);

    const auto placements = transmittance_viz::ComputeWindowPlacements(
        worldToLightViewProj, inputs.cameraPosition, render_system::fog::kTransmittanceMapResolution, m_windowSize);

    for (size_t i = 0; i < placements.cells.size(); ++i)
    {
        auto &instance = m_cells->getInstance(i);
        instance.setPosition(placements.cells[i].position);
        instance.setScale(placements.cells[i].size);
    }
}
