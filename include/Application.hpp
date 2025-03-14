#pragma once 

#include "BasicObject.hpp"
#include "StarEngine.hpp"
#include "StarApplication.hpp"
#include "ConfigFile.hpp"
#include "Time.hpp"
#include "Interactivity.hpp"
#include "DebugHelpers.hpp"
#include "ShaderManager.hpp"
#include "LightManager.hpp"
#include "KeyStates.hpp"
#include "Terrain.hpp"
#include "BasicObject.hpp"
#include "Square.hpp"
#include "Volume.hpp"
#include "OffscreenRenderer.hpp"
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <openvdb/openvdb.h>
#include <openvdb/tools/SignedFloodFill.h>

#include <ctime>
#include <string> 
#include <memory> 
#include <sstream>


class Application :
    public star::StarApplication
{
public:
    Application(star::StarScene& scene);

    void Load();

    void onKeyPress(int key, int scancode, int mods) override;

protected:

private: 
	std::unique_ptr<star::StarScene> offscreenScene = std::unique_ptr<star::StarScene>();
    std::unique_ptr<OffscreenRenderer> offscreenSceneRenderer = std::unique_ptr<OffscreenRenderer>();

    Volume* vol = nullptr;

    void onKeyRelease(int key, int scancode, int mods) override;
    void onMouseMovement(double xpos, double ypos) override;
    void onMouseButtonAction(int button, int action, int mods) override;
    void onScroll(double xoffset, double yoffset) override;
    void onWorldUpdate(const uint32_t& frameInFlightIndex) override;
};
