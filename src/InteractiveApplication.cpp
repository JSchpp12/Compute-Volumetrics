#include "InteractiveApplication.hpp"

#ifdef STAR_ENABLE_PRESENTATION
#include <starlight/event/TriggerScreenshot.hpp>

#include <star_common/helper/StringHelpers.hpp>

#include <star_windowing/InteractivityBus.hpp>
#include <star_windowing/SwapChainRenderer.hpp>
#include <star_windowing/event/RequestSwapChainFromService.hpp>

void InteractiveApplication::frameUpdate(star::core::SystemContext &context)
{
    if (m_actDir[star::Type::Axis::x])
    {
        if (m_mode == ModifyMode::movement)
        {
            const glm::vec3 dir{10.0f, 0.0f, 0.0f};
            m_volume->getInstance(0).moveRelative(m_invAct ? -dir : dir);
        }
        else
        {
            m_volume->getInstance(0).rotateRelative(star::Type::Axis::x, 90);
            m_actDir[star::Type::Axis::x] = false;
        }
    }
    else if (m_actDir[star::Type::Axis::y])
    {
        if (m_mode == ModifyMode::movement)
        {
            const glm::vec3 dir{0.0f, 10.0f, 0.0f};
            m_volume->getInstance(0).moveRelative(m_invAct ? -dir : dir);
        }
        else
        {
            const float amt{90.0f};
            m_volume->getInstance(0).rotateRelative(star::Type::Axis::y, m_invAct ? -amt : amt);
            m_actDir[star::Type::Axis::y] = false;
        }
    }
    else if (m_actDir[star::Type::Axis::z])
    {
        if (m_mode == ModifyMode::movement)
        {
            const glm::vec3 dir{0.0f, 0.0f, 10.0f};
            m_volume->getInstance(0).moveRelative(m_invAct ? -dir : dir);
        }
        else
        {
            const float amt{90.0f};
            m_volume->getInstance(0).rotateRelative(star::Type::Axis::z, m_invAct ? -amt : amt);
            m_actDir[star::Type::Axis::z] = false;
        }
    }

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
    }

    if (m_switchMode)
    {
        if (m_mode == ModifyMode::movement)
        {
            m_mode = ModifyMode::rotation;
            star::core::logging::info("Set mode: rotation");
        }
        else
        {
            m_mode = ModifyMode::movement;
            star::core::logging::info("Set mode: movement");
        }
        m_switchMode = false;
    }

    if (m_triggerScreenshot)
    {
        auto &d = context.getAllDevices().getData()[0];
        TriggerSimUpdate(d.getCmdBus(), *m_volume, *m_mainScene->getCamera());
        triggerScreenshot(d);

        if (CheckIfControllerIsDone(d.getCmdBus()))
        {
            m_triggerScreenshot = false;
        }
    }
}

