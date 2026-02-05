#include "InteractiveApplication.hpp"

#ifdef STAR_ENABLE_PRESENTATION

#include "Terrain.hpp"
#include "command/image_metrics/TriggerCapture.hpp"

#include <starlight/command/CreateObject.hpp>
#include <starlight/command/detail/create_object/DirectObjCreation.hpp>
#include <starlight/command/detail/create_object/FromObjFileLoader.hpp>
#include <starlight/common/ConfigFile.hpp>
#include <starlight/common/objects/BasicObject.hpp>
#include <starlight/event/TriggerScreenshot.hpp>

#include <star_windowing/InteractivityBus.hpp>
#include <star_windowing/SwapChainRenderer.hpp>
#include <star_windowing/event/RequestSwapChainFromService.hpp>

OffscreenRenderer InteractiveApplication::createOffscreenRenderer(star::core::device::DeviceContext &context,
                                                                  const uint8_t &numFramesInFlight,
                                                                  std::shared_ptr<star::windowing::BasicCamera> camera,
                                                                  std::shared_ptr<std::vector<star::Light>> mainLight)
{
    std::vector<std::shared_ptr<star::StarObject>> objects;
    const auto mediaDirectoryPath = star::ConfigFile::getSetting(star::Config_Settings::mediadirectory);

    //{
    //    auto terrainInfoPath = mediaDirectoryPath + "terrains/height_info.json";
    //    auto cmd = star::command::CreateObject::Builder()
    //                   .setLoader(std::make_unique<star::command::create_object::DirectObjCreation>(
    //                       std::make_shared<Terrain>(context, terrainInfoPath)))
    //                   .setUniqueName("terrain")
    //                   .build();
    //    context.begin().set(cmd).submit();
    //    objects.emplace_back(cmd.getReply().get());
    //}

    // auto terrain = std::make_shared<Terrain>(context, terrainInfoPath);
    // terrain->init(context);
    // terrain->createInstance();
    // std::vector<std::shared_ptr<star::StarObject>> objects{terrain};

    auto horsePath = mediaDirectoryPath + "models/horse/WildHorse.obj";
    auto cmd = star::command::CreateObject::Builder()
                   .setLoader(std::make_unique<star::command::create_object::FromObjFileLoader>(horsePath))
                   .setUniqueName("horse")
                   .build();
    context.begin().set(cmd).submit();
    cmd.getReply().get()->init(context);
    objects.emplace_back(cmd.getReply().get());
    return {context, numFramesInFlight, objects, std::move(mainLight), camera};
}

void InteractiveApplication::frameUpdate(star::core::SystemContext &context)
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
        star::core::logging::info(oss.str());

        m_flipScreenshotState = false;
    }
    if (m_triggerScreenshot)
    {
        triggerScreenshot(context.getAllDevices().getData()[0], context.getAllDevices().getData()[0].getFrameTracker());
    }
}

