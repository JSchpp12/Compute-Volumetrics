#include "Application.hpp"

#include "DeclareDependentPasses.hpp"
#include "OffscreenRenderPhase.hpp"
#include "OffscreenRenderPhaseProvider.hpp"
#include "VolumeShadowRenderPhaseProvider.hpp"
#include "command/image_metrics/GetTransferCopyPass.hpp"
#include "command/image_metrics/RegisterVolumeRecordInfo.hpp"
#include "command/image_metrics/TriggerCapture.hpp"
#include "command/sim_controller/CheckIfDone.hpp"
#include "renderer/finalization/HeadlessPhaseProvider.hpp"
#include "util/Distance.hpp"

#include <starlight/ShaderResolver.hpp>
#include <starlight/command/CreateLight.hpp>
#include <starlight/command/CreateObject.hpp>
#include <starlight/command/SaveSceneState.hpp>
#include <starlight/command/command_order/TriggerPass.hpp>
#include <starlight/command/detail/create_object/DeferredObjCreation.hpp>
#include <starlight/command/headless_render_result_write/GetFileNameForFrame.hpp>
#include <starlight/command/headless_render_result_write/GetSetOutputDir.hpp>
#include <starlight/common/ConfigFile.hpp>
#include <starlight/core/logging/LoggingFactory.hpp>
#include <starlight/virtual/StarCamera.hpp>

#include <star_common/helper/StringHelpers.hpp>

#include <string>

using namespace star;

static void TriggerSubmissionOfTerrainDraw(star::core::device::manager::ManagerCommandBuffer &mgrCmdBuff,
                                           const star::core::CommandBus &cmdBus, const star::common::FrameTracker &ft,
                                           const OffscreenRenderPhase &offscreenPhase) noexcept
{
    const size_t ii = static_cast<size_t>(ft.getCurrent().getFrameInFlightIndex());
    const auto &c = offscreenPhase.getCommandBuffer();

    cmdBus.submit(star::command_order::TriggerPass()
                      .setTimelineSemaphore(offscreenPhase.getTimelineSemaphroes()[ii])
                      .setSignalValue(ft.getCurrent().getNumTimesFrameProcessed() + 1)
                      .setPass(c));
};

static void TriggerSubmissionOfCompute(const star::core::CommandBus &cmdBus,
                                       star::core::device::manager::Semaphore &mgrSemaphore,
                                       star::common::EventBus &evtBus, const Volume &volume,
                                       const star::common::FrameTracker &ft) noexcept
{
    const size_t ii = static_cast<size_t>(ft.getCurrent().getFrameInFlightIndex());

    const auto value = volume.getRenderer().getTimelineSignalValue(ft);

    cmdBus.submit(star::command_order::TriggerPass()
                      .setPass(volume.getRenderer().getCommandBuffer())
                      .setTimelineSemaphore(volume.getRenderer().getTimelineSemaphores()[ii])
                      .setSignalValue(value));
}

std::vector<std::shared_ptr<star::StarObject>> Application::createOffscreenObjects(
    star::core::device::DeviceContext &context, std::shared_ptr<star::StarCamera> camera,
    const std::string &terrainPath)
{
    assert(m_loaderFn);

    std::vector<std::shared_ptr<star::StarObject>> objects;
    const std::filesystem::path mediaPath{star::ConfigFile::getSetting(star::Config_Settings::mediadirectory)};

    auto desc = m_loaderFn(context, mediaPath, terrainPath);
    objects.reserve(desc.getCount());
    for (uint32_t i{0}; i < desc.getCount(); i++)
    {
        auto *sqComponent = desc.getSquareComponent(i);
        auto obj = desc.getObject(i);
        if (obj)
        {
            objects.push_back(std::move(obj));
        }
        else if (sqComponent != nullptr)
        {
            // make starlight object
            auto &cubeInfos = sqComponent->cubeInfos;
            // duplicate colors
            const size_t currentSize{cubeInfos.size()};
            cubeInfos.reserve(currentSize * 2);
            for (size_t i{0}; i < currentSize; i++)
            {
                cubeInfos.push_back(cubeInfos[i]);
            }

            const std::filesystem::path cubeShaderDir = mediaPath / "shaders" / "debugCube";
            star::ShaderResolver cubeResolver =
                star::ShaderResolver::Builder{context.getCmdBus()}
                    .setShader(star::Shader_Stage::vertex, (cubeShaderDir / "debugCube.vert").string())
                    .setShader(star::Shader_Stage::fragment, (cubeShaderDir / "debugCube.frag").string())
                    .build();
            auto cube = star::debug::CreateCube(sqComponent->cubeInfos, cubeResolver);
            m_debugCubeInfo = std::make_optional(
                DebugCubeInfo{.debugCube = cube, .numUniqueCubes = sqComponent->numberOfDebugSquares});

            placeDebugCubes(camera->getForwardVector(), camera->getPosition());
            objects.push_back(std::move(cube));
        }
    }

    m_loaderFn = nullptr;

    return objects;
}

