#include "Application.hpp"

using namespace star;

Application::Application(star::StarScene& scene) : StarApplication(scene) {}

void Application::Load()
{
    //this->camera.setPosition(glm::vec3{ 3.0, 0.0f, 2.0f });
    this->camera.setPosition(glm::vec3{4.0f, 0.0f, 0.0f });
    this->camera.setForwardVector(glm::vec3{0.0, 0.0, 0.0} - this->camera.getPosition());

    auto mediaDirectoryPath = star::ConfigFile::getSetting(star::Config_Settings::mediadirectory);

    auto horsePath = mediaDirectoryPath + "models/horse/WildHorse.obj";
    auto horse = star::BasicObject::New(horsePath);
    auto& h_i = horse->createInstance();
    h_i.setScale(glm::vec3{ 0.1, 0.1, 0.1 });
    this->scene.add(std::move(horse));

    {
        auto terrainPath = mediaDirectoryPath + "terrains/final.tif";
        auto terrainTexture = mediaDirectoryPath + "terrains/super_texture.jpg";

        float top = 39.22153016394154;
        float bottom = 39.20776235809999;
        float yDiff = top - bottom;

        float left = -82.24766761017314;
        float right = -82.2299693221875;
        float xDiff = left - right;

        top = 0.0f + (yDiff / 2);
        bottom = 0.0f - (yDiff / 2);
        left = 0.0f - (xDiff / 2);
        right = 0.0f + (xDiff / 2);

        auto terrain = std::make_unique<Terrain>(terrainPath, terrainTexture, glm::vec3{ top, left, 0 }, glm::vec3{ bottom, right, 0 });
        auto& t_i = terrain->createInstance();
        //terrain->isVisible = false; 
        t_i.setScale(glm::vec3(0.01, 0.01, 0.01)); 
        t_i.rotateGlobal(star::Type::Axis::z, 90);
        terrain->drawNormals = true; 

        this->scene.add(std::move(terrain));
    }

    //auto sphere = std::make_unique<Volume>(1280, 720, this->scene.getLights());
    //sphere->drawBoundingBox = true; 
    //sphere->isVisible = false;
    //auto& s_i = sphere->createInstance();
    ////s_i.setPosition(glm::vec3{ 0.0, 0.0, -1.0 });
    //s_i.setScale(glm::vec3{ 0.05, 0.05, 0.05 });
    //sphere->updateGridTransforms();
    //auto handle = this->scene.add(std::move(sphere));
    //StarObject* obj = &this->scene.getObject(handle); 
    //this->vol = static_cast<Volume*>(obj);

    this->scene.add(std::make_unique<star::Light>(star::Type::Light::directional, glm::vec3{ 0, 10, 0 }, glm::vec3{-1.0, 0.0, 0.0}));

    std::cout << "Application Controls" << std::endl; 
    std::cout << "H - trigger volume render" << std::endl; 
    std::cout << "M - trigger normal path traced volume render" << std::endl;
    std::cout << "V - trigger volume visibility" << std::endl;
    std::cout << "J - set ray marching to volume active boundry" << std::endl; 
    std::cout << "K - set ray marching to AABB" << std::endl; 
    std::cout << std::endl; 
}

void Application::onKeyPress(int key, int scancode, int mods)
{
    if (key == star::KEY::P) {
        auto time = std::time(nullptr); 
        auto tm = *std::localtime(&time);
        std::ostringstream oss;
        oss << std::put_time(&tm, "%Y-%m-%d-%H-%M-%S") << ".png";
        auto stringName = oss.str();
        star::StarEngine::takeScreenshot(stringName);
    }

    if (key == star::KEY::H && !this->vol->udpdateVolumeRender)
        this->vol->udpdateVolumeRender = true;
    if (key == star::KEY::V)
        this->vol->isVisible = !this->vol->isVisible;
    if (key == star::KEY::M) {
        this->vol->rayMarchToAABB = false; 
        this->vol->rayMarchToVolumeBoundry = false; 
    }
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
    if (key == star::KEY::P) {
		star::StarEngine::takeScreenshot("screenshot.png");
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
    //if (this->vol->udpdateVolumeRender) {
    //    glm::vec3 cameraRotations{
    //        cos(this->camera.getYaw()) * cos(this->camera.getPitch()),
    //        sin(this->camera.getYaw()) * cos(this->camera.getPitch()),
    //        sin(this->camera.getPitch())
    //    };

    //    auto proj = glm::inverse(this->camera.getViewMatrix());

    //    this->vol->renderVolume(glm::radians(45.0f * 0.5), this->camera.getPosition(), glm::inverse(this->camera.getViewMatrix()), this->camera.getProjectionMatrix());
    //}
}