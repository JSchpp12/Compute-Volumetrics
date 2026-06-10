#include "loader/SceneDescription.hpp"

namespace loader
{
void SceneDescription::addObject(std::shared_ptr<star::StarObject> obj)
{
    assert(!m_objectComponents.contains(m_counter));
    m_objectComponents[m_counter] = std::move(obj);
    m_counter++;
}

void SceneDescription::addDebugCube(DebugCubeComponent cube)
{
    assert(!m_cubeComponents.contains(m_counter) && !m_cubeComponents.contains(m_counter));
    m_cubeComponents[m_counter] = std::move(cube);
    m_counter++;
}

DebugCubeComponent* SceneDescription::getSquareComponent(uint32_t index)
{
    if (m_cubeComponents.contains(index))
    {
        return &m_cubeComponents[index];
    }

    return nullptr; 
}

std::shared_ptr<star::StarObject> SceneDescription::getObject(uint32_t index)
{
    if (m_objectComponents.contains(index))
    {
        return m_objectComponents[index];
    }

    return nullptr; 
}
} // namespace loader