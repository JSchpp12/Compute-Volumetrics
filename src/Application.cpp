#include "Application.hpp"

using namespace star;

Application::Application(star::StarScene &scene) : StarApplication(scene)
{
}

void Application::Load()
{
    // this->camera.setPosition(glm::vec3{ 3.0, 0.0f, 2.0f });
    this->scene.getCamera()->setPosition(glm::vec3{4.0f, 0.0f, 0.0f});
    this->scene.getCamera()->setForwardVector(glm::vec3{0.0, 0.0, 0.0} - this->scene.getCamera()->getPosition());

    auto mediaDirectoryPath = star::ConfigFile::getSetting(star::Config_Settings::mediadirectory);
    auto horsePath = mediaDirectoryPath + "models/horse/WildHorse.obj";

    {
        int framesInFLight = std::stoi(star::ConfigFile::getSetting(star::Config_Settings::frames_in_flight));
        std::vector<star::Handle> globalInfos(framesInFLight);
        std::vector<star::Handle> lightInfos(framesInFLight);

        for (int i = 0; i < framesInFLight; i++)
        {
            globalInfos.at(i) = this->scene.getGlobalInfoBuffer(i);
            lightInfos.at(i) = this->scene.getLightInfoBuffer(i);
        }

        this->offscreenScene =
            std::make_unique<star::StarScene>(framesInFLight, this->scene.getCamera(), globalInfos, lightInfos);
        this->offscreenSceneRenderer = std::make_unique<OffscreenRenderer>(*this->offscreenScene);

        star::StarCamera *camera = this->scene.getCamera();
        assert(camera != nullptr);

        auto screen = std::make_unique<Volume>(
            *camera, std::stoi(star::ConfigFile::getSetting(star::Config_Settings::resolution_x)),
            std::stoi(star::ConfigFile::getSetting(star::Config_Settings::resolution_x)), this->scene.getLights(),
            this->offscreenSceneRenderer->getRenderToColorImages(),
            this->offscreenSceneRenderer->getRenderToDepthImages(), globalInfos, lightInfos);

        screen->drawBoundingBox = true;
        auto &s_i = screen->createInstance();
        s_i.setScale(glm::vec3{0.005, 0.005, 0.005});
        auto handle = this->scene.add(std::move(screen));
        StarObject *obj = &this->scene.getObject(handle);
        this->vol = static_cast<Volume *>(obj);
    }

    auto horse = star::BasicObject::New(horsePath);
    auto &h_i = horse->createInstance();
    // horse->drawBoundingBox = true;
    h_i.setPosition(glm::vec3{0.885, 5.0, 0.0});
    this->testObject = &h_i;
    this->offscreenScene->add(std::move(horse));
    this->offscreenScene->add(
        std::make_unique<star::Light>(star::Type::Light::directional, glm::vec3{0, 10, 0}, glm::vec3{-1.0, 0.0, 0.0}));

    {
        auto terrainInfoPath = mediaDirectoryPath + "terrains/height_info.json";

        auto terrain = std::make_unique<Terrain>(terrainInfoPath);
        auto &t_i = terrain->createInstance();
        this->offscreenScene->add(std::move(terrain));
    }

    this->scene.add(
        std::make_unique<star::Light>(star::Type::Light::directional, glm::vec3{0, 10, 0}, glm::vec3{-1.0, 0.0, 0.0}));

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
    //  if (key == star::KEY::H && !this->vol->udpdateVolumeRender)
    //      this->vol->udpdateVolumeRender = true;
    //  if (key == star::KEY::V)
    //      this->vol->isVisible = !this->vol->isVisible;
    //  if (key == star::KEY::M) {
    //      this->vol->rayMarchToAABB = false;
    //      this->vol->rayMarchToVolumeBoundry = false;
    //  }
    //  if (key == star::KEY::J)
    //  {
    //      this->vol->rayMarchToVolumeBoundry =
    //      !this->vol->rayMarchToVolumeBoundry; this->vol->rayMarchToAABB =
    //      false;
    //  }
    //  if (key == star::KEY::K)
    //  {
    //      this->vol->rayMarchToAABB = !this->vol->rayMarchToAABB;
    //      this->vol->rayMarchToVolumeBoundry = false;
    //  }
}

void Application::onKeyRelease(int key, int scancode, int mods)
{
    const float MILES_TO_METERS = 1609.35;

    if (key == star::KEY::P)
    {
        auto time = std::time(nullptr);
        auto tm = *std::localtime(&time);
        std::ostringstream oss;
        oss << std::put_time(&tm, "%Y-%m-%d-%H-%M-%S") << ".png";
        auto stringName = oss.str();
        star::StarEngine::takeScreenshot(stringName);
    }

    if (key == star::KEY::SPACE)
    {
        auto camPosition = this->scene.getCamera()->getPosition();
        auto camLookDirection = this->scene.getCamera()->getForwardVector();

        this->testObject->setPosition(glm::vec3{camPosition.x + (camLookDirection.x * MILES_TO_METERS),
                                                camPosition.y + (camLookDirection.y * MILES_TO_METERS),
                                                camPosition.z + (camLookDirection.z * MILES_TO_METERS)});
    }

    if (key == star::KEY::L)
    {
        this->vol->setFogType(VolumeRenderer::FogType::linear);
    }

    if (key == star::KEY::K)
    {
        this->vol->setFogType(VolumeRenderer::FogType::marched);
    }
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

void Application::onWorldUpdate(const uint32_t &frameInFlightIndex)
{
    // this->vol->renderVolume(glm::radians(this->scene.getCamera()->getFieldOfView()),
    // this->scene.getCamera()->getPosition(),
    // glm::inverse(this->scene.getCamera()->getViewMatrix()),
    // this->scene.getCamera()->getProjectionMatrix());
}