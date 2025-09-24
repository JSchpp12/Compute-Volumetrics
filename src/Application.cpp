#include "Application.hpp"

#include "BasicObject.hpp"
#include "ConfigFile.hpp"
#include "DebugHelpers.hpp"
#include "Terrain.hpp"

#include "ManagerController_RenderResource_GlobalInfo.hpp"

#include <sstream>
#include <string>


using namespace star;

std::shared_ptr<StarScene> Application::loadScene(core::device::DeviceContext &context, const StarWindow &window,
                                                  const uint8_t &numFramesInFlight)
{
    auto mediaDirectoryPath = star::ConfigFile::getSetting(star::Config_Settings::mediadirectory);

    std::shared_ptr<star::BasicCamera> camera = std::make_shared<star::BasicCamera>(
        window.getExtent().width, window.getExtent().height, 90.0f, 0.1f, 20000.0f, 500.0f, 0.1f);
    // camera->setPosition(glm::vec3{4.0f, 0.0f, 0.0f});
    // camera->setForwardVector(glm::vec3{0.0, 0.0, 0.0} - camera->getPosition());
    // camera->setPosition(glm::vec3{-305.11, 93.597, 161.739});

    camera->setPosition(glm::vec3{4.0, 4.0, 4.0});
    camera->setForwardVector(glm::vec3{0.0, 0.0, 0.0} - camera->getPosition());

    uint8_t numInFlight;
    {
        int framesInFlight = std::stoi(star::ConfigFile::getSetting(star::Config_Settings::frames_in_flight));
        star::CastHelpers::SafeCast<int, uint8_t>(framesInFlight, numInFlight);
    }

    m_mainLight = std::make_shared<star::Light>(glm::vec3{0, 0, 0}, star::Type::Light::directional,
                                                glm::vec3{-1.0, 0.0, 0.0});

    auto offscreenRenderer = CreateOffscreenRenderer(context, numInFlight, camera, m_mainLight);

    {
        const uint32_t width = window.getExtent().width;
        const uint32_t height = window.getExtent().height;
        std::vector<star::Handle> globalInfos(numInFlight);
        std::vector<star::Handle> lightInfos(numInFlight);
        std::vector<star::Handle> lightLists(numInFlight);

        size_t fNumFramesInFlight = 0;
        star::CastHelpers::SafeCast<uint8_t, size_t>(numFramesInFlight, fNumFramesInFlight);

        std::string vdbPath = mediaDirectoryPath + "volumes/utahteapot.vdb";
        m_volume = std::make_shared<Volume>(
            context, vdbPath, fNumFramesInFlight, camera, width, height, offscreenRenderer->getRenderToColorImages(),
            offscreenRenderer->getRenderToDepthImages(), offscreenRenderer->getCameraInfoBuffers(),
            offscreenRenderer->getLightInfoBuffers(), offscreenRenderer->getLightListBuffers());

        auto &s_i = m_volume->createInstance();
        s_i.setScale(glm::vec3{10.0, 10.0, 10.0});
        s_i.setPosition(glm::vec3{0.0, 10.0, 0.0}); 
        std::vector<std::shared_ptr<StarObject>> objects{m_volume};

        std::vector<std::shared_ptr<star::core::renderer::Renderer>> additionals{offscreenRenderer};
        std::shared_ptr<star::core::renderer::SwapChainRenderer> presentationRenderer =
            std::make_shared<star::core::renderer::SwapChainRenderer>(
                context, numFramesInFlight, objects, std::vector<std::shared_ptr<star::Light>>{m_mainLight}, camera,
                window);

        m_mainScene = std::make_shared<star::StarScene>(context.getDeviceID(), numFramesInFlight, camera,
                                                        presentationRenderer, additionals);
    }

    m_volume->getFogControlInfo().marchedInfo.defaultDensity = 0.0001;
    m_volume->getFogControlInfo().marchedInfo.stepSizeDist = 0.5;
    m_volume->getFogControlInfo().marchedInfo.stepSizeDist_light = 1;
    m_volume->getFogControlInfo().marchedInfo.sigmaAbsorption = 0.7;
    m_volume->getFogControlInfo().marchedInfo.sigmaScattering = 0;

    // std::cout << "Application Controls" << std::endl;
    // std::cout << "B - Modify fog properties" << std::endl;
    // std::cout << "L - Set to linear fog rendering" << std::endl;
    // std::cout << "K - Set to marched fog rendering" << std::endl;
    // std::cout << "J - Set to exp fog rendering" << std::endl;
    // std::cout << std::endl;

    return m_mainScene;
}