Application::Application(LoaderFn objectLoader, std::string terrainPath, std::string volumeName,
                         VolumeRenderingOptions volumeOptions)
    : m_loaderFn(std::move(objectLoader)), m_terrainDir(std::move(terrainPath)), m_volumeName(std::move(volumeName)),
      m_screenshotRegistrations(), m_debugCubeInfo(), m_mainScene(nullptr), m_volume(), m_mainLight(),
      m_volumeOptions(volumeOptions)
{
    // const std::filesystem::path terrain(m_terrainDir);
    // if (!std::filesystem::exists(terrain))
    //{
    //     STAR_THROW("Provided terrain path does not exist: " + m_terrainDir);
    // }
}

std::shared_ptr<star::StarScene> Application::loadScene(star::core::device::DeviceContext &context)
{
    initImageOutputDir(context.getCmdBus());
    initListeners(context);

    auto camera = createMainCamera(context);

    std::vector<std::shared_ptr<star::StarObject>> allObjects;

    auto mediaDirectoryPath = star::ConfigFile::getSetting(star::Config_Settings::mediadirectory);

    {
        star::command::CreateLight lCmd = star::command::CreateLight().setName("sun");
        context.getCmdBus().submit(lCmd);

        auto [addResult, light] = lCmd.getReply().get();
        m_mainLight = light;
        if (addResult == star::command::create_light::fail)
        {
            m_mainLight->emplace_back(Application::CreateMainLight({0.0, 4.0, 0.0}));
        }
    }

    uint8_t numInFlight;
    {
        int framesInFlight = std::stoi(star::ConfigFile::getSetting(star::Config_Settings::frames_in_flight));
        star::common::casts::SafeCast<int, uint8_t>(framesInFlight, numInFlight);
    }

    {
        auto terrainObjects = createOffscreenObjects(context, camera, m_terrainDir);
        for (auto &object : terrainObjects)
        {
            allObjects.emplace_back(object);
        }

        auto offscreenProvider =
            std::make_unique<OffscreenRenderPhaseProvider>(context, std::move(terrainObjects), m_mainLight, camera);
        m_offscreenFrameData = offscreenProvider->getFrameData();

        const uint32_t width = context.getEngineResolution().width;
        const uint32_t height = context.getEngineResolution().height;
        std::vector<star::Handle> globalInfos(numInFlight);
        std::vector<star::Handle> lightInfos(numInFlight);

        size_t fNumFramesInFlight = 0;
        star::common::casts::SafeCast<uint8_t, size_t>(context.frameTracker().getSetup().getNumFramesInFlight(),
                                                       fNumFramesInFlight);

        {
            auto vdbPath = std::filesystem::path(mediaDirectoryPath) / "volumes" / m_volumeName;
            std::string vdbPathString = vdbPath.string();

            star::ShaderResolver volumeResolver =
                star::ShaderResolver::Builder{context.getCmdBus()}
                    .setShader(star::Shader_Stage::vertex, (std::filesystem::path(mediaDirectoryPath) / "shaders" /
                                                            "screenWithTexture" / "screenWithTexture.vert")
                                                               .string())
                    .setShader(star::Shader_Stage::fragment, (std::filesystem::path(mediaDirectoryPath) / "shaders" /
                                                              "screenWithTexture" / "screenWithTexture.frag")
                                                                 .string())
                    .build();

            auto cmd =
                star::command::CreateObject::Builder()
                    .setLoader(std::make_unique<star::command::create_object::DeferredObjCreation>(
                        [&, vdbPathString, fNumFramesInFlight, camera, width, height](star::ShaderResolver &resolver) {
                            return std::make_shared<Volume>(context, vdbPathString, fNumFramesInFlight, camera, width,
                                                            height, m_offscreenFrameData,
                                                            m_volumeOptions.enableCutoffHighlighting, resolver);
                        }))
                    .setShaderResolver(std::move(volumeResolver))
                    .setUniqueName(m_volumeName)
                    .build();

            context.begin().set(cmd).submit();
            m_volume = std::dynamic_pointer_cast<Volume>(cmd.getReply().get());
            allObjects.emplace_back(cmd.getReply().get());

            context.getCmdBus().submit(image_metrics::RegisterVolumeRecordInfo().setVolumeName(m_volumeName));

            // thread the transfer-neighbor handle and the initial fog config into the
            // volume provider before it is handed to the scene. The phase is not built
            // until prepRender, so build() applies these to the phase then.
            {
                image_metrics::GetTransferCopyPass getTransferCopyPassCmd;
                context.getCmdBus().submit(getTransferCopyPassCmd);
                m_volume->getProvider().setTransferNeighborHandle(
                    getTransferCopyPassCmd.getReply().get().commandBuffer);
            }
            m_volume->getProvider().getFogInfo().marchedInfo.defaultDensity = 0.0f;
            m_volume->getProvider().getFogInfo().marchedInfo.stepSizeDist = 150.0f;
            m_volume->getProvider().getFogInfo().marchedInfo.stepSizeDist_light = 300.0f;
            m_volume->getProvider().getFogInfo().marchedInfo.setSigmaAbsorption(0.001f);
            m_volume->getProvider().getFogInfo().marchedInfo.setSigmaScattering(0.005f);
            m_volume->getProvider().getFogInfo().marchedInfo.setLightPropertyDirG(0.9f);
            m_volume->getProvider().setFogType(Fog::Type::sLinear);
            m_volume->getProvider().getFogInfo().linearInfo.nearDist = 0.01f;
            m_volume->getProvider().getFogInfo().linearInfo.farDist = 16000.0f;
            m_volume->getProvider().getFogInfo().expFogInfo.density = 0.6f;
            m_volume->getProvider().getFogInfo().marchedInfo.setDensityMultiplier(0.1f);
            m_volume->getProvider().getFogInfo().marchedInfo.setColorTransparencyCutoff(0.000001f);
            m_volume->getProvider().getFogInfo().marchedInfo.setDistanceTransparencyCutoff(0.000001f);
        }

        std::vector<std::shared_ptr<star::StarObject>> objects{m_volume};

        auto mainRendererProvider = createMainRenderer(context, objects, camera);
        m_mainScene = std::make_shared<star::StarScene>(
            star::star_scene::makeWaitForAllObjectsReadyPolicy(std::move(allObjects)), std::move(camera));
        // volume phase first (its compute images are bound onto the volume
        // StarObject's screen-quad material during the finalization phase
        // build), then the finalization phase.
        m_offscreenPhaseHandle = m_mainScene->addProvider(std::move(offscreenProvider));
        m_volume->getProvider().setOffscreenPhaseHandle(m_offscreenPhaseHandle);

        auto volumeShadowProvider = std::make_unique<VolumeShadowRenderPhaseProvider>(
            m_offscreenFrameData, m_offscreenPhaseHandle, m_mainScene->getCamera().get(),
            /*enableShadowCasting=*/false);
        m_volumeShadowPhaseHandle = m_mainScene->addProvider(std::move(volumeShadowProvider));

        m_volumePhaseHandle = m_mainScene->addProvider(m_volume->takePhaseProvider());
        m_volume->setVolumePhase(m_mainScene.get(), m_volumePhaseHandle);
        m_finalizationPhaseHandle = m_mainScene->addProvider(std::move(mainRendererProvider));
    }

    DeclareDependentPasses::Builder(context.getEventBus(), context.getCmdBus())
        .setConsumer([this]() -> star::Handle { return this->m_volume->getRenderer().getCommandBuffer(); }) // volume
        .setProducer([this]() -> star::Handle {
            return m_mainScene->getPhase(m_offscreenPhaseHandle)->getCommandBuffer();
        }) // terrain
        .build();

    DeclareDependentPasses::Builder(context.getEventBus(), context.getCmdBus())
        .setConsumer(
            [this]() -> star::Handle { return getFinalizationCommandBuffer(); }) // final square screen renderer thing
        .setProducer([this]() -> star::Handle { return m_volume->getRenderer().getCommandBuffer(); }) // volume
        .build();

    return m_mainScene;
}

