#include "controller/detail/simulation_bounds_file/Reader.hpp"

#include "fog_info/JsonUtils.hpp"
#include "util/Math.hpp"

#include <starlight/common/helpers/FileHelpers.hpp>

#include <nlohmann/json.hpp>

#include <fstream>

using nlohmann::json;

namespace controller::simulation_bounds_file
{
static SimulationBounds LoadBoundsInfoFromFile(const std::string &path)
{
    SimulationBounds bounds;

    try
    {
        std::ifstream is(path, std::ios::binary);
        if (is)
        {
            json j;
            is >> j;

            fog_info::from_json(j["startData"], bounds.start);
            fog_info::from_json(j["stopData"], bounds.stop);
            bounds.numSteps = j["numSteps"];
        }
    }
    catch (...)
    {
        // null
    }

    return bounds;
}

static FogInfo::LinearFogInfo CalcSteps(int numSteps, const FogInfo::LinearFogInfo &start,
                                        const FogInfo::LinearFogInfo &stop)
{
    return {util::CalcDiff(static_cast<float>(numSteps), start.nearDist, stop.nearDist),
            util::CalcDiff(static_cast<float>(numSteps), start.farDist, stop.farDist)};
}

static FogInfo::ExpFogInfo CalcSteps(int numSteps, const FogInfo::ExpFogInfo &start, const FogInfo::ExpFogInfo &stop)
{
    return {util::CalcDiff(static_cast<float>(numSteps), start.density, stop.density)};
}

static FogInfo::MarchedFogInfo CalcSteps(int numSteps, const FogInfo::MarchedFogInfo &start,
                                         const FogInfo::MarchedFogInfo &stop)
{
    return {util::CalcDiff(static_cast<float>(numSteps), start.defaultDensity, stop.defaultDensity),
            util::CalcDiff(static_cast<float>(numSteps), start.getSigmaAbsorption(), stop.getSigmaAbsorption()),
            util::CalcDiff(static_cast<float>(numSteps), start.getSigmaScattering(), stop.getSigmaScattering()),
            util::CalcDiff(static_cast<float>(numSteps), start.getLightPropertyDirG(), stop.getLightPropertyDirG()),
            util::CalcDiff(static_cast<float>(numSteps), start.stepSizeDist, stop.stepSizeDist),
            util::CalcDiff(static_cast<float>(numSteps), start.stepSizeDist_light, stop.stepSizeDist_light)};
}

static FogInfo::HomogenousRendering CalcSteps(int numSteps, const FogInfo::HomogenousRendering &start,
                                              const FogInfo::HomogenousRendering &stop)
{
    return FogInfo::HomogenousRendering(
        util::CalcDiff(static_cast<uint32_t>(numSteps), start.maxNumSteps, stop.maxNumSteps));
}

SimulationSteps CalculateSimSteps(const SimulationBounds &bounds)
{
    SimulationSteps steps;

    steps.numSteps = bounds.numSteps;
    steps.start = bounds.start; 
    steps.fogInfoChanges.linearInfo = CalcSteps(bounds.numSteps, bounds.start.linearInfo, bounds.stop.linearInfo);
    steps.fogInfoChanges.expFogInfo = CalcSteps(bounds.numSteps, bounds.start.expFogInfo, bounds.stop.expFogInfo);
    steps.fogInfoChanges.marchedInfo = CalcSteps(bounds.numSteps, bounds.start.marchedInfo, bounds.stop.marchedInfo);
    steps.fogInfoChanges.homogenousInfo =
        CalcSteps(bounds.numSteps, bounds.start.homogenousInfo, bounds.stop.homogenousInfo);

    return steps;
}

int Reader::operator()(const std::string &filePath)
{
    auto bounds = LoadBoundsInfoFromFile(filePath);
    m_loadedBounds.set_value(CalculateSimSteps(bounds));
    return 0;
}
} // namespace controller::simulation_bounds_file