void Application::onKeyPress(int key, int scancode, int mods)
{
    //  if (key == star::KEY::H && !m_volume->udpdateVolumeRender)
    //      m_volume->udpdateVolumeRender = true;
    //  if (key == star::KEY::V)
    //      m_volume->isVisible = !m_volume->isVisible;
    //  if (key == star::KEY::M) {
    //      m_volume->rayMarchToAABB = false;
    //      m_volume->rayMarchToVolumeBoundry = false;
    //  }
    //  if (key == star::KEY::J)
    //  {
    //      m_volume->rayMarchToVolumeBoundry =
    //      !m_volume->rayMarchToVolumeBoundry; m_volume->rayMarchToAABB =
    //      false;
    //  }
    //  if (key == star::KEY::K)
    //  {
    //      m_volume->rayMarchToAABB = !m_volume->rayMarchToAABB;
    //      m_volume->rayMarchToVolumeBoundry = false;
    //  }
}

void Application::onKeyRelease(int key, int scancode, int mods)
{
    // const float MILES_TO_METERS = 1609.35;

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
        //     auto camPosition = this->scene->getCamera()->getPosition();
        //     auto camLookDirection = this->scene->getCamera()->getForwardVector();

        //     this->testObject->setPosition(glm::vec3{camPosition.x, camPosition.y, camPosition.z + 10});

        // this->testObject->setPosition(glm::vec3{
        //     camPosition.x + (static_cast<float>(MathHelpers::MilesToMeters(camLookDirection.x))),
        //     camPosition.y + (static_cast<float>(MathHelpers::MilesToMeters(camLookDirection.y))),
        //     camPosition.z + (static_cast<float>(MathHelpers::MilesToMeters(camLookDirection.z)))});
    }

    if (key == star::KEY::T)
    {
        // std::cout << this->scene->getCamera()->getPosition().x << "," << this->scene->getCamera()->getPosition().y
        //           << "," << this->scene->getCamera()->getPosition().z << std::endl;
    }

    if (key == star::KEY::B)
    {
        std::cout << "Select fog property to change" << std::endl;
        std::cout << "1 - LinearFog: Fog Near Distance" << std::endl;
        std::cout << "2 - LinearFog: Fog Far Distance" << std::endl;
        std::cout << "3 - ExpFog: Fog Density" << std::endl;
        std::cout << "4 - MarchedFog: Default Density" << std::endl;
        std::cout << "5 - MarchedFog: Sigma Absorption" << std::endl;
        std::cout << "6 - MarchedFog: Sigma Scattering" << std::endl;
        std::cout << "7 - MarchedFog: Light PropertyDirG" << std::endl;
        std::cout << "8 - MarchedFog: Step Size" << std::endl;
        std::cout << "9 - MarchedFog: Step Size Light" << std::endl;

        int selectedMode;

        {
            std::string inputOption = std::string();
            std::getline(std::cin, inputOption);
            try
            {
                selectedMode = std::stoi(inputOption);
            }
            catch (const std::exception &ex)
            {
                std::cout << "Invalid option" << std::endl;
                return;
            }
        }

        switch (selectedMode)
        {
        case (1):
            m_volume->getFogControlInfo().linearInfo.nearDist = PromptForFloat("Select visibility");
            break;
        case (2):
            m_volume->getFogControlInfo().linearInfo.farDist = PromptForFloat("Select distance");
            break;
        case (3):
            m_volume->getFogControlInfo().expFogInfo.density = PromptForFloat("Select density");
            break;
        case (4):
            m_volume->getFogControlInfo().marchedInfo.defaultDensity = PromptForFloat("Select density");
            break;
        case (5):
            m_volume->getFogControlInfo().marchedInfo.sigmaAbsorption = PromptForFloat("Select sigma");
            break;
        case (6):
            m_volume->getFogControlInfo().marchedInfo.sigmaScattering = PromptForFloat("Select sigma");
            break;
        case (7):
            m_volume->getFogControlInfo().marchedInfo.lightPropertyDirG = PromptForFloat("Select light prop", true);
            break;
        case (8):
            m_volume->getFogControlInfo().marchedInfo.stepSizeDist = PromptForFloat("Select step size");
            break;
        case (9):
            m_volume->getFogControlInfo().marchedInfo.stepSizeDist_light = PromptForFloat("Select step size light");
            break;
        default:
            std::cout << "Unknown option" << std::endl;
        }
    }

    if (key == star::KEY::L)
    {
        std::cout << "Setting fog type to: Linear" << std::endl;
        m_volume->setFogType(VolumeRenderer::FogType::linear);
    }

    if (key == star::KEY::K)
    {
        std::cout << "Setting fog type to: Ray Marched" << std::endl;
        m_volume->setFogType(VolumeRenderer::FogType::marched);
    }

    if (key == star::KEY::J)
    {
        std::cout << "Setting fog type to: Exponential" << std::endl;
        m_volume->setFogType(VolumeRenderer::FogType::exp);
    }

    if (key == star::KEY::H){
        std::cout << "Setting fog type to: Nano Bounding Box" << std::endl;
        m_volume->setFogType(VolumeRenderer::FogType::nano_boundingBox);
    }

    if (key == star::KEY::G){
        std::cout << "Setting fog type to: NANO Surface" << std::endl;
        m_volume->setFogType(VolumeRenderer::FogType::nano_surface); 
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
    // m_volume->renderVolume(glm::radians(this->scene.getCamera()->getFieldOfView()),
    // this->scene.getCamera()->getPosition(),
    // glm::inverse(this->scene.getCamera()->getViewMatrix()),
    // this->scene.getCamera()->getProjectionMatrix());
}