void Application::shutdown(star::core::device::DeviceContext &context)
{
    auto cmd = star::command::SaveSceneState();
    context.begin().set(cmd).submit();
}

void Application::submitPasses(star::core::device::DeviceContext &context)
{
    auto &cmdBus = context.getCmdBus();
    TriggerSubmissionOfCompute(cmdBus, context.getSemaphoreManager(), context.getEventBus(), *m_volume,
                               context.frameTracker());
    TriggerSubmissionOfTerrainDraw(
        context.getManagerCommandBuffer().m_manager, context.getCmdBus(), context.frameTracker(),
        static_cast<const OffscreenRenderPhase &>(*m_mainScene->getPhase(m_offscreenPhaseHandle)));
}

void Application::initImageOutputDir(star::core::CommandBus &bus)
{
    m_imageOutputDir = std::filesystem::path(star::common::strings::GetStartTime());
    bus.submit(star::headless_render_result_write::GetSetOutputDir::Builder().setSetDir(m_imageOutputDir).build());
}

void Application::frameUpdate(star::core::SystemContext &context)
{
    auto &d = context.getAllDevices().getData()[0];

    submitPasses(d);

    if (!CheckIfControllerIsDone(d.getCmdBus()))
    {
        auto cmd = star::headless_render_result_write::GetFileNameForFrame();
        d.begin().set(cmd).submit();

        const auto result = TriggerSimUpdate(d.getCmdBus(), *m_volume, *m_mainScene->getCamera());
        if (m_debugCubeInfo.has_value() && result.cameraViewDirection)
            placeDebugCubes(m_mainScene->getCamera()->getForwardVector(), m_mainScene->getCamera()->getPosition());

        triggerImageRecord(d, d.frameTracker(), cmd.getReply().get());
    }
}

