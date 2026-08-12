#pragma once

#include "loader/DebugSquareComponent.hpp"

#include <absl/container/flat_hash_map.h>
#include <starlight/object/StarObject.hpp>

namespace loader
{
struct SceneDescription
{
    absl::flat_hash_map<uint32_t, DebugCubeComponent> m_cubeComponents;
    absl::flat_hash_map<uint32_t, std::shared_ptr<star::StarObject>> m_objectComponents;
    std::shared_ptr<star::StarObject> m_shadowObject;
    uint32_t m_counter{0};

  public:
    void addObject(std::shared_ptr<star::StarObject> obj);

    void addDebugCube(DebugCubeComponent cube);

    DebugCubeComponent *getSquareComponent(uint32_t index);

    std::shared_ptr<star::StarObject> getObject(uint32_t index);

    /// Shadow-cast terrain object, kept separate from the color object list so the
    /// application can route it to the terrain shadow render phase instead of the
    /// offscreen color phase.
    void addShadowObject(std::shared_ptr<star::StarObject> obj);

    std::shared_ptr<star::StarObject> getShadowObject() const
    {
        return m_shadowObject;
    }

    uint32_t getCount() const
    {
        return m_counter;
    }
};
} // namespace loader