void InteractiveApplication::onKeyRelease(const int &key, const int &scancode, const int &mods)
{
    if (key == GLFW_KEY_SPACE)
    {
        m_flipScreenshotState = true;
    }

    if (key == GLFW_KEY_X)
    {
        m_volume->getInstance().rotateRelative(star::Type::Axis::x, 90);
    }

    if (key == GLFW_KEY_C)
    {
        m_volume->getInstance().rotateRelative(star::Type::Axis::y, 90);
    }
    if (key == GLFW_KEY_Z)
    {
        m_volume->getInstance().rotateRelative(star::Type::Axis::z, 90);
    }

    if (key == GLFW_KEY_UP)
    {
        m_volume->getInstance().moveRelative(glm::vec3{0.0, 1.0, 0.0});
    }
    if (key == GLFW_KEY_DOWN)
    {
        m_volume->getInstance().moveRelative(glm::vec3{0.0, -1.0, 0.0});
    }

    const float MILES_TO_METERS = 1609.35;

    // if (key == GLFW_KEY_P)
    // {
    //     auto time = std::time(nullptr);
    //     auto tm = *std::localtime(&time);
    //     std::ostringstream oss;
    //     oss << std::put_time(&tm, "%Y-%m-%d-%H-%M-%S") << ".png";
    //     auto stringName = oss.str();
    //     star::StarEngine::takeScreenshot(stringName);
    // }

    // if (key == GLFW_KEY_SPACE)
    // {
    //     m_flipScreenshotState = true;
    //     auto camPosition = this->m_mainScene->getCamera()->getPosition();
    //     camPosition.z += 1.0f;
    //     this->m_mainScene->getCamera()->setPosition(camPosition);

    //     // auto camPosition = this->scene->getCamera()->getPosition();
    //     // auto camLookDirection = this->scene->getCamera()->getForwardVector();

    //     this->testObject->setPosition(glm::vec3{camPosition.x, camPosition.y, camPosition.z + 10});

    //     // this->testObject->setPosition(
    //     //     glm::vec3{camPosition.x + (static_cast<float>(MathHelpers::MilesToMeters(camLookDirection.x))),
    //     //               camPosition.y + (static_cast<float>(MathHelpers::MilesToMeters(camLookDirection.y))),
    //     //               camPosition.z + (static_cast<float>(MathHelpers::MilesToMeters(camLookDirection.z)))});
    // }

    if (key == GLFW_KEY_T)
    {
        std::cout << m_mainScene->getCamera()->getPosition().x << "," << m_mainScene->getCamera()->getPosition().y
                  << "," << m_mainScene->getCamera()->getPosition().z << std::endl;
    }

    if (key == GLFW_KEY_B)
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

        std::ostringstream oss;
        oss << "Current value: ";

        switch (selectedMode)
        {
        case (1):
            oss << std::to_string(m_volume->getFogControlInfo().linearInfo.nearDist);
            std::cout << oss.str() << std::endl;
            m_volume->getFogControlInfo().linearInfo.nearDist = PromptForFloat("Select visibility");
            break;
        case (2):
            oss << std::to_string(m_volume->getFogControlInfo().linearInfo.farDist);
            std::cout << oss.str() << std::endl;
            m_volume->getFogControlInfo().linearInfo.farDist = PromptForFloat("Select distance");
            break;
        case (3):
            oss << std::to_string(m_volume->getFogControlInfo().expFogInfo.density);
            std::cout << oss.str() << std::endl;
            m_volume->getFogControlInfo().expFogInfo.density = PromptForFloat("Select density");
            break;
        case (4):
            oss << std::to_string(m_volume->getFogControlInfo().marchedInfo.defaultDensity);
            std::cout << oss.str() << std::endl;
            m_volume->getFogControlInfo().marchedInfo.defaultDensity = PromptForFloat("Select density");
            break;
        case (5):
            oss << std::to_string(m_volume->getFogControlInfo().marchedInfo.getSigmaAbsorption());
            std::cout << oss.str() << std::endl;
            m_volume->getFogControlInfo().marchedInfo.setSigmaAbsorption(PromptForFloat("Select sigma"));
            break;
        case (6):
            oss << std::to_string(m_volume->getFogControlInfo().marchedInfo.getSigmaScattering());
            std::cout << oss.str() << std::endl;
            m_volume->getFogControlInfo().marchedInfo.setSigmaScattering(PromptForFloat("Select sigma"));
            break;
        case (7):
            oss << std::to_string(m_volume->getFogControlInfo().marchedInfo.getLightPropertyDirG());
            std::cout << oss.str() << std::endl;
            m_volume->getFogControlInfo().marchedInfo.setLightPropertyDirG(PromptForFloat("Select light prop", true));
            break;
        case (8):
            oss << std::to_string(m_volume->getFogControlInfo().marchedInfo.stepSizeDist);
            std::cout << oss.str() << std::endl;
            m_volume->getFogControlInfo().marchedInfo.stepSizeDist = PromptForFloat("Select step size");
            break;
        case (9):
            oss << std::to_string(m_volume->getFogControlInfo().marchedInfo.stepSizeDist_light);
            std::cout << oss.str() << std::endl;
            m_volume->getFogControlInfo().marchedInfo.stepSizeDist_light = PromptForFloat("Select step size light");
            break;
        default:
            std::cout << "Unknown option" << std::endl;
        }
    }

    if (key == GLFW_KEY_L)
    {
        std::cout << "Setting fog type to: Linear" << std::endl;
        m_volume->setFogType(VolumeRenderer::FogType::linear);
    }

    if (key == GLFW_KEY_K)
    {
        std::cout << "Setting fog type to: Ray Marched" << std::endl;
        m_volume->setFogType(VolumeRenderer::FogType::marched);
    }

    if (key == GLFW_KEY_J)
    {
        std::cout << "Setting fog type to: Exponential" << std::endl;
        m_volume->setFogType(VolumeRenderer::FogType::exp);
    }

    if (key == GLFW_KEY_H)
    {
        std::cout << "Setting fog type to: Nano Bounding Box" << std::endl;
        m_volume->setFogType(VolumeRenderer::FogType::nano_boundingBox);
    }

    if (key == GLFW_KEY_G)
    {
        std::cout << "Setting fog type to: NANO Surface" << std::endl;
        m_volume->setFogType(VolumeRenderer::FogType::nano_surface);
    }

    if (key == GLFW_KEY_P)
    {
        auto pos = m_volume->getInstance(0).getPosition();
        pos.x += 10;
        pos.y += 10;
        m_volume->getInstance(0).setPosition(pos);
    }
    if (key == GLFW_KEY_O)
    {
        glm::vec3 newScale = static_cast<const star::StarObject *>(m_volume.get())->getInstance(0).getScale();
        newScale.x += 1.0f;
        newScale.y += 1.0f;
        newScale.z += 1.0f;

        m_volume->getInstance(0).setScale(newScale);
    }
    if (key == GLFW_KEY_I)
    {
        m_volume->getInstance(0).rotateGlobal(star::Type::Axis::y, 90);
    }
    if (key == GLFW_KEY_U)
    {
        m_volume->getInstance(0).rotateGlobal(star::Type::Axis::z, 90);
    }
    if (key == GLFW_KEY_Y)
    {
        m_volume->getInstance(0).rotateGlobal(star::Type::Axis::x, 90);
    }
}
std::shared_ptr<star::StarScene> InteractiveApplication::loadScene(star::core::device::DeviceContext &context,
                                                                   const uint8_t &numFramesInFlight)
{
    star::windowing::HandleKeyReleasePolicy<InteractiveApplication>::init(context.getEventBus());
    star::windowing::InteractivityBus::Init(&context.getEventBus(), m_winContext);

    m_captureTrigger = context.begin();
    m_captureTrigger.setName(image_metrics::TriggerCapture::GetUniqueTypeName());

    m_screenshotRegistrations.resize(context.getFrameTracker().getSetup().getNumUniqueTargetFramesForFinalization());

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

    {
        auto oRenderer =
            star::common::Renderer(createOffscreenRenderer(context, numFramesInFlight, camera, m_mainLight));
        auto *offscreenRenderer = oRenderer.getRaw<OffscreenRenderer>();

        const uint32_t &width = context.getEngineResolution().width;
        const uint32_t &height = context.getEngineResolution().height;
        std::vector<star::Handle> globalInfos(numFramesInFlight);
        std::vector<star::Handle> lightInfos(numFramesInFlight);

        size_t fNumFramesInFlight = 0;
        star::common::helper::SafeCast<uint8_t, size_t>(numFramesInFlight, fNumFramesInFlight);

        {
            std::string vdbPath = mediaDirectoryPath + "volumes/flat_plane_wind";
            m_volume = std::make_shared<Volume>(context, vdbPath, fNumFramesInFlight, camera, width, height,
                                                offscreenRenderer, offscreenRenderer->getCameraInfoBuffers(),
                                                offscreenRenderer->getLightInfoBuffers(),
                                                offscreenRenderer->getLightListBuffers());
            auto cmd = star::command::CreateObject::Builder()
                           .setLoader(std::make_unique<star::command::create_object::DirectObjCreation>(m_volume))
                           .setUniqueName("flat_plane_wind")
                           .build();

            context.begin().set(cmd).submit();
            auto shared = cmd.getReply().get();
        }

        m_volume->init(context, numFramesInFlight);

        auto &s_i = m_volume->createInstance();
        s_i.setPosition(camPos);
        s_i.setScale(glm::vec3{1.0f, 1.0f, 1.0f});
        s_i.rotateRelative(star::Type::Axis::y, 90);

        std::vector<std::shared_ptr<star::StarObject>> objects{m_volume};
        std::vector<star::common::Renderer> additional;
        additional.emplace_back(std::move(oRenderer));

        vk::SwapchainKHR swapchain{VK_NULL_HANDLE};
        context.getEventBus().emit(star::windowing::event::RequestSwapChainFromService{swapchain});
        star::common::Renderer sc{star::windowing::SwapChainRenderer{m_winContext, std::move(swapchain), context,
                                                                     numFramesInFlight, objects, m_mainLight, camera}};
        m_mainScene = std::make_shared<star::StarScene>(std::move(camera), std::move(sc), std::move(additional));
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

    // std::cout << "Application Controls" << std::endl;
    // std::cout << "B - Modify fog properties" << std::endl;
    // std::cout << "L - Set to linear fog rendering" << std::endl;
    // std::cout << "K - Set to marched fog rendering" << std::endl;
    // std::cout << "J - Set to exp fog rendering" << std::endl;
    // std::cout << std::endl;

    return m_mainScene;
}

void InteractiveApplication::triggerScreenshot(star::core::device::DeviceContext &context,
                                               const star::common::FrameTracker &frameTracker)
{
    std::ostringstream oss;
    oss << "Test" << std::to_string(context.getFrameTracker().getCurrent().getGlobalFrameCounter());
    oss << ".png";

    size_t index = static_cast<size_t>(frameTracker.getCurrent().getFinalTargetImageIndex());
    auto *render = m_mainScene->getPrimaryRenderer().getRaw<star::windowing::SwapChainRenderer>();
    auto targetTexture = context.getImageManager().get(render->getRenderToColorImages()[index])->texture;

    // submit screenshot processing
    context.getEventBus().emit(star::event::TriggerScreenshot{
        std::move(targetTexture), oss.str(), render->getCommandBuffer(), m_screenshotRegistrations[index]});

    //image_metrics::TriggerCapture trigger(oss.str(), *m_volume);
    //m_captureTrigger.update(trigger).submit();

    // submit file writing processing for distance information
    // m_imageInfoManager.recordThisFrame(context, oss.str());
    //
    // need a command buffer to handle copying the storage buffer from compute pass into host accessible memory

    // pass host visible buffer to secondary thread for processing and writing of data
}

#endif