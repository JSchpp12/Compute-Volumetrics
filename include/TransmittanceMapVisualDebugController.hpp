#pragma once

#include <star_terrain/rendering/BoundsDefinition.hpp>

#include <glm/glm.hpp>

#include <array>
#include <memory>
#include <optional>

namespace star
{
class StarCamera;
class StarObject;
} // namespace star

/// @brief Drives the debug visualization of the precomputed transmittance map: a window of wireframe cells, one per map
/// texel, placed with the same light projection the volume precompute uses.
class TransmittanceMapVisualDebugController
{
  public:
    /// Everything the cell placement depends on.
    struct PlacementInputs
    {
        glm::vec3 cameraPosition{0.0f};
        glm::vec3 cameraForward{0.0f, 0.0f, -1.0f};
        glm::vec3 lightDirection{0.0f, -1.0f, 0.0f};
        /// World bounds the transmittance window is fitted to (the volume's world AABB).
        star::terrain::rendering::BoundsDefinition volumeWorldBounds{};

        bool operator==(const PlacementInputs &other) const = default;

        /// Capture the camera state alongside the light direction and volume bounds.
        static PlacementInputs FromCamera(const star::StarCamera &camera, const glm::vec3 &lightDirection,
                                          const star::terrain::rendering::BoundsDefinition &volumeWorldBounds);
    };

    TransmittanceMapVisualDebugController(std::shared_ptr<star::StarObject> cells, std::array<int, 3> windowSize);

    /// Re-place the cells when `newInputs` differ from the inputs of the last placement.
    /// @return true when the cells were re-placed.
    bool update(const star::StarCamera &camera, const PlacementInputs &newInputs);

    /// Re-place the cells regardless of what was applied before.
    void refresh(const star::StarCamera &camera, const PlacementInputs &newInputs);

    /// The inputs of the most recent placement (the "old" values). Empty until the first placement.
    const std::optional<PlacementInputs> &getAppliedInputs() const noexcept
    {
        return m_appliedInputs;
    }

    const std::shared_ptr<star::StarObject> &getCells() const noexcept
    {
        return m_cells;
    }

  private:
    void place(const star::StarCamera &camera, const PlacementInputs &inputs);

    std::shared_ptr<star::StarObject> m_cells;
    std::array<int, 3> m_windowSize{};
    std::optional<PlacementInputs> m_appliedInputs{std::nullopt};
};
