#pragma once

#include <GLFW/glfw3.h>
#include <openvdb/openvdb.h>
#include <openvdb/tools/SignedFloodFill.h>

#include <ctime>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <memory>
#include <sstream>
#include <string>

#include "BasicObject.hpp"
#include "ConfigFile.hpp"
#include "DebugHelpers.hpp"
#include "Interactivity.hpp"
#include "KeyStates.hpp"
#include "LightManager.hpp"
#include "OffscreenRenderer.hpp"
#include "ShaderManager.hpp"
#include "Square.hpp"
#include "StarApplication.hpp"
#include "StarEngine.hpp"
#include "Terrain.hpp"
#include "Time.hpp"
#include "Volume.hpp"

class Application : public star::StarApplication
{
  public:
    Application(star::StarScene &scene);

    void Load();

    void onKeyPress(int key, int scancode, int mods) override;

  protected:
  private:
    std::unique_ptr<star::StarScene> offscreenScene = std::unique_ptr<star::StarScene>();
    std::unique_ptr<OffscreenRenderer> offscreenSceneRenderer = std::unique_ptr<OffscreenRenderer>();

    star::StarObjectInstance *testObject = nullptr;
    Volume *vol = nullptr;

    void onKeyRelease(int key, int scancode, int mods) override;
    void onMouseMovement(double xpos, double ypos) override;
    void onMouseButtonAction(int button, int action, int mods) override;
    void onScroll(double xoffset, double yoffset) override;
    void onWorldUpdate(const uint32_t &frameInFlightIndex) override;
};
