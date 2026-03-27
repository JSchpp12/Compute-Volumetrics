#include "Application.hpp"

#include "DeclareDependentPasses.hpp"
#include "GetCmdBuffer.hpp"
#include "OffscreenRenderer.hpp"
#include "command/image_metrics/TriggerCapture.hpp"
#include "command/sim_controller/CheckIfDone.hpp"
#include "command/sim_controller/TriggerUpdate.hpp"
#include "renderer/FinalizationRenderer.hpp"

#include <starlight/command/CreateObject.hpp>
#include <starlight/command/SaveSceneState.hpp>
#include <starlight/command/command_order/TriggerPass.hpp>

#include <starlight/command/GetScreenCaptureSyncInfo.hpp>
#include <starlight/command/detail/create_object/DirectObjCreation.hpp>
#include <starlight/command/detail/create_object/FromObjFileLoader.hpp>
#include <starlight/command/headless_render_result_write/GetFileNameForFrame.hpp>
#include <starlight/command/headless_render_result_write/GetSetOutputDir.hpp>
#include <starlight/common/ConfigFile.hpp>
#include <starlight/core/logging/LoggingFactory.hpp>
#include <starlight/core/renderer/HeadlessRenderer.hpp>
#include <starlight/event/RegisterMainGraphicsRenderer.hpp>
#include <starlight/virtual/StarCamera.hpp>

#include <star_common/helper/StringHelpers.hpp>

#include <string>

using namespace star;

static void CreateWaiterForDefineDependencies(star::common::EventBus &eventBus, const star::core::CommandBus &cmdBus,
                                              const Volume &volume, const OffscreenRenderer &offscreenRenderer) noexcept
{
}

static void TriggerSubmissionOfTerrainDraw(star::core::device::manager::ManagerCommandBuffer &mgrCmdBuff,
                                           const star::core::CommandBus &cmdBus, const star::common::FrameTracker &ft,
                                           const OffscreenRenderer &offscreenRenderer) noexcept
{
    const size_t ii = static_cast<size_t>(ft.getCurrent().getFrameInFlightIndex());
    const auto &c = offscreenRenderer.getCommandBuffer();

    cmdBus.submit(star::command_order::TriggerPass()
                      .setTimelineSemaphore(offscreenRenderer.getTimelineSemaphroes()[ii])
                      .setSignalValue(ft.getCurrent().getNumTimesFrameProcessed() + 1)
                      .setPass(c));
};

static void TriggerSubmissionOfCompute(const star::core::CommandBus &cmdBus,
                                       star::core::device::manager::Semaphore &mgrSemaphore,
                                       star::common::EventBus &evtBus, const Volume &volume,
                                       const star::common::FrameTracker &ft) noexcept
{
    const size_t ii = static_cast<size_t>(ft.getCurrent().getFrameInFlightIndex());

    const auto value = ft.getCurrent().getNumTimesFrameProcessed() + 1;

    cmdBus.submit(star::command_order::TriggerPass()
                      .setPass(volume.getRenderer().getCommandBuffer())
                      .setTimelineSemaphore(volume.getRenderer().getTimelineSemaphores()[ii])
                      .setSignalValue(std::move(value)));
}

static void TriggerSubmissionOfFinalization(const star::core::CommandBus &cmdBus,
                                            const star::core::renderer::HeadlessRenderer &finalizationRenderer,
                                            size_t currentNumTimesFrameProcessed, size_t currentFrameInFlight)
{
    cmdBus.submit(star::command_order::TriggerPass()
                      .setPass(finalizationRenderer.getCommandBuffer())
                      .setTimelineSemaphore(finalizationRenderer.getTimelineSemaphores()[currentFrameInFlight])
                      .setSignalValue(++currentNumTimesFrameProcessed));
}

OffscreenRenderer Application::CreateOffscreenRenderer(star::core::device::DeviceContext &context,
                                                       const uint8_t &numFramesInFlight,
                                                       std::shared_ptr<star::StarCamera> camera,
                                                       const std::string &terrainPath,
                                                       std::shared_ptr<std::vector<star::Light>> mainLight)
{
    std::vector<std::shared_ptr<star::StarObject>> objects;
    const auto mediaDirectoryPath = star::ConfigFile::getSetting(star::Config_Settings::mediadirectory);

    //{
    //    auto cmd = star::command::CreateObject::Builder()
    //                   .setLoader(std::make_unique<star::command::create_object::DirectObjCreation>(
    //                       std::make_shared<Terrain>(context, terrainPath)))
    //                   .setUniqueName("terrain")
    //                   .build();
    //    context.begin().set(cmd).submit();
    //    objects.emplace_back(cmd.getReply().get());
    //}

    {
        auto horsePath = mediaDirectoryPath + "models/horse/WildHorse.obj";
        auto cmd = star::command::CreateObject::Builder()
                       .setLoader(std::make_unique<star::command::create_object::FromObjFileLoader>(horsePath))
                       .setUniqueName("horse")
                       .build();
        context.begin().set(cmd).submit();
        cmd.getReply().get()->init(context);
        objects.emplace_back(cmd.getReply().get());
    }

    return {context, numFramesInFlight, objects, std::move(mainLight), camera};
}

