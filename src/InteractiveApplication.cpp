#include "InteractiveApplication.hpp"

#ifdef STAR_ENABLE_PRESENTATION

#include "Terrain.hpp"
#include "command/image_metrics/TriggerCapture.hpp"

#include <starlight/command/CreateObject.hpp>
#include <starlight/command/command_order/DeclareDependency.hpp>
#include <starlight/command/command_order/DeclarePass.hpp>
#include <starlight/command/detail/create_object/DirectObjCreation.hpp>
#include <starlight/command/detail/create_object/FromObjFileLoader.hpp>
#include <starlight/common/ConfigFile.hpp>
#include <starlight/common/objects/BasicObject.hpp>
#include <starlight/event/TriggerScreenshot.hpp>

#include <star_windowing/InteractivityBus.hpp>
#include <star_windowing/SwapChainRenderer.hpp>
#include <star_windowing/event/RequestSwapChainFromService.hpp>

#include <boost/filesystem/path.hpp>

OffscreenRenderer InteractiveApplication::createOffscreenRenderer(
    star::core::device::DeviceContext &context, const uint8_t &numFramesInFlight,
    std::shared_ptr<star::windowing::BasicCamera> m_camera, std::shared_ptr<std::vector<star::Light>> mainLight)
{
    std::vector<std::shared_ptr<star::StarObject>> objects;
    const auto mediaDirectoryPath = star::ConfigFile::getSetting(star::Config_Settings::mediadirectory);

    {
        auto terrainInfoPath = mediaDirectoryPath + "terrains/height_info.json";
        auto cmd = star::command::CreateObject::Builder()
                       .setLoader(std::make_unique<star::command::create_object::DirectObjCreation>(
                           std::make_shared<Terrain>(context, terrainInfoPath)))
                       .setUniqueName("terrain")
                       .build();
        context.begin().set(cmd).submit();
        objects.emplace_back(cmd.getReply().get());
    }

    //{
    //    auto horsePath = mediaDirectoryPath + "models/horse/WildHorse.obj";
    //    auto cmd = star::command::CreateObject::Builder()
    //                   .setLoader(std::make_unique<star::command::create_object::FromObjFileLoader>(horsePath))
    //                   .setUniqueName("horse")
    //                   .build();
    //    context.begin().set(cmd).submit();
    //    cmd.getReply().get()->init(context);
    //    objects.emplace_back(cmd.getReply().get());
    //}

    return {context, numFramesInFlight, objects, std::move(mainLight), m_camera};
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
        if (!m_cameraController->isDone())
        {
            m_cameraController->frameUpdate(context.getAllDevices().getData()[0]);
            triggerScreenshot(context.getAllDevices().getData()[0]);
        }
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
        const auto pos = m_mainScene->getCamera()->getPosition();
        m_mainScene->getCamera()->setPosition(glm::vec3{pos.x, pos.y + 1.0, pos.z});
    }

    if (key == GLFW_KEY_DOWN)
    {
        const auto pos = m_mainScene->getCamera()->getPosition();
        m_mainScene->getCamera()->setPosition(glm::vec3{pos.x, pos.y - 1.0, pos.z});
    }

    const float MILES_TO_METERS = 1609.35;

    if (key == GLFW_KEY_T)
    {
        std::cout << m_mainScene->getCamera()->getPosition().x << "," << m_mainScene->getCamera()->getPosition().y
                  << "," << m_mainScene->getCamera()->getPosition().z << std::endl;
    }

    if (key == GLFW_KEY_RIGHT)
    {
        m_camera->rotateGlobal(star::Type::Axis::y, 1.0);
    }

    if (key == GLFW_KEY_LEFT)
    {
        m_camera->rotateRelative(star::Type::Axis::y, -1.0);
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
        std::cout << "10 - HomogenousRendering: Max Num Steps" << std::endl;

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
            oss << std::to_string(m_volume->getRenderer().getFogInfo().linearInfo.nearDist);
            std::cout << oss.str() << std::endl;
            m_volume->getRenderer().getFogInfo().linearInfo.nearDist = PromptForFloat("Select visibility");
            break;
        case (2):
            oss << std::to_string(m_volume->getRenderer().getFogInfo().linearInfo.farDist);
            std::cout << oss.str() << std::endl;
            m_volume->getRenderer().getFogInfo().linearInfo.farDist = PromptForFloat("Select distance");
            break;
        case (3):
            oss << std::to_string(m_volume->getRenderer().getFogInfo().expFogInfo.density);
            std::cout << oss.str() << std::endl;
            m_volume->getRenderer().getFogInfo().expFogInfo.density = PromptForFloat("Select density");
            break;
        case (4):
            oss << std::to_string(m_volume->getRenderer().getFogInfo().marchedInfo.defaultDensity);
            std::cout << oss.str() << std::endl;
            m_volume->getRenderer().getFogInfo().marchedInfo.defaultDensity = PromptForFloat("Select density");
            break;
        case (5):
            oss << std::to_string(m_volume->getRenderer().getFogInfo().marchedInfo.getSigmaAbsorption());
            std::cout << oss.str() << std::endl;
            m_volume->getRenderer().getFogInfo().marchedInfo.setSigmaAbsorption(PromptForFloat("Select sigma"));
            break;
        case (6):
            oss << std::to_string(m_volume->getRenderer().getFogInfo().marchedInfo.getSigmaScattering());
            std::cout << oss.str() << std::endl;
            m_volume->getRenderer().getFogInfo().marchedInfo.setSigmaScattering(PromptForFloat("Select sigma"));
            break;
        case (7):
            oss << std::to_string(m_volume->getRenderer().getFogInfo().marchedInfo.getLightPropertyDirG());
            std::cout << oss.str() << std::endl;
            m_volume->getRenderer().getFogInfo().marchedInfo.setLightPropertyDirG(
                PromptForFloat("Select light prop", true));
            break;
        case (8):
            oss << std::to_string(m_volume->getRenderer().getFogInfo().marchedInfo.stepSizeDist);
            std::cout << oss.str() << std::endl;
            m_volume->getRenderer().getFogInfo().marchedInfo.stepSizeDist = PromptForFloat("Select step size");
            break;
        case (9):
            oss << std::to_string(m_volume->getRenderer().getFogInfo().marchedInfo.stepSizeDist_light);
            std::cout << oss.str() << std::endl;
            m_volume->getRenderer().getFogInfo().marchedInfo.stepSizeDist_light =
                PromptForFloat("Select step size light");
            break;
        case (10):
            oss << std::to_string(m_volume->getRenderer().getFogInfo().homogenousInfo.getMaxNumSteps());
            std::cout << oss.str() << std::endl;
            m_volume->getRenderer().getFogInfo().homogenousInfo.setMaxNumSteps(
                PromptForInt("Select max number of steps"));
            break;
        default:
            std::cout << "Unknown option" << std::endl;
        }
    }

    if (key == GLFW_KEY_L)
    {
        std::cout << "Setting fog type to: Linear" << std::endl;
        m_volume->setFogType(Fog::Type::linear);
    }

    if (key == GLFW_KEY_K)
    {
        std::cout << "Setting fog type to: Ray Marched" << std::endl;
        m_volume->setFogType(Fog::Type::marched);
    }

    if (key == GLFW_KEY_J)
    {
        std::cout << "Setting fog type to: Exponential" << std::endl;
        m_volume->setFogType(Fog::Type::exp);
    }

    if (key == GLFW_KEY_H)
    {
        std::cout << "Setting fog type to: Nano Bounding Box" << std::endl;
        m_volume->setFogType(Fog::Type::nano_boundingBox);
    }

    if (key == GLFW_KEY_G)
    {
        std::cout << "Setting fog type to: NANO Surface" << std::endl;
        m_volume->setFogType(Fog::Type::nano_surface);
    }

    if (key == GLFW_KEY_F)
    {
        std::cout << "Setting fog type to: Homogenous" << std::endl;
        m_volume->setFogType(Fog::Type::marched_homogenous);
    }

    if (key == GLFW_KEY_P)
    {
        auto pos = m_volume->getInstance(0).getPosition();
        pos.x += 10;
        pos.y += 10;
        m_volume->getInstance(0).setPosition(pos);
    }

    if (key == GLFW_KEY_V)
    {
        auto camPos = this->m_mainScene->getCamera()->getPosition();
        this->m_mainScene->getCamera()->setPosition(glm::vec3{camPos.x, camPos.y, 0});
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

    if (key == GLFW_KEY_Q)
    {
        m_mainScene->getCamera()->setForwardVector(glm::vec3{1.0, 0.0, 0.0});
    }
}

