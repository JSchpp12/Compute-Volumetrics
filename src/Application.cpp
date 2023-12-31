#include "Application.hpp"

using namespace star;

Application::Application(star::StarScene& scene) : StarApplication(scene) {}

void Application::Load()
{
    this->camera.setPosition(glm::vec3{ 0.0, 0.0f, 0.0f });
    this->camera.setLookDirection(glm::vec3{0.0, 0.0, 1.0});

    auto mediaDirectoryPath = star::ConfigFile::getSetting(star::Config_Settings::mediadirectory);
    auto lionPath = star::ConfigFile::getSetting(star::Config_Settings::mediadirectory) + "models/lion-statue/source/rapid.obj";
    auto materialsPath = mediaDirectoryPath + "models/lion-statue/source";
    auto plantPath = star::ConfigFile::getSetting(star::Config_Settings::mediadirectory) + "models/aloevera/aloevera.obj";
    auto airplanePath = mediaDirectoryPath + "models/airplane/Airplane.obj";

    //auto airplane = star::BasicObject::New(airplanePath); 
    //auto& a_i = airplane->createInstance(); 
    //a_i.setScale(glm::vec3{ 0.001, 0.001, 0.001 });
    //this->scene.add(std::move(airplane));

    auto sphere = std::make_unique<Volume>(1280, 720);
    auto& s_i = sphere->createInstance();
    s_i.setScale(glm::vec3{ 0.01, 0.01, 0.01 }); 
    auto handle = this->scene.add(std::move(sphere));
    StarObject* obj = &this->scene.getObject(handle); 
    this->vol = static_cast<Volume*>(obj);

    //auto dicePath = mediaDirectoryPath + "models/icoSphere/low_poly_icoSphere.obj"; 
    //auto dice = star::BasicObject::New(dicePath);
    //auto& dice_i = dice->createInstance(); 
    //dice_i.setScale(glm::vec3{ 0.1, 0.1, 0.1 });
    //this->scene.add(std::move(dice)); 

    //auto plant = star::BasicObject::New(plantPath); 
    //auto p_i = plant->createInstance();
    //this->scene.add(std::move(plant));

    //auto lion = BasicObject::New(lionPath);
    //auto& lion_i = lion->createInstance();
    //lion_i.setScale(glm::vec3{ 0.04f, 0.04f, 0.04f });
    //lion_i.setPosition(glm::vec3{ 0.0, 0.0, 0.0 });
    //lion_i.rotateGlobal(star::Type::Axis::x, -90);
    //lion_i.moveRelative(glm::vec3{ 0.0, -1.0, 0.0 });
    //this->scene.add(std::move(lion));

    this->scene.add(std::make_unique<star::Light>(star::Type::Light::directional, glm::vec3{ 10, 10, 10 }));

    glm::vec3 cameraRotations{
        cos(this->camera.getYaw()) * cos(this->camera.getPitch()),
        sin(this->camera.getYaw()) * cos(this->camera.getPitch()),
        sin(this->camera.getPitch())
    };


    this->vol->renderVolume(this->camera.getPosition(), cameraRotations, 50, 50, 10, 1000, 90, this->camera.getInverseViewMatrix());

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