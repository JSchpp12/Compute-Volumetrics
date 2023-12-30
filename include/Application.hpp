#pragma once 

#include "BasicObject.hpp"
#include "StarEngine.hpp"
#include "StarApplication.hpp"
#include "ConfigFile.hpp"
#include "Time.hpp"
#include "Interactivity.hpp"
#include "DebugHelpers.hpp"
#include "ShaderManager.hpp"
#include "TextureManager.hpp"
#include "LightManager.hpp"
#include "KeyStates.hpp"
#include "Key.hpp"
#include "BasicObject.hpp"

#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <openvdb/openvdb.h>

#include <string> 
#include <memory> 

class Application :
    public star::StarApplication
{
public:
    Application(star::StarScene& scene);

    void Load();

    void Update();

    virtual std::string getApplicationName() { return "Starlight Application"; }

    void onKeyPress(int key, int scancode, int mods) override;

protected:

private:
    void onKeyRelease(int key, int scancode, int mods) override;
    void onMouseMovement(double xpos, double ypos) override;
    void onMouseButtonAction(int button, int action, int mods) override;
    void onScroll(double xoffset, double yoffset) override;
    void onWorldUpdate() override;
};
