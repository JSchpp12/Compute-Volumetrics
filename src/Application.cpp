#include "Application.hpp"

#include "Terrain.hpp"

#include <starlight/common/ConfigFile.hpp>
#include <starlight/common/objects/BasicObject.hpp>
#include <starlight/event/TriggerScreenshot.hpp>
#include <starlight/service/ScreenCapture.hpp>

#include "ManagerController_RenderResource_GlobalInfo.hpp"
#include "core/logging/LoggingFactory.hpp"

#include <star_windowing/InteractivityBus.hpp>
#include <star_windowing/SwapChainRenderer.hpp>

#include <sstream>
#include <string>

using namespace star;

std::shared_ptr<StarScene> Application::loadScene(core::device::DeviceContext &context,
                                                  const uint8_t &numFramesInFlight)
{
    star::windowing::InteractivityBus::Init(&context.getEventBus(), m_winContext);

    m_screenshotRegistrations.resize(numFramesInFlight);

    auto mediaDirectoryPath = star::ConfigFile::getSetting(star::Config_Settings::mediadirectory);
    const glm::vec3 camPos{-50.9314, 135.686, 25.9329};
    const glm::vec3 volumePos{50.0, 10.0, 0.0};
    const glm::vec3 lightPos = volumePos + glm::vec3{0.0f, 500.0f, 0.0f};
    std::shared_ptr<star::windowing::BasicCamera> camera = std::make_shared<star::windowing::BasicCamera>(
        context.getEngineResolution().width, context.getEngineResolution().height, 90.0f, 1.0f, 20000.0f, 100.0f, 0.1f);
    camera->init(context.getEventBus());

    camera->setPosition(camPos);
    camera->setForwardVector(volumePos - camera->getPosition());

    m_mainLight = std::make_shared<std::vector<star::Light>>(
        std::vector<star::Light>{star::Light(lightPos, star::Type::Light::directional, glm::vec3{-1.0, 0.0, 0.0})});

    uint8_t numInFlight;
    {
        int framesInFlight = std::stoi(star::ConfigFile::getSetting(star::Config_Settings::frames_in_flight));
        star::common::helper::SafeCast<int, uint8_t>(framesInFlight, numInFlight);
    }

    {
        auto oRenderer = star::common::Renderer(CreateOffscreenRenderer(context, numInFlight, camera, m_mainLight));
        auto *offscreenRenderer = oRenderer.getRaw<OffscreenRenderer>();

        const uint32_t width = context.getEngineResolution().width;
        const uint32_t height = context.getEngineResolution().height;
        std::vector<star::Handle> globalInfos(numInFlight);
        std::vector<star::Handle> lightInfos(numInFlight);

        size_t fNumFramesInFlight = 0;
        star::common::helper::SafeCast<uint8_t, size_t>(numFramesInFlight, fNumFramesInFlight);

        std::string vdbPath = mediaDirectoryPath + "volumes/dragon.vdb";
        m_volume = std::make_shared<Volume>(context, vdbPath, fNumFramesInFlight, camera, width, height,
                                            offscreenRenderer, offscreenRenderer->getCameraInfoBuffers(),
                                            offscreenRenderer->getLightInfoBuffers(),
                                            offscreenRenderer->getLightListBuffers());

        m_volume->init(context, numFramesInFlight);

        auto &s_i = m_volume->createInstance();
        s_i.setPosition(camPos);
        s_i.setScale(glm::vec3{1.0f, 1.0f, 1.0f});
        s_i.rotateRelative(star::Type::Axis::y, 90);

        std::vector<std::shared_ptr<StarObject>> objects{m_volume};
        std::vector<star::common::Renderer> additionals;
        additionals.emplace_back(std::move(oRenderer));
        star::common::Renderer sc{
            star::windowing::SwapChainRenderer{m_winContext, context, numFramesInFlight, objects, m_mainLight, camera}};
        m_mainScene = std::make_shared<star::StarScene>(std::move(camera), std::move(sc), std::move(additionals));
    }

    m_volume->getFogControlInfo().marchedInfo.defaultDensity = 0.0001f;
    m_volume->getFogControlInfo().marchedInfo.stepSizeDist = 3.0f;
    m_volume->getFogControlInfo().marchedInfo.stepSizeDist_light = 5.0f;
    m_volume->getFogControlInfo().marchedInfo.setSigmaAbsorption(0.00001f);
    m_volume->getFogControlInfo().marchedInfo.setSigmaScattering(0.8f);
    m_volume->getFogControlInfo().marchedInfo.setLightPropertyDirG(0.3f);
    m_volume->setFogType(VolumeRenderer::FogType::marched);

    // std::cout << "Application Controls" << std::endl;
    // std::cout << "B - Modify fog properties" << std::endl;
    // std::cout << "L - Set to linear fog rendering" << std::endl;
    // std::cout << "K - Set to marched fog rendering" << std::endl;
    // std::cout << "J - Set to exp fog rendering" << std::endl;
    // std::cout << std::endl;

    return m_mainScene;
}