void Application::placeDebugCubes(const glm::vec3 &direction, const glm::vec3 &startPosition)
{
    assert(m_debugCubeInfo && "Debug cubes object was never registered");

    constexpr float degPerStep = 3.0f;

    // duplicate cubes in opposite directions
    glm::vec3 scale{20.0f, 20000.0f, 20.0f};
    for (uint8_t i{0}; i < 2; i++)
    {
        glm::vec3 offset{};
        glm::vec3 rotAxis = i == 0 ? glm::vec3(0.0f, 1.0f, 0.0f) : glm::vec3(0.0, -1.0, 0.0);
        float rotDeg = degPerStep;

        for (uint8_t j{0}; j < m_debugCubeInfo.value().numUniqueCubes; j++)
        {
            const glm::quat rotQuat = glm::angleAxis(glm::radians(rotDeg), rotAxis);
            const glm::vec3 rotForward = rotQuat * direction;
            const float distance = util::mileToMeters(j + 1);
            offset = {rotForward.x * distance, rotForward.y * distance, rotForward.z * distance};

            auto &instance =
                m_debugCubeInfo.value().debugCube->getInstance(i * m_debugCubeInfo.value().numUniqueCubes + j);
            instance.setPosition(startPosition + offset);
            instance.setScale(scale);

            rotDeg += degPerStep;
        }
    }
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

std::unique_ptr<star::core::renderer::IRenderPhaseProvider> Application::createMainRenderer(
    star::core::device::DeviceContext &context, std::vector<std::shared_ptr<star::StarObject>> objects,
    std::shared_ptr<star::StarCamera> camera)
{
    return std::make_unique<renderer::finalization::HeadlessPhaseProvider>(
        context, std::move(objects), m_offscreenFrameData, vk::PipelineStageFlagBits::eAllCommands);
}

star::Handle Application::getFinalizationCommandBuffer()
{
    return m_mainScene->getPhase(m_finalizationPhaseHandle)->getCommandBuffer();
}

std::shared_ptr<star::StarCamera> Application::createMainCamera(star::core::device::DeviceContext &context)
{
    return std::make_shared<star::StarCamera>(context.getEngineResolution().width, context.getEngineResolution().height,
                                              90.0f, 1.0f, 25000.0f);
}

void Application::triggerImageRecord(star::core::device::DeviceContext &context,
                                     const star::common::FrameTracker &frameTracker,
                                     const std::string &targetImageFileName)
{
    const std::string outputFilePath = (m_imageOutputDir / targetImageFileName).string();
    context.getCmdBus().submit(
        image_metrics::TriggerCapture{outputFilePath, m_mainLight->front(), *m_volume, *m_mainScene->getCamera()});
}

sim_controller::UpdateStatus Application::TriggerSimUpdate(star::core::CommandBus &cmd, Volume &volume,
                                                           star::StarCamera &camera)
{
    sim_controller::TriggerUpdate trigger(volume, camera);
    cmd.submit(trigger);

    return trigger.getReply().get();
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
        .setLuminance(20)
        .setDirection({0.0f, -1.0f, 0.0f});
}