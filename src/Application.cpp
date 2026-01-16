#include "Application.hpp"

#include "Terrain.hpp"

#include "ManagerController_RenderResource_GlobalInfo.hpp"
#include "OffscreenRenderer.hpp"
#include <starlight/common/ConfigFile.hpp>
#include <starlight/common/objects/BasicObject.hpp>
#include <starlight/core/logging/LoggingFactory.hpp>
#include <starlight/core/renderer/HeadlessRenderer.hpp>
#include <starlight/event/RegisterMainGraphicsRenderer.hpp>
#include <starlight/virtual/StarCamera.hpp>

#include <sstream>
#include <string>

using namespace star;

OffscreenRenderer CreateOffscreenRenderer(star::core::device::DeviceContext &context, const uint8_t &numFramesInFlight,
                                          std::shared_ptr<star::StarCamera> camera,
                                          std::shared_ptr<std::vector<star::Light>> mainLight)
{
    auto mediaDirectoryPath = star::ConfigFile::getSetting(star::Config_Settings::mediadirectory);

    auto terrainInfoPath = mediaDirectoryPath + "terrains/height_info.json";
    auto terrain = std::make_shared<Terrain>(context, terrainInfoPath);
    terrain->init(context);
    terrain->createInstance();
    std::vector<std::shared_ptr<star::StarObject>> objects{terrain};

     //auto horsePath = mediaDirectoryPath + "models/horse/WildHorse.obj";
     //auto horse = std::make_shared<star::BasicObject>(horsePath);
     //auto &h_i = horse->createInstance();
     //h_i.setPosition(glm::vec3{0.0, 0.0, 0.0});
     //horse->init(context);
     //std::vector<std::shared_ptr<star::StarObject>> objects{horse};

    return {context, numFramesInFlight, objects, std::move(mainLight), camera};
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

std::shared_ptr<star::StarScene> Application::loadScene(star::core::device::DeviceContext &context,
                                                        const uint8_t &numFramesInFlight)
{
    auto mediaDirectoryPath = star::ConfigFile::getSetting(star::Config_Settings::mediadirectory);
    const glm::vec3 camPos{-50.9314, 135.686, 25.9329};
    const glm::vec3 volumePos{50.0, 10.0, 0.0};
    const glm::vec3 lightPos = volumePos + glm::vec3{0.0f, 500.0f, 0.0f};
    std::shared_ptr<star::StarCamera> camera = std::make_shared<star::StarCamera>(
        context.getEngineResolution().width, context.getEngineResolution().height, 90.0f, 1.0f, 20000.0f);

    camera->setPosition(camPos);
    camera->setForwardVector(volumePos - camera->getPosition());

    m_mainLight = std::make_shared<std::vector<star::Light>>(
        std::vector<star::Light>{star::Light(lightPos, star::Type::Light::directional, glm::vec3{-1.0, 0.0, 0.0})});
    m_mainLight->at(0).ambient = glm::vec4{1.0, 1.0, 1.0, 1.0};

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
        s_i.setScale(glm::vec3{3.0f, 3.0f, 3.0f});
        s_i.rotateRelative(star::Type::Axis::y, 90);

        std::vector<std::shared_ptr<star::StarObject>> objects{m_volume};
        std::vector<star::common::Renderer> additionals;
        additionals.emplace_back(std::move(oRenderer));

        star::common::Renderer sc{
            star::core::renderer::HeadlessRenderer{context, numFramesInFlight, objects, m_mainLight, camera}};

        auto *renderer = sc.getRaw<star::core::renderer::HeadlessRenderer>();
        context.getEventBus().emit(star::event::RegisterMainGraphicsRenderer{renderer});
        m_mainScene = std::make_shared<star::StarScene>(std::move(camera), std::move(sc), std::move(additionals));
    }

    m_volume->getFogControlInfo().marchedInfo.defaultDensity = 0.0001f;
    m_volume->getFogControlInfo().marchedInfo.stepSizeDist = 3.0f;
    m_volume->getFogControlInfo().marchedInfo.stepSizeDist_light = 5.0f;
    m_volume->getFogControlInfo().marchedInfo.setSigmaAbsorption(0.00001f);
    m_volume->getFogControlInfo().marchedInfo.setSigmaScattering(0.8f);
    m_volume->getFogControlInfo().marchedInfo.setLightPropertyDirG(0.3f);
    m_volume->setFogType(VolumeRenderer::FogType::marched);
    m_volume->getFogControlInfo().linearInfo.nearDist = 0.01f;
    m_volume->getFogControlInfo().linearInfo.farDist = 1000.0f;
    m_volume->getFogControlInfo().expFogInfo.density = 0.6f;
    return m_mainScene;
}

void Application::frameUpdate(star::core::SystemContext &context)
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