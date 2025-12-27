#include "InteractiveApplication.hpp"

#include "Terrain.hpp"
#include <starlight/event/TriggerScreenshot.hpp>
#include <star_windowing/InteractivityBus.hpp>
#include <star_windowing/SwapChainRenderer.hpp>
#include <star_windowing/event/RequestSwapChainFromService.hpp>
#include <starlight/common/ConfigFile.hpp>
#include <starlight/common/objects/BasicObject.hpp>

OffscreenRenderer CreateOffscreenRenderer(star::core::device::DeviceContext &context, const uint8_t &numFramesInFlight,
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
void InteractiveApplication::frameUpdate(star::core::SystemContext &context, const uint8_t &frameInFlightIndex)
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
        oss << context.getAllDevices().getData()[0].getFrameTracker().getCurrent().getGlobalFrameCounter();
        star::core::logging::log(boost::log::trivial::info, oss.str());

        m_flipScreenshotState = false;
    }
    if (m_triggerScreenshot)
    {
        triggerScreenshot(context.getAllDevices().getData()[0], frameInFlightIndex);
    }
}

void InteractiveApplication::onKeyRelease(const int &key, const int &scancode, const int &mods)
{
    if (key == GLFW_KEY_SPACE)
    {
        m_flipScreenshotState = true;
    }
}
std::shared_ptr<star::StarScene> InteractiveApplication::loadScene(star::core::device::DeviceContext &context,
                                                                   const uint8_t &numFramesInFlight)
{
    star::windowing::HandleKeyReleasePolicy<InteractiveApplication>::init(context.getEventBus());

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

        std::vector<std::shared_ptr<star::StarObject>> objects{m_volume};
        std::vector<star::common::Renderer> additionals;
        additionals.emplace_back(std::move(oRenderer));

        vk::SwapchainKHR swapchain{VK_NULL_HANDLE};

        context.getEventBus().emit(star::windowing::event::RequestSwapChainFromService{swapchain});

        star::common::Renderer sc{star::windowing::SwapChainRenderer{m_winContext, std::move(swapchain), context,
                                                                     numFramesInFlight, objects, m_mainLight, camera}};
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

void InteractiveApplication::triggerScreenshot(star::core::device::DeviceContext &context,
                                               const uint8_t &frameInFlightIndex)
{
    std::ostringstream oss;
    oss << "Test" << std::to_string(context.getFrameTracker().getCurrent().getGlobalFrameCounter());
    oss << ".png";

    auto *render = m_mainScene->getPrimaryRenderer().getRaw<star::windowing::SwapChainRenderer>();
    auto targetTexture = context.getImageManager().get(render->getRenderToColorImages()[frameInFlightIndex])->texture;

    context.getEventBus().emit(star::event::TriggerScreenshot{std::move(targetTexture), render->getCommandBuffer(),
                                                              m_screenshotRegistrations[frameInFlightIndex],
                                                              frameInFlightIndex, oss.str(), true});
}