// void Application::onKeyPress(int key, int scancode, int mods)
// {
//     //  if (key == star::KEY::H && !m_volume->udpdateVolumeRender)
//     //      m_volume->udpdateVolumeRender = true;
//     //  if (key == star::KEY::V)
//     //      m_volume->isVisible = !m_volume->isVisible;
//     //  if (key == star::KEY::M) {
//     //      m_volume->rayMarchToAABB = false;
//     //      m_volume->rayMarchToVolumeBoundry = false;
//     //  }
//     //  if (key == star::KEY::J)
//     //  {
//     //      m_volume->rayMarchToVolumeBoundry =
//     //      !m_volume->rayMarchToVolumeBoundry; m_volume->rayMarchToAABB =
//     //      false;
//     //  }
//     //  if (key == star::KEY::K)
//     //  {
//     //      m_volume->rayMarchToAABB = !m_volume->rayMarchToAABB;
//     //      m_volume->rayMarchToVolumeBoundry = false;
//     //  }
// }

// void Application::onKeyRelease(int key, int scancode, int mods)
// {
//     // const float MILES_TO_METERS = 1609.35;

//     // if (key == star::KEY::P)
//     // {
//     //     auto time = std::time(nullptr);
//     //     auto tm = *std::localtime(&time);
//     //     std::ostringstream oss;
//     //     oss << std::put_time(&tm, "%Y-%m-%d-%H-%M-%S") << ".png";
//     //     auto stringName = oss.str();
//     //     star::StarEngine::takeScreenshot(stringName);
//     // }

//     if (key == star::KEY::SPACE)
//     {
//         m_flipScreenshotState = true;
//         // auto camPosition = this->m_mainScene->getCamera()->getPosition();
//         // camPosition.z += 1.0f;
//         // this->m_mainScene->getCamera()->setPosition(camPosition);

//         //     auto camPosition = this->scene->getCamera()->getPosition();
//         //     auto camLookDirection = this->scene->getCamera()->getForwardVector();

//         //     this->testObject->setPosition(glm::vec3{camPosition.x, camPosition.y, camPosition.z + 10});

//         // this->testObject->setPosition(glm::vec3{
//         //     camPosition.x + (static_cast<float>(MathHelpers::MilesToMeters(camLookDirection.x))),
//         //     camPosition.y + (static_cast<float>(MathHelpers::MilesToMeters(camLookDirection.y))),
//         //     camPosition.z + (static_cast<float>(MathHelpers::MilesToMeters(camLookDirection.z)))});
//     }

//     if (key == star::KEY::T)
//     {
//         std::cout << m_mainScene->getCamera()->getPosition().x << "," << m_mainScene->getCamera()->getPosition().y
//                   << "," << m_mainScene->getCamera()->getPosition().z << std::endl;
//     }

