#include "Application.hpp"

#include <sstream>
#include <string>

#include "BasicCamera.hpp"
#include "BasicObject.hpp"
#include "ConfigFile.hpp"
#include "DebugHelpers.hpp"
#include "KeyStates.hpp"
#include "Terrain.hpp"
#include "Time.hpp"


using namespace star;

Application::Application()
{
}

std::shared_ptr<StarScene> Application::createInitialScene(StarDevice &device, const StarWindow &window,
                                                           const uint8_t &numFramesInFlight)
{
    std::shared_ptr<star::BasicCamera> camera = std::make_shared<star::BasicCamera>(
        window.getExtent().width, window.getExtent().height, 90.0f, 0.1f, 20000.0f, 500.0f, 0.1f);

    camera->setPosition(glm::vec3{4.0f, 0.0f, 0.0f});
    camera->setForwardVector(glm::vec3{0.0, 0.0, 0.0} - camera->getPosition());

    std::shared_ptr<StarScene> newScene = std::make_shared<star::StarScene>(numFramesInFlight, camera);

    return newScene;
}

void Application::startup(star::StarDevice &device, const star::StarWindow &window, const uint8_t &numFramesInFlight)
{
    auto mediaDirectoryPath = star::ConfigFile::getSetting(star::Config_Settings::mediadirectory);
    auto horsePath = mediaDirectoryPath + "models/horse/WildHorse.obj";

    {
        int framesInFLight = std::stoi(star::ConfigFile::getSetting(star::Config_Settings::frames_in_flight));
        std::vector<star::Handle> globalInfos(framesInFLight);
        std::vector<star::Handle> lightInfos(framesInFLight);

        for (int i = 0; i < framesInFLight; i++)
        {
            globalInfos.at(i) = this->scene->getGlobalInfoBuffer(i);
            lightInfos.at(i) = this->scene->getLightInfoBuffer(i);
        }

        this->offscreenScene =
            std::make_shared<star::StarScene>(framesInFLight, this->scene->getCamera(), globalInfos, lightInfos);
        this->offscreenSceneRenderer = std::make_unique<OffscreenRenderer>(this->offscreenScene);

        const uint32_t width = window.getExtent().width;
        const uint32_t height = window.getExtent().height;
        auto screen =
            std::make_unique<Volume>(this->scene->getCamera(), width, height, this->scene->getLights(),
                                     this->offscreenSceneRenderer->getRenderToColorImages(),
                                     this->offscreenSceneRenderer->getRenderToDepthImages(), globalInfos, lightInfos);

        auto &s_i = screen->createInstance();
        s_i.setScale(glm::vec3{2.0, 2.0, 2.0});
        auto handle = this->scene->add(std::move(screen));
        StarObject *obj = &this->scene->getObject(handle);
        this->vol = static_cast<Volume *>(obj);
    }

    auto horse = star::BasicObject::New(horsePath);
    auto &h_i = horse->createInstance();
    // horse->drawBoundingBox = true;
    h_i.setPosition(glm::vec3{0.885, 5.0, 0.0});
    this->testObject = &h_i;
    this->offscreenScene->add(std::move(horse));
    this->offscreenScene->add(
        std::make_unique<star::Light>(glm::vec3{0, 10, 0}, star::Type::Light::directional, glm::vec3{-1.0, 0.0, 0.0}));

    {
        auto terrainInfoPath = mediaDirectoryPath + "terrains/height_info.json";

        auto terrain = std::make_unique<Terrain>(terrainInfoPath);
        auto &t_i = terrain->createInstance();
        this->offscreenScene->add(std::move(terrain));
    }

    this->scene->add(
        std::make_unique<star::Light>(glm::vec3{0, 10, 0}, star::Type::Light::directional, glm::vec3{-1.0, 0.0, 0.0}));

    std::cout << "Application Controls" << std::endl;
    std::cout << "B - Modify fog properties" << std::endl;
    std::cout << "L - Set to linear fog rendering" << std::endl;
    std::cout << "K - Set to marched fog rendering" << std::endl;
    std::cout << "J - Set to exp fog rendering" << std::endl;
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

    // if (key == star::KEY::P)
    // {
    //     auto time = std::time(nullptr);
    //     auto tm = *std::localtime(&time);
    //     std::ostringstream oss;
    //     oss << std::put_time(&tm, "%Y-%m-%d-%H-%M-%S") << ".png";
    //     auto stringName = oss.str();
    //     star::StarEngine::takeScreenshot(stringName);
    // }

    if (key == star::KEY::SPACE)
    {
        auto camPosition = this->scene->getCamera()->getPosition();
        auto camLookDirection = this->scene->getCamera()->getForwardVector();

        this->testObject->setPosition(glm::vec3{
            camPosition.x + (static_cast<float>(MathHelpers::MilesToMeters(camLookDirection.x))),
            camPosition.y + (static_cast<float>(MathHelpers::MilesToMeters(camLookDirection.y))),
            camPosition.z + (static_cast<float>(MathHelpers::MilesToMeters(camLookDirection.z * MILES_TO_METERS)))});
    }

    if (key == star::KEY::B)
    {
        std::cout << "Select fog property to change" << std::endl;
        std::cout << "1 - Fog Near Distance" << std::endl;
        std::cout << "2 - Fog Far Distance" << std::endl;
        std::cout << "3 - Fog Density" << std::endl;

        int selectedMode;

        {
            std::string inputOption = std::string();
            std::getline(std::cin, inputOption);
            selectedMode = std::stoi(inputOption);
        }

        switch (selectedMode)
        {
        case (1):
            this->vol->getFogControlInfo().linearInfo.nearDist = PromptForVisibilityDistance();
            break;
        case (2):
            this->vol->getFogControlInfo().linearInfo.farDist = PromptForVisibilityDistance();
            break;
        case (3):
            this->vol->getFogControlInfo().expFogInfo.density = PromptForDensity();
            break;
        default:
            std::cout << "Unknown option" << std::endl;
        }
    }

    if (key == star::KEY::L)
    {
        std::cout << "Setting fog type to: Linear" << std::endl;
        this->vol->setFogType(VolumeRenderer::FogType::linear);
    }

    if (key == star::KEY::K)
    {
        std::cout << "Setting fog type to: Ray Marched" << std::endl;
        this->vol->setFogType(VolumeRenderer::FogType::marched);
    }

    if (key == star::KEY::J)
    {
        std::cout << "Setting fog type to: Exponential" << std::endl;
        this->vol->setFogType(VolumeRenderer::FogType::exp);
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

float Application::PromptForVisibilityDistance()
{
    std::cout << "Select Distance" << std::endl;
    return ProcessFloatInput();
}

float Application::PromptForDensity()
{
    std::cout << "Select Density" << std::endl;
    return ProcessFloatInput();
}

float Application::ProcessFloatInput()
{
    float selectedDistance;

    std::string inputDistance;
    {
        std::string inputOption;
        std::getline(std::cin, inputOption);
        selectedDistance = std::stof(inputOption);
    }

    if (selectedDistance < 0.0)
    {
        std::cout << "Invalid value provided. Defaulting to 0.0" << std::endl;
        selectedDistance = 0.0f;
    }

    return selectedDistance;
}
