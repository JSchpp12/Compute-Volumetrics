#include "Application.hpp"

using namespace star; 

Application::Application(star::StarScene& scene) : StarApplication(scene){}

void Application::Load()
{
    this->camera.setPosition(glm::vec3{ -2.0, 1.0f, -2.0f });
    auto camPosition = this->camera.getPosition();
    this->camera.setLookDirection(-camPosition);

    auto mediaDirectoryPath = star::ConfigFile::getSetting(star::Config_Settings::mediadirectory);
    auto lionPath = star::ConfigFile::getSetting(star::Config_Settings::mediadirectory) + "models/lion-statue/source/rapid.obj";
    auto materialsPath = mediaDirectoryPath + "models/lion-statue/source";
    auto plantPath = star::ConfigFile::getSetting(star::Config_Settings::mediadirectory) + "models/aloevera/aloevera.obj";
    
    auto lion = BasicObject::New(lionPath);
    auto plant = BasicObject::New(plantPath); 
    auto& lion_i = lion->createInstance(); 
    lion_i.setScale(glm::vec3{ 0.04f, 0.04f, 0.04f });
    lion_i.setPosition(glm::vec3{ 0.0, 0.0, 0.0 });
    lion_i.rotateGlobal(star::Type::Axis::x, -90);
    lion_i.moveRelative(glm::vec3{ 0.0, -1.0, 0.0 });

    auto& p_i = plant->createInstance(); 
    p_i.setPosition(glm::vec3{ -0.8, 0.0, 0.0 }); 

    this->scene.add(std::move(lion)); 
    this->scene.add(std::move(plant));

    this->scene.add(std::make_unique<star::Light>(star::Type::Light::directional, glm::vec3{10, 10, 10}));

    loadFogModel(); 
    

    loadFogModel(); 
    
}

void Application::Update()
{
}

void Application::onKeyPress(int key, int scancode, int mods)
{
}

void Application::onKeyRelease(int key, int scancode, int mods)
{
}

void Application::onMouseMovement(double xpos, double ypos)
{
}

void Application::onMouseButtonAction(int button, int action, int mods)
{
}

void Application::onScroll(double xoffset, double yoffset)
{
}

void Application::onWorldUpdate()
{
}