//     if (key == star::KEY::B)
//     {
//         std::cout << "Select fog property to change" << std::endl;
//         std::cout << "1 - LinearFog: Fog Near Distance" << std::endl;
//         std::cout << "2 - LinearFog: Fog Far Distance" << std::endl;
//         std::cout << "3 - ExpFog: Fog Density" << std::endl;
//         std::cout << "4 - MarchedFog: Default Density" << std::endl;
//         std::cout << "5 - MarchedFog: Sigma Absorption" << std::endl;
//         std::cout << "6 - MarchedFog: Sigma Scattering" << std::endl;
//         std::cout << "7 - MarchedFog: Light PropertyDirG" << std::endl;
//         std::cout << "8 - MarchedFog: Step Size" << std::endl;
//         std::cout << "9 - MarchedFog: Step Size Light" << std::endl;

//         int selectedMode;

//         {
//             std::string inputOption = std::string();
//             std::getline(std::cin, inputOption);
//             try
//             {
//                 selectedMode = std::stoi(inputOption);
//             }
//             catch (const std::exception &ex)
//             {
//                 std::cout << "Invalid option" << std::endl;
//                 return;
//             }
//         }

//         std::ostringstream oss;
//         oss << "Current value: ";

//         switch (selectedMode)
//         {
//         case (1):
//             oss << std::to_string(m_volume->getFogControlInfo().linearInfo.nearDist);
//             std::cout << oss.str() << std::endl;
//             m_volume->getFogControlInfo().linearInfo.nearDist = PromptForFloat("Select visibility");
//             break;
//         case (2):
//             oss << std::to_string(m_volume->getFogControlInfo().linearInfo.farDist);
//             std::cout << oss.str() << std::endl;
//             m_volume->getFogControlInfo().linearInfo.farDist = PromptForFloat("Select distance");
//             break;
//         case (3):
//             oss << std::to_string(m_volume->getFogControlInfo().expFogInfo.density);
//             std::cout << oss.str() << std::endl;
//             m_volume->getFogControlInfo().expFogInfo.density = PromptForFloat("Select density");
//             break;
//         case (4):
//             oss << std::to_string(m_volume->getFogControlInfo().marchedInfo.defaultDensity);
//             std::cout << oss.str() << std::endl;
//             m_volume->getFogControlInfo().marchedInfo.defaultDensity = PromptForFloat("Select density");
//             break;
//         case (5):
//             oss << std::to_string(m_volume->getFogControlInfo().marchedInfo.getSigmaAbsorption());
//             std::cout << oss.str() << std::endl;
//             m_volume->getFogControlInfo().marchedInfo.setSigmaAbsorption(PromptForFloat("Select sigma"));
//             break;
//         case (6):
//             oss << std::to_string(m_volume->getFogControlInfo().marchedInfo.getSigmaScattering());
//             std::cout << oss.str() << std::endl;
//             m_volume->getFogControlInfo().marchedInfo.setSigmaScattering(PromptForFloat("Select sigma"));
//             break;
//         case (7):
//             oss << std::to_string(m_volume->getFogControlInfo().marchedInfo.getLightPropertyDirG());
//             std::cout << oss.str() << std::endl;
//             m_volume->getFogControlInfo().marchedInfo.setLightPropertyDirG(PromptForFloat("Select light prop",
//             true)); break;
//         case (8):
//             oss << std::to_string(m_volume->getFogControlInfo().marchedInfo.stepSizeDist);
//             std::cout << oss.str() << std::endl;
//             m_volume->getFogControlInfo().marchedInfo.stepSizeDist = PromptForFloat("Select step size");
//             break;
//         case (9):
//             oss << std::to_string(m_volume->getFogControlInfo().marchedInfo.stepSizeDist_light);
//             std::cout << oss.str() << std::endl;
//             m_volume->getFogControlInfo().marchedInfo.stepSizeDist_light = PromptForFloat("Select step size light");
//             break;
//         default:
//             std::cout << "Unknown option" << std::endl;
//         }
//     }

//     if (key == star::KEY::L)
//     {
//         std::cout << "Setting fog type to: Linear" << std::endl;
//         m_volume->setFogType(VolumeRenderer::FogType::linear);
//     }

//     if (key == star::KEY::K)
//     {
//         std::cout << "Setting fog type to: Ray Marched" << std::endl;
//         m_volume->setFogType(VolumeRenderer::FogType::marched);
//     }

