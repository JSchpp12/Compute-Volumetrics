#include "loader/SceneDescription.hpp"

namespace loader
{
void SceneDescription::addObject(std::shared_ptr<star::StarObject> obj)
{
    assert(!m_objectComponents.contains(m_counter));
    m_objectComponents[m_counter] = std::move(obj);
    m_counter++;
}

void SceneDescription::addShadowObject(std::shared_ptr<star::StarObject> obj)
{
    assert(!m_shadowObject && "Shadow object has already been set");
    m_shadowObject = std::move(obj);
}

void SceneDescription::addDebugCube(DebugCubeComponent cube)
{
    assert(!m_cubeComponents.contains(m_counter) && !m_cubeComponents.contains(m_counter));
    m_cubeComponents[m_counter] = std::move(cube);
    m_counter++;
}

void SceneDescription::addTransmittanceViz(TransmittanceVizComponent viz)
{
    assert(!m_transmittanceVizComponents.contains(m_counter));
    m_transmittanceVizComponents[m_counter] = std::move(viz);
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

TransmittanceVizComponent* SceneDescription::getTransmittanceVizComponent(uint32_t index)
{
    if (m_transmittanceVizComponents.contains(index))
    {
        return &m_transmittanceVizComponents[index];
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