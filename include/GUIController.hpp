#pragma once

#ifdef STAR_ENABLE_PRESENTATION

namespace star
{
class StarScene;
} // namespace star

class Volume;
class GUIController
{
  public:
    void setScene(star::StarScene *scene) noexcept;
    void setVolume(Volume *volume) noexcept;
    void draw();

  private:
    star::StarScene *m_scene = nullptr;
    Volume *m_volume = nullptr;
};

#endif
