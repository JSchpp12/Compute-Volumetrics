#pragma once

#ifdef STAR_ENABLE_PRESENTATION

namespace star
{
class StarScene;
} // namespace star

class Volume;
class SkyboxController;
/// @brief Immediate-mode overlay. Its callbacks are the last phase's frameUpdate, which decides when edits land.
class GUIController
{
  public:
    GUIController() = default;
    GUIController(star::StarScene *scene, Volume *volume, SkyboxController *controller)
        : m_scene(scene), m_volume(volume), m_skyboxController(controller)
    {
    }
    void draw();

  private:
    star::StarScene *m_scene = nullptr;
    Volume *m_volume = nullptr;
    SkyboxController *m_skyboxController = nullptr;
};

#endif
