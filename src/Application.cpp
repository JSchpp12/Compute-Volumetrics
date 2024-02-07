#include "Application.hpp"

using namespace star;

Application::Application(star::StarScene& scene) : StarApplication(scene) {}

void Application::Load()
{
    //this->camera.setPosition(glm::vec3{ 3.0, 0.0f, 2.0f });
    this->camera.setPosition(glm::vec3{4.0f, 0.0f, 0.0f });
    this->camera.setForwardVector(glm::vec3{0.0, 0.0, 0.0} - this->camera.getPosition());

    auto mediaDirectoryPath = star::ConfigFile::getSetting(star::Config_Settings::mediadirectory);
    auto lionPath = star::ConfigFile::getSetting(star::Config_Settings::mediadirectory) + "models/lion-statue/source/rapid.obj";
    auto materialsPath = mediaDirectoryPath + "models/lion-statue/source";
    auto plantPath = star::ConfigFile::getSetting(star::Config_Settings::mediadirectory) + "models/aloevera/aloevera.obj";
    auto airplanePath = mediaDirectoryPath + "models/airplane/Airplane.obj";

    //auto airplane = star::BasicObject::New(airplanePath); 
    //auto& a_i = airplane->createInstance(); 
    //airplane->drawBoundingBox = true;
    //a_i.setScale(glm::vec3{ 0.01, 0.01, 0.01 });
    //this->scene.add(std::move(airplane));

    auto sphere = std::make_unique<Volume>(1280, 720, this->scene.getLights());
    sphere->drawBoundingBox = true; 
    sphere->isVisible = false;
    auto& s_i = sphere->createInstance();
    //s_i.setPosition(glm::vec3{ 0.0, 0.0, -1.0 });
    s_i.setScale(glm::vec3{ 0.05, 0.05, 0.05 });
    sphere->updateGridTransforms();
    auto handle = this->scene.add(std::move(sphere));
    StarObject* obj = &this->scene.getObject(handle); 
    this->vol = static_cast<Volume*>(obj);

    this->scene.add(std::make_unique<star::Light>(star::Type::Light::directional, glm::vec3{ 0, 10, 0 }, glm::vec3{-1.0, 0.0, 0.0}));
}

void Application::onKeyPress(int key, int scancode, int mods)
{
    if (key == star::KEY::H && !this->vol->udpdateVolumeRender)
        this->vol->udpdateVolumeRender = true;
    if (key == star::KEY::V)
        this->vol->isVisible = !this->vol->isVisible;
    if (key == star::KEY::J)
    {
        this->vol->rayMarchToVolumeBoundry = !this->vol->rayMarchToVolumeBoundry;
        this->vol->rayMarchToAABB = false; 
    }
    if (key == star::KEY::K)
    {
        this->vol->rayMarchToAABB = !this->vol->rayMarchToAABB;
        this->vol->rayMarchToVolumeBoundry = false; 
    }
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
    if (this->vol->udpdateVolumeRender) {
        glm::vec3 cameraRotations{
            cos(this->camera.getYaw()) * cos(this->camera.getPitch()),
            sin(this->camera.getYaw()) * cos(this->camera.getPitch()),
            sin(this->camera.getPitch())
        };

        auto proj = glm::inverse(this->camera.getViewMatrix());

        this->vol->renderVolume(glm::radians(45.0f * 0.5), this->camera.getPosition(), glm::inverse(this->camera.getViewMatrix()), this->camera.getProjectionMatrix());
    }
}