void InteractiveApplication::initListeners(star::core::device::DeviceContext &context)
{
    star::windowing::InteractivityBus::Init(&context.getEventBus(), m_winContext);

    star::windowing::HandleKeyReleasePolicy<InteractiveApplication>::init(context.getEventBus());
    star::windowing::HandleKeyPressPolicy<InteractiveApplication>::init(context.getEventBus());

    m_screenshotRegistrations.resize(context.getFrameTracker().getSetup().getNumUniqueTargetFramesForFinalization());
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
        m_mainScene->getCamera()->rotateGlobal(star::Type::Axis::y, 1.0);
    }

    if (key == GLFW_KEY_LEFT)
    {
        m_mainScene->getCamera()->rotateRelative(star::Type::Axis::y, -1.0);
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
        std::cout << "11 - MarchedFog: VDB Density Multiplier" << std::endl;

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
        case (11):
            oss << std::to_string(m_volume->getRenderer().getFogInfo().marchedInfo.getDensityMultiplier());
            std::cout << oss.str() << std::endl;
            m_volume->getRenderer().getFogInfo().marchedInfo.setDensityMultiplier(PromptForFloat("Select multipler"));
            break;
        default:
            std::cout << "Unknown option" << std::endl;
        }
    }

    if (key == GLFW_KEY_L)
    {
        std::cout << "Setting fog type to: Linear" << std::endl;
        m_volume->setFogType(Fog::Type::sLinear);
    }

    if (key == GLFW_KEY_K)
    {
        std::cout << "Setting fog type to: Ray Marched" << std::endl;
        m_volume->setFogType(Fog::Type::sMarched);
    }

    if (key == GLFW_KEY_J)
    {
        std::cout << "Setting fog type to: Exponential" << std::endl;
        m_volume->setFogType(Fog::Type::sExponential);
    }

    if (key == GLFW_KEY_H)
    {
        std::cout << "Setting fog type to: Nano Bounding Box" << std::endl;
        m_volume->setFogType(Fog::Type::sNanoBoundingBox);
    }

    if (key == GLFW_KEY_G)
    {
        std::cout << "Setting fog type to: NANO Surface" << std::endl;
        m_volume->setFogType(Fog::Type::sNanoSurface);
    }

    if (key == GLFW_KEY_F)
    {
        std::cout << "Setting fog type to: Homogenous" << std::endl;
        m_volume->setFogType(Fog::Type::sMarchedHomogenous);
    }

    if (key == GLFW_KEY_V)
    {
        const auto camPos = this->m_mainScene->getCamera()->getPosition();
        // this->m_mainScene->getCamera()->setPosition(glm::vec3{camPos.x, camPos.y, 0});
        std::ostringstream oss;

        star::core::logging::info("Cam position: " + std::to_string(camPos.x) + ',' + std::to_string(camPos.y) + ',' +
                                  std::to_string(camPos.z));
    }

    if (key == GLFW_KEY_O)
    {
        glm::vec3 newScale = static_cast<const star::StarObject *>(m_volume.get())->getInstance(0).getScale();
        newScale.x += 1.0f;
        newScale.y += 1.0f;
        newScale.z += 1.0f;

        m_volume->getInstance(0).setScale(newScale);
    }

    if (key == GLFW_KEY_Y)
    {
        m_actDir[star::Type::Axis::x] = false;
    }

    if (key == GLFW_KEY_U)
    {
        m_actDir[star::Type::Axis::y] = false;
    }

    if (key == GLFW_KEY_I)
    {
        m_actDir[star::Type::Axis::z] = false;
    }

    if (key == GLFW_KEY_P)
    {
        m_switchMode = true;
    }

    if (key == GLFW_KEY_M)
    {
        m_invAct = !m_invAct;
    }

    if (key == GLFW_KEY_SPACE)
    {
        m_actDir[0] = false; 
        m_actDir[1] = false; 
        m_actDir[2] = false; 
    }
}

void InteractiveApplication::onKeyPress(const int &key, const int &scancode, const int &mods)
{
    if (key == GLFW_KEY_Y)
    {
        m_actDir[star::Type::Axis::x] = true;
    }

    if (key == GLFW_KEY_U)
    {
        m_actDir[star::Type::Axis::y] = true;
    }

    if (key == GLFW_KEY_I)
    {
        m_actDir[star::Type::Axis::z] = true;
    }
}
void InteractiveApplication::initImageOutputDir(star::core::CommandBus &bus)
{
    m_imageOutputDir = std::filesystem::path(star::common::strings::GetStartTime());
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

    triggerImageRecord(context, frameTracker, name);
}

std::shared_ptr<star::StarCamera> InteractiveApplication::createMainCamera(star::core::device::DeviceContext &context)
{
    auto camera = std::make_shared<star::windowing::BasicCamera>(
        context.getEngineResolution().width, context.getEngineResolution().height, 90.0f, 0.5f, 25000.0f, 100.0f, 0.1f);

    camera->init(context.getEventBus());
    return camera;
}

star::common::Renderer InteractiveApplication::createMainRenderer(
    star::core::device::DeviceContext &context, std::vector<std::shared_ptr<star::StarObject>> objects,
    std::shared_ptr<star::StarCamera> camera)
{
    vk::SwapchainKHR swapchain{VK_NULL_HANDLE};
    context.getEventBus().emit(star::windowing::event::RequestSwapChainFromService{swapchain});
    return star::common::Renderer{star::windowing::SwapChainRenderer{
        m_winContext, std::move(swapchain), context, context.getFrameTracker().getSetup().getNumFramesInFlight(),
        objects, m_mainLight, camera}};
}

#endif