//     if (key == star::KEY::J)
//     {
//         std::cout << "Setting fog type to: Exponential" << std::endl;
//         m_volume->setFogType(VolumeRenderer::FogType::exp);
//     }

//     if (key == star::KEY::H)
//     {
//         std::cout << "Setting fog type to: Nano Bounding Box" << std::endl;
//         m_volume->setFogType(VolumeRenderer::FogType::nano_boundingBox);
//     }

//     if (key == star::KEY::G)
//     {
//         std::cout << "Setting fog type to: NANO Surface" << std::endl;
//         m_volume->setFogType(VolumeRenderer::FogType::nano_surface);
//     }

//     if (key == star::KEY::P)
//     {
//         auto pos = m_volume->getInstance(0).getPosition();
//         pos.x += 10;
//         pos.y += 10;
//         m_volume->getInstance(0).setPosition(pos);
//     }
//     if (key == star::KEY::O)
//     {
//         glm::vec3 newScale = m_volume->getInstance(0).getScale() + 1.0f;
//         m_volume->getInstance(0).setScale(newScale);
//     }
//     if (key == star::KEY::I)
//     {
//         m_volume->getInstance(0).rotateGlobal(star::Type::Axis::y, 90);
//     }
// }

// void Application::onMouseMovement(double xpos, double ypos)
// {
// }

// void Application::onMouseButtonAction(int button, int action, int mods)
// {
// }

// void Application::onScroll(double xoffset, double yoffset)
// {
// }

void Application::frameUpdate(star::core::SystemContext &context, const uint8_t &frameInFlightIndex)
{
    if (m_flipScreenshotState)
    {
        m_triggerScreenshot = !m_triggerScreenshot;
        std::ostringstream oss;
        if (m_triggerScreenshot)
        {
            oss << "Starting screen capture on frame: ";
        }
        else
        {
            oss << "Ending screen capture on frame: ";
        }
        oss << context.getAllDevices().getData()[0].getCurrentFrameIndex();
        star::core::logging::log(boost::log::trivial::info, oss.str());

        m_flipScreenshotState = false;
    }
    if (m_triggerScreenshot)
    {
        triggerScreenshot(context.getAllDevices().getData()[0], frameInFlightIndex);
    }
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

OffscreenRenderer Application::CreateOffscreenRenderer(star::core::device::DeviceContext &context,
                                                       const uint8_t &numFramesInFlight,
                                                       std::shared_ptr<star::windowing::BasicCamera> camera,
                                                       std::shared_ptr<std::vector<star::Light>> mainLight)
{
    auto mediaDirectoryPath = star::ConfigFile::getSetting(star::Config_Settings::mediadirectory);
    auto terrainInfoPath = mediaDirectoryPath + "terrains/height_info.json";
    auto terrain = std::make_shared<Terrain>(context, terrainInfoPath);
    terrain->init(context);
    terrain->createInstance();
    std::vector<std::shared_ptr<star::StarObject>> objects{terrain};

    // auto horsePath = mediaDirectoryPath + "models/horse/WildHorse.obj";
    // auto horse = std::make_shared<star::BasicObject>(horsePath);
    // auto h_i = horse->createInstance();
    // h_i.setPosition(glm::vec3{0.0, 0.0, 0.0});
    // horse->init(context);
    // std::vector<std::shared_ptr<star::StarObject>> objects{horse};

    return {context, numFramesInFlight, objects, std::move(mainLight), camera};
}

void Application::triggerScreenshot(star::core::device::DeviceContext &context, const uint8_t &frameInFlightIndex)
{
    std::ostringstream oss;
    oss << "Test" << std::to_string(context.getCurrentFrameIndex());
    oss << ".png";

    auto *render = m_mainScene->getPrimaryRenderer().getRaw<star::windowing::SwapChainRenderer>();
    auto targetTexture = context.getImageManager().get(render->getRenderToColorImages()[frameInFlightIndex])->texture;

    context.getEventBus().emit(star::event::TriggerScreenshot{std::move(targetTexture), render->getCommandBuffer(),
                                                              m_screenshotRegistrations[frameInFlightIndex],
                                                              frameInFlightIndex, oss.str()});
}