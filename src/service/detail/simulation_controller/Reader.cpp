#include "service/detail/simulation_controller/Reader.hpp"

#include "service/detail/simulation_controller/FogEnabler.hpp"
#include "service/detail/simulation_controller/FogEnabler_json.hpp"
#include "service/detail/simulation_controller/SimulationBounds.hpp"
#include "service/detail/simulation_controller/camera_controller/Circle.hpp"
#include "service/detail/simulation_controller/camera_controller/Circle_json.hpp"
#include "util/Math.hpp"
#include "util/json/FogInfoStruct.hpp"

#include <starlight/common/helpers/FileHelpers.hpp>
#include <starlight/core/Exceptions.hpp>

#include <nlohmann/json.hpp>

#include <fstream>

using nlohmann::json;

namespace service::simulation_controller
{
static FogInfo::LinearFogInfo CalcSteps(const FogInfo::LinearFogInfo &start, const FogInfo::LinearFogInfo &stop)
{
    return {util::CalcDiff(start.nearDist, stop.nearDist), util::CalcDiff(start.farDist, stop.farDist)};
}

static FogInfo::ExpFogInfo CalcSteps(const FogInfo::ExpFogInfo &start, const FogInfo::ExpFogInfo &stop)
{
    return {util::CalcDiff(start.density, stop.density)};
}

static FogInfo::MarchedFogInfo CalcSteps(const FogInfo::MarchedFogInfo &start, const FogInfo::MarchedFogInfo &stop)
{
    return {util::CalcDiff(start.defaultDensity, stop.defaultDensity),
            util::CalcDiff(start.getSigmaAbsorption(), stop.getSigmaAbsorption()),
            util::CalcDiff(start.getSigmaScattering(), stop.getSigmaScattering()),
            util::CalcDiff(start.getLightPropertyDirG(), stop.getLightPropertyDirG()),
            util::CalcDiff(start.stepSizeDist, stop.stepSizeDist),
            util::CalcDiff(start.stepSizeDist_light, stop.stepSizeDist_light)};
}

static FogInfo::HomogenousRendering CalcSteps(const FogInfo::HomogenousRendering &start,
                                              const FogInfo::HomogenousRendering &stop)
{
    return FogInfo::HomogenousRendering(util::CalcDiff(start.maxNumSteps, stop.maxNumSteps));
}

SimulationSteps CalculateSimSteps(const SimulationBounds &bounds)
{
    SimulationSteps steps;

    steps.numSteps = bounds.numSteps;
    steps.start = bounds.start;
    steps.fogInfoChanges.linearInfo = CalcSteps(bounds.start.linearInfo, bounds.stop.linearInfo);
    steps.fogInfoChanges.expFogInfo = CalcSteps(bounds.start.expFogInfo, bounds.stop.expFogInfo);
    steps.fogInfoChanges.marchedInfo = CalcSteps(bounds.start.marchedInfo, bounds.stop.marchedInfo);
    steps.fogInfoChanges.homogenousInfo = CalcSteps(bounds.start.homogenousInfo, bounds.stop.homogenousInfo);

    return steps;
}

static SimulationData LoadBoundsInfoFromFile(const std::string &path)
{
    SimulationData data;

    try
    {
        std::ifstream is(path, std::ios::binary);
        if (is)
        {
            json j;
            is >> j;

            SimulationBounds bounds;
            util::from_json(j["startData"], bounds.start);
            util::from_json(j["stopData"], bounds.stop);

            std::string type = j["camera_controller_type"].get<std::string>();
            if (type == "circle")
            {
                // create a circle type
                camera_controller::Circle circle;
                camera_controller::from_json(j["circle_controller_settings"], circle);

                data.cameraController = CameraController(std::move(circle));
            }
            else
            {
                STAR_THROW("Invalid controller type: " + type);
            }

            bounds.numSteps = j["numSteps"];

            data.steps = CalculateSimSteps(bounds);
            data.initialCameraHeightAboveGround = j["initial_camera_height_above_ground"].get<int>();
            from_json(j["enabled_fog_types"], data.fogStatus);
        }
    }
    catch (const std::exception &ex)
    {
        std::ostringstream oss;
        oss << "Failed to parse json file for SimulationData" << std::endl
            << "File: " << path << std::endl
            << "Error: " << ex.what();
        STAR_THROW(oss.str());
    }

    return data;
}

int Reader::operator()(const std::string &filePath)
{
    assert(std::filesystem::path(filePath).extension() == ".json" && "Requested file is not an expected json file");

    m_loadedBounds.set_value(LoadBoundsInfoFromFile(filePath));
    return 0;
}
} // namespace service::simulation_controller
