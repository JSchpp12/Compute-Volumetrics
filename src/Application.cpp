#include "Application.hpp"

#include "ManagerController_RenderResource_GlobalInfo.hpp"
#include "OffscreenRenderer.hpp"
#include "Terrain.hpp"
#include "TerrainChunk.hpp"
#include "command/image_metrics/TriggerCapture.hpp"
#include "command/sim_controller/TriggerUpdate.hpp"

#include <starlight/command/CreateObject.hpp>
#include <starlight/command/SaveSceneState.hpp>
#include <starlight/command/detail/create_object/DirectObjCreation.hpp>
#include <starlight/command/detail/create_object/FromObjFileLoader.hpp>
#include <starlight/command/headless_render_result_write/GetFileNameForFrame.hpp>
#include <starlight/common/ConfigFile.hpp>
#include <starlight/common/objects/BasicObject.hpp>
#include <starlight/core/logging/LoggingFactory.hpp>
#include <starlight/core/renderer/HeadlessRenderer.hpp>
#include <starlight/event/RegisterMainGraphicsRenderer.hpp>
#include <starlight/virtual/StarCamera.hpp>

#include <star_common/helper/PathHelpers.hpp>

#include <sstream>
#include <string>

using namespace star;

OffscreenRenderer CreateOffscreenRenderer(star::core::device::DeviceContext &context, const uint8_t &numFramesInFlight,
                                          std::shared_ptr<star::StarCamera> camera,
                                          std::shared_ptr<std::vector<star::Light>> mainLight)
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

    // {
    // auto horsePath = mediaDirectoryPath + "models/horse/WildHorse.obj";
    // auto cmd = star::command::CreateObject::Builder()
    //                .setLoader(std::make_unique<star::command::create_object::FromObjFileLoader>(horsePath))
    //                .setUniqueName("horse")
    //                .build();
    // context.begin().set(cmd).submit();
    // cmd.getReply().get()->init(context);
    // objects.emplace_back(cmd.getReply().get());
    // }

    return {context, numFramesInFlight, objects, std::move(mainLight), camera};
}

static std::string CreateImageDir()
{
    const auto fullDir = star::file_helpers::GetExecutableDirectory() / "images"; 
    star::file_helpers::CreateDirectoryIfDoesNotExist(fullDir);

    return fullDir.string();
}

Application::Application()
    : m_captureTrigger(), m_imageOutputDir(CreateImageDir())
{
}

std::shared_ptr<star::StarScene> Application::loadScene(star::core::device::DeviceContext &context,
                                                        const uint8_t &numFramesInFlight)
{
    std::vector<std::shared_ptr<star::StarObject>> allObjects;

    m_captureTrigger = context.begin();
    m_captureTrigger.setType(image_metrics::TriggerCapture::GetUniqueTypeName());

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

        for (auto &object : offscreenRenderer->getObjects())
        {
            allObjects.emplace_back(object);
        }

        const uint32_t width = context.getEngineResolution().width;
        const uint32_t height = context.getEngineResolution().height;
        std::vector<star::Handle> globalInfos(numInFlight);
        std::vector<star::Handle> lightInfos(numInFlight);

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

            allObjects.emplace_back(shared);
        }

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
        m_mainScene =
            std::make_shared<star::StarScene>(star::star_scene::makeWaitForAllObjectsReadyPolicy(std::move(allObjects)),
                                              std::move(camera), std::move(sc), std::move(additionals));
    }

    m_volume->getRenderer().getFogInfo().marchedInfo.defaultDensity = 0.0001f;
    m_volume->getRenderer().getFogInfo().marchedInfo.stepSizeDist = 3.0f;
    m_volume->getRenderer().getFogInfo().marchedInfo.stepSizeDist_light = 5.0f;
    m_volume->getRenderer().getFogInfo().marchedInfo.setSigmaAbsorption(0.00001f);
    m_volume->getRenderer().getFogInfo().marchedInfo.setSigmaScattering(0.8f);
    m_volume->getRenderer().getFogInfo().marchedInfo.setLightPropertyDirG(0.3f);
    m_volume->getRenderer().setFogType(Fog::Type::marched);
    m_volume->getRenderer().getFogInfo().linearInfo.nearDist = 0.01f;
    m_volume->getRenderer().getFogInfo().linearInfo.farDist = 1000.0f;
    m_volume->getRenderer().getFogInfo().expFogInfo.density = 0.6f;
    return m_mainScene;
}

void Application::shutdown(star::core::device::DeviceContext &context)
{
    auto cmd = star::command::SaveSceneState();
    context.begin().set(cmd).submit();
}

void Application::frameUpdate(star::core::SystemContext &context)
{
    auto &d = context.getAllDevices().getData()[0];
    auto cmd = star::headless_render_result_write::GetFileNameForFrame();
    d.begin().set(cmd).submit();

    TriggerSimUpdate(d.getCmdBus(), *m_volume, *m_mainScene->getCamera()); 
    triggerImageRecord(d, d.getFrameTracker(), cmd.getReply().get());
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

void Application::triggerImageRecord(star::core::device::DeviceContext &context,
                                     const star::common::FrameTracker &frameTracker,
                                     const std::string &targetImageFileName)
{
    image_metrics::TriggerCapture trigger(targetImageFileName, *m_volume);
    m_captureTrigger.update(trigger).submit();
}

void Application::TriggerSimUpdate(star::core::CommandBus& cmd, Volume& volume, star::StarCamera& camera)
{
    sim_controller::TriggerUpdate trigger(volume, camera); 
    cmd.submit(trigger); 
}