std::shared_ptr<star::StarScene> InteractiveApplication::loadScene(star::core::device::DeviceContext &context,
                                                                   const uint8_t &numFramesInFlight)
{
    star::windowing::HandleKeyReleasePolicy<InteractiveApplication>::init(context.getEventBus());
    star::windowing::InteractivityBus::Init(&context.getEventBus(), m_winContext);

    m_captureTrigger = context.begin();
    m_captureTrigger.setType(image_metrics::TriggerCapture::GetUniqueTypeName());

    m_screenshotRegistrations.resize(context.getFrameTracker().getSetup().getNumUniqueTargetFramesForFinalization());

    auto mediaDirectoryPath = star::ConfigFile::getSetting(star::Config_Settings::mediadirectory);
    const glm::vec3 camPos{-50.9314, 40.686, 25.9329};
    const glm::vec3 volumePos{50.0, 10.0, 0.0};
    const glm::vec3 lightPos = volumePos + glm::vec3{0.0f, 500.0f, 0.0f};
    m_camera = std::make_shared<star::windowing::BasicCamera>(
        context.getEngineResolution().width, context.getEngineResolution().height, 90.0f, 1.0f, 20000.0f, 100.0f, 0.1f);
    m_camera->init(context.getEventBus());

    m_camera->setPosition(camPos);
    m_camera->setForwardVector(volumePos - m_camera->getPosition());

    m_mainLight = std::make_shared<std::vector<star::Light>>(
        std::vector<star::Light>{star::Light(lightPos, star::Type::Light::directional, glm::vec3{0.0, -1.0, 0.0})});

    {
        auto oRenderer =
            star::common::Renderer(createOffscreenRenderer(context, numFramesInFlight, m_camera, m_mainLight));
        auto *offscreenRenderer = oRenderer.getRaw<OffscreenRenderer>();

        const uint32_t &width = context.getEngineResolution().width;
        const uint32_t &height = context.getEngineResolution().height;
        std::vector<star::Handle> globalInfos(numFramesInFlight);
        std::vector<star::Handle> lightInfos(numFramesInFlight);

        size_t fNumFramesInFlight = 0;
        star::common::helper::SafeCast<uint8_t, size_t>(numFramesInFlight, fNumFramesInFlight);

        {
            std::string vdbPath;
            {
                boost::filesystem::path rPath =
                    boost::filesystem::path(mediaDirectoryPath) / "volumes" / "flat_plane_wind";
                vdbPath = rPath.string();
            }

            m_volume = std::make_shared<Volume>(context, vdbPath, fNumFramesInFlight, m_camera, width, height,
                                                offscreenRenderer, offscreenRenderer->getCameraInfoBuffers(),
                                                offscreenRenderer->getLightInfoBuffers(),
                                                offscreenRenderer->getLightListBuffers());
            auto cmd = star::command::CreateObject::Builder()
                           .setLoader(std::make_unique<star::command::create_object::DirectObjCreation>(m_volume))
                           .setUniqueName("flatWind")
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
        star::common::Renderer sc{star::windowing::SwapChainRenderer{
            m_winContext, std::move(swapchain), context, numFramesInFlight, objects, m_mainLight, m_camera}};
        m_mainScene = std::make_shared<star::StarScene>(star::star_scene::makeAlwaysReadyPolicy(), m_camera,
                                                        std::move(sc), std::move(additional));
    }

    m_cameraController = std::make_unique<CircleCameraController>(m_camera, m_volume);
    m_cameraController->init(context);

    m_volume->getRenderer().getFogInfo().marchedInfo.defaultDensity = 0.03f;
    m_volume->getRenderer().getFogInfo().marchedInfo.stepSizeDist = 3.0f;
    m_volume->getRenderer().getFogInfo().marchedInfo.stepSizeDist_light = 5.0f;
    m_volume->getRenderer().getFogInfo().marchedInfo.setSigmaAbsorption(0.0f);
    m_volume->getRenderer().getFogInfo().marchedInfo.setSigmaScattering(0.8f);
    m_volume->getRenderer().getFogInfo().marchedInfo.setLightPropertyDirG(0.7f);
    m_volume->setFogType(Fog::Type::marched);
    m_volume->getRenderer().getFogInfo().linearInfo.nearDist = 0.01f;
    m_volume->getRenderer().getFogInfo().linearInfo.farDist = 1000.0f;
    m_volume->getRenderer().getFogInfo().expFogInfo.density = 0.03f;
    return m_mainScene;
}

void InteractiveApplication::triggerScreenshot(star::core::device::DeviceContext &context)
{
    const auto &frameTracker = context.getFrameTracker();

    const std::string name =
        "Test" + std::to_string(context.getFrameTracker().getCurrent().getGlobalFrameCounter()) + ".png";
    const auto path = (std::filesystem::path(m_imageOutputDir) / name).string();

    size_t index = static_cast<size_t>(frameTracker.getCurrent().getFinalTargetImageIndex());
    auto *render = m_mainScene->getPrimaryRenderer().getRaw<star::windowing::SwapChainRenderer>();

    // submit screenshot processing
    context.getEventBus().emit(
        star::event::TriggerScreenshot(context.getImageManager().get(render->getRenderToColorImages()[index])->texture,
                                       path, render->getCommandBuffer(), m_screenshotRegistrations[index]));

    triggerImageRecord(context, frameTracker, path);
}

#endif