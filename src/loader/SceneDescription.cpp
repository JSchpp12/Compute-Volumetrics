#include "loader/SceneDescription.hpp"

namespace loader
{
void SceneDescription::addObject(std::shared_ptr<star::StarObject> obj)
{
    assert(!m_objectComponents.contains(m_counter));
    m_objectComponents[m_counter] = std::move(obj);
    m_counter++;
}

void SceneDescription::addDeugSquare(std::shared_ptr<star::StarObject> obj, DebugCubeComponent square)
{
    assert(!m_objectComponents.contains(m_counter) && !m_squareComponents.contains(m_counter));
    m_objectComponents[m_counter] = std::move(obj);
    m_squareComponents[m_counter] = std::move(square);
    m_counter++;
}
DebugCubeComponent* SceneDescription::getSquareComponent(uint32_t index)
{
    if (m_squareComponents.contains(index))
    {
        return &m_squareComponents[index];
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