float Application::PromptForFloat(const std::string &prompt, const bool &allowNegatives)
{
    std::cout << prompt.c_str() << std::endl;
    return ProcessFloatInput(allowNegatives);
}

int Application::PromptForInt(const std::string &prompt)
{
    std::cout << prompt.c_str() << std::endl;
    return ProcessIntInput();
}

float Application::ProcessFloatInput(const bool &allowNegatives)
{
    float selectedDistance = 0.0f;

    {
        std::string inputOption;
        std::getline(std::cin, inputOption);
        selectedDistance = std::stof(inputOption);
    }

    if (!allowNegatives && selectedDistance < 0.0)
    {
        std::cout << "Invalid value provided. Defaulting to 0.0" << std::endl;
        selectedDistance = 0.0f;
    }

    return selectedDistance;
}

int Application::ProcessIntInput()
{
    int selectedValue = 0;

    {
        std::string inputOption = std::string();
        std::getline(std::cin, inputOption);
        selectedValue = std::stoi(inputOption);
    }

    if (selectedValue < 0)
    {
        std::cout << "Invalid value provided. Defaulting to 0" << std::endl;
        selectedValue = 0;
    }

    return selectedValue;
}

std::shared_ptr<OffscreenRenderer> Application::CreateOffscreenRenderer(star::core::device::DeviceContext &context,
                                                                        const uint8_t &numFramesInFlight,
                                                                        std::shared_ptr<star::BasicCamera> camera,
                                                                        std::shared_ptr<star::Light> mainLight)
{
    auto mediaDirectoryPath = star::ConfigFile::getSetting(star::Config_Settings::mediadirectory);
    auto terrainInfoPath = mediaDirectoryPath + "terrains/height_info.json";
    auto terrain = std::make_shared<Terrain>(context, terrainInfoPath);
    terrain->createInstance();

    // auto horsePath = mediaDirectoryPath + "models/horse/WildHorse.obj";
    // auto horse = std::make_shared<star::BasicObject>(horsePath); 
    // auto h_i = horse->createInstance();
    // h_i.setPosition(glm::vec3{0.0, 0.0, 0.0}); 

    std::vector<std::shared_ptr<star::StarObject>> objects{
        terrain
        // horse
    };
    std::vector<std::shared_ptr<star::Light>> lights{mainLight};

    return std::make_shared<OffscreenRenderer>(context, numFramesInFlight, objects, lights, camera);
}