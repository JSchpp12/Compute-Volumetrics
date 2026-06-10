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
    uint32_t m_counter{0};

  public:
    void addObject(std::shared_ptr<star::StarObject> obj);

    void addDebugCube(DebugCubeComponent cube);

    DebugCubeComponent *getSquareComponent(uint32_t index);

    std::shared_ptr<star::StarObject> getObject(uint32_t index);

    uint32_t getCount() const
    {
        return m_counter;
    }
};
} // namespace loader