Application::Application(std::string &&terrainPath) : m_terrainDir(std::move(terrainPath))
{
    // const std::filesystem::path terrain(m_terrainDir);
    // if (!std::filesystem::exists(terrain))
    //{
    //     STAR_THROW("Provided terrain path does not exist: " + m_terrainDir);
    // }
}

std::shared_ptr<star::StarScene> Application::loadScene(star::core::device::DeviceContext &context,
                                                        const uint8_t &numFramesInFlight)
{
    initImageOutputDir(context.getCmdBus());
    initListeners(context);

    auto camera = createMainCamera(context);

    std::vector<std::shared_ptr<star::StarObject>> allObjects;

    auto mediaDirectoryPath = star::ConfigFile::getSetting(star::Config_Settings::mediadirectory);

    m_mainLight = std::make_shared<std::vector<star::Light>>(
        std::vector<star::Light>{Application::CreateMainLight({0.0, 4.0, 0.0})});

    uint8_t numInFlight;
    {
        int framesInFlight = std::stoi(star::ConfigFile::getSetting(star::Config_Settings::frames_in_flight));
        star::common::casts::SafeCast<int, uint8_t>(framesInFlight, numInFlight);
    }

    star::StarObjectInstance *volumeInstance = nullptr;
    {
        auto oRenderer =
            star::common::Renderer(CreateOffscreenRenderer(context, numInFlight, camera, m_terrainDir, m_mainLight));
        m_offRenderer = oRenderer.getRaw<OffscreenRenderer>();

        for (auto &object : m_offRenderer->getObjects())
        {
            allObjects.emplace_back(object);
        }

        const uint32_t width = context.getEngineResolution().width;
        const uint32_t height = context.getEngineResolution().height;
        std::vector<star::Handle> globalInfos(numInFlight);
        std::vector<star::Handle> lightInfos(numInFlight);

        size_t fNumFramesInFlight = 0;
        star::common::casts::SafeCast<uint8_t, size_t>(numFramesInFlight, fNumFramesInFlight);

        {
            std::string vdbPath = mediaDirectoryPath + "volumes/ambient";
            m_volume =
                std::make_shared<Volume>(context, vdbPath, fNumFramesInFlight, camera, width, height, m_offRenderer,
                                         m_offRenderer->getCameraInfoBuffers(), m_offRenderer->getLightInfoBuffers(),
                                         m_offRenderer->getLightListBuffers());
            auto cmd = star::command::CreateObject::Builder()
                           .setLoader(std::make_unique<star::command::create_object::DirectObjCreation>(m_volume))
                           .setUniqueName("flatWind")
                           .build();

            context.begin().set(cmd).submit();
            allObjects.emplace_back(cmd.getReply().get());
        }

        std::vector<std::shared_ptr<star::StarObject>> objects{m_volume};
        std::vector<star::common::Renderer> additionals;
        additionals.emplace_back(std::move(oRenderer));

        auto sc = createMainRenderer(context, objects, camera);
        m_finalizationCmds = sc.getRaw<star::core::renderer::DefaultRenderer>();
        m_mainScene =
            std::make_shared<star::StarScene>(star::star_scene::makeWaitForAllObjectsReadyPolicy(std::move(allObjects)),
                                              std::move(camera), std::move(sc), std::move(additionals));
    }

    DeclareDependentPasses::Builder(context.getEventBus(), context.getCmdBus())
        .setConsumer([this]() -> star::Handle { return this->m_volume->getRenderer().getCommandBuffer(); }) //volume
        .setProducer([this]() -> star::Handle { return this->m_offRenderer->getCommandBuffer(); })  //terrain
        .build();
    DeclareDependentPasses::Builder(context.getEventBus(), context.getCmdBus())
        .setConsumer([this]() -> star::Handle { return m_finalizationCmds->getCommandBuffer(); }) //final square screen renderer thing
        .setProducer([this]() -> star::Handle { return m_volume->getRenderer().getCommandBuffer(); })   //volume
        .build();
    // find a way to declare dependency between the headless renderer and the copy commands
    // DeclareDependentPasses<star::core::renderer::HeadlessRenderer, GetCmdBuffer>::Builder(context.getEventBus(),
    // context.getCmdBus())
    //     .setConsumer()
    //     .setProducer()

    m_volume->getRenderer().getFogInfo().marchedInfo.defaultDensity = 0.0001f;
    m_volume->getRenderer().getFogInfo().marchedInfo.stepSizeDist = 100.0f;
    m_volume->getRenderer().getFogInfo().marchedInfo.stepSizeDist_light = 250.0f;
    m_volume->getRenderer().getFogInfo().marchedInfo.setSigmaAbsorption(0.00001f);
    m_volume->getRenderer().getFogInfo().marchedInfo.setSigmaScattering(0.40f);
    m_volume->getRenderer().getFogInfo().marchedInfo.setLightPropertyDirG(0.3f);
    m_volume->getRenderer().setFogType(Fog::Type::sLinear);
    m_volume->getRenderer().getFogInfo().linearInfo.nearDist = 0.01f;
    m_volume->getRenderer().getFogInfo().linearInfo.farDist = 16000.0f;
    m_volume->getRenderer().getFogInfo().expFogInfo.density = 0.6f;
    m_volume->getRenderer().getFogInfo().marchedInfo.setDensityMultiplier(1.0f);
    return m_mainScene;
}

void Application::shutdown(star::core::device::DeviceContext &context)
{
    auto cmd = star::command::SaveSceneState();
    context.begin().set(cmd).submit();
}

void Application::initImageOutputDir(star::core::CommandBus &bus)
{
    m_imageOutputDir = std::filesystem::path(star::common::strings::GetStartTime());
    bus.submit(star::headless_render_result_write::GetSetOutputDir::Builder().setSetDir(m_imageOutputDir).build());
}

void Application::frameUpdate(star::core::SystemContext &context)
{
    auto &d = context.getAllDevices().getData()[0];

    {
        size_t fi = static_cast<size_t>(d.getFrameTracker().getCurrent().getFrameInFlightIndex());
        size_t gfProcessed = static_cast<size_t>(d.getFrameTracker().getCurrent().getNumTimesFrameProcessed());

        TriggerSubmissionOfCompute(d.getCmdBus(), d.getSemaphoreManager(), d.getEventBus(), *m_volume,
                                   d.getFrameTracker());
        TriggerSubmissionOfTerrainDraw(d.getManagerCommandBuffer().m_manager, d.getCmdBus(), d.getFrameTracker(),
                                       *m_offRenderer);
        TriggerSubmissionOfFinalization(d.getCmdBus(), *m_finalizationCmds, gfProcessed, fi);
    }

    //if (!CheckIfControllerIsDone(d.getCmdBus()))
    //{
    //    auto cmd = star::headless_render_result_write::GetFileNameForFrame();
    //    d.begin().set(cmd).submit();

    //    TriggerSimUpdate(d.getCmdBus(), *m_volume, *m_mainScene->getCamera());
    //    triggerImageRecord(d, d.getFrameTracker(), cmd.getReply().get());
    //}
}

void Application::setHeadlessServiceOutputDir(star::core::device::DeviceContext &context) const
{
    auto cmd = star::headless_render_result_write::GetSetOutputDir::Builder().setSetDir(m_imageOutputDir).build();

    context.getCmdBus().submit(cmd);
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

star::common::Renderer Application::createMainRenderer(star::core::device::DeviceContext &context,
                                                       std::vector<std::shared_ptr<star::StarObject>> objects,
                                                       std::shared_ptr<star::StarCamera> camera)
{
    star::common::Renderer sc{
        star::core::renderer::HeadlessRenderer{context, context.getFrameTracker().getSetup().getNumFramesInFlight(),
                                               objects, m_mainLight, camera, vk::PipelineStageFlagBits::eAllCommands}};

    auto *renderer = sc.getRaw<star::core::renderer::HeadlessRenderer>();
    context.getEventBus().emit(star::event::RegisterMainGraphicsRenderer{renderer});

    return sc;
}

std::shared_ptr<star::StarCamera> Application::createMainCamera(star::core::device::DeviceContext &context)
{
    return std::make_shared<star::StarCamera>(context.getEngineResolution().width, context.getEngineResolution().height,
                                              90.0f, 1.0f, 20000.0f);
}

void Application::triggerImageRecord(star::core::device::DeviceContext &context,
                                     const star::common::FrameTracker &frameTracker,
                                     const std::string &targetImageFileName)
{
    const std::string outputFilePath = (m_imageOutputDir / targetImageFileName).string();
    context.getCmdBus().submit(image_metrics::TriggerCapture{outputFilePath, *m_volume, *m_mainScene->getCamera()});
}

void Application::TriggerSimUpdate(star::core::CommandBus &cmd, Volume &volume, star::StarCamera &camera)
{
    sim_controller::TriggerUpdate trigger(volume, camera);
    cmd.submit(trigger);
}

bool Application::CheckIfControllerIsDone(star::core::CommandBus &cmd)
{
    sim_controller::CheckIfDone check;
    cmd.submit(check);
    return check.getReply().get();
}

void Application::SetVolumeToCamera(Volume &volume, star::StarCamera &camera)
{
}

star::Light Application::CreateMainLight(glm::vec3 position)
{
    return star::Light()
        .setPosition(std::move(position))
        .setType(star::Type::Light::directional)
        .setAmbient({1.0f, 1.0f, 1.0f})
        .setLuminance(40)
        .setDirection({0.0f, -1.0f, 